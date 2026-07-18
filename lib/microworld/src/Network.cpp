#include <MicroWorld/Network.h>

namespace MicroWorld
{

FNetwork::FNetwork(const FTickConfiguration TickConfiguration) noexcept : FTickable(TickConfiguration) {}

ERuntimeResult FNetwork::BeginPlay(const TimePointMilliseconds NowMilliseconds) noexcept
{
	const ERuntimeResult BeginResult = Lifecycle.Begin();
	if (BeginResult != ERuntimeResult::Success)
	{
		return BeginResult;
	}
	BeginPrimaryTickLifecycle(NowMilliseconds);
	OnNetworkBeginPlay();
	return ERuntimeResult::Success;
}

ERuntimeResult FNetwork::Advance(const TimePointMilliseconds NowMilliseconds) noexcept
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
		TickNetwork(Decision.Context);
	}
	return ERuntimeResult::Success;
}

ERuntimeResult FNetwork::EndPlay() noexcept
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
	OnNetworkEndPlay();
	EndPrimaryTickLifecycle();
	return ERuntimeResult::Success;
}

} // namespace MicroWorld
