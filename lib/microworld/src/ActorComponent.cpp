#include <MicroWorld/ActorComponent.h>

namespace MicroWorld
{

FActorComponent::FActorComponent(const FTickConfiguration TickConfiguration) noexcept : FTickable(TickConfiguration) {}

FActorBase* FActorComponent::GetOwner() const noexcept
{
	return Owner;
}

ERuntimeResult FActorComponent::AssignOwner(FActorBase& NewOwner) noexcept
{
	if (Owner != nullptr)
	{
		return ERuntimeResult::AlreadyOwned;
	}
	Owner = &NewOwner;
	return ERuntimeResult::Success;
}

ERuntimeResult FActorComponent::DispatchBeginPlay(const TimePointMilliseconds NowMilliseconds) noexcept
{
	const ERuntimeResult BeginResult = Lifecycle.Begin();
	if (BeginResult != ERuntimeResult::Success)
	{
		return BeginResult;
	}
	BeginPrimaryTickLifecycle(NowMilliseconds);
	BeginPlay();
	return ERuntimeResult::Success;
}

ERuntimeResult FActorComponent::DispatchAdvance(const TimePointMilliseconds NowMilliseconds) noexcept
{
	const ERuntimeResult PlayingResult = Lifecycle.RequirePlaying();
	if (PlayingResult != ERuntimeResult::Success)
	{
		return PlayingResult;
	}
	const FTickDecision Decision = AdvancePrimaryTick(NowMilliseconds);
	if (Decision.Result != ERuntimeResult::Success)
	{
		return Decision.Result;
	}
	if (Decision.bShouldTick)
	{
		TickComponent(Decision.Context);
	}
	return ERuntimeResult::Success;
}

ERuntimeResult FActorComponent::DispatchEndPlay() noexcept
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
	EndPrimaryTickLifecycle();
	return ERuntimeResult::Success;
}

} // namespace MicroWorld
