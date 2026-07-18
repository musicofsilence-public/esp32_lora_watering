#include <MicroWorld/Actor.h>
#include <MicroWorld/World.h>

namespace MicroWorld
{

FActorBase::FActorBase(const FTickConfiguration TickConfiguration) noexcept : FTickable(TickConfiguration) {}

FWorldBase* FActorBase::GetWorld() const noexcept
{
	return World;
}

bool FActorBase::IsRegistrationOpen() const noexcept
{
	return Lifecycle.State() == ELifecycleState::Constructed;
}

ERuntimeResult FActorBase::AssignComponentOwner(FActorComponent& Component) noexcept
{
	return Component.AssignOwner(*this);
}

ERuntimeResult FActorBase::BeginComponent(FActorComponent& Component, const TimePointMilliseconds NowMilliseconds) noexcept
{
	return Component.DispatchBeginPlay(NowMilliseconds);
}

ERuntimeResult FActorBase::AdvanceComponent(FActorComponent& Component, const TimePointMilliseconds NowMilliseconds) noexcept
{
	return Component.DispatchAdvance(NowMilliseconds);
}

ERuntimeResult FActorBase::EndComponent(FActorComponent& Component) noexcept
{
	return Component.DispatchEndPlay();
}

ERuntimeResult FActorBase::AssignWorld(FWorldBase& NewWorld) noexcept
{
	if (World != nullptr)
	{
		return ERuntimeResult::AlreadyOwned;
	}
	World = &NewWorld;
	return ERuntimeResult::Success;
}

ERuntimeResult FActorBase::DispatchBeginPlay(const TimePointMilliseconds NowMilliseconds) noexcept
{
	const ERuntimeResult BeginResult = Lifecycle.Begin();
	if (BeginResult != ERuntimeResult::Success)
	{
		return BeginResult;
	}
	BeginPrimaryTickLifecycle(NowMilliseconds);

	const ERuntimeResult ComponentsResult = BeginComponents(NowMilliseconds);
	if (ComponentsResult != ERuntimeResult::Success)
	{
		EndPrimaryTickLifecycle();
		Lifecycle.Fail();
		return ComponentsResult;
	}
	BeginPlay();
	return ERuntimeResult::Success;
}

ERuntimeResult FActorBase::DispatchAdvance(const TimePointMilliseconds NowMilliseconds) noexcept
{
	const ERuntimeResult PlayingResult = Lifecycle.RequirePlaying();
	if (PlayingResult != ERuntimeResult::Success)
	{
		return PlayingResult;
	}

	const ERuntimeResult ComponentsResult = AdvanceComponents(NowMilliseconds);
	if (ComponentsResult != ERuntimeResult::Success)
	{
		return ComponentsResult;
	}

	const FTickDecision Decision = AdvancePrimaryTick(NowMilliseconds);
	if (Decision.Result != ERuntimeResult::Success)
	{
		return Decision.Result;
	}
	if (Decision.bShouldTick)
	{
		Tick(Decision.Context);
	}
	return ERuntimeResult::Success;
}

ERuntimeResult FActorBase::DispatchEndPlay() noexcept
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

	EndPlay();
	const ERuntimeResult ComponentsResult = EndComponents();
	EndPrimaryTickLifecycle();
	return ComponentsResult;
}

} // namespace MicroWorld
