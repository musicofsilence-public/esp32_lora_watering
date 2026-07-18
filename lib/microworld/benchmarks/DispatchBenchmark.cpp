#include <MicroWorld/Actor.h>
#include <MicroWorld/ActorComponent.h>
#include <MicroWorld/Network.h>
#include <MicroWorld/TickFunction.h>
#include <MicroWorld/World.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>

namespace
{

/** Counts process allocations so steady-state dispatch can prove it allocates nothing. */
std::size_t AllocationCount = 0;

} // namespace

/** Intercepts scalar allocations solely to measure benchmark steady-state behavior. */
void* operator new(const std::size_t Size)
{
	++AllocationCount;
	if (void* const Memory = std::malloc(Size))
	{
		return Memory;
	}
	throw std::bad_alloc();
}

/** Routes array allocations through the same counter so the metric is complete. */
void* operator new[](const std::size_t Size)
{
	return ::operator new(Size);
}

/** Completes the scalar allocation override without changing deallocation behavior. */
void operator delete(void* const Memory) noexcept
{
	std::free(Memory);
}

/** Completes the array allocation override without changing deallocation behavior. */
void operator delete[](void* const Memory) noexcept
{
	std::free(Memory);
}

/** Supports compilers that select sized scalar deletion for the measured objects. */
void operator delete(void* const Memory, const std::size_t) noexcept
{
	std::free(Memory);
}

/** Supports compilers that select sized array deletion for the measured objects. */
void operator delete[](void* const Memory, const std::size_t) noexcept
{
	std::free(Memory);
}

namespace
{

/** Produces enough independent samples for stable median and p95 summaries. */
constexpr std::size_t TrialCount = 30;

/** Warms instruction/data paths before timing so startup effects do not dominate. */
constexpr std::uint32_t WarmupUpdates = 1000;

/** Amortizes host clock-read overhead across a fixed dispatch workload. */
constexpr std::uint32_t MeasuredUpdates = 10000;

/** Counts Component dispatch for fixed host benchmark workloads. */
class FBenchmarkComponent final : public MicroWorld::FActorComponent
{
public:
	/** Supplies the all-due configuration required by fixed-capacity arrays. */
	FBenchmarkComponent() noexcept : FActorComponent({true, true, 0}) {}

	/** Allows each profile to select cadence without a separate fake type. */
	explicit FBenchmarkComponent(const MicroWorld::FTickConfiguration Configuration) noexcept : FActorComponent(Configuration) {}

private:
	/** Records observable executions so future semantic checks can use the same fake. */
	void TickComponent(const MicroWorld::FTickContext&) override { ++ExecutedTicks; }

	/** Accumulates executions without allocating or perturbing dispatch structure. */
	std::uint32_t ExecutedTicks{0};
};

/** Counts Actor dispatch with the benchmark's maximum Component capacity. */
class FBenchmarkActor final : public MicroWorld::TActor<4>
{
public:
	/** Allows each workload to select Actor cadence with one fixed-capacity type. */
	explicit FBenchmarkActor(const MicroWorld::FTickConfiguration Configuration = {}) noexcept : TActor(Configuration) {}

private:
	/** Records observable Actor executions without adding product behavior. */
	void Tick(const MicroWorld::FTickContext&) override { ++ExecutedTicks; }

	/** Accumulates executions without allocating or perturbing dispatch structure. */
	std::uint32_t ExecutedTicks{0};
};

/** Captures comparable size and dispatch measurements for one build profile. */
struct FPerformanceSample
{
	/** Reserves a common schema field for target linker-map evidence. */
	std::size_t FlashBytes;

	/** Reserves a common schema field for target static-RAM evidence. */
	std::size_t StaticRamBytes;

	/** Reserves a common schema field for target stack evidence. */
	std::size_t PeakStackBytes;

	/** Carries the host dispatch metric this executable can observe. */
	std::uint64_t NanosecondsPerUpdate;

	/** Carries the allocation invariant that host overrides can observe. */
	std::uint32_t SteadyStateAllocations;
};

/** Measures one warmed workload while amortizing clock reads over fixed updates. */
template<typename FAdvance>
std::uint64_t MeasureTrial(FAdvance&& Advance)
{
	for (std::uint32_t Update = 0; Update < WarmupUpdates; ++Update)
	{
		Advance(Update);
	}
	const auto Start = std::chrono::steady_clock::now();
	for (std::uint32_t Update = 0; Update < MeasuredUpdates; ++Update)
	{
		Advance(WarmupUpdates + Update);
	}
	const auto End = std::chrono::steady_clock::now();
	const auto Nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(End - Start);
	return static_cast<std::uint64_t>(Nanoseconds.count()) / MeasuredUpdates;
}

/** Summarizes fixed trials without retaining dynamic benchmark state. */
template<typename FFactory>
void RunProfile(const char* const Name, FFactory&& Factory)
{
	std::array<std::uint64_t, TrialCount> Samples{};
	const std::size_t AllocationsBefore = AllocationCount;
	for (std::size_t Trial = 0; Trial < TrialCount; ++Trial)
	{
		Samples[Trial] = Factory(Trial);
	}
	const std::size_t SteadyStateAllocations = AllocationCount - AllocationsBefore;
	std::sort(Samples.begin(), Samples.end());
	const std::size_t P95Index = (TrialCount * 95 + 99) / 100 - 1;
	std::printf(
		"%s,median_ns=%llu,p95_ns=%llu,worst_ns=%llu,allocations=%zu\n",
		Name,
		static_cast<unsigned long long>(Samples[TrialCount / 2]),
		static_cast<unsigned long long>(Samples[P95Index]),
		static_cast<unsigned long long>(Samples.back()),
		SteadyStateAllocations);
}

/** Exercises the early disabled-tick path while lifecycle dispatch remains active. */
std::uint64_t RunDisabledTrial(const std::size_t Trial)
{
	FBenchmarkActor Actor({true, false, 0});
	MicroWorld::TWorld<1> World;
	(void)World.AddActor(Actor);
	(void)World.BeginPlay(0);
	const MicroWorld::TimePointMilliseconds Base = Trial * (WarmupUpdates + MeasuredUpdates);
	const std::uint64_t Result = MeasureTrial([&](const std::uint32_t Update) { (void)World.Advance(Base + Update); });
	(void)World.EndPlay();
	return Result;
}

/** Exercises one Actor/Component pair with caller-selected independent cadences. */
std::uint64_t RunSingleActorTrial(
	const std::size_t Trial, const MicroWorld::DurationMilliseconds ActorInterval, const MicroWorld::DurationMilliseconds ComponentInterval)
{
	FBenchmarkComponent Component({true, true, ComponentInterval});
	FBenchmarkActor Actor({true, true, ActorInterval});
	MicroWorld::TWorld<1> World;
	(void)Actor.AddComponent(Component);
	(void)World.AddActor(Actor);
	(void)World.BeginPlay(0);
	const MicroWorld::TimePointMilliseconds Base = Trial * (WarmupUpdates + MeasuredUpdates);
	const std::uint64_t Result = MeasureTrial([&](const std::uint32_t Update) { (void)World.Advance(Base + Update); });
	(void)World.EndPlay();
	return Result;
}

/** Exercises every registration slot to expose capacity-dependent dispatch cost. */
std::uint64_t RunMaximumCapacityTrial(const std::size_t Trial)
{
	std::array<std::array<FBenchmarkComponent, 4>, 8> Components{};
	std::array<FBenchmarkActor, 8> Actors{
		FBenchmarkActor({true, true, 0}),
		FBenchmarkActor({true, true, 0}),
		FBenchmarkActor({true, true, 0}),
		FBenchmarkActor({true, true, 0}),
		FBenchmarkActor({true, true, 0}),
		FBenchmarkActor({true, true, 0}),
		FBenchmarkActor({true, true, 0}),
		FBenchmarkActor({true, true, 0}),
	};
	MicroWorld::TWorld<8> World;
	for (std::size_t ActorIndex = 0; ActorIndex < Actors.size(); ++ActorIndex)
	{
		for (FBenchmarkComponent& Component : Components[ActorIndex])
		{
			(void)Actors[ActorIndex].AddComponent(Component);
		}
		(void)World.AddActor(Actors[ActorIndex]);
	}
	(void)World.BeginPlay(0);
	const MicroWorld::TimePointMilliseconds Base = Trial * (WarmupUpdates + MeasuredUpdates);
	const std::uint64_t Result = MeasureTrial([&](const std::uint32_t Update) { (void)World.Advance(Base + Update); });
	(void)World.EndPlay();
	return Result;
}

} // namespace

/** Emits object sizes and the four comparable host workload summaries. */
int main()
{
	std::printf(
		"sizes,TickFunction=%zu,ActorComponent=%zu,Actor4=%zu,World8=%zu,"
		"Network=%zu,PerformanceSample=%zu\n",
		sizeof(MicroWorld::FTickFunction),
		sizeof(MicroWorld::FActorComponent),
		sizeof(FBenchmarkActor),
		sizeof(MicroWorld::TWorld<8>),
		sizeof(MicroWorld::FNetwork),
		sizeof(FPerformanceSample));
	RunProfile("disabled", RunDisabledTrial);
	RunProfile("all-due", [](const std::size_t Trial) { return RunSingleActorTrial(Trial, 0, 0); });
	RunProfile("mixed-rate", [](const std::size_t Trial) { return RunSingleActorTrial(Trial, 5, 2); });
	RunProfile("maximum-capacity", RunMaximumCapacityTrial);
	return 0;
}
