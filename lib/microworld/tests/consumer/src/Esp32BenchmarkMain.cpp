#include <MicroWorld/Actor.h>
#include <MicroWorld/ActorComponent.h>
#include <MicroWorld/Version.h>
#include <MicroWorld/World.h>

#include "esp_cpu.h"
#include "esp_heap_caps.h"
#include "esp_private/esp_clk.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <array>
#include <cinttypes>
#include <cstdint>
#include <cstdio>

namespace
{

/** Warms instruction/data paths before target counters start. */
constexpr std::uint32_t WarmupUpdates = 1000;

/** Provides a fixed measurement window shared with the host harness. */
constexpr std::uint32_t MeasuredUpdates = 10000;

/** Provides enough samples for later median, p95, and worst summaries. */
constexpr std::uint32_t TrialCount = 30;

/** Keeps trial offsets monotonic across warm-up and measured updates. */
constexpr std::uint32_t UpdatesPerTrial = WarmupUpdates + MeasuredUpdates;

/** Exercises a slower Actor schedule in the mixed-rate profile. */
constexpr MicroWorld::DurationMilliseconds MixedActorInterval = 5;

/** Exercises an independently faster Component schedule in the mixed-rate profile. */
constexpr MicroWorld::DurationMilliseconds MixedComponentInterval = 2;

/** Calculates semantic expectations so timing output is rejected if dispatch drifts. */
constexpr std::uint32_t ExpectedTicksPerTrial(const MicroWorld::DurationMilliseconds IntervalMilliseconds) noexcept
{
	return IntervalMilliseconds == 0 ? UpdatesPerTrial : 1 + (UpdatesPerTrial - 1) / IntervalMilliseconds;
}

static_assert(UpdatesPerTrial % MixedActorInterval == 0);
static_assert(UpdatesPerTrial % MixedComponentInterval == 0);

/** Counts benchmark Component dispatch without adding product behavior. */
class FBenchmarkComponent final : public MicroWorld::FActorComponent
{
public:
	/** Supplies the all-due configuration required by fixed-capacity arrays. */
	FBenchmarkComponent() noexcept : FActorComponent({true, true, 0}) {}

	/** Allows each profile to select Component cadence with one fake type. */
	explicit FBenchmarkComponent(const MicroWorld::FTickConfiguration Configuration) noexcept : FActorComponent(Configuration) {}

	/** Exposes cumulative executions for semantic validation after every trial. */
	std::uint32_t TickCount() const noexcept { return ExecutedTicks; }

private:
	/** Records Component execution without adding product or allocation behavior. */
	void TickComponent(const MicroWorld::FTickContext&) override { ++ExecutedTicks; }

	/** Accumulates executions so later trials detect missing or duplicate ticks. */
	std::uint32_t ExecutedTicks{0};
};

/** Counts benchmark Actor dispatch with a fixed four-Component capacity. */
class FBenchmarkActor final : public MicroWorld::TActor<4>
{
public:
	/** Allows each profile to select Actor cadence with one fixed-capacity type. */
	explicit FBenchmarkActor(const MicroWorld::FTickConfiguration Configuration) noexcept : TActor(Configuration) {}

	/** Exposes cumulative executions for semantic validation after every trial. */
	std::uint32_t TickCount() const noexcept { return ExecutedTicks; }

private:
	/** Records Actor execution without adding product or allocation behavior. */
	void Tick(const MicroWorld::FTickContext&) override { ++ExecutedTicks; }

	/** Accumulates executions so later trials detect missing or duplicate ticks. */
	std::uint32_t ExecutedTicks{0};
};

/** Captures one fixed target benchmark trial and its validity evidence. */
struct FTrialResult
{
	/** Captures measured dispatch work after subtracting counter-read overhead. */
	std::uint32_t Cycles;

	/** Makes the platform counter cost explicit for result review. */
	std::uint32_t CounterOverhead;

	/** Detects steady-state allocation or release during measured dispatch. */
	std::int32_t HeapDelta;

	/** Preserves the minimum observed task stack margin for safety review. */
	UBaseType_t StackHighWaterWords;

	/** Prevents invalid lifecycle/tick semantics from being reported as performance. */
	bool bPassed;
};

/** Samples back-to-back counter reads so measurement overhead is not hidden. */
std::uint32_t MeasureCounterOverhead() noexcept
{
	const std::uint32_t Start = esp_cpu_get_cycle_count();
	const std::uint32_t End = esp_cpu_get_cycle_count();
	return End - Start;
}

/** Measures one fixed target workload while validating every requested advance. */
template<typename FAdvance>
FTrialResult RunTrial(FAdvance&& Advance) noexcept
{
	for (std::uint32_t Update = 0; Update < WarmupUpdates; ++Update)
	{
		if (!Advance(Update))
		{
			return {0, 0, 0, uxTaskGetStackHighWaterMark(nullptr), false};
		}
	}

	const std::uint32_t CounterOverhead = MeasureCounterOverhead();
	const std::uint32_t HeapBefore = heap_caps_get_free_size(MALLOC_CAP_8BIT);
	const std::uint32_t StartCycles = esp_cpu_get_cycle_count();
	for (std::uint32_t Update = 0; Update < MeasuredUpdates; ++Update)
	{
		if (!Advance(WarmupUpdates + Update))
		{
			return {0, CounterOverhead, 0, uxTaskGetStackHighWaterMark(nullptr), false};
		}
	}
	const std::uint32_t EndCycles = esp_cpu_get_cycle_count();
	const std::uint32_t HeapAfter = heap_caps_get_free_size(MALLOC_CAP_8BIT);
	return {
		EndCycles - StartCycles - CounterOverhead,
		CounterOverhead,
		static_cast<std::int32_t>(HeapAfter) - static_cast<std::int32_t>(HeapBefore),
		uxTaskGetStackHighWaterMark(nullptr),
		true,
	};
}

/** Emits one stable CSV record so target evidence can be archived and compared. */
void PrintTrial(const char* const Workload, const std::uint32_t Trial, const FTrialResult& Result)
{
	std::printf(
		"microworld,0.1.0,%s,%" PRIu32 ",%" PRIu32 ",%" PRIu32 ",%" PRId32 ",%u,%" PRIu32 ",%s\n",
		Workload,
		Trial,
		Result.Cycles,
		Result.CounterOverhead,
		Result.HeapDelta,
		static_cast<unsigned>(Result.StackHighWaterWords),
		static_cast<std::uint32_t>(esp_clk_cpu_freq()),
		Result.bPassed ? "pass" : "fail");
}

/** Measures lifecycle dispatch with an intentionally disabled Actor schedule. */
void RunDisabledTrials()
{
	FBenchmarkActor Actor({true, false, 0});
	MicroWorld::TWorld<1> World;
	const bool bSetupPassed =
		World.AddActor(Actor) == MicroWorld::ERuntimeResult::Success && World.BeginPlay(0) == MicroWorld::ERuntimeResult::Success;
	for (std::uint32_t Trial = 0; Trial < TrialCount; ++Trial)
	{
		const MicroWorld::TimePointMilliseconds BaseTime = static_cast<MicroWorld::TimePointMilliseconds>(Trial) * UpdatesPerTrial;
		FTrialResult Result = RunTrial([&](const std::uint32_t Update)
									   { return bSetupPassed && World.Advance(BaseTime + Update) == MicroWorld::ERuntimeResult::Success; });
		Result.bPassed = Result.bPassed && Actor.TickCount() == 0;
		PrintTrial("disabled", Trial, Result);
	}
	(void)World.EndPlay();
}

/** Measures one Actor/Component pair and validates their independent cadences. */
void RunSingleActorTrials(
	const char* const Workload,
	const MicroWorld::FTickConfiguration ActorConfiguration,
	const MicroWorld::FTickConfiguration ComponentConfiguration,
	const std::uint32_t ExpectedActorTicksPerTrial,
	const std::uint32_t ExpectedComponentTicksPerTrial)
{
	FBenchmarkComponent Component(ComponentConfiguration);
	FBenchmarkActor Actor(ActorConfiguration);
	MicroWorld::TWorld<1> World;
	const bool bSetupPassed = Actor.AddComponent(Component) == MicroWorld::ERuntimeResult::Success
		&& World.AddActor(Actor) == MicroWorld::ERuntimeResult::Success && World.BeginPlay(0) == MicroWorld::ERuntimeResult::Success;
	for (std::uint32_t Trial = 0; Trial < TrialCount; ++Trial)
	{
		const MicroWorld::TimePointMilliseconds BaseTime = static_cast<MicroWorld::TimePointMilliseconds>(Trial) * UpdatesPerTrial;
		FTrialResult Result = RunTrial([&](const std::uint32_t Update)
									   { return bSetupPassed && World.Advance(BaseTime + Update) == MicroWorld::ERuntimeResult::Success; });
		const std::uint32_t CompletedTrialCount = Trial + 1;
		Result.bPassed = Result.bPassed && Actor.TickCount() == CompletedTrialCount * ExpectedActorTicksPerTrial
			&& Component.TickCount() == CompletedTrialCount * ExpectedComponentTicksPerTrial;
		PrintTrial(Workload, Trial, Result);
	}
	(void)World.EndPlay();
}

/** Measures all registration slots and validates every Actor and Component execution. */
void RunMaximumCapacityTrials()
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
	bool bSetupPassed = true;
	for (std::size_t ActorIndex = 0; ActorIndex < Actors.size(); ++ActorIndex)
	{
		for (FBenchmarkComponent& Component : Components[ActorIndex])
		{
			bSetupPassed = bSetupPassed && Actors[ActorIndex].AddComponent(Component) == MicroWorld::ERuntimeResult::Success;
		}
		bSetupPassed = bSetupPassed && World.AddActor(Actors[ActorIndex]) == MicroWorld::ERuntimeResult::Success;
	}
	bSetupPassed = bSetupPassed && World.BeginPlay(0) == MicroWorld::ERuntimeResult::Success;
	for (std::uint32_t Trial = 0; Trial < TrialCount; ++Trial)
	{
		const MicroWorld::TimePointMilliseconds BaseTime = static_cast<MicroWorld::TimePointMilliseconds>(Trial) * UpdatesPerTrial;
		FTrialResult Result = RunTrial([&](const std::uint32_t Update)
									   { return bSetupPassed && World.Advance(BaseTime + Update) == MicroWorld::ERuntimeResult::Success; });
		const std::uint32_t ExpectedTicks = (Trial + 1) * UpdatesPerTrial;
		for (std::size_t ActorIndex = 0; ActorIndex < Actors.size(); ++ActorIndex)
		{
			Result.bPassed = Result.bPassed && Actors[ActorIndex].TickCount() == ExpectedTicks;
			for (const FBenchmarkComponent& Component : Components[ActorIndex])
			{
				Result.bPassed = Result.bPassed && Component.TickCount() == ExpectedTicks;
			}
		}
		PrintTrial("maximum-capacity", Trial, Result);
	}
	(void)World.EndPlay();
}

} // namespace

static_assert(MicroWorld::Version.Major == 0);
static_assert(MicroWorld::Version.Minor == 1);
static_assert(MicroWorld::Version.Patch == 0);

/** Runs the fixed target suite and emits its self-describing CSV schema. */
extern "C" void app_main()
{
	std::printf("suite,version,workload,trial,cycles,counter_overhead,heap_delta,"
				"stack_high_water_words,cpu_hz,status\n");
	RunDisabledTrials();
	RunSingleActorTrials("all-due", {true, true, 0}, {true, true, 0}, ExpectedTicksPerTrial(0), ExpectedTicksPerTrial(0));
	RunSingleActorTrials(
		"mixed-rate",
		{true, true, MixedActorInterval},
		{true, true, MixedComponentInterval},
		ExpectedTicksPerTrial(MixedActorInterval),
		ExpectedTicksPerTrial(MixedComponentInterval));
	RunMaximumCapacityTrials();
}
