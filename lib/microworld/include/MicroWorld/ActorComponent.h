#pragma once

#include <MicroWorld/Lifecycle.h>
#include <MicroWorld/Tickable.h>

namespace MicroWorld
{

class FActorBase;

/** Defines reusable behavior registered with exactly one externally owned Actor. */
class FActorComponent : public FTickable
{
public:
	/** Preserves the address stored by the owning Actor. */
	FActorComponent(const FActorComponent&) = delete;

	/** Prevents ownership and lifecycle state from being duplicated. */
	FActorComponent& operator=(const FActorComponent&) = delete;

	/** Preserves the address stored by the owning Actor. */
	FActorComponent(FActorComponent&&) = delete;

	/** Prevents ownership and lifecycle state from moving after registration. */
	FActorComponent& operator=(FActorComponent&&) = delete;

	/** Exposes the non-owning relationship for validation without transferring ownership. */
	FActorBase* GetOwner() const noexcept;

protected:
	/** Gives each Component an independent primary schedule selected by its consumer. */
	explicit FActorComponent(FTickConfiguration TickConfiguration) noexcept;

	/** Allows safe polymorphic destruction by the consumer that owns the object. */
	virtual ~FActorComponent() = default;

	/** Lets a derived Component initialize behavior after lifecycle validation. */
	virtual void BeginPlay() {}

	/** Lets a derived Component react only when its independent schedule is due. */
	virtual void TickComponent(const FTickContext&) {}

	/** Lets a derived Component release behavior before its lifecycle becomes inaccessible. */
	virtual void EndPlay() {}

private:
	friend class FActorBase;

	/** Establishes the one-Actor invariant before registration becomes visible. */
	ERuntimeResult AssignOwner(FActorBase& NewOwner) noexcept;

	/** Keeps lifecycle validation and tick startup outside consumer hooks. */
	ERuntimeResult DispatchBeginPlay(TimePointMilliseconds NowMilliseconds) noexcept;

	/** Keeps schedule decisions and lifecycle errors outside consumer hooks. */
	ERuntimeResult DispatchAdvance(TimePointMilliseconds NowMilliseconds) noexcept;

	/** Makes Component shutdown idempotent and stops its primary schedule. */
	ERuntimeResult DispatchEndPlay() noexcept;

	/** Records a non-owning parent solely to prevent multiple registrations. */
	FActorBase* Owner{nullptr};

	/** Replaces ambiguous lifecycle booleans with one forward-only state machine. */
	FLifecycleGuard Lifecycle;
};

} // namespace MicroWorld
