#include <MicroWorld/Engine/Actor.h>
#include <MicroWorld/Engine/EngineClassIds.h>
#include <MicroWorld/Engine/EngineStorage.h>
#include <MicroWorld/Engine/World.h>

#include <MicroWorld/Object/ClassDescriptor.h>
#include <MicroWorld/Object/GarbageCollector.h>
#include <MicroWorld/Object/Object.h>
#include <MicroWorld/Object/ObjectStore.h>
#include <MicroWorld/Object/ObjectPtr.h>

#include <utility>

namespace MicroWorld
{

UWorld::UWorld(FWorldActorRegistryBase ActorStorage) noexcept : UObject(), Actors(std::move(ActorStorage)) {}

UWorld::~UWorld() noexcept = default;

const FClassDescriptor& UWorld::StaticClassDescriptor() noexcept
{
	static const FClassDescriptor Descriptor = MakeClassDescriptor<UWorld>(UWorldClassId, "UWorld", nullptr, &TraceManagedObjectReferences);
	return Descriptor;
}

EEngineResult UWorld::RegisterActor(const TObjectPtr<AActor> Actor) noexcept
{
	// Registration is only permitted before BeginPlay can begin dispatch.
	if (Lifecycle.State() != ELifecycleState::Constructed)
	{
		return EEngineResult::LifecycleLocked;
	}
	FObjectStore* const ObjectStore = GetObjectStore();
	if (ObjectStore == nullptr)
	{
		return EEngineResult::InvalidReference;
	}
	if (ObjectStore->IsMutationLocked())
	{
		return EEngineResult::LifecycleLocked;
	}
	AActor* const Resolved = Actor.Get();
	if (Resolved == nullptr)
	{
		return EEngineResult::InvalidReference;
	}
	// The actor must belong to the same canonical store as this world so a
	// foreign handle can never be traced through this owner.
	if (!Actor.BelongsTo(*ObjectStore))
	{
		return EEngineResult::CrossStore;
	}
	if (!Actors.IsValid())
	{
		return EEngineResult::CapacityExceeded;
	}
	// A duplicate of an actor already registered with this world is reported
	// before the cross-owner check so a repeated registration stays honest.
	for (std::size_t Index = 0; Index < Actors.GetCount(); ++Index)
	{
		if (Actors.At(Index).Handle() == Actor.Handle())
		{
			return EEngineResult::Duplicate;
		}
	}
	// Capacity (including zero capacity) is a structural property of this world,
	// so it is reported before the candidate's existing ownership is inspected.
	if (Actors.GetCount() >= Actors.GetCapacity())
	{
		return EEngineResult::CapacityExceeded;
	}
	if (Resolved->HasAssignedWorld())
	{
		return EEngineResult::AlreadyOwned;
	}

	// Atomic publish: every fallible check precedes the parent link and registry update.
	Resolved->AssignWorld(GetObjectHandle());
	Actors.Add(Actor);
	return EEngineResult::Success;
}

ERuntimeResult UWorld::BeginPlay(const TimePointMilliseconds NowMilliseconds) noexcept
{
	if (Lifecycle.State() != ELifecycleState::Constructed)
	{
		return ERuntimeResult::InvalidLifecycle;
	}
	if (!Actors.IsValid())
	{
		return ERuntimeResult::CapacityExceeded;
	}
	FObjectStore* const ObjectStore = GetObjectStore();
	if (ObjectStore == nullptr)
	{
		return ERuntimeResult::InvalidLifecycle;
	}
	FObjectStoreDispatchGuard DispatchGuard(*ObjectStore);
	if (!DispatchGuard.IsAcquired())
	{
		return ERuntimeResult::LifecycleLocked;
	}

	const ERuntimeResult BeginResult = Lifecycle.Begin();
	if (BeginResult != ERuntimeResult::Success)
	{
		return BeginResult;
	}
	LastUpdateMilliseconds = NowMilliseconds;

	// Actors begin in registration order; on first failure the previously begun
	// actors are ended in reverse so the world never observes a partially begun
	// set and its own lifecycle becomes terminal.
	std::size_t BegunActorCount = 0;
	for (std::size_t Index = 0; Index < Actors.GetCount(); ++Index)
	{
		AActor* const Actor = Actors.At(Index).Get();
		const ERuntimeResult ActorResult = Actor != nullptr ? DispatchActorBegin(*Actor, NowMilliseconds) : ERuntimeResult::InvalidLifecycle;
		if (ActorResult != ERuntimeResult::Success)
		{
			for (std::size_t RollbackIndex = BegunActorCount; RollbackIndex > 0; --RollbackIndex)
			{
				if (AActor* const Begun = Actors.At(RollbackIndex - 1).Get())
				{
					(void)Begun->DispatchEndPlay();
				}
			}
			Lifecycle.Fail();
			return ActorResult;
		}
		++BegunActorCount;
	}
	return ERuntimeResult::Success;
}

ERuntimeResult UWorld::Advance(const TimePointMilliseconds NowMilliseconds) noexcept
{
	const ERuntimeResult PlayingResult = Lifecycle.RequirePlaying();
	if (PlayingResult != ERuntimeResult::Success)
	{
		return PlayingResult;
	}
	// The world rejects rollback before any actor observes the timestamp so a
	// non-monotonic dispatcher cannot corrupt per-actor scheduling baselines.
	if (NowMilliseconds < LastUpdateMilliseconds)
	{
		return ERuntimeResult::NonMonotonicTime;
	}
	FObjectStore* const ObjectStore = GetObjectStore();
	if (ObjectStore == nullptr)
	{
		return ERuntimeResult::InvalidLifecycle;
	}
	FObjectStoreDispatchGuard DispatchGuard(*ObjectStore);
	if (!DispatchGuard.IsAcquired())
	{
		return ERuntimeResult::LifecycleLocked;
	}
	LastUpdateMilliseconds = NowMilliseconds;

	for (std::size_t Index = 0; Index < Actors.GetCount(); ++Index)
	{
		AActor* const Actor = Actors.At(Index).Get();
		if (Actor == nullptr)
		{
			return ERuntimeResult::InvalidLifecycle;
		}
		const ERuntimeResult ActorResult = DispatchActorAdvance(*Actor, NowMilliseconds);
		if (ActorResult != ERuntimeResult::Success)
		{
			return ActorResult;
		}
	}
	return ERuntimeResult::Success;
}

ERuntimeResult UWorld::EndPlay() noexcept
{
	// EndPlay is idempotent after a successful end so repeated shutdown paths
	// never re-enter the actor end cascade.
	if (Lifecycle.State() == ELifecycleState::Ended)
	{
		return ERuntimeResult::Success;
	}
	if (Lifecycle.State() != ELifecycleState::Playing)
	{
		return ERuntimeResult::InvalidLifecycle;
	}
	FObjectStore* const ObjectStore = GetObjectStore();
	if (ObjectStore == nullptr)
	{
		return ERuntimeResult::InvalidLifecycle;
	}
	FObjectStoreDispatchGuard DispatchGuard(*ObjectStore);
	if (!DispatchGuard.IsAcquired())
	{
		return ERuntimeResult::LifecycleLocked;
	}
	const ERuntimeResult EndResult = Lifecycle.End();
	if (EndResult != ERuntimeResult::Success)
	{
		return EndResult;
	}

	// Actors end in reverse registration order; the first error is retained but
	// every actor still receives its EndPlay so shutdown stays symmetric.
	ERuntimeResult FirstError = ERuntimeResult::Success;
	for (std::size_t Index = Actors.GetCount(); Index > 0; --Index)
	{
		if (AActor* const Actor = Actors.At(Index - 1).Get())
		{
			const ERuntimeResult ActorResult = DispatchActorEnd(*Actor);
			if (FirstError == ERuntimeResult::Success && ActorResult != ERuntimeResult::Success)
			{
				FirstError = ActorResult;
			}
		}
	}
	return FirstError;
}

ERuntimeResult UWorld::DispatchActorBegin(AActor& Actor, const TimePointMilliseconds NowMilliseconds) noexcept
{
	return Actor.DispatchBeginPlay(NowMilliseconds);
}

ERuntimeResult UWorld::DispatchActorAdvance(AActor& Actor, const TimePointMilliseconds NowMilliseconds) noexcept
{
	return Actor.DispatchAdvance(NowMilliseconds);
}

ERuntimeResult UWorld::DispatchActorEnd(AActor& Actor) noexcept
{
	return Actor.DispatchEndPlay();
}

void UWorld::VisitReferences(FReferenceCollector& Collector) noexcept
{
	// Every registered actor is a traced downward edge; the world owns no other
	// managed references.
	for (std::size_t Index = 0; Index < Actors.GetCount(); ++Index)
	{
		Collector.AddReferencedObject(Actors.At(Index));
	}
}

} // namespace MicroWorld
