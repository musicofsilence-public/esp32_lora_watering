#pragma once

#include <MicroWorld/Delegates/Delegate.h>
#include <MicroWorld/Time.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

namespace MicroWorld
{

/** Reports every bounded timer operation without borrowing unrelated lifecycle errors. */
enum class ETimerResult : std::uint8_t
{
	/** Confirms that the requested timer operation completed. */
	Success,

	/** Reports that no reusable timer slot remains, including zero capacity and retired generations. */
	CapacityExceeded,

	/** Rejects an unbound delegate before any slot is consumed or callback ownership moves. */
	InvalidCallback,

	/** Rejects a default, sentinel, or out-of-range handle before consulting slot state. */
	InvalidHandle,

	/** Rejects a handle whose slot is free, retired, removed, expired, or holds another generation. */
	StaleHandle,

	/** Rejects a timer mode that is neither OneShot nor Looping. */
	InvalidMode,

	/** Prevents Schedule, Cancel, and nested Advance from mutating an active dispatch. */
	DispatchLocked,

	/** Prevents unsigned time arithmetic from accepting a rolled-back caller clock. */
	NonMonotonicTime,
};

/** Selects one timer schedule shape independently of its bound callback. */
enum class ETimerMode : std::uint8_t
{
	/** Rejects scheduling so an uninitialized mode never silently becomes OneShot or Looping. */
	None,

	/** Fires once and removes the timer so its handle becomes stale. */
	OneShot,

	/** Reschedules from the accepted Now after each fire and stays in insertion order. */
	Looping,
};

/** Identifies one live timer without exposing storage or extending callback lifetime. */
struct FTimerHandle final
{
	/** Reserves the maximum index as the invalid sentinel independent of manager capacity. */
	static constexpr std::uint16_t InvalidIndex = std::numeric_limits<std::uint16_t>::max();

	/** Selects the fixed slot while preserving an explicit invalid sentinel. */
	std::uint16_t Index{InvalidIndex};

	/** Distinguishes successive schedules that occupy the same slot. */
	std::uint32_t Generation{0};

	/** Reports whether the value can identify a timer before consulting its owning manager. */
	constexpr bool IsValid() const noexcept { return Index != InvalidIndex && Generation != 0; }

	/** Compares the complete stable timer identity. */
	friend constexpr bool operator==(const FTimerHandle Left, const FTimerHandle Right) noexcept
	{
		return Left.Index == Right.Index && Left.Generation == Right.Generation;
	}

	/** Distinguishes handles whose slot or generation identity differs. */
	friend constexpr bool operator!=(const FTimerHandle Left, const FTimerHandle Right) noexcept { return !(Left == Right); }
};

/**
 * Confirms that one more live generation can be published without wrapping.
 *
 * A manager permanently retires the slot when this query is false; wrapping a
 * generation and making an old handle valid again is forbidden.
 */
constexpr bool CanAdvanceTimerGeneration(const std::uint32_t CurrentGeneration) noexcept
{
	return CurrentGeneration < std::numeric_limits<std::uint32_t>::max();
}

/**
 * Owns a bounded set of caller-scheduled timers with deterministic dispatch.
 *
 * The caller owns the manager value and supplies every clock reading; the
 * manager stores the last accepted time and never reads a hidden clock.
 */
template<std::size_t MaxTimers, std::size_t InlineCallbackBytes>
class TTimerManager final
{
	static_assert(MaxTimers < FTimerHandle::InvalidIndex, "A timer manager capacity must fit below the reserved handle index.");
	static_assert(InlineCallbackBytes > 0, "A timer manager must reserve inline callback storage for its delegates.");

public:
	/** Stores the caller's initial clock as the scheduling baseline for every later operation. */
	explicit TTimerManager(const TimePointMilliseconds InitialNow) noexcept : LastAcceptedNowMilliseconds{InitialNow} {}

	/** Prevents copying fixed slots and their uniquely owned inline callbacks. */
	TTimerManager(const TTimerManager&) = delete;

	/** Prevents copy assignment from duplicating timer identity and callback ownership. */
	TTimerManager& operator=(const TTimerManager&) = delete;

	/** Keeps timer slot addresses and insertion-order storage stable for handle identity. */
	TTimerManager(TTimerManager&&) = delete;

	/** Keeps timer slot addresses and insertion-order storage stable for handle identity. */
	TTimerManager& operator=(TTimerManager&&) = delete;

	/** Destroys every bound callback without invoking any of them. */
	~TTimerManager() noexcept
	{
		for (std::size_t SlotIndex = 0; SlotIndex < MaxTimers; ++SlotIndex)
		{
			Slots[SlotIndex].Callback.Reset();
		}
	}

	/**
	 * Schedules one bound delegate using a single duration as first delay and repeat period.
	 *
	 * Failure clears OutHandle and leaves Callback bound; success moves the
	 * delegate into a reusable slot and publishes a fresh generation-checked handle.
	 */
	ETimerResult Schedule(
		TDelegate<void(), InlineCallbackBytes>&& Callback,
		const DurationMilliseconds Duration,
		const ETimerMode Mode,
		FTimerHandle& OutHandle) noexcept
	{
		OutHandle = {};
		if (bDispatchActive)
		{
			return ETimerResult::DispatchLocked;
		}
		if (Mode == ETimerMode::None)
		{
			return ETimerResult::InvalidMode;
		}
		if (!Callback.IsBound())
		{
			return ETimerResult::InvalidCallback;
		}

		FTimerSlot* const AvailableSlot = FindAvailableSlot();
		if (AvailableSlot == nullptr)
		{
			return ETimerResult::CapacityExceeded;
		}

		const std::size_t SlotIndex = static_cast<std::size_t>(AvailableSlot - Slots);
		AvailableSlot->Callback = std::move(Callback);
		AvailableSlot->DeadlineMilliseconds = SaturatingAdd(LastAcceptedNowMilliseconds, Duration);
		AvailableSlot->PeriodMilliseconds = (Mode == ETimerMode::Looping) ? Duration : DurationMilliseconds{0};
		AvailableSlot->LastFiredMilliseconds = TimePointMilliseconds{0};
		AvailableSlot->Mode = Mode;
		AvailableSlot->bActive = true;

		const FTimerHandle PublishedHandle{static_cast<std::uint16_t>(SlotIndex), AvailableSlot->Generation};
		InsertionOrder[ActiveTimerCount] = PublishedHandle;
		++ActiveTimerCount;
		OutHandle = PublishedHandle;
		return ETimerResult::Success;
	}

	/** Removes exactly the timer identified by a current generation-checked handle. */
	ETimerResult Cancel(const FTimerHandle Handle) noexcept
	{
		if (bDispatchActive)
		{
			return ETimerResult::DispatchLocked;
		}
		if (!Handle.IsValid() || static_cast<std::size_t>(Handle.Index) >= MaxTimers)
		{
			return ETimerResult::InvalidHandle;
		}

		FTimerSlot& Slot = Slots[Handle.Index];
		if (!Slot.bActive || Slot.Generation != Handle.Generation)
		{
			return ETimerResult::StaleHandle;
		}

		RemoveActiveSlot(Slot, Handle);
		return ETimerResult::Success;
	}

	/**
	 * Fires each timer due at the caller-supplied time in stable insertion order.
	 *
	 * A rolled-back clock is rejected transactionally; a nested Advance is
	 * rejected while another dispatch is still active.
	 */
	ETimerResult Advance(const TimePointMilliseconds Now) noexcept
	{
		if (bDispatchActive)
		{
			return ETimerResult::DispatchLocked;
		}
		if (Now < LastAcceptedNowMilliseconds)
		{
			return ETimerResult::NonMonotonicTime;
		}
		LastAcceptedNowMilliseconds = Now;

		const std::size_t InitialCount = ActiveTimerCount;
		for (std::size_t SnapshotIndex = 0; SnapshotIndex < InitialCount; ++SnapshotIndex)
		{
			DispatchSnapshot[SnapshotIndex] = InsertionOrder[SnapshotIndex];
		}

		bDispatchActive = true;
		for (std::size_t SnapshotIndex = 0; SnapshotIndex < InitialCount; ++SnapshotIndex)
		{
			const FTimerHandle Handle = DispatchSnapshot[SnapshotIndex];
			if (static_cast<std::size_t>(Handle.Index) >= MaxTimers)
			{
				continue;
			}

			FTimerSlot& Slot = Slots[Handle.Index];
			if (!Slot.bActive || Slot.Generation != Handle.Generation)
			{
				continue;
			}
			if (Now < Slot.DeadlineMilliseconds)
			{
				continue;
			}
			// Guards a nonzero-period looping timer against refiring when Now has not advanced
			// past the previously accepted timestamp, including after deadline saturation.
			if (Slot.PeriodMilliseconds != 0 && Slot.LastFiredMilliseconds == Now)
			{
				continue;
			}

			// Execute is noexcept and succeeds for every active slot because Reset is reached
			// only through removal paths that this dispatch lock blocks for caller mutations.
			(void)Slot.Callback.Execute();

			if (Slot.Mode == ETimerMode::OneShot)
			{
				RemoveActiveSlot(Slot, Handle);
			}
			else
			{
				if (Slot.PeriodMilliseconds != 0)
				{
					Slot.LastFiredMilliseconds = Now;
				}
				Slot.DeadlineMilliseconds = SaturatingAdd(Now, Slot.PeriodMilliseconds);
			}
		}
		bDispatchActive = false;
		return ETimerResult::Success;
	}

	/** Reports the exact number of timers that the next successful Advance may visit. */
	std::size_t TimerCount() const noexcept { return ActiveTimerCount; }

	/** Reports the compile-time upper bound on live timers and dispatch work. */
	static constexpr std::size_t Capacity() noexcept { return MaxTimers; }

private:
	/** Owns one reusable inline callback plus its schedule and identity state. */
	struct FTimerSlot final
	{
		/** Owns the callable only while this slot is active. */
		TDelegate<void(), InlineCallbackBytes> Callback;

		/** Stores the absolute time at which this timer next becomes due. */
		TimePointMilliseconds DeadlineMilliseconds{0};

		/** Stores the looping repeat period; zero marks one-shot or zero-period looping. */
		DurationMilliseconds PeriodMilliseconds{0};

		/** Guards nonzero-period looping timers against refiring at the same accepted Now. */
		TimePointMilliseconds LastFiredMilliseconds{0};

		/** Distinguishes successive schedules that occupy this slot. */
		std::uint32_t Generation{1};

		/** Records the schedule shape that owns this slot's removal or reschedule behavior. */
		ETimerMode Mode{ETimerMode::None};

		/** Distinguishes a live timer from reusable unoccupied slot state. */
		bool bActive{false};

		/** Permanently removes this slot once its generation space is exhausted. */
		bool bRetired{false};
	};

	/** Finds the lowest reusable slot while insertion order remains separately recorded. */
	FTimerSlot* FindAvailableSlot() noexcept
	{
		for (std::size_t SlotIndex = 0; SlotIndex < MaxTimers; ++SlotIndex)
		{
			FTimerSlot& Slot = Slots[SlotIndex];
			if (!Slot.bActive && !Slot.bRetired)
			{
				return &Slot;
			}
		}
		return nullptr;
	}

	/** Clears one active timer, retires its slot as needed, and drops it from insertion order. */
	void RemoveActiveSlot(FTimerSlot& Slot, const FTimerHandle Handle) noexcept
	{
		Slot.Callback.Reset();
		Slot.bActive = false;
		AdvanceGenerationOrRetire(Slot);
		RemoveInsertionOrderAt(Handle);
		--ActiveTimerCount;
	}

	/** Advances a reusable slot identity or retires it before generation wrap can cause ABA. */
	static void AdvanceGenerationOrRetire(FTimerSlot& Slot) noexcept
	{
		if (!CanAdvanceTimerGeneration(Slot.Generation))
		{
			Slot.bRetired = true;
			return;
		}
		++Slot.Generation;
	}

	/** Compacts insertion order after removal without changing any remaining slot identity. */
	void RemoveInsertionOrderAt(const FTimerHandle RemovedHandle) noexcept
	{
		std::size_t OrderIndex = ActiveTimerCount;
		for (std::size_t SearchIndex = 0; SearchIndex < ActiveTimerCount; ++SearchIndex)
		{
			if (InsertionOrder[SearchIndex] == RemovedHandle)
			{
				OrderIndex = SearchIndex;
				break;
			}
		}
		if (OrderIndex == ActiveTimerCount)
		{
			return;
		}
		for (std::size_t ShiftIndex = OrderIndex; ShiftIndex + 1U < ActiveTimerCount; ++ShiftIndex)
		{
			InsertionOrder[ShiftIndex] = InsertionOrder[ShiftIndex + 1U];
		}
		InsertionOrder[ActiveTimerCount - 1U] = {};
	}

	/** Adds two time values while saturating at the TimePointMilliseconds maximum. */
	static constexpr TimePointMilliseconds SaturatingAdd(const TimePointMilliseconds Base, const DurationMilliseconds Addend) noexcept
	{
		const TimePointMilliseconds MaximumTime = std::numeric_limits<TimePointMilliseconds>::max();
		return (MaximumTime - Base < static_cast<TimePointMilliseconds>(Addend)) ? MaximumTime : Base + static_cast<TimePointMilliseconds>(Addend);
	}

	/** Owns all bounded callback storage independently of insertion order. */
	FTimerSlot Slots[MaxTimers == 0 ? 1 : MaxTimers];

	/** Preserves deterministic insertion order while slots are removed and reused. */
	FTimerHandle InsertionOrder[MaxTimers == 0 ? 1 : MaxTimers];

	/** Snapshots the timers active at Advance entry so dispatch visits each at most once. */
	FTimerHandle DispatchSnapshot[MaxTimers == 0 ? 1 : MaxTimers];

	/** Stores the last accepted caller time so scheduling never reads a hidden clock. */
	TimePointMilliseconds LastAcceptedNowMilliseconds{0};

	/** Bounds insertion-order traversal and makes current timer count observable. */
	std::size_t ActiveTimerCount{0};

	/** Rejects Schedule, Cancel, and nested Advance while dispatch iteration is active. */
	bool bDispatchActive{false};
};

} // namespace MicroWorld
