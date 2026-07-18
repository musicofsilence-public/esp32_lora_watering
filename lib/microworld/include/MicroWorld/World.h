#pragma once

#include <MicroWorld/Actor.h>

#include <array>
#include <cstddef>

namespace MicroWorld
{

/** Holds common non-owning World state needed by registered Actors. */
class FWorldBase
{
public:
	/** Preserves the address stored by every registered Actor. */
	FWorldBase(const FWorldBase&) = delete;

	/** Prevents assigning identity across World ownership boundaries. */
	FWorldBase& operator=(const FWorldBase&) = delete;

	/** Preserves the address stored by every registered Actor. */
	FWorldBase(FWorldBase&&) = delete;

	/** Prevents moving a World behind Actor-held non-owning pointers. */
	FWorldBase& operator=(FWorldBase&&) = delete;

protected:
	/** Restricts base construction to a fixed-capacity World specialization. */
	FWorldBase() = default;

	/** Prevents public deletion through a base that does not own Actors. */
	~FWorldBase() = default;
};

/** Deterministically dispatches a compile-time-bounded set of non-owning Actors. */
template<std::size_t MaxActors>
class TWorld final : public FWorldBase
{
public:
	/** Creates an empty registration boundary with no heap or hidden ownership. */
	TWorld() = default;

	/** Registers a pointer-stable Actor before play without taking ownership. */
	ERuntimeResult AddActor(FActorBase& Actor) noexcept
	{
		if (Lifecycle.State() != ELifecycleState::Constructed)
		{
			return ERuntimeResult::LifecycleLocked;
		}
		for (std::size_t Index = 0; Index < ActorCount; ++Index)
		{
			if (Actors[Index] == &Actor)
			{
				return ERuntimeResult::Duplicate;
			}
		}
		if (Actor.GetWorld() != nullptr)
		{
			return ERuntimeResult::AlreadyOwned;
		}
		if (ActorCount >= MaxActors)
		{
			return ERuntimeResult::CapacityExceeded;
		}

		const ERuntimeResult OwnerResult = Actor.AssignWorld(*this);
		if (OwnerResult != ERuntimeResult::Success)
		{
			return OwnerResult;
		}
		Actors[ActorCount] = &Actor;
		++ActorCount;
		return ERuntimeResult::Success;
	}

	/** Starts Actors in registration order from one canonical time point. */
	ERuntimeResult BeginPlay(const TimePointMilliseconds NowMilliseconds) noexcept
	{
		const ERuntimeResult BeginResult = Lifecycle.Begin();
		if (BeginResult != ERuntimeResult::Success)
		{
			return BeginResult;
		}
		LastUpdateMilliseconds = NowMilliseconds;

		for (std::size_t Index = 0; Index < ActorCount; ++Index)
		{
			const ERuntimeResult Result = Actors[Index]->DispatchBeginPlay(NowMilliseconds);
			if (Result != ERuntimeResult::Success)
			{
				for (std::size_t RollbackIndex = Index; RollbackIndex > 0; --RollbackIndex)
				{
					Actors[RollbackIndex - 1]->DispatchEndPlay();
				}
				Lifecycle.Fail();
				return Result;
			}
		}
		return ERuntimeResult::Success;
	}

	/** Advances every Actor once after validating monotonic World time. */
	ERuntimeResult Advance(const TimePointMilliseconds NowMilliseconds) noexcept
	{
		const ERuntimeResult PlayingResult = Lifecycle.RequirePlaying();
		if (PlayingResult != ERuntimeResult::Success)
		{
			return PlayingResult;
		}
		if (NowMilliseconds < LastUpdateMilliseconds)
		{
			return ERuntimeResult::NonMonotonicTime;
		}
		LastUpdateMilliseconds = NowMilliseconds;

		for (std::size_t Index = 0; Index < ActorCount; ++Index)
		{
			const ERuntimeResult Result = Actors[Index]->DispatchAdvance(NowMilliseconds);
			if (Result != ERuntimeResult::Success)
			{
				return Result;
			}
		}
		return ERuntimeResult::Success;
	}

	/** Ends Actors in reverse registration order and is idempotent after success. */
	ERuntimeResult EndPlay() noexcept
	{
		if (Lifecycle.State() == ELifecycleState::Ended)
		{
			return ERuntimeResult::Success;
		}
		const ERuntimeResult EndResult = Lifecycle.End();
		if (EndResult != ERuntimeResult::Success)
		{
			return EndResult;
		}

		ERuntimeResult FirstError = ERuntimeResult::Success;
		for (std::size_t Index = ActorCount; Index > 0; --Index)
		{
			const ERuntimeResult Result = Actors[Index - 1]->DispatchEndPlay();
			if (FirstError == ERuntimeResult::Success && Result != ERuntimeResult::Success)
			{
				FirstError = Result;
			}
		}
		return FirstError;
	}

private:
	/** Stores bounded non-owning registrations without steady-state allocation. */
	std::array<FActorBase*, MaxActors> Actors{};

	/** Limits dispatch to initialized registrations and avoids scanning unused capacity. */
	std::size_t ActorCount{0};

	/** Locks registration at begin and makes successful shutdown idempotent. */
	FLifecycleGuard Lifecycle;

	/** Rejects backward World time before any registered Actor observes it. */
	TimePointMilliseconds LastUpdateMilliseconds{0};
};

} // namespace MicroWorld
