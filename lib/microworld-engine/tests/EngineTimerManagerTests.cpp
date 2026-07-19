#include "EngineAllocationCounters.h"
#include "TestSupport.h"

#include <MicroWorld/Engine/Timer.h>
#include <MicroWorld/Time.h>

#include <MicroWorld/Delegates/Delegate.h>

#include <cstdint>
#include <limits>

namespace
{
using MicroWorld::CanAdvanceTimerGeneration;
using MicroWorld::DurationMilliseconds;
using MicroWorld::ETimerMode;
using MicroWorld::ETimerResult;
using MicroWorld::FTimerHandle;
using MicroWorld::TDelegate;
using MicroWorld::TimePointMilliseconds;
using MicroWorld::TTimerManager;
using MicroWorld::Tests::GlobalAllocationCount;

/** Inline callback storage shared by every timer test so capturing lambdas fit one fixed size. */
constexpr std::size_t TestInlineCallbackBytes = 64;

/** Capacity large enough for ordering and mutation tests without masking capacity behavior. */
constexpr std::size_t TestTimerCapacity = 4;

using FTestManager = TTimerManager<TestTimerCapacity, TestInlineCallbackBytes>;
using FTestDelegate = TDelegate<void(), TestInlineCallbackBytes>;

/** Counts callback invocations for one timer without allocating. */
struct FFireCounter final
{
	/** Records one observed callback invocation. */
	std::uint32_t Count{0};
};

/** Binds a nothrow inline callback that increments the supplied counter when invoked. */
FTestDelegate MakeCounterCallback(FFireCounter& Counter) noexcept
{
	FTestDelegate Delegate;
	(void)Delegate.Bind([&Counter]() noexcept { ++Counter.Count; });
	return Delegate;
}

// ---------------------------------------------------------------------------
// Category 1: One-shot scheduling
// ---------------------------------------------------------------------------

/** Proves a one-shot timer does not fire before its deadline. */
MW_TEST_CASE(EngineTimerOneShotDoesNotFireBeforeDeadline)
{
	FFireCounter Counter;
	FTestManager Manager{1000};
	FTimerHandle Handle{};

	const ETimerResult Result = Manager.Schedule(MakeCounterCallback(Counter), 100, ETimerMode::OneShot, Handle);

	MW_EXPECT_EQ(Test, ETimerResult::Success, Result, "A valid one-shot schedule should succeed");
	MW_EXPECT_TRUE(Test, Handle.IsValid(), "A successful schedule publishes a valid handle");

	const ETimerResult AdvanceResult = Manager.Advance(1050);
	MW_EXPECT_EQ(Test, ETimerResult::Success, AdvanceResult, "Advance before deadline should succeed");
	MW_EXPECT_EQ(Test, 0u, Counter.Count, "A one-shot timer must not fire before its deadline");
}

/** Proves a one-shot timer fires exactly once when its deadline arrives. */
MW_TEST_CASE(EngineTimerOneShotFiresOnceWhenDue)
{
	FFireCounter Counter;
	FTestManager Manager{0};
	FTimerHandle Handle{};

	(void)Manager.Schedule(MakeCounterCallback(Counter), 100, ETimerMode::OneShot, Handle);
	(void)Manager.Advance(100);

	MW_EXPECT_EQ(Test, 1u, Counter.Count, "A one-shot timer should fire exactly once when due");
}

/** Proves a fired one-shot timer is removed and no longer occupies a slot. */
MW_TEST_CASE(EngineTimerOneShotIsRemovedAfterFiring)
{
	FFireCounter Counter;
	FTestManager Manager{0};
	FTimerHandle Handle{};

	(void)Manager.Schedule(MakeCounterCallback(Counter), 100, ETimerMode::OneShot, Handle);
	(void)Manager.Advance(100);

	MW_EXPECT_EQ(Test, 0u, Manager.TimerCount(), "A fired one-shot timer should leave no occupied slot");
}

/** Proves a fired one-shot timer's handle becomes stale. */
MW_TEST_CASE(EngineTimerOneShotHandleBecomesStaleAfterFiring)
{
	FFireCounter Counter;
	FTestManager Manager{0};
	FTimerHandle Handle{};

	(void)Manager.Schedule(MakeCounterCallback(Counter), 100, ETimerMode::OneShot, Handle);
	(void)Manager.Advance(100);

	const ETimerResult CancelResult = Manager.Cancel(Handle);
	MW_EXPECT_EQ(Test, ETimerResult::StaleHandle, CancelResult, "A fired one-shot handle must be stale");
}

// ---------------------------------------------------------------------------
// Category 2: Looping scheduling
// ---------------------------------------------------------------------------

/** Proves a looping timer fires on its repeat cadence. */
MW_TEST_CASE(EngineTimerLoopingFiresOnCadence)
{
	FFireCounter Counter;
	FTestManager Manager{0};
	FTimerHandle Handle{};

	(void)Manager.Schedule(MakeCounterCallback(Counter), 100, ETimerMode::Looping, Handle);
	(void)Manager.Advance(100);
	(void)Manager.Advance(200);
	(void)Manager.Advance(300);

	MW_EXPECT_EQ(Test, 3u, Counter.Count, "A looping timer should fire once per period step");
}

/** Proves a looping timer fires at most once per Advance even when its deadline is far overdue. */
MW_TEST_CASE(EngineTimerLoopingFiresAtMostOncePerAdvance)
{
	FFireCounter Counter;
	FTestManager Manager{0};
	FTimerHandle Handle{};

	(void)Manager.Schedule(MakeCounterCallback(Counter), 100, ETimerMode::Looping, Handle);
	(void)Manager.Advance(100);
	(void)Manager.Advance(500);

	MW_EXPECT_EQ(Test, 2u, Counter.Count, "A delayed Advance must not produce a catch-up burst");
}

/** Proves a looping timer's next deadline is computed from the actual accepted Now, not its previous deadline. */
MW_TEST_CASE(EngineTimerLoopingNextDeadlineUsesActualNow)
{
	FFireCounter Counter;
	FTestManager Manager{0};
	FTimerHandle Handle{};

	(void)Manager.Schedule(MakeCounterCallback(Counter), 100, ETimerMode::Looping, Handle);
	(void)Manager.Advance(100);
	(void)Manager.Advance(250);

	// After firing at Now=250 with period 100, the Now-based next deadline is 350; a previous-deadline
	// based design would set 300 and refire here.
	(void)Manager.Advance(300);
	MW_EXPECT_EQ(Test, 2u, Counter.Count, "The looping deadline must advance from actual Now, not the previous deadline");
	(void)Manager.Advance(350);
	MW_EXPECT_EQ(Test, 3u, Counter.Count, "The looping timer refires at the actual-Now-derived deadline");
}

/** Proves a zero-period looping timer fires once per Advance at the same timestamp. */
MW_TEST_CASE(EngineTimerZeroPeriodLoopingFiresOncePerAdvance)
{
	FFireCounter Counter;
	FTestManager Manager{1000};
	FTimerHandle Handle{};

	(void)Manager.Schedule(MakeCounterCallback(Counter), 0, ETimerMode::Looping, Handle);
	(void)Manager.Advance(1000);
	(void)Manager.Advance(1000);

	MW_EXPECT_EQ(Test, 2u, Counter.Count, "A zero-period looping timer fires once per Advance, including at the same timestamp");
}

/** Proves a nonzero-period looping timer does not refire at the same saturated timestamp. */
MW_TEST_CASE(EngineTimerNonzeroPeriodLoopingDoesNotRepeatAtSaturatedTimestamp)
{
	FFireCounter Counter;
	const TimePointMilliseconds SaturatedNow = std::numeric_limits<TimePointMilliseconds>::max();
	FTestManager Manager{SaturatedNow};
	FTimerHandle Handle{};

	const ETimerResult ScheduleResult = Manager.Schedule(MakeCounterCallback(Counter), 100, ETimerMode::Looping, Handle);
	MW_EXPECT_EQ(Test, ETimerResult::Success, ScheduleResult, "Scheduling near saturation should succeed");

	const ETimerResult FirstAdvance = Manager.Advance(SaturatedNow);
	const ETimerResult SecondAdvance = Manager.Advance(SaturatedNow);
	MW_EXPECT_EQ(Test, ETimerResult::Success, FirstAdvance, "Advance at saturation should succeed");
	MW_EXPECT_EQ(Test, ETimerResult::Success, SecondAdvance, "A repeated Advance at the same saturated Now should succeed without refiring");

	MW_EXPECT_EQ(Test, 1u, Counter.Count, "A nonzero-period looping timer must not repeat at the same saturated timestamp");
}

// ---------------------------------------------------------------------------
// Category 3: Cancellation
// ---------------------------------------------------------------------------

/** Proves cancellation before the deadline prevents any invocation. */
MW_TEST_CASE(EngineTimerCancellationBeforeDuePreventsInvocation)
{
	FFireCounter Counter;
	FTestManager Manager{0};
	FTimerHandle Handle{};

	(void)Manager.Schedule(MakeCounterCallback(Counter), 100, ETimerMode::OneShot, Handle);
	const ETimerResult CancelResult = Manager.Cancel(Handle);
	(void)Manager.Advance(200);

	MW_EXPECT_EQ(Test, ETimerResult::Success, CancelResult, "Cancellation of an active timer should succeed");
	MW_EXPECT_EQ(Test, 0u, Counter.Count, "A canceled timer must not fire");
}

/** Proves successful cancellation reduces observable occupancy. */
MW_TEST_CASE(EngineTimerCancellationReducesOccupancy)
{
	FFireCounter Counter;
	FTestManager Manager{0};
	FTimerHandle Handle{};

	(void)Manager.Schedule(MakeCounterCallback(Counter), 100, ETimerMode::OneShot, Handle);
	MW_EXPECT_EQ(Test, 1u, Manager.TimerCount(), "One schedule should occupy one slot");
	(void)Manager.Cancel(Handle);
	MW_EXPECT_EQ(Test, 0u, Manager.TimerCount(), "Cancellation should release the slot");
}

/** Proves repeated cancellation of the same handle returns StaleHandle. */
MW_TEST_CASE(EngineTimerRepeatedCancellationReturnsStaleHandle)
{
	FFireCounter Counter;
	FTestManager Manager{0};
	FTimerHandle Handle{};

	(void)Manager.Schedule(MakeCounterCallback(Counter), 100, ETimerMode::OneShot, Handle);
	const ETimerResult FirstCancel = Manager.Cancel(Handle);
	const ETimerResult SecondCancel = Manager.Cancel(Handle);

	MW_EXPECT_EQ(Test, ETimerResult::Success, FirstCancel, "The first cancellation should succeed");
	MW_EXPECT_EQ(Test, ETimerResult::StaleHandle, SecondCancel, "A repeated cancellation must return StaleHandle");
}

/** Proves cancellation after one-shot completion returns StaleHandle. */
MW_TEST_CASE(EngineTimerCancellationAfterCompletionReturnsStaleHandle)
{
	FFireCounter Counter;
	FTestManager Manager{0};
	FTimerHandle Handle{};

	(void)Manager.Schedule(MakeCounterCallback(Counter), 100, ETimerMode::OneShot, Handle);
	(void)Manager.Advance(100);
	const ETimerResult CancelResult = Manager.Cancel(Handle);

	MW_EXPECT_EQ(Test, ETimerResult::StaleHandle, CancelResult, "Canceling a completed one-shot must return StaleHandle");
}

// ---------------------------------------------------------------------------
// Category 4: Handles and generation safety
// ---------------------------------------------------------------------------

/** Proves default, sentinel, and out-of-range handles return InvalidHandle. */
MW_TEST_CASE(EngineTimerInvalidHandleIndicesRejected)
{
	FTestManager Manager{0};

	const FTimerHandle DefaultHandle{};
	const FTimerHandle SentinelHandle{FTimerHandle::InvalidIndex, 1u};
	const FTimerHandle OutOfRangeHandle{static_cast<std::uint16_t>(TestTimerCapacity + 1), 1u};

	MW_EXPECT_EQ(Test, ETimerResult::InvalidHandle, Manager.Cancel(DefaultHandle), "A default handle must be rejected as InvalidHandle");
	MW_EXPECT_EQ(Test, ETimerResult::InvalidHandle, Manager.Cancel(SentinelHandle), "A sentinel-index handle must be rejected as InvalidHandle");
	MW_EXPECT_EQ(Test, ETimerResult::InvalidHandle, Manager.Cancel(OutOfRangeHandle), "An out-of-range handle must be rejected as InvalidHandle");
}

/** Proves a canceled handle and a generation-mismatched handle return StaleHandle. */
MW_TEST_CASE(EngineTimerStaleAndGenerationMismatchedHandlesRejected)
{
	FFireCounter Counter;
	FTestManager Manager{0};
	FTimerHandle Handle{};

	(void)Manager.Schedule(MakeCounterCallback(Counter), 100, ETimerMode::Looping, Handle);
	const std::uint16_t SlotIndex = Handle.Index;
	const std::uint32_t PublishedGeneration = Handle.Generation;

	(void)Manager.Cancel(Handle);
	const FTimerHandle CanceledHandle{SlotIndex, PublishedGeneration};
	const FTimerHandle MismatchedHandle{SlotIndex, PublishedGeneration + 1u};

	MW_EXPECT_EQ(Test, ETimerResult::StaleHandle, Manager.Cancel(CanceledHandle), "A canceled handle must return StaleHandle");
	MW_EXPECT_EQ(Test, ETimerResult::StaleHandle, Manager.Cancel(MismatchedHandle), "A generation-mismatched handle must return StaleHandle");
}

/** Proves slot reuse publishes a different generation than the retired handle. */
MW_TEST_CASE(EngineTimerSlotReusePublishesDifferentGeneration)
{
	FFireCounter Counter;
	FTestManager Manager{0};
	FTimerHandle FirstHandle{};

	(void)Manager.Schedule(MakeCounterCallback(Counter), 100, ETimerMode::OneShot, FirstHandle);
	(void)Manager.Cancel(FirstHandle);

	FTimerHandle SecondHandle{};
	(void)Manager.Schedule(MakeCounterCallback(Counter), 100, ETimerMode::OneShot, SecondHandle);

	MW_EXPECT_TRUE(Test, FirstHandle.Index == SecondHandle.Index, "A freed slot should be reused first");
	MW_EXPECT_TRUE(Test, FirstHandle.Generation != SecondHandle.Generation, "Reused slot must publish a different generation");
}

/** Proves a stale handle cannot cancel a slot's replacement timer. */
MW_TEST_CASE(EngineTimerStaleHandleCannotAffectReplacement)
{
	FFireCounter Counter;
	FTestManager Manager{0};
	FTimerHandle FirstHandle{};

	(void)Manager.Schedule(MakeCounterCallback(Counter), 100, ETimerMode::Looping, FirstHandle);
	(void)Manager.Cancel(FirstHandle);

	FTimerHandle SecondHandle{};
	(void)Manager.Schedule(MakeCounterCallback(Counter), 100, ETimerMode::Looping, SecondHandle);
	MW_EXPECT_EQ(Test, 1u, Manager.TimerCount(), "The replacement should occupy one slot");

	const ETimerResult StaleCancel = Manager.Cancel(FirstHandle);
	MW_EXPECT_EQ(Test, ETimerResult::StaleHandle, StaleCancel, "The retired handle must not cancel the replacement");
	MW_EXPECT_EQ(Test, 1u, Manager.TimerCount(), "A stale cancel must not change occupancy");

	const ETimerResult LiveCancel = Manager.Cancel(SecondHandle);
	MW_EXPECT_EQ(Test, ETimerResult::Success, LiveCancel, "The replacement handle should still cancel successfully");
}

/** Proves the generation helper refuses wrap at the type maximum and accepts every earlier value. */
MW_TEST_CASE(EngineTimerGenerationHelperRefusesWrap)
{
	constexpr bool AtZero = CanAdvanceTimerGeneration(0u);
	constexpr bool AtFirst = CanAdvanceTimerGeneration(1u);
	constexpr bool BeforeWrap = CanAdvanceTimerGeneration(std::numeric_limits<std::uint32_t>::max() - 1u);
	constexpr bool AtWrap = CanAdvanceTimerGeneration(std::numeric_limits<std::uint32_t>::max());

	MW_EXPECT_TRUE(Test, AtZero, "Generation zero must be advanceable");
	MW_EXPECT_TRUE(Test, AtFirst, "Generation one must be advanceable");
	MW_EXPECT_TRUE(Test, BeforeWrap, "The last finite generation must be advanceable");
	MW_EXPECT_TRUE(Test, !AtWrap, "The maximum generation must refuse to advance and trigger retirement");
}

// ---------------------------------------------------------------------------
// Category 5: Capacity and invalid input
// ---------------------------------------------------------------------------

/** Proves a zero-capacity manager reports CapacityExceeded on every schedule attempt. */
MW_TEST_CASE(EngineTimerZeroCapacityRejectsSchedule)
{
	TTimerManager<0, TestInlineCallbackBytes> Manager{0};
	FFireCounter Counter;
	FTimerHandle Handle{};

	const ETimerResult Result = Manager.Schedule(MakeCounterCallback(Counter), 1, ETimerMode::OneShot, Handle);

	MW_EXPECT_EQ(Test, ETimerResult::CapacityExceeded, Result, "A zero-capacity manager must reject scheduling");
	MW_EXPECT_TRUE(Test, !Handle.IsValid(), "A failed schedule must clear the output handle");
}

/** Proves a full manager returns CapacityExceeded without consuming the supplied callback. */
MW_TEST_CASE(EngineTimerFullManagerPreservesCallbackOnFailure)
{
	TTimerManager<2, TestInlineCallbackBytes> Manager{0};
	FFireCounter FirstCounter;
	FFireCounter SecondCounter;
	FFireCounter ThirdCounter;
	FTimerHandle FirstHandle{};
	FTimerHandle SecondHandle{};
	FTimerHandle ThirdHandle{};

	(void)Manager.Schedule(MakeCounterCallback(FirstCounter), 100, ETimerMode::Looping, FirstHandle);
	(void)Manager.Schedule(MakeCounterCallback(SecondCounter), 100, ETimerMode::Looping, SecondHandle);

	FTestDelegate ThirdCallback = MakeCounterCallback(ThirdCounter);
	const ETimerResult Result = Manager.Schedule(std::move(ThirdCallback), 100, ETimerMode::OneShot, ThirdHandle);

	MW_EXPECT_EQ(Test, ETimerResult::CapacityExceeded, Result, "A full manager must report CapacityExceeded");
	MW_EXPECT_TRUE(Test, !ThirdHandle.IsValid(), "The failed schedule must clear the output handle");
	MW_EXPECT_TRUE(Test, ThirdCallback.IsBound(), "A rejected schedule must leave its input delegate bound to the caller");
	MW_EXPECT_EQ(Test, 2u, Manager.TimerCount(), "A failed schedule must not change occupancy");
}

/** Proves an unbound callback is rejected as InvalidCallback before any slot is consumed. */
MW_TEST_CASE(EngineTimerUnboundCallbackRejected)
{
	FTestManager Manager{0};
	FTestDelegate Unbound;
	FTimerHandle Handle{};

	const ETimerResult Result = Manager.Schedule(std::move(Unbound), 100, ETimerMode::OneShot, Handle);

	MW_EXPECT_EQ(Test, ETimerResult::InvalidCallback, Result, "An unbound delegate must be rejected");
	MW_EXPECT_TRUE(Test, !Handle.IsValid(), "The failed schedule must clear the output handle");
	MW_EXPECT_EQ(Test, 0u, Manager.TimerCount(), "Invalid callback rejection must not change occupancy");
}

/** Proves an invalid mode is rejected before any slot is consumed. */
MW_TEST_CASE(EngineTimerInvalidModeRejected)
{
	FFireCounter Counter;
	FTestManager Manager{0};
	FTimerHandle Handle{};

	const ETimerResult Result = Manager.Schedule(MakeCounterCallback(Counter), 100, ETimerMode::None, Handle);

	MW_EXPECT_EQ(Test, ETimerResult::InvalidMode, Result, "The None mode must be rejected");
	MW_EXPECT_TRUE(Test, !Handle.IsValid(), "The failed schedule must clear the output handle");
	MW_EXPECT_EQ(Test, 0u, Manager.TimerCount(), "Invalid mode rejection must not change occupancy");
}

// ---------------------------------------------------------------------------
// Category 6: Caller-supplied time
// ---------------------------------------------------------------------------

/** Proves a zero-delay timer becomes due on the next Advance at InitialNow rather than firing synchronously. */
MW_TEST_CASE(EngineTimerZeroDelayBecomesDueOnNextAdvance)
{
	FFireCounter Counter;
	FTestManager Manager{1000};
	FTimerHandle Handle{};

	(void)Manager.Schedule(MakeCounterCallback(Counter), 0, ETimerMode::OneShot, Handle);
	MW_EXPECT_EQ(Test, 0u, Counter.Count, "Schedule must not synchronously invoke a zero-delay timer");

	(void)Manager.Advance(1000);
	MW_EXPECT_EQ(Test, 1u, Counter.Count, "A zero-delay timer must fire on the first Advance at InitialNow");
}

/** Proves a rolled-back Advance is rejected transactionally without changing any observable state. */
MW_TEST_CASE(EngineTimerRollbackAdvanceRejectedTransactionally)
{
	FFireCounter Counter;
	FTestManager Manager{0};
	FTimerHandle Handle{};

	// Deadline 200 keeps the timer not-yet-due after the first Advance so the rollback is isolated.
	(void)Manager.Schedule(MakeCounterCallback(Counter), 200, ETimerMode::OneShot, Handle);
	(void)Manager.Advance(100);

	const std::size_t OccupancyBeforeRollback = Manager.TimerCount();
	const ETimerResult RollbackResult = Manager.Advance(50);
	const std::size_t OccupancyAfterRollback = Manager.TimerCount();

	MW_EXPECT_EQ(Test, ETimerResult::NonMonotonicTime, RollbackResult, "A rolled-back Advance must return NonMonotonicTime");
	MW_EXPECT_EQ(Test, OccupancyBeforeRollback, OccupancyAfterRollback, "A rejected Advance must not change occupancy");
	MW_EXPECT_EQ(Test, 0u, Counter.Count, "A rejected Advance must not invoke any callback");
}

/** Proves first-deadline arithmetic saturates at the TimePointMilliseconds maximum. */
MW_TEST_CASE(EngineTimerFirstDeadlineSaturatesWithoutOverflow)
{
	FFireCounter Counter;
	const TimePointMilliseconds NearMaximum = std::numeric_limits<TimePointMilliseconds>::max() - 1u;
	const DurationMilliseconds HugeDuration = std::numeric_limits<DurationMilliseconds>::max();
	FTestManager Manager{NearMaximum};
	FTimerHandle Handle{};

	const ETimerResult ScheduleResult = Manager.Schedule(MakeCounterCallback(Counter), HugeDuration, ETimerMode::Looping, Handle);
	MW_EXPECT_EQ(Test, ETimerResult::Success, ScheduleResult, "Scheduling near saturation should succeed");

	const ETimerResult AdvanceResult = Manager.Advance(NearMaximum);
	MW_EXPECT_EQ(Test, ETimerResult::Success, AdvanceResult, "Advance at the saturated boundary should succeed");
	MW_EXPECT_EQ(Test, 0u, Counter.Count, "The saturated deadline must not be reached before the maximum timestamp");

	const ETimerResult MaximumAdvanceResult = Manager.Advance(std::numeric_limits<TimePointMilliseconds>::max());
	MW_EXPECT_EQ(Test, ETimerResult::Success, MaximumAdvanceResult, "Advance at the maximum timestamp should succeed");
	MW_EXPECT_EQ(Test, 1u, Counter.Count, "The saturated deadline should fire exactly once at the maximum timestamp");
}

// ---------------------------------------------------------------------------
// Category 7: Deterministic ordering
// ---------------------------------------------------------------------------

/** Records the identity of each callback in stable dispatch order without allocating. */
struct FDispatchOrderRecorder final
{
	/** Bounds the recorded sequence so the test fixtures stay allocation-free and fixed-size. */
	static constexpr std::size_t MaximumEntries = 8;

	/** Tracks the next write position so later reads observe insertion-order dispatch. */
	std::size_t Count{0};

	/** Stores the caller-supplied identity of each fired callback. */
	int Identities[MaximumEntries]{0};

	/** Appends one observed identity when space remains. */
	void Record(const int Identity) noexcept
	{
		if (Count < MaximumEntries)
		{
			Identities[Count] = Identity;
			++Count;
		}
	}
};

/** Binds a callback that records its identity in the shared recorder. */
FTestDelegate MakeOrderCallback(FDispatchOrderRecorder& Recorder, const int Identity) noexcept
{
	FTestDelegate Delegate;
	(void)Delegate.Bind([&Recorder, Identity]() noexcept { Recorder.Record(Identity); });
	return Delegate;
}

/** Proves simultaneously due timers fire in insertion order rather than slot order. */
MW_TEST_CASE(EngineTimerSimultaneouslyDueTimersFireInInsertionOrder)
{
	FDispatchOrderRecorder Recorder;
	FTestManager Manager{0};
	FTimerHandle HandleA{};
	FTimerHandle HandleB{};
	FTimerHandle HandleC{};

	(void)Manager.Schedule(MakeOrderCallback(Recorder, 1), 100, ETimerMode::OneShot, HandleA);
	(void)Manager.Schedule(MakeOrderCallback(Recorder, 2), 100, ETimerMode::OneShot, HandleB);
	(void)Manager.Schedule(MakeOrderCallback(Recorder, 3), 100, ETimerMode::OneShot, HandleC);
	(void)Manager.Advance(100);

	MW_EXPECT_EQ(Test, std::size_t{3}, Recorder.Count, "All three due timers should fire");
	MW_EXPECT_EQ(Test, 1, Recorder.Identities[0], "The first-inserted timer should fire first");
	MW_EXPECT_EQ(Test, 2, Recorder.Identities[1], "The second-inserted timer should fire second");
	MW_EXPECT_EQ(Test, 3, Recorder.Identities[2], "The third-inserted timer should fire third");
}

/** Proves canceling and reusing a slot appends the replacement at the tail of insertion order. */
MW_TEST_CASE(EngineTimerCancelReuseAppendsReplacementAtInsertionTail)
{
	FDispatchOrderRecorder Recorder;
	FTestManager Manager{0};
	FTimerHandle HandleA{};
	FTimerHandle HandleB{};
	FTimerHandle HandleC{};
	FTimerHandle HandleD{};

	(void)Manager.Schedule(MakeOrderCallback(Recorder, 1), 100, ETimerMode::OneShot, HandleA);
	(void)Manager.Schedule(MakeOrderCallback(Recorder, 2), 100, ETimerMode::OneShot, HandleB);
	(void)Manager.Schedule(MakeOrderCallback(Recorder, 3), 100, ETimerMode::OneShot, HandleC);
	(void)Manager.Cancel(HandleB);

	// The replacement reuses the freed slot but must dispatch after C, not between A and C.
	(void)Manager.Schedule(MakeOrderCallback(Recorder, 4), 100, ETimerMode::OneShot, HandleD);
	(void)Manager.Advance(100);

	MW_EXPECT_EQ(Test, std::size_t{3}, Recorder.Count, "Three timers should fire after the cancel/reuse");
	MW_EXPECT_EQ(Test, 1, Recorder.Identities[0], "A remains first in insertion order");
	MW_EXPECT_EQ(Test, 3, Recorder.Identities[1], "C retains its insertion position ahead of the reused slot");
	MW_EXPECT_EQ(Test, 4, Recorder.Identities[2], "The reused slot's replacement dispatches at the insertion tail");
}

// ---------------------------------------------------------------------------
// Category 8: Mutation rules during dispatch
// ---------------------------------------------------------------------------

/** Records the result of one attempted mutation performed from inside a callback. */
struct FCapturedMutation final
{
	/** Holds the result of the attempted in-callback operation. */
	ETimerResult Result{ETimerResult::Success};

	/** Remembers whether the captured operation has executed. */
	bool bObserved{false};
};

/** Proves a callback-issued Schedule returns DispatchLocked and preserves the supplied delegate. */
MW_TEST_CASE(EngineTimerCallbackScheduleRejectedAndDelegatePreserved)
{
	FTestManager Manager{0};
	FFireCounter FirstCounter;
	FFireCounter SecondCounter;
	FTimerHandle FirstHandle{};
	FTimerHandle RejectedHandle{};

	FTestDelegate RejectedDelegate = MakeCounterCallback(SecondCounter);
	FCapturedMutation CapturedSchedule{};

	FTestDelegate FirstCallback;
	(void)FirstCallback.Bind(
		[&Manager, &RejectedDelegate, &RejectedHandle, &CapturedSchedule]() noexcept
		{
			CapturedSchedule.Result = Manager.Schedule(std::move(RejectedDelegate), 100, ETimerMode::OneShot, RejectedHandle);
			CapturedSchedule.bObserved = true;
		});

	const ETimerResult ScheduleResult = Manager.Schedule(std::move(FirstCallback), 100, ETimerMode::OneShot, FirstHandle);
	MW_EXPECT_EQ(Test, ETimerResult::Success, ScheduleResult, "The observing timer should schedule successfully");

	(void)Manager.Advance(100);

	MW_EXPECT_TRUE(Test, CapturedSchedule.bObserved, "The callback should have executed");
	MW_EXPECT_EQ(Test, ETimerResult::DispatchLocked, CapturedSchedule.Result, "In-callback Schedule must return DispatchLocked");
	MW_EXPECT_TRUE(Test, !RejectedHandle.IsValid(), "The rejected in-callback schedule must clear its output handle");
	MW_EXPECT_TRUE(Test, RejectedDelegate.IsBound(), "The rejected in-callback schedule must leave its delegate bound to the caller");
	MW_EXPECT_EQ(Test, 0u, Manager.TimerCount(), "The rejected in-callback schedule must not change occupancy");
}

/** Proves in-callback cancellation is rejected while another due timer still fires later in the same Advance. */
MW_TEST_CASE(EngineTimerCallbackCancellationRejectedAndOtherTimerStillFires)
{
	FTestManager Manager{0};
	FDispatchOrderRecorder Recorder;
	FCapturedMutation SelfCancel{};
	FCapturedMutation OtherCancel{};
	FTimerHandle LoopingHandle{};
	FTimerHandle OneShotHandle{};

	FTestDelegate LoopingCallback;
	(void)LoopingCallback.Bind(
		[&Manager, &LoopingHandle, &OneShotHandle, &SelfCancel, &OtherCancel]() noexcept
		{
			SelfCancel.Result = Manager.Cancel(LoopingHandle);
			OtherCancel.Result = Manager.Cancel(OneShotHandle);
			SelfCancel.bObserved = true;
		});

	(void)Manager.Schedule(std::move(LoopingCallback), 100, ETimerMode::Looping, LoopingHandle);
	(void)Manager.Schedule(MakeOrderCallback(Recorder, 7), 100, ETimerMode::OneShot, OneShotHandle);
	(void)Manager.Advance(100);

	MW_EXPECT_TRUE(Test, SelfCancel.bObserved, "The looping callback should have executed");
	MW_EXPECT_EQ(Test, ETimerResult::DispatchLocked, SelfCancel.Result, "In-callback self-cancel must return DispatchLocked");
	MW_EXPECT_EQ(Test, ETimerResult::DispatchLocked, OtherCancel.Result, "In-callback other-cancel must return DispatchLocked");
	MW_EXPECT_EQ(Test, std::size_t{1}, Recorder.Count, "The other due timer must still fire after the rejected cancels");
	MW_EXPECT_EQ(Test, 7, Recorder.Identities[0], "The other due timer's identity should be recorded");
	MW_EXPECT_EQ(Test, 1u, Manager.TimerCount(), "The looping timer must remain active after rejected in-callback cancels");
}

/** Proves a nested Advance issued from a callback returns DispatchLocked without changing accepted time. */
MW_TEST_CASE(EngineTimerNestedAdvanceRejected)
{
	FTestManager Manager{0};
	FCapturedMutation NestedAdvance{};
	FTimerHandle Handle{};

	FTestDelegate Callback;
	(void)Callback.Bind(
		[&Manager, &NestedAdvance]() noexcept
		{
			NestedAdvance.Result = Manager.Advance(200);
			NestedAdvance.bObserved = true;
		});

	(void)Manager.Schedule(std::move(Callback), 100, ETimerMode::OneShot, Handle);
	(void)Manager.Advance(100);

	MW_EXPECT_TRUE(Test, NestedAdvance.bObserved, "The callback should have executed");
	MW_EXPECT_EQ(Test, ETimerResult::DispatchLocked, NestedAdvance.Result, "A nested Advance must return DispatchLocked");
}

// ---------------------------------------------------------------------------
// Category 9: Allocation-free steady-state operation
// ---------------------------------------------------------------------------

/** Proves Schedule, Advance dispatch, Cancel, slot reuse, and looping operation perform no observable allocation. */
MW_TEST_CASE(EngineTimerOperationsPerformNoObservableAllocation)
{
	FFireCounter Counter;
	FTestManager Manager{0};
	FTimerHandle OneShotHandle{};
	FTimerHandle LoopingHandle{};

	const std::uint32_t AllocationsBefore = GlobalAllocationCount;

	(void)Manager.Schedule(MakeCounterCallback(Counter), 100, ETimerMode::OneShot, OneShotHandle);
	(void)Manager.Schedule(MakeCounterCallback(Counter), 50, ETimerMode::Looping, LoopingHandle);
	(void)Manager.Advance(50);
	(void)Manager.Advance(100);
	(void)Manager.Cancel(LoopingHandle);

	FTimerHandle ReusedHandle{};
	(void)Manager.Schedule(MakeCounterCallback(Counter), 100, ETimerMode::OneShot, ReusedHandle);
	(void)Manager.Advance(100);

	const std::uint32_t AllocationsAfter = GlobalAllocationCount;

	MW_EXPECT_EQ(Test, AllocationsBefore, AllocationsAfter, "Timer schedule, dispatch, cancel, reuse, and looping must not allocate");
}

} // namespace
