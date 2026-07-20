#include "TestSupport.h"

#include <MicroWorld/Containers/Span.h>
#include <MicroWorld/Net/HostLoopback.h>
#include <MicroWorld/Net/NetDriver.h>
#include <MicroWorld/Net/NetResult.h>

#include <array>
#include <cstddef>
#include <cstdint>

namespace
{

using MicroWorld::ENetResult;
using MicroWorld::FHostLoopback;
using MicroWorld::FNetReceiveResult;
using MicroWorld::INetDriver;
using MicroWorld::TSpan;

/** Proves a fresh loopback is empty and reports its fixed capacities. */
MW_TEST_CASE(HostLoopbackStartsEmptyWithFixedCapacities)
{
	FHostLoopback<2, 4> Loopback;

	MW_EXPECT_EQ(Test, true, Loopback.IsEmpty(), "A fresh loopback must be empty");
	MW_EXPECT_EQ(Test, false, Loopback.IsFull(), "A fresh loopback must not be full");
	MW_EXPECT_EQ(Test, static_cast<std::size_t>(2), Loopback.QueueCapacity(), "Queue capacity must match the template parameter");
	MW_EXPECT_EQ(Test, static_cast<std::size_t>(4), Loopback.MaximumPacketBytes(), "Max packet bytes must match the template parameter");
	MW_EXPECT_EQ(Test, static_cast<std::size_t>(0), Loopback.QueuedCountValue(), "A fresh loopback must report zero queued packets");
}

/** Proves send followed by receive delivers the same bytes in FIFO order. */
MW_TEST_CASE(HostLoopbackDeliversPacketsInFifoOrder)
{
	FHostLoopback<2, 4> Loopback;

	const std::array<std::uint8_t, 2> FirstPacket{0x10, 0x20};
	const std::array<std::uint8_t, 3> SecondPacket{0x30, 0x40, 0x50};
	MW_EXPECT_EQ(
		Test, ENetResult::Success, Loopback.TrySend(TSpan<const std::uint8_t>(FirstPacket.data(), FirstPacket.size())), "First send must succeed");
	MW_EXPECT_EQ(
		Test, ENetResult::Success, Loopback.TrySend(TSpan<const std::uint8_t>(SecondPacket.data(), SecondPacket.size())), "Second send must succeed");
	MW_EXPECT_EQ(Test, static_cast<std::size_t>(2), Loopback.QueuedCountValue(), "Two sends must queue two packets");

	std::array<std::uint8_t, 4> Destination{};
	FNetReceiveResult FirstReceive{};
	MW_EXPECT_EQ(
		Test,
		ENetResult::Success,
		Loopback.TryReceive(TSpan<std::uint8_t>(Destination.data(), Destination.size()), FirstReceive),
		"First receive must succeed");
	MW_EXPECT_EQ(Test, static_cast<std::size_t>(2), FirstReceive.BytesReceived, "First receive must report the head packet length");
	MW_EXPECT_EQ(Test, static_cast<std::uint8_t>(0x10), Destination[0], "First receive must deliver the first head byte");
	MW_EXPECT_EQ(Test, static_cast<std::uint8_t>(0x20), Destination[1], "First receive must deliver the second head byte");

	std::array<std::uint8_t, 4> SecondDestination{};
	FNetReceiveResult SecondReceive{};
	MW_EXPECT_EQ(
		Test,
		ENetResult::Success,
		Loopback.TryReceive(TSpan<std::uint8_t>(SecondDestination.data(), SecondDestination.size()), SecondReceive),
		"Second receive must succeed");
	MW_EXPECT_EQ(Test, static_cast<std::size_t>(3), SecondReceive.BytesReceived, "Second receive must report the next head packet length");
	MW_EXPECT_EQ(Test, static_cast<std::uint8_t>(0x30), SecondDestination[0], "Second receive must deliver the second packet first byte");
	MW_EXPECT_EQ(Test, true, Loopback.IsEmpty(), "Loopback must be empty after draining both packets");
}

/** Proves a full queue rejects further sends without overwriting accepted packets. */
MW_TEST_CASE(HostLoopbackFullQueueDoesNotOverwriteAcceptedPackets)
{
	FHostLoopback<1, 4> Loopback;

	const std::array<std::uint8_t, 2> Accepted{0xAA, 0xBB};
	const std::array<std::uint8_t, 2> Rejected{0xCC, 0xDD};
	MW_EXPECT_EQ(
		Test,
		ENetResult::Success,
		Loopback.TrySend(TSpan<const std::uint8_t>(Accepted.data(), Accepted.size())),
		"Send into an empty queue must succeed");
	MW_EXPECT_EQ(Test, true, Loopback.IsFull(), "A one-slot queue must be full after one send");

	const ENetResult OverflowResult = Loopback.TrySend(TSpan<const std::uint8_t>(Rejected.data(), Rejected.size()));
	MW_EXPECT_EQ(Test, ENetResult::Full, OverflowResult, "Send into a full queue must return Full");
	MW_EXPECT_EQ(Test, static_cast<std::size_t>(1), Loopback.QueuedCountValue(), "Overflow must not change the queued count");

	std::array<std::uint8_t, 4> Destination{};
	FNetReceiveResult ReceiveResult{};
	Loopback.TryReceive(TSpan<std::uint8_t>(Destination.data(), Destination.size()), ReceiveResult);
	MW_EXPECT_EQ(Test, static_cast<std::uint8_t>(0xAA), Destination[0], "Overflow must not overwrite the accepted head packet");
	MW_EXPECT_EQ(Test, static_cast<std::uint8_t>(0xBB), Destination[1], "Overflow must not overwrite the accepted head packet");
}

/** Proves an empty receive returns Unavailable without touching its destination or byte count. */
MW_TEST_CASE(HostLoopbackEmptyReceiveReturnsUnavailable)
{
	FHostLoopback<2, 4> Loopback;

	std::array<std::uint8_t, 4> Destination{0xFF, 0xFF, 0xFF, 0xFF};
	FNetReceiveResult ReceiveResult{std::size_t{0xEE}};
	const ENetResult EmptyResult = Loopback.TryReceive(TSpan<std::uint8_t>(Destination.data(), Destination.size()), ReceiveResult);

	MW_EXPECT_EQ(Test, ENetResult::Unavailable, EmptyResult, "Receive from an empty loopback must return Unavailable");
	MW_EXPECT_EQ(Test, static_cast<std::size_t>(0xEE), ReceiveResult.BytesReceived, "Failed receive must leave BytesReceived unchanged");
	MW_EXPECT_EQ(Test, static_cast<std::uint8_t>(0xFF), Destination[0], "Failed receive must not modify the destination");
}

/** Proves a too-small destination returns Full and leaves the head packet and outputs intact. */
MW_TEST_CASE(HostLoopbackTooSmallDestinationRetainsHeadPacket)
{
	FHostLoopback<1, 4> Loopback;

	const std::array<std::uint8_t, 3> HeadPacket{0x01, 0x02, 0x03};
	Loopback.TrySend(TSpan<const std::uint8_t>(HeadPacket.data(), HeadPacket.size()));

	std::array<std::uint8_t, 2> TooSmall{0xFF, 0xFF};
	FNetReceiveResult ReceiveResult{std::size_t{0xEE}};
	const ENetResult SmallResult = Loopback.TryReceive(TSpan<std::uint8_t>(TooSmall.data(), TooSmall.size()), ReceiveResult);

	MW_EXPECT_EQ(Test, ENetResult::Full, SmallResult, "A destination too small for the head must return Full");
	MW_EXPECT_EQ(Test, static_cast<std::size_t>(0xEE), ReceiveResult.BytesReceived, "Failed receive must leave BytesReceived unchanged");
	MW_EXPECT_EQ(Test, static_cast<std::uint8_t>(0xFF), TooSmall[0], "Failed receive must not modify the destination");
	MW_EXPECT_EQ(Test, static_cast<std::size_t>(1), Loopback.QueuedCountValue(), "Too-small receive must retain the head packet");

	std::array<std::uint8_t, 4> LargeDestination{};
	FNetReceiveResult RetryResult{std::size_t{0xEE}};
	const ENetResult RetrySendResult = Loopback.TryReceive(TSpan<std::uint8_t>(LargeDestination.data(), LargeDestination.size()), RetryResult);
	MW_EXPECT_EQ(Test, ENetResult::Success, RetrySendResult, "Retry with a larger destination must succeed");
	MW_EXPECT_EQ(Test, static_cast<std::size_t>(3), RetryResult.BytesReceived, "Retained head must deliver its original length");
	MW_EXPECT_EQ(Test, static_cast<std::uint8_t>(0x01), LargeDestination[0], "Retained head must deliver its original bytes");
}

/** Proves Drain empties the queue so capacity can be reused. */
MW_TEST_CASE(HostLoopbackDrainRestoresCapacityForReuse)
{
	FHostLoopback<2, 2> Loopback;

	const std::array<std::uint8_t, 2> FirstPacket{0x11, 0x22};
	const std::array<std::uint8_t, 2> SecondPacket{0x33, 0x44};
	Loopback.TrySend(TSpan<const std::uint8_t>(FirstPacket.data(), FirstPacket.size()));
	Loopback.TrySend(TSpan<const std::uint8_t>(SecondPacket.data(), SecondPacket.size()));
	MW_EXPECT_EQ(Test, true, Loopback.IsFull(), "Two sends must fill the two-slot queue");

	Loopback.Drain();
	MW_EXPECT_EQ(Test, true, Loopback.IsEmpty(), "Drain must empty the queue");
	MW_EXPECT_EQ(Test, static_cast<std::size_t>(0), Loopback.QueuedCountValue(), "Drain must reset the queued count");

	const std::array<std::uint8_t, 2> ReusePacket{0x55, 0x66};
	MW_EXPECT_EQ(
		Test,
		ENetResult::Success,
		Loopback.TrySend(TSpan<const std::uint8_t>(ReusePacket.data(), ReusePacket.size())),
		"Send after drain must reuse the freed capacity");
}

/** Proves a zero-length packet is enqueued and delivered as a zero-byte receive. */
MW_TEST_CASE(HostLoopbackAcceptsZeroLengthPacketRoundTrip)
{
	FHostLoopback<1, 2> Loopback;

	const ENetResult ZeroSendResult = Loopback.TrySend(TSpan<const std::uint8_t>(nullptr, 0));
	MW_EXPECT_EQ(Test, ENetResult::Success, ZeroSendResult, "A zero-length send must succeed as a valid no-op");
	MW_EXPECT_EQ(Test, static_cast<std::size_t>(1), Loopback.QueuedCountValue(), "Zero-length send must still occupy one slot");

	std::array<std::uint8_t, 2> Destination{0xFF, 0xFF};
	FNetReceiveResult ReceiveResult{};
	const ENetResult ZeroReceiveResult = Loopback.TryReceive(TSpan<std::uint8_t>(Destination.data(), Destination.size()), ReceiveResult);
	MW_EXPECT_EQ(Test, ENetResult::Success, ZeroReceiveResult, "Receive of a queued zero-length packet must succeed");
	MW_EXPECT_EQ(Test, static_cast<std::size_t>(0), ReceiveResult.BytesReceived, "Zero-length receive must report zero bytes");
	MW_EXPECT_EQ(Test, static_cast<std::uint8_t>(0xFF), Destination[0], "Zero-length receive must not modify the destination");
}

/** Proves an oversized packet is rejected as Invalid without queueing. */
MW_TEST_CASE(HostLoopbackRejectsOversizedPacket)
{
	FHostLoopback<2, 2> Loopback;

	const std::array<std::uint8_t, 4> Oversized{0x01, 0x02, 0x03, 0x04};
	const ENetResult OversizedResult = Loopback.TrySend(TSpan<const std::uint8_t>(Oversized.data(), Oversized.size()));
	MW_EXPECT_EQ(Test, ENetResult::Invalid, OversizedResult, "A packet larger than MaximumPacketBytes must return Invalid");
	MW_EXPECT_EQ(Test, true, Loopback.IsEmpty(), "Oversized send must not queue a packet");
}

/** Proves a null packet with nonzero length is rejected as Invalid without queueing. */
MW_TEST_CASE(HostLoopbackRejectsNullPacketWithNonzeroLength)
{
	FHostLoopback<2, 4> Loopback;

	const ENetResult NullResult = Loopback.TrySend(TSpan<const std::uint8_t>(nullptr, 2));
	MW_EXPECT_EQ(Test, ENetResult::Invalid, NullResult, "Null data with nonzero length must return Invalid");
	MW_EXPECT_EQ(Test, true, Loopback.IsEmpty(), "Invalid send must not queue a packet");
}

/**
 * Proves a null destination with nonzero length returns Invalid even when the loopback is empty,
 * and that this transactional rejection leaves the destination, BytesReceived, and queue state unchanged.
 */
MW_TEST_CASE(HostLoopbackEmptyReceiveNullDestinationReturnsInvalid)
{
	FHostLoopback<2, 4> Loopback;
	MW_EXPECT_EQ(Test, true, Loopback.IsEmpty(), "Precondition: the loopback must start empty");

	// Sentinel output bytes and BytesReceived so an unchanged failure is provable.
	std::array<std::uint8_t, 4> Destination{0xFF, 0xFF, 0xFF, 0xFF};
	FNetReceiveResult ReceiveResult{std::size_t{0xEE}};

	const ENetResult NullResult = Loopback.TryReceive(TSpan<std::uint8_t>(nullptr, 4), ReceiveResult);

	MW_EXPECT_EQ(Test, ENetResult::Invalid, NullResult, "A null destination with nonzero length must return Invalid even on an empty loopback");
	MW_EXPECT_EQ(Test, static_cast<std::size_t>(0xEE), ReceiveResult.BytesReceived, "Invalid receive must leave BytesReceived unchanged");
	MW_EXPECT_EQ(Test, true, Loopback.IsEmpty(), "Invalid receive must not change the queue state");
	MW_EXPECT_EQ(Test, static_cast<std::size_t>(0), Loopback.QueuedCountValue(), "Invalid receive must not change the queued count");

	// The caller-supplied sentinel destination storage must be untouched even though the loopback owns no packet to copy.
	MW_EXPECT_EQ(Test, static_cast<std::uint8_t>(0xFF), Destination[0], "Invalid receive must not modify the destination");
	MW_EXPECT_EQ(Test, static_cast<std::uint8_t>(0xFF), Destination[3], "Invalid receive must not modify the destination tail");
}

/**
 * Proves a null destination with nonzero length returns Invalid even when the loopback has a queued head,
 * and that the head packet survives for a later valid retry.
 */
MW_TEST_CASE(HostLoopbackNullDestinationRetainsHeadPacketAndOutputs)
{
	FHostLoopback<1, 4> Loopback;

	const std::array<std::uint8_t, 2> HeadPacket{0x11, 0x22};
	Loopback.TrySend(TSpan<const std::uint8_t>(HeadPacket.data(), HeadPacket.size()));
	MW_EXPECT_EQ(Test, static_cast<std::size_t>(1), Loopback.QueuedCountValue(), "Precondition: the head packet must be queued");

	std::array<std::uint8_t, 4> Destination{0xFF, 0xFF, 0xFF, 0xFF};
	FNetReceiveResult ReceiveResult{std::size_t{0xEE}};

	const ENetResult NullResult = Loopback.TryReceive(TSpan<std::uint8_t>(nullptr, 4), ReceiveResult);

	MW_EXPECT_EQ(Test, ENetResult::Invalid, NullResult, "A null destination with nonzero length must return Invalid even with a queued head");
	MW_EXPECT_EQ(Test, static_cast<std::size_t>(0xEE), ReceiveResult.BytesReceived, "Invalid receive must leave BytesReceived unchanged");
	MW_EXPECT_EQ(Test, static_cast<std::uint8_t>(0xFF), Destination[0], "Invalid receive must not modify the destination");
	MW_EXPECT_EQ(Test, static_cast<std::size_t>(1), Loopback.QueuedCountValue(), "Invalid receive must retain the head packet");

	// The retained head must still be deliverable to a valid destination.
	std::array<std::uint8_t, 4> RetryDestination{0};
	FNetReceiveResult RetryResult{std::size_t{0xEE}};
	const ENetResult RetryResultValue = Loopback.TryReceive(TSpan<std::uint8_t>(RetryDestination.data(), RetryDestination.size()), RetryResult);
	MW_EXPECT_EQ(Test, ENetResult::Success, RetryResultValue, "Retained head must be deliverable to a valid destination");
	MW_EXPECT_EQ(Test, static_cast<std::size_t>(2), RetryResult.BytesReceived, "Retained head must deliver its original length");
	MW_EXPECT_EQ(Test, static_cast<std::uint8_t>(0x11), RetryDestination[0], "Retained head must deliver its original first byte");
}

/** Proves the loopback satisfies the INetDriver interface so a driver reference is usable. */
MW_TEST_CASE(HostLoopbackSatisfiesINetDriverInterface)
{
	FHostLoopback<1, 4> Loopback;
	INetDriver& Driver = Loopback;

	const std::array<std::uint8_t, 2> Packet{0x07, 0x08};
	MW_EXPECT_EQ(
		Test,
		ENetResult::Success,
		Driver.TrySend(TSpan<const std::uint8_t>(Packet.data(), Packet.size())),
		"Interface send must route to the loopback");

	std::array<std::uint8_t, 4> Destination{};
	FNetReceiveResult ReceiveResult{};
	MW_EXPECT_EQ(
		Test,
		ENetResult::Success,
		Driver.TryReceive(TSpan<std::uint8_t>(Destination.data(), Destination.size()), ReceiveResult),
		"Interface receive must route to the loopback");
	MW_EXPECT_EQ(Test, static_cast<std::size_t>(2), ReceiveResult.BytesReceived, "Interface receive must report the head packet length");
}

} // namespace
