#pragma once

#include <MicroWorld/Containers/Span.h>
#include <MicroWorld/Net/NetDriver.h>
#include <MicroWorld/Net/NetPacketStorage.h>
#include <MicroWorld/Net/NetResult.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace MicroWorld
{

/**
 * Fixed-capacity outbound queue and direct receive over one externally referenced `INetDriver`.
 *
 * The manager holds one driver and one `FNetPacketStorage` by reference; the
 * caller owns both. `QueueSend` copies a complete accepted packet into the
 * caller-owned FIFO tail, one `AdvanceSend` attempts at most the head (retaining
 * it on any driver failure), and `Receive` performs at most one direct driver
 * receive without building an inbound queue.
 */
template<std::size_t MaxPackets, std::size_t MaxPacketBytes>
class FNetManager final
{
	static_assert(MaxPackets > 0, "FNetManager requires at least one packet slot.");
	static_assert(MaxPacketBytes > 0, "FNetManager requires a nonzero per-packet byte capacity.");

public:
	/** Binds the manager to one externally referenced driver and caller-owned packet storage. */
	FNetManager(INetDriver& Driver, FNetPacketStorage<MaxPackets, MaxPacketBytes>& Storage) noexcept : Driver(Driver), Storage(Storage) {}

	/** Prevents copying so one manager value binds one driver and one storage instance. */
	FNetManager(const FNetManager&) = delete;

	/** Prevents copying so one manager value binds one driver and one storage instance. */
	FNetManager& operator=(const FNetManager&) = delete;

	/** Defaulted so a manager with automatic storage destructs without side effects. */
	~FNetManager() noexcept = default;

	/**
	 * Copies one complete packet into the outbound FIFO tail.
	 * Returns `Invalid` for a null packet with nonzero length or a packet larger than
	 * `MaxPacketBytes` (it can never fit). Returns `Full` when the FIFO has no free slot.
	 * A non-success result leaves the FIFO contents and order unchanged.
	 */
	ENetResult QueueSend(TSpan<const std::uint8_t> Packet) noexcept
	{
		const std::size_t PacketSize = Packet.Size();
		if (PacketSize == 0)
		{
			if (QueuedCount >= MaxPackets)
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
		if (PacketSize > MaxPacketBytes)
		{
			// The packet can never fit a slot; the request is malformed.
			return ENetResult::Invalid;
		}
		if (QueuedCount >= MaxPackets)
		{
			return ENetResult::Full;
		}
		StorePacketAt(TailIndex, Packet, PacketSize);
		AdvanceTail();
		return ENetResult::Success;
	}

	/**
	 * Attempts to send the FIFO head through the driver.
	 * Performs at most one driver send. On `Success` the head packet is removed.
	 * On `Full`, `Unavailable`, or `Invalid` from the driver, the head packet is
	 * retained and FIFO order is preserved so the next advance can retry it.
	 * Returns `Unavailable` when the FIFO is empty, so a caller can distinguish
	 * "nothing to send" from a transient driver rejection.
	 */
	ENetResult AdvanceSend() noexcept
	{
		if (QueuedCount == 0)
		{
			return ENetResult::Unavailable;
		}
		const TSpan<const std::uint8_t> HeadPacket(Storage.PacketBytes[HeadIndex].data(), Storage.PacketLengths[HeadIndex]);
		const ENetResult SendResult = Driver.TrySend(HeadPacket);
		if (SendResult != ENetResult::Success)
		{
			// Retain the head and preserve order; the caller retries on the next advance.
			return SendResult;
		}
		Storage.PacketLengths[HeadIndex] = 0;
		HeadIndex = (HeadIndex + 1) % MaxPackets;
		--QueuedCount;
		return ENetResult::Success;
	}

	/**
	 * Performs at most one direct driver receive into caller storage.
	 * The operation is transactional: on `Full`, `Invalid`, or `Unavailable` the
	 * destination and `OutResult.BytesReceived` are unchanged. The manager never
	 * queues inbound packets.
	 */
	ENetResult Receive(TSpan<std::uint8_t> Destination, FNetReceiveResult& OutResult) noexcept { return Driver.TryReceive(Destination, OutResult); }

	/** Reports the fixed packet-slot capacity of this manager's outbound FIFO. */
	static constexpr std::size_t QueueCapacity() noexcept { return MaxPackets; }

	/** Reports the maximum byte length accepted per queued packet. */
	static constexpr std::size_t MaximumPacketBytes() noexcept { return MaxPacketBytes; }

	/** Reports how many packets are currently queued for send. */
	constexpr std::size_t QueuedCountValue() const noexcept { return QueuedCount; }

	/** Distinguishes an empty FIFO so a caller can skip `AdvanceSend`. */
	constexpr bool IsEmpty() const noexcept { return QueuedCount == 0; }

	/** Distinguishes a full FIFO so a caller can observe backpressure. */
	constexpr bool IsFull() const noexcept { return QueuedCount >= MaxPackets; }

private:
	/** Copies one accepted packet and its length into the slot at `Index`. */
	void StorePacketAt(const std::size_t Index, TSpan<const std::uint8_t> Packet, const std::size_t PacketSize) noexcept
	{
		if (PacketSize > 0)
		{
			std::memcpy(Storage.PacketBytes[Index].data(), Packet.Data(), PacketSize);
		}
		Storage.PacketLengths[Index] = PacketSize;
	}

	/** Advances the tail and count after one accepted packet. */
	void AdvanceTail() noexcept
	{
		TailIndex = (TailIndex + 1) % MaxPackets;
		++QueuedCount;
	}

	/** Holds the externally referenced driver; the caller owns its lifetime. */
	INetDriver& Driver;

	/** Holds the externally referenced caller-owned packet storage. */
	FNetPacketStorage<MaxPackets, MaxPacketBytes>& Storage;

	/** Indexes the next packet to send so the FIFO order is preserved. */
	std::size_t HeadIndex{0};

	/** Indexes the next free slot so queues append without overwriting the head. */
	std::size_t TailIndex{0};

	/** Tracks occupancy so full and empty states are observable without wrap arithmetic. */
	std::size_t QueuedCount{0};
};

} // namespace MicroWorld
