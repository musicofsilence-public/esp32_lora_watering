#pragma once

#include <MicroWorld/Engine/Actor.h>
#include <MicroWorld/Engine/EngineRegistryView.h>
#include <MicroWorld/Engine/EngineResult.h>
#include <MicroWorld/Lifecycle.h>
#include <MicroWorld/Object/Object.h>
#include <MicroWorld/Object/ObjectPtr.h>
#include <MicroWorld/Time.h>

namespace MicroWorld
{

class AActor;
struct FClassDescriptor;
class FObjectStore;
class FReferenceCollector;

/**
 * The smallest managed world anchored on UObject.
 *
 * The application creates one UWorld (or a user-derived class) inside an
 * FObjectStore, registers zero or more AActor instances before BeginPlay, then
 * roots the world with one TStrongObjectPtr<UWorld>. UWorld traces its actors;
 * it does not tick on its own, matching Core's non-managed TWorld dispatch.
 */
class UWorld : public UObject
{
public:
	UWorld(const UWorld&) = delete;
	UWorld& operator=(const UWorld&) = delete;
	UWorld(UWorld&&) = delete;
	UWorld& operator=(UWorld&&) = delete;

	/** Returns the stable descriptor that lets the store construct and trace this type. */
	static const FClassDescriptor& StaticClassDescriptor() noexcept;

	/**
	 * Binds this world to the unique caller-owned actor registry lease that will
	 * hold its registered actors.
	 *
	 * The object store assigns canonical ownership only after construction
	 * publishes this UObject, so callers cannot supply a second store identity.
	 */
	explicit UWorld(FWorldActorRegistryBase ActorStorage) noexcept;

	/** Keeps exact derived destruction behind the descriptor/store boundary. */
	~UWorld() noexcept override;

	/**
	 * Registers one actor before BeginPlay.
	 *
	 * Rejects duplicates, exhausted or zero capacity, lifecycle-locked worlds,
	 * actors already owned by another world, cross-store actors, and empty,
	 * stale, or non-resolvable references atomically: a rejected registration
	 * leaves the world and the actor unchanged.
	 */
	EEngineResult RegisterActor(TObjectPtr<AActor> Actor) noexcept;

	/** Starts every registered actor in registration order from one canonical time. */
	ERuntimeResult BeginPlay(TimePointMilliseconds NowMilliseconds) noexcept;

	/** Advances every registered actor once after validating monotonic world time. */
	ERuntimeResult Advance(TimePointMilliseconds NowMilliseconds) noexcept;

	/** Ends every registered actor in reverse registration order; idempotent after success. */
	ERuntimeResult EndPlay() noexcept;

private:
	/** Begins one actor's lifecycle while letting the world roll back on failure. */
	ERuntimeResult DispatchActorBegin(AActor& Actor, TimePointMilliseconds NowMilliseconds) noexcept;

	/** Advances one actor for one dispatcher step. */
	ERuntimeResult DispatchActorAdvance(AActor& Actor, TimePointMilliseconds NowMilliseconds) noexcept;

	/** Ends one actor while the world retains the first error and still ends every actor. */
	ERuntimeResult DispatchActorEnd(AActor& Actor) noexcept;

	/** Presents every registered actor to the active iterative collector. */
	void VisitReferences(FReferenceCollector& Collector) noexcept override;

	/** Holds the unique caller-owned actor registry lease for this world's lifetime. */
	FWorldActorRegistryBase Actors;

	/** Guards the forward-only world lifecycle without scattering boolean flags. */
	FLifecycleGuard Lifecycle;

	/** Caches the last observed dispatcher time so rollback stays observable. */
	TimePointMilliseconds LastUpdateMilliseconds{0};
};

} // namespace MicroWorld
