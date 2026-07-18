#pragma once

#include <MicroWorld/ActorComponent.h>

#include <array>
#include <cstddef>

namespace MicroWorld
{

class FWorldBase;
template<std::size_t MaxActors>
class TWorld;

/** Supplies common lifecycle and owner state for fixed-capacity Actor templates. */
class FActorBase : public FTickable
{
public:
	/** Preserves the address stored by the owning World. */
	FActorBase(const FActorBase&) = delete;

	/** Prevents World ownership and lifecycle state from being duplicated. */
	FActorBase& operator=(const FActorBase&) = delete;

	/** Preserves the address stored by the owning World. */
	FActorBase(FActorBase&&) = delete;

	/** Prevents ownership and registered Component state from moving. */
	FActorBase& operator=(FActorBase&&) = delete;

	/** Exposes the non-owning relationship for validation without transferring ownership. */
	FWorldBase* GetWorld() const noexcept;

protected:
	/** Gives each Actor an independent primary schedule selected by its consumer. */
	explicit FActorBase(FTickConfiguration TickConfiguration) noexcept;

	/** Allows safe polymorphic destruction by the consumer that owns the Actor. */
	virtual ~FActorBase() = default;

	/** Lets a derived Actor initialize after all Components start successfully. */
	virtual void BeginPlay() {}

	/** Lets a derived Actor react after due Components have advanced. */
	virtual void Tick(const FTickContext& Context) = 0;

	/** Lets a derived Actor stop before its Components are ended in reverse order. */
	virtual void EndPlay() {}

	/** Prevents structural mutation after any lifecycle dispatch can observe it. */
	bool IsRegistrationOpen() const noexcept;

	/** Delegates one-owner validation without exposing Component internals to templates. */
	ERuntimeResult AssignComponentOwner(FActorComponent& Component) noexcept;

	/** Gives templates access to validated Component startup without public dispatch APIs. */
	ERuntimeResult BeginComponent(FActorComponent& Component, TimePointMilliseconds NowMilliseconds) noexcept;

	/** Gives templates access to validated Component updates without public dispatch APIs. */
	ERuntimeResult AdvanceComponent(FActorComponent& Component, TimePointMilliseconds NowMilliseconds) noexcept;

	/** Gives templates access to idempotent Component shutdown without public dispatch APIs. */
	ERuntimeResult EndComponent(FActorComponent& Component) noexcept;

private:
	template<std::size_t MaxActors>
	friend class TWorld;

	/** Establishes the one-World invariant before registration becomes visible. */
	ERuntimeResult AssignWorld(FWorldBase& NewWorld) noexcept;

	/** Starts Actor scheduling only after lifecycle validation succeeds. */
	ERuntimeResult DispatchBeginPlay(TimePointMilliseconds NowMilliseconds) noexcept;

	/** Enforces Component-before-Actor update order and one Actor tick decision. */
	ERuntimeResult DispatchAdvance(TimePointMilliseconds NowMilliseconds) noexcept;

	/** Enforces Actor-before-Component shutdown and schedule termination. */
	ERuntimeResult DispatchEndPlay() noexcept;

	/** Lets each capacity specialization start its registered Components deterministically. */
	virtual ERuntimeResult BeginComponents(TimePointMilliseconds NowMilliseconds) noexcept = 0;

	/** Lets each capacity specialization advance its registered Components deterministically. */
	virtual ERuntimeResult AdvanceComponents(TimePointMilliseconds NowMilliseconds) noexcept = 0;

	/** Lets each capacity specialization end its Components in reverse order. */
	virtual ERuntimeResult EndComponents() noexcept = 0;

	/** Records a non-owning parent solely to prevent multiple World registrations. */
	FWorldBase* World{nullptr};

	/** Centralizes Actor registration locking and dispatch legality. */
	FLifecycleGuard Lifecycle;
};

/** Registers and dispatches a compile-time-bounded set of non-owning Components. */
template<std::size_t MaxComponents>
class TActor : public FActorBase
{
public:
	/** Binds the fixed-capacity Component aggregate to one Actor tick schedule. */
	explicit TActor(const FTickConfiguration TickConfiguration = {}) noexcept : FActorBase(TickConfiguration) {}

	/** Registers a pointer-stable Component before play without taking ownership. */
	ERuntimeResult AddComponent(FActorComponent& Component) noexcept
	{
		if (!IsRegistrationOpen())
		{
			return ERuntimeResult::LifecycleLocked;
		}
		for (std::size_t Index = 0; Index < ComponentCount; ++Index)
		{
			if (Components[Index] == &Component)
			{
				return ERuntimeResult::Duplicate;
			}
		}
		if (Component.GetOwner() != nullptr)
		{
			return ERuntimeResult::AlreadyOwned;
		}
		if (ComponentCount >= MaxComponents)
		{
			return ERuntimeResult::CapacityExceeded;
		}

		const ERuntimeResult OwnerResult = AssignComponentOwner(Component);
		if (OwnerResult != ERuntimeResult::Success)
		{
			return OwnerResult;
		}
		Components[ComponentCount] = &Component;
		++ComponentCount;
		return ERuntimeResult::Success;
	}

private:
	/** Rolls back previously started Components if a later begin hook fails. */
	ERuntimeResult BeginComponents(const TimePointMilliseconds NowMilliseconds) noexcept override
	{
		for (std::size_t Index = 0; Index < ComponentCount; ++Index)
		{
			const ERuntimeResult Result = BeginComponent(*Components[Index], NowMilliseconds);
			if (Result != ERuntimeResult::Success)
			{
				for (std::size_t RollbackIndex = Index; RollbackIndex > 0; --RollbackIndex)
				{
					EndComponent(*Components[RollbackIndex - 1]);
				}
				return Result;
			}
		}
		return ERuntimeResult::Success;
	}

	/** Preserves registration order so sibling Component behavior is deterministic. */
	ERuntimeResult AdvanceComponents(const TimePointMilliseconds NowMilliseconds) noexcept override
	{
		for (std::size_t Index = 0; Index < ComponentCount; ++Index)
		{
			const ERuntimeResult Result = AdvanceComponent(*Components[Index], NowMilliseconds);
			if (Result != ERuntimeResult::Success)
			{
				return Result;
			}
		}
		return ERuntimeResult::Success;
	}

	/** Retains the first error while still giving every started Component shutdown. */
	ERuntimeResult EndComponents() noexcept override
	{
		ERuntimeResult FirstError = ERuntimeResult::Success;
		for (std::size_t Index = ComponentCount; Index > 0; --Index)
		{
			const ERuntimeResult Result = EndComponent(*Components[Index - 1]);
			if (FirstError == ERuntimeResult::Success && Result != ERuntimeResult::Success)
			{
				FirstError = Result;
			}
		}
		return FirstError;
	}

	/** Stores bounded non-owning registrations without steady-state allocation. */
	std::array<FActorComponent*, MaxComponents> Components{};

	/** Limits dispatch to initialized registrations and avoids scanning unused capacity. */
	std::size_t ComponentCount{0};
};

} // namespace MicroWorld
