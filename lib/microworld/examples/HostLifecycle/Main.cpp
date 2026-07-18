#include <MicroWorld/Actor.h>
#include <MicroWorld/ActorComponent.h>
#include <MicroWorld/World.h>

#include <cstdio>

namespace
{

/** Samples a host value at its own cadence for the lifecycle example. */
class FSensorComponent final : public MicroWorld::FActorComponent
{
public:
	/** Selects a 100 ms schedule so the trace includes due and not-due updates. */
	FSensorComponent() noexcept : FActorComponent({true, true, 100}) {}

private:
	/** Marks Component startup so its order relative to the Actor is visible. */
	void BeginPlay() override { std::printf("sensor begin\n"); }

	/** Prints canonical time and per-Component delta to demonstrate schedule ownership. */
	void TickComponent(const MicroWorld::FTickContext& Context) override
	{
		std::printf(
			"sensor tick now=%llu delta=%u\n",
			static_cast<unsigned long long>(Context.NowMilliseconds),
			static_cast<unsigned>(Context.DeltaMilliseconds));
	}

	/** Marks Component shutdown so reverse lifecycle order is visible. */
	void EndPlay() override { std::printf("sensor end\n"); }
};

/** Aggregates Component state while its own primary tick remains disabled. */
class FDeviceActor final : public MicroWorld::TActor<1>
{
public:
	/** Disables only the Actor schedule so Component independence is observable. */
	FDeviceActor() noexcept : TActor({true, false, 0}) {}

private:
	/** Marks Actor startup after the Component begin hook. */
	void BeginPlay() override { std::printf("actor begin (primary tick disabled)\n"); }

	/** Would expose an incorrect Actor execution if disabled scheduling regressed. */
	void Tick(const MicroWorld::FTickContext&) override { std::printf("actor tick\n"); }

	/** Marks Actor shutdown before the Component end hook. */
	void EndPlay() override { std::printf("actor end\n"); }
};

} // namespace

/** Builds a dependency-safe stack composition and prints deterministic lifecycle evidence. */
int main()
{
	FSensorComponent Sensor;
	FDeviceActor Device;
	MicroWorld::TWorld<1> World;

	if (Device.AddComponent(Sensor) != MicroWorld::ERuntimeResult::Success || World.AddActor(Device) != MicroWorld::ERuntimeResult::Success
		|| World.BeginPlay(0) != MicroWorld::ERuntimeResult::Success)
	{
		return 1;
	}

	/** Includes early, exact-deadline, and late updates for the 100 ms Component. */
	constexpr MicroWorld::TimePointMilliseconds Updates[] = {
		0,
		50,
		100,
		175,
		200,
	};
	for (const MicroWorld::TimePointMilliseconds Now : Updates)
	{
		if (World.Advance(Now) != MicroWorld::ERuntimeResult::Success)
		{
			return 1;
		}
	}
	return World.EndPlay() == MicroWorld::ERuntimeResult::Success ? 0 : 1;
}
