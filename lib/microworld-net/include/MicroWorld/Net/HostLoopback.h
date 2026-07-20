#pragma once

#include <MicroWorld/Containers/Span.h>
#include <MicroWorld/Net/NetDriver.h>
#include <MicroWorld/Net/NetResult.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace MicroWorld
{

/**
 * Deterministic fixed-capacity loopback `INetDriver` for host tests.
 *
 * The loopback keeps a FIFO of at most `CapacityPackets` packets, each at most
 * `PacketBytes` long, in fixed storage embedded in the driver value. A full send
 * never overwrites an accepted packet, an empty receive returns `Unavailable`,
 * and a too-small receive destination returns `Full` while retaining the head.
 */
template<std::size_t CapacityPackets, std::size_t PacketBytes>
class FHostLoopback final : public INetDriver
{
	static_assert(CapacityPackets > 0, "FHostLoopback requires at least one packet slot.");
	static_assert(PacketBytes > 0, "FHostLoopback requires a nonzero per-packet byte capacity.");

public:
	/** Defaulted so the loopback can live in automatic or static storage without side effects. */
	FHostLoopback() noexcept = default;

	/** Prevents copying so one driver value owns its fixed packet storage. */
	FHostLoopback(const FHostLoopback&) = delete;

	/** Prevents copying so one driver value owns its fixed packet storage. */
	FHostLoopback& operator=(const FHostLoopback&) = delete;

	/** Defaulted so a driver with automatic storage destructs without side effects. */
	~FHostLoopback() noexcept override = default;

	/**
	 * Copies one complete packet onto the FIFO tail.
	 * Returns `Full` when no packet slot is free, `Invalid` for a null packet with
	 * nonzero length or a packet larger than `PacketBytes`. A non-success result
	 * leaves every accepted packet and the FIFO order unchanged.
	 */
	ENetResult TrySend(TSpan<const std::uint8_t> Packet) noexcept override
	{
		const std::size_t PacketSize = Packet.Size();
		if (PacketSize == 0)
		{
			// A zero-length packet is a valid transport op; enqueue it so receive mirrors send.
			if (QueuedCount >= CapacityPackets)
			{
				return ENetResult::Full;
			}
			StorePacketAt(TailIndex, Packet, 0);
			AdvanceTail();
			return ENetResult::Success;
		}
		if (Packet.Data() == nullptr)
		{
			return ENetResult::Invalid;
		}
		if (PacketSize > PacketBytes)
		{
			// The packet can never fit a slot; the request is malformed.
			return ENetResult::Invalid;
		}
		if (QueuedCount >= CapacityPackets)
		{
			return ENetResult::Full;
		}
		StorePacketAt(TailIndex, Packet, PacketSize);
		AdvanceTail();
		return ENetResult::Success;
	}

	/**
	 * Pops the FIFO head into the caller-owned destination.
	 * The operation is transactional: on `Full`, `Invalid`, or `Unavailable` the
	 * destination and `OutResult.BytesReceived` are unchanged. Returns `Success`
	 * only when a packet was delivered, `Unavailable` when the FIFO is empty,
	 * `Full` when the destination is too small for the queued head packet, and
	 * `Invalid` for a null destination with nonzero length, even when the FIFO
	 * is empty.
	 */
	ENetResult TryReceive(TSpan<std::uint8_t> Destination, FNetReceiveResult& OutResult) noexcept override
	{
		// A null destination with nonzero length is an invalid request independent of the
		// FIFO state: validate it before the empty-queue check so an empty loopback still
		// returns Invalid for a malformed destination.
		if (Destination.Size() != 0 && Destination.Data() == nullptr)
		{
			return ENetResult::Invalid;
		}
		if (QueuedCount == 0)
		{
			return ENetResult::Unavailable;
		}
		const std::size_t HeadSize = PacketLengths[HeadIndex];
		const std::size_t DestinationSize = Destination.Size();
		if (DestinationSize == 0)
		{
			// An empty destination cannot accept even a zero-length head packet
			// without losing the ability to signal that a packet was delivered.
			if (HeadSize != 0)
			{
				return ENetResult::Full;
			}
		}
		if (HeadSize > DestinationSize)
		{
			// Keep the head packet so the caller can retry with a larger buffer.
			return ENetResult::Full;
		}
		if (HeadSize > 0)
		{
			std::memcpy(Destination.Data(), PacketStorage[HeadIndex].data(), HeadSize);
		}
		OutResult.BytesReceived = HeadSize;
		PacketLengths[HeadIndex] = 0;
		HeadIndex = (HeadIndex + 1) % CapacityPackets;
		--QueuedCount;
		return ENetResult::Success;
	}

	/** Reports the fixed packet-slot capacity of this loopback driver. */
	constexpr std::size_t QueueCapacity() const noexcept { return CapacityPackets; }

	/** Reports the maximum byte length accepted per packet. */
	constexpr std::size_t MaximumPacketBytes() const noexcept { return PacketBytes; }

	/** Reports how many packets are currently queued for receive. */
	constexpr std::size_t QueuedCountValue() const noexcept { return QueuedCount; }

	/** Distinguishes an empty FIFO without inspecting packet storage. */
	constexpr bool IsEmpty() const noexcept { return QueuedCount == 0; }

	/** Distinguishes a full FIFO so a caller can observe backpressure. */
	constexpr bool IsFull() const noexcept { return QueuedCount >= CapacityPackets; }

	/** Drops every queued packet so the FIFO capacity can be reused deterministically. */
	void Drain() noexcept
	{
		PacketLengths.fill(0);
		HeadIndex = 0;
		TailIndex = 0;
		QueuedCount = 0;
	}

private:
	/** Copies one accepted packet and its length into the slot at `Index`. */
	void StorePacketAt(const std::size_t Index, TSpan<const std::uint8_t> Packet, const std::size_t PacketSize) noexcept
	{
		if (PacketSize > 0)
		{
			std::memcpy(PacketStorage[Index].data(), Packet.Data(), PacketSize);
		}
		PacketLengths[Index] = PacketSize;
	}

	/** Advances the tail and count after one accepted packet. */
	void AdvanceTail() noexcept
	{
		TailIndex = (TailIndex + 1) % CapacityPackets;
		++QueuedCount;
	}

	/** Fixed per-packet byte storage; only the leading `PacketLengths[i]` bytes are valid. */
	std::array<std::array<std::uint8_t, PacketBytes>, CapacityPackets> PacketStorage{};

	/** Records the valid byte length of each queued packet so receives stay exact. */
	std::array<std::size_t, CapacityPackets> PacketLengths{};

	/** Indexes the next packet to receive so the FIFO order is preserved. */
	std::size_t HeadIndex{0};

	/** Indexes the next free slot so sends append without overwriting the head. */
	std::size_t TailIndex{0};

	/** Tracks occupancy so full and empty states are observable without wrap arithmetic. */
	std::size_t QueuedCount{0};
};

} // namespace MicroWorld
