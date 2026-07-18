#include "TestSupport.h"

#include <MicroWorld/Application.h>
#include <MicroWorld/Network.h>
#include <MicroWorld/World.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace
{

using MicroWorld::DurationMilliseconds;
using MicroWorld::ERuntimeResult;
using MicroWorld::FApplication;
using MicroWorld::FNetwork;
using MicroWorld::FTickConfiguration;
using MicroWorld::FTickContext;
using MicroWorld::TActor;
using MicroWorld::TimePointMilliseconds;
using MicroWorld::TWorld;

enum class EApplicationEvent
{
	NetworkBegin, ///< Identifies the transport-boundary startup phase.
	ActorBegin,	  ///< Identifies World-owned runtime startup.
	NetworkTick,  ///< Identifies independent Network schedule execution.
	RouteInput,	  ///< Identifies consumer policy between subsystem updates.
	ActorTick,	  ///< Identifies World-owned runtime execution.
	ActorEnd,	  ///< Identifies World-owned runtime shutdown.
	NetworkEnd,	  ///< Identifies the transport-boundary shutdown phase.
};

template<std::size_t Capacity>
/** Records a bounded application/subsystem ordering trace. */
class TApplicationEventLog final
{
public:
	/** Retains observable composition order without allocation. */
	void Add(const EApplicationEvent Event) noexcept
	{
		if (Count < Capacity)
		{
			Events[Count] = Event;
			++Count;
		}
	}

	/** Reuses fixed storage when one test needs a fresh observation phase. */
	void Clear() noexcept { Count = 0; }

	/** Exposes only initialized events so tests never inspect spare capacity. */
	std::size_t Num() const noexcept { return Count; }

	/** Lets tests compare exact consumer-selected subsystem order by index. */
	EApplicationEvent At(const std::size_t Index) const noexcept { return Events[Index]; }

private:
	/** Keeps composition traces deterministic and independent from heap availability. */
	std::array<EApplicationEvent, Capacity> Events{};

	/** Separates initialized observations from unused fixed capacity. */
	std::size_t Count{0};
};

/** Gives composition tests enough bounded trace capacity for their largest scenario. */
using FApplicationEventLog = TApplicationEventLog<24>;

/** Records policy-free Network lifecycle and opaque staged input. */
class FRecordingNetwork final : public FNetwork
{
public:
	/** Shares caller-owned evidence while selecting the schedule under test. */
	FRecordingNetwork(FApplicationEventLog& InEvents, const FTickConfiguration TickConfiguration) noexcept
		: FNetwork(TickConfiguration), Events(InEvents)
	{
	}

	/** Separates product sampling from scheduled transport application. */
	void StageInput(const std::uint32_t Value) noexcept { StagedInput = Value; }

	/** Proves Network startup executes exactly once when accepted. */
	std::size_t BeginCount{0};

	/** Proves Network cadence is independent from World cadence. */
	std::size_t TickCount{0};

	/** Proves Network shutdown executes exactly once when accepted. */
	std::size_t EndCount{0};

	/** Makes opaque staged-data delivery observable without protocol semantics. */
	std::uint32_t AppliedInput{0};

	/** Preserves the latest independent Network timing context for assertions. */
	FTickContext LastTickContext{};

private:
	/** Records accepted Network startup at the consumer hook boundary. */
	void OnNetworkBeginPlay() override
	{
		++BeginCount;
		Events.Add(EApplicationEvent::NetworkBegin);
	}

	/** Applies staged opaque input only when the Network schedule is due. */
	void TickNetwork(const FTickContext& Context) override
	{
		++TickCount;
		LastTickContext = Context;
		AppliedInput = StagedInput;
		Events.Add(EApplicationEvent::NetworkTick);
	}

	/** Records accepted Network shutdown at the consumer hook boundary. */
	void OnNetworkEndPlay() override
	{
		++EndCount;
		Events.Add(EApplicationEvent::NetworkEnd);
	}

	/** Shares the caller-owned trace without changing production ownership rules. */
	FApplicationEventLog& Events;

	/** Holds opaque consumer data until scheduled transport work observes it. */
	std::uint32_t StagedInput{0};
};

/** Records Actor lifecycle within the application composition test. */
class FRecordingApplicationActor final : public TActor<0>
{
public:
	/** Shares caller-owned evidence while selecting the Actor schedule under test. */
	FRecordingApplicationActor(FApplicationEventLog& InEvents, const FTickConfiguration TickConfiguration) noexcept
		: TActor<0>(TickConfiguration), Events(InEvents)
	{
	}

	/** Proves Actor startup executes exactly once inside composition order. */
	std::size_t BeginCount{0};

	/** Proves World updates execute independently from Network cadence. */
	std::size_t TickCount{0};

	/** Proves Actor shutdown executes exactly once inside composition order. */
	std::size_t EndCount{0};

private:
	/** Records Actor startup at the World-owned runtime boundary. */
	void BeginPlay() override
	{
		++BeginCount;
		Events.Add(EApplicationEvent::ActorBegin);
	}

	/** Records Actor execution without adding product behavior. */
	void Tick(const FTickContext&) override
	{
		++TickCount;
		Events.Add(EApplicationEvent::ActorTick);
	}

	/** Records Actor shutdown at the World-owned runtime boundary. */
	void EndPlay() override
	{
		++EndCount;
		Events.Add(EApplicationEvent::ActorEnd);
	}

	/** Shares the caller-owned trace without changing production ownership rules. */
	FApplicationEventLog& Events;
};

/** Defines consumer-owned Network, routing, and World order for tests. */
class FSequencedApplication final : public FApplication
{
public:
	/** Receives non-owning subsystem references so tests model a real composition root. */
	FSequencedApplication(FApplicationEventLog& InEvents, FRecordingNetwork& InNetwork, TWorld<1>& InWorld) noexcept
		: Events(InEvents), Network(InNetwork), World(InWorld)
	{
	}

	/** Models product sampling before the canonical application frame begins. */
	void StageProductInput(const std::uint32_t Value) noexcept { Network.StageInput(Value); }

	/** Proves consumer policy executes between Network and World updates. */
	std::size_t RouteCount{0};

private:
	/** Starts Network before World to demonstrate consumer-owned composition order. */
	ERuntimeResult OnBeginPlay(const TimePointMilliseconds NowMilliseconds) override
	{
		const ERuntimeResult NetworkResult = Network.BeginPlay(NowMilliseconds);
		if (NetworkResult != ERuntimeResult::Success)
		{
			return NetworkResult;
		}

		const ERuntimeResult WorldResult = World.BeginPlay(NowMilliseconds);
		if (WorldResult != ERuntimeResult::Success)
		{
			Network.EndPlay();
			return WorldResult;
		}
		return ERuntimeResult::Success;
	}

	/** Rolls back both subsystems because a failed outer begin becomes terminal. */
	void OnBeginPlayFailed() noexcept override
	{
		World.EndPlay();
		Network.EndPlay();
	}

	/** Routes opaque input after Network work and before World work. */
	ERuntimeResult OnAdvance(const TimePointMilliseconds NowMilliseconds) override
	{
		const ERuntimeResult NetworkResult = Network.Advance(NowMilliseconds);
		if (NetworkResult != ERuntimeResult::Success)
		{
			return NetworkResult;
		}

		++RouteCount;
		Events.Add(EApplicationEvent::RouteInput);
		return World.Advance(NowMilliseconds);
	}

	/** Stops World before Network to reverse the selected startup dependency order. */
	void OnEndPlay() override
	{
		World.EndPlay();
		Network.EndPlay();
	}

	/** Shares the caller-owned trace used to prove composition order. */
	FApplicationEventLog& Events;

	/** References the independently scheduled transport boundary owned by the test. */
	FRecordingNetwork& Network;

	/** References the deterministic runtime hierarchy owned by the test. */
	TWorld<1>& World;
};

/** Simulates a partial consumer begin that must roll back once. */
class FFailingCompositionApplication final : public FApplication
{
public:
	/** Retains the started Network so the failure path can prove rollback. */
	explicit FFailingCompositionApplication(FRecordingNetwork& InNetwork) noexcept : Network(InNetwork) {}

	/** Detects accidental repeated startup after terminal failure. */
	std::size_t BeginAttemptCount{0};

	/** Detects missing or repeated rollback callbacks. */
	std::size_t RollbackCount{0};

	/** Detects any update that escapes a terminal begin failure. */
	std::size_t AdvanceCount{0};

	/** Detects any normal shutdown hook that escapes a terminal begin failure. */
	std::size_t EndCount{0};

private:
	/** Starts one subsystem before returning a deliberate failure for rollback coverage. */
	ERuntimeResult OnBeginPlay(const TimePointMilliseconds NowMilliseconds) override
	{
		++BeginAttemptCount;
		const ERuntimeResult NetworkResult = Network.BeginPlay(NowMilliseconds);
		if (NetworkResult != ERuntimeResult::Success)
		{
			return NetworkResult;
		}
		return ERuntimeResult::CapacityExceeded;
	}

	/** Stops the partially started subsystem exactly once. */
	void OnBeginPlayFailed() noexcept override
	{
		++RollbackCount;
		Network.EndPlay();
	}

	/** Records any invalid post-failure dispatch that reaches consumer policy. */
	ERuntimeResult OnAdvance(TimePointMilliseconds) override
	{
		++AdvanceCount;
		return ERuntimeResult::Success;
	}

	/** Records any invalid normal shutdown that reaches consumer policy. */
	void OnEndPlay() override { ++EndCount; }

	/** References the only partially started subsystem that must be rolled back. */
	FRecordingNetwork& Network;
};

/** Exposes configurable consumer outcomes at the Application boundary. */
class FConfigurableApplication final : public FApplication
{
public:
	/** Lets each test select startup success or failure without a second fake type. */
	ERuntimeResult BeginOutcome{ERuntimeResult::Success};

	/** Lets each test select frame success or failure without platform policy. */
	ERuntimeResult AdvanceOutcome{ERuntimeResult::Success};

	/** Proves lifecycle validation controls startup-hook reachability. */
	std::size_t BeginCount{0};

	/** Proves failed startup invokes rollback exactly once. */
	std::size_t RollbackCount{0};

	/** Proves lifecycle validation controls frame-hook reachability. */
	std::size_t AdvanceCount{0};

	/** Proves successful shutdown invokes the consumer hook exactly once. */
	std::size_t EndCount{0};

	/** Proves accepted canonical time reaches consumer policy unchanged. */
	TimePointMilliseconds LastAdvanceTime{0};

private:
	/** Returns the selected outcome so outer lifecycle propagation can be tested. */
	ERuntimeResult OnBeginPlay(TimePointMilliseconds) override
	{
		++BeginCount;
		return BeginOutcome;
	}

	/** Records the rollback callback selected by a failed begin outcome. */
	void OnBeginPlayFailed() noexcept override { ++RollbackCount; }

	/** Records canonical time before returning the selected frame outcome. */
	ERuntimeResult OnAdvance(const TimePointMilliseconds NowMilliseconds) override
	{
		++AdvanceCount;
		LastAdvanceTime = NowMilliseconds;
		return AdvanceOutcome;
	}

	/** Records accepted shutdown without adding subsystem behavior. */
	void OnEndPlay() override { ++EndCount; }
};

/** Stages product input before delegating canonical time to Application. */
class FProductRuntime final
{
public:
	/** References the composition root that receives staged product input. */
	explicit FProductRuntime(FSequencedApplication& InApplication) noexcept : Application(InApplication) {}

	/** Preserves the product boundary: sample first, then dispatch one canonical frame. */
	ERuntimeResult Tick(const std::uint32_t SampledInput, const TimePointMilliseconds NowMilliseconds) noexcept
	{
		Application.StageProductInput(SampledInput);
		return Application.Advance(NowMilliseconds);
	}

private:
	/** Keeps product shell ownership outside the framework and test adapter. */
	FSequencedApplication& Application;
};

/** Removes cadence delay when composition tests need every update to execute. */
constexpr FTickConfiguration EnabledEveryUpdate{
	true,
	true,
	DurationMilliseconds{0},
};

static_assert(!std::is_copy_constructible<FApplication>::value);
static_assert(!std::is_copy_assignable<FApplication>::value);
static_assert(!std::is_move_constructible<FApplication>::value);
static_assert(!std::is_move_assignable<FApplication>::value);
static_assert(!std::is_copy_constructible<FNetwork>::value);
static_assert(!std::is_copy_assignable<FNetwork>::value);
static_assert(!std::is_move_constructible<FNetwork>::value);
static_assert(!std::is_move_assignable<FNetwork>::value);

/** Proves the consumer composition root, not MicroWorld, owns subsystem order. */
MW_TEST_CASE(DerivedApplicationControlsNetworkAndWorldOrder)
{
	FApplicationEventLog Events;
	FRecordingNetwork Network(Events, EnabledEveryUpdate);
	FRecordingApplicationActor Actor(Events, EnabledEveryUpdate);
	TWorld<1> World;
	const ERuntimeResult AddActorResult = World.AddActor(Actor);
	FSequencedApplication Application(Events, Network, World);

	const ERuntimeResult BeginResult = Application.BeginPlay(10);
	const ERuntimeResult AdvanceResult = Application.Advance(11);
	const ERuntimeResult EndResult = Application.EndPlay();

	constexpr std::array<EApplicationEvent, 7> ExpectedEvents{
		EApplicationEvent::NetworkBegin,
		EApplicationEvent::ActorBegin,
		EApplicationEvent::NetworkTick,
		EApplicationEvent::RouteInput,
		EApplicationEvent::ActorTick,
		EApplicationEvent::ActorEnd,
		EApplicationEvent::NetworkEnd,
	};
	const std::size_t EventCount = Events.Num();
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, AddActorResult, "Application Actor registers before composition begins");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, BeginResult, "Derived Application begins Network and World");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, AdvanceResult, "Derived Application advances its composition");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, EndResult, "Derived Application ends its composition");
	MW_EXPECT_EQ(Test, ExpectedEvents.size(), EventCount, "Application lifecycle records every expected composition event");
	for (std::size_t Index = 0; Index < ExpectedEvents.size(); ++Index)
	{
		const EApplicationEvent ActualEvent = Events.At(Index);
		const EApplicationEvent ExpectedEvent = ExpectedEvents[Index];
		MW_EXPECT_EQ(Test, ExpectedEvent, ActualEvent, "Derived Application controls Network routing and World order");
	}
}

/** Proves a policy-free Network keeps cadence independent from World dispatch. */
MW_TEST_CASE(NetworkTickIntervalIsIndependentFromWorldUpdate)
{
	FApplicationEventLog Events;
	const FTickConfiguration NetworkTickConfiguration{
		true,
		true,
		DurationMilliseconds{10},
	};
	FRecordingNetwork Network(Events, NetworkTickConfiguration);
	FRecordingApplicationActor Actor(Events, EnabledEveryUpdate);
	TWorld<1> World;
	const ERuntimeResult AddActorResult = World.AddActor(Actor);
	FSequencedApplication Application(Events, Network, World);
	const ERuntimeResult BeginResult = Application.BeginPlay(0);

	const ERuntimeResult FirstAdvanceResult = Application.Advance(0);
	const ERuntimeResult EarlyAdvanceResult = Application.Advance(5);
	const ERuntimeResult DueAdvanceResult = Application.Advance(10);

	const std::size_t NetworkTickCount = Network.TickCount;
	const std::size_t ActorTickCount = Actor.TickCount;
	const std::size_t RouteCount = Application.RouteCount;
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, AddActorResult, "World Actor registers for independent scheduling test");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, BeginResult, "Application begins independent Network and World schedules");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, FirstAdvanceResult, "First Application update executes due work");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, EarlyAdvanceResult, "Early Application update succeeds without forcing Network tick");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, DueAdvanceResult, "Due Application update executes Network tick");
	MW_EXPECT_EQ(Test, std::size_t{2}, NetworkTickCount, "Network ticks only at its own configured cadence");
	MW_EXPECT_EQ(Test, std::size_t{3}, ActorTickCount, "World Actor ticks on every World update independently");
	MW_EXPECT_EQ(Test, std::size_t{3}, RouteCount, "Application routing occurs on every accepted update");
}

/** Proves partial composition startup is rolled back once and cannot resume. */
MW_TEST_CASE(FailedApplicationBeginRollsBackOnceAndBecomesTerminal)
{
	FApplicationEventLog Events;
	FRecordingNetwork Network(Events, EnabledEveryUpdate);
	FFailingCompositionApplication Application(Network);

	const ERuntimeResult BeginResult = Application.BeginPlay(25);
	const ERuntimeResult RepeatedBeginResult = Application.BeginPlay(25);
	const ERuntimeResult AdvanceResult = Application.Advance(26);
	const ERuntimeResult EndResult = Application.EndPlay();

	const std::size_t BeginAttemptCount = Application.BeginAttemptCount;
	const std::size_t RollbackCount = Application.RollbackCount;
	const std::size_t AdvanceCount = Application.AdvanceCount;
	const std::size_t EndCount = Application.EndCount;
	const std::size_t NetworkBeginCount = Network.BeginCount;
	const std::size_t NetworkEndCount = Network.EndCount;
	MW_EXPECT_EQ(Test, ERuntimeResult::CapacityExceeded, BeginResult, "Consumer begin failure is returned by Application");
	MW_EXPECT_EQ(Test, ERuntimeResult::InvalidLifecycle, RepeatedBeginResult, "Failed Application rejects a repeated BeginPlay");
	MW_EXPECT_EQ(Test, ERuntimeResult::InvalidLifecycle, AdvanceResult, "Failed Application rejects subsequent Advance");
	MW_EXPECT_EQ(Test, ERuntimeResult::InvalidLifecycle, EndResult, "Failed Application rejects subsequent EndPlay");
	MW_EXPECT_EQ(Test, std::size_t{1}, BeginAttemptCount, "Terminal failure prevents a second consumer begin attempt");
	MW_EXPECT_EQ(Test, std::size_t{1}, RollbackCount, "Failed begin invokes rollback exactly once");
	MW_EXPECT_EQ(Test, std::size_t{0}, AdvanceCount, "Terminal Application failure dispatches no consumer advance");
	MW_EXPECT_EQ(Test, std::size_t{0}, EndCount, "Terminal Application failure dispatches no normal end hook");
	MW_EXPECT_EQ(Test, std::size_t{1}, NetworkBeginCount, "Composition Network begins once before consumer failure");
	MW_EXPECT_EQ(Test, std::size_t{1}, NetworkEndCount, "Application rollback ends the begun Network exactly once");
}

/** Proves the outer guard blocks frames outside the composition lifecycle. */
MW_TEST_CASE(ApplicationRejectsAdvanceBeforeBeginAndAfterEnd)
{
	FConfigurableApplication Application;

	const ERuntimeResult BeforeBeginResult = Application.Advance(1);
	const ERuntimeResult BeginResult = Application.BeginPlay(2);
	const ERuntimeResult AcceptedAdvanceResult = Application.Advance(3);
	const ERuntimeResult EndResult = Application.EndPlay();
	const ERuntimeResult AfterEndResult = Application.Advance(4);

	const std::size_t AdvanceCount = Application.AdvanceCount;
	const std::size_t EndCount = Application.EndCount;
	MW_EXPECT_EQ(Test, ERuntimeResult::InvalidLifecycle, BeforeBeginResult, "Application rejects Advance before BeginPlay");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, BeginResult, "Application begins before accepted Advance");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, AcceptedAdvanceResult, "Playing Application accepts monotonic Advance");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, EndResult, "Playing Application accepts EndPlay");
	MW_EXPECT_EQ(Test, ERuntimeResult::InvalidLifecycle, AfterEndResult, "Application rejects Advance after EndPlay");
	MW_EXPECT_EQ(Test, std::size_t{1}, AdvanceCount, "Only playing Application Advance reaches consumer hook");
	MW_EXPECT_EQ(Test, std::size_t{1}, EndCount, "Application end hook executes exactly once");
}

/** Proves consumers can stage product data before one canonical framework update. */
MW_TEST_CASE(ProductTickStagesInputThenCallsApplicationAdvance)
{
	FApplicationEventLog Events;
	FRecordingNetwork Network(Events, EnabledEveryUpdate);
	FRecordingApplicationActor Actor(Events, EnabledEveryUpdate);
	TWorld<1> World;
	const ERuntimeResult AddActorResult = World.AddActor(Actor);
	FSequencedApplication Application(Events, Network, World);
	FProductRuntime Product(Application);
	const ERuntimeResult BeginResult = Application.BeginPlay(0);
	constexpr std::uint32_t SampledInput = 0xA5A55A5AU;

	const ERuntimeResult TickResult = Product.Tick(SampledInput, 1);

	const std::uint32_t AppliedInput = Network.AppliedInput;
	const std::size_t RouteCount = Application.RouteCount;
	const std::size_t ActorTickCount = Actor.TickCount;
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, AddActorResult, "Product World Actor registers before runtime starts");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, BeginResult, "Product Application begins before Tick input");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, TickResult, "Product Tick forwards staged input through Application Advance");
	MW_EXPECT_EQ(Test, SampledInput, AppliedInput, "Network consumes the product input staged for this update");
	MW_EXPECT_EQ(Test, std::size_t{1}, RouteCount, "Product Tick causes exactly one Application routing pass");
	MW_EXPECT_EQ(Test, std::size_t{1}, ActorTickCount, "Product Tick reaches the World through Application Advance");
}

/** Proves the application boundary reports consumer failure without logging policy. */
MW_TEST_CASE(ApplicationPropagatesConsumerErrorsWithoutPlatformPolicy)
{
	FConfigurableApplication Application;
	Application.AdvanceOutcome = ERuntimeResult::AlreadyOwned;
	const ERuntimeResult BeginResult = Application.BeginPlay(100);

	const ERuntimeResult ConsumerErrorResult = Application.Advance(101);

	const std::size_t AdvanceCountAfterError = Application.AdvanceCount;
	const TimePointMilliseconds LastAdvanceTime = Application.LastAdvanceTime;
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, BeginResult, "Application begins before consumer error propagation");
	MW_EXPECT_EQ(Test, ERuntimeResult::AlreadyOwned, ConsumerErrorResult, "Application returns the consumer-defined error unchanged");
	MW_EXPECT_EQ(Test, std::size_t{1}, AdvanceCountAfterError, "Application invokes consumer update once without hidden retry");
	MW_EXPECT_EQ(Test, TimePointMilliseconds{101}, LastAdvanceTime, "Application forwards canonical time without platform translation");

	Application.AdvanceOutcome = ERuntimeResult::Success;
	const ERuntimeResult RecoveryResult = Application.Advance(102);
	const std::size_t AdvanceCountAfterRecovery = Application.AdvanceCount;
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, RecoveryResult, "Consumer error does not impose a framework terminal policy");
	MW_EXPECT_EQ(Test, std::size_t{2}, AdvanceCountAfterRecovery, "Later consumer update remains explicitly controlled by consumer");
}

/** Proves scheduling can transport opaque consumer state without protocol semantics. */
MW_TEST_CASE(NetworkSchedulingCarriesOpaqueConsumerDataWithoutProtocolPolicy)
{
	FApplicationEventLog Events;
	const FTickConfiguration TickConfiguration{
		true,
		true,
		DurationMilliseconds{20},
	};
	FRecordingNetwork Network(Events, TickConfiguration);
	constexpr std::uint32_t FirstOpaqueValue = 0x10203040U;
	constexpr std::uint32_t SecondOpaqueValue = 0x55667788U;
	Network.StageInput(FirstOpaqueValue);
	const ERuntimeResult BeginResult = Network.BeginPlay(50);

	const ERuntimeResult FirstAdvanceResult = Network.Advance(50);
	Network.StageInput(SecondOpaqueValue);
	const ERuntimeResult EarlyAdvanceResult = Network.Advance(69);
	const std::uint32_t ValueBeforeDueTime = Network.AppliedInput;
	const ERuntimeResult DueAdvanceResult = Network.Advance(70);

	const std::uint32_t ValueAtDueTime = Network.AppliedInput;
	const std::size_t TickCount = Network.TickCount;
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, BeginResult, "Policy-free Network begins with consumer-selected cadence");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, FirstAdvanceResult, "First Network update executes its consumer hook");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, EarlyAdvanceResult, "Early Network update succeeds without dispatch");
	MW_EXPECT_EQ(Test, FirstOpaqueValue, ValueBeforeDueTime, "Scheduler does not interpret or prematurely consume staged data");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, DueAdvanceResult, "Due Network update dispatches its consumer hook");
	MW_EXPECT_EQ(Test, SecondOpaqueValue, ValueAtDueTime, "Network consumer defines opaque data semantics at dispatch");
	MW_EXPECT_EQ(Test, std::size_t{2}, TickCount, "Network scheduler dispatches once per due update without retries");
}

} // namespace
