#include "TestSupport.h"

#include <MicroWorld/Actor.h>
#include <MicroWorld/ActorComponent.h>
#include <MicroWorld/World.h>

#include <array>
#include <cstddef>
#include <type_traits>

namespace
{

using MicroWorld::DurationMilliseconds;
using MicroWorld::ERuntimeResult;
using MicroWorld::FActorBase;
using MicroWorld::FActorComponent;
using MicroWorld::FTickConfiguration;
using MicroWorld::FTickContext;
using MicroWorld::FWorldBase;
using MicroWorld::TActor;
using MicroWorld::TimePointMilliseconds;
using MicroWorld::TWorld;

enum class ERecordedEvent
{
	FirstComponentBegin,  ///< Identifies first registration startup order.
	SecondComponentBegin, ///< Identifies second registration startup order.
	ActorBegin,			  ///< Identifies the parent startup boundary.
	FirstComponentTick,	  ///< Identifies first registration update order.
	SecondComponentTick,  ///< Identifies second registration update order.
	ActorTick,			  ///< Identifies the parent update boundary.
	ActorEnd,			  ///< Identifies the parent shutdown boundary.
	SecondComponentEnd,	  ///< Identifies reverse second registration shutdown.
	FirstComponentEnd,	  ///< Identifies reverse first registration shutdown.
};

template<std::size_t Capacity>
/** Records a bounded observable lifecycle trace for World tests. */
class TEventLog final
{
public:
	/** Retains observable ordering without allocation affecting behavior tests. */
	void Add(const ERecordedEvent Event) noexcept
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

	/** Lets tests compare exact registration/lifecycle order by index. */
	ERecordedEvent At(const std::size_t Index) const noexcept { return Events[Index]; }

private:
	/** Keeps traces deterministic and independent from heap availability. */
	std::array<ERecordedEvent, Capacity> Events{};

	/** Separates initialized observations from unused fixed capacity. */
	std::size_t Count{0};
};

/** Gives World tests enough bounded trace capacity for the largest scenario. */
using FEventLog = TEventLog<24>;

/** Exposes Component hook calls as bounded observable test state. */
class FRecordingComponent final : public FActorComponent
{
public:
	/** Maps each hook to a caller-selected event so two Components stay distinguishable. */
	FRecordingComponent(
		FEventLog& InEvents,
		const ERecordedEvent InBeginEvent,
		const ERecordedEvent InTickEvent,
		const ERecordedEvent InEndEvent,
		const FTickConfiguration TickConfiguration = {}) noexcept
		: FActorComponent(TickConfiguration), Events(InEvents), BeginEvent(InBeginEvent), TickEvent(InTickEvent), EndEvent(InEndEvent)
	{
	}

	/** Proves startup hooks execute exactly once when accepted. */
	std::size_t BeginCount{0};

	/** Proves independent Component scheduling executes the expected number of times. */
	std::size_t TickCount{0};

	/** Proves shutdown hooks execute exactly once when accepted. */
	std::size_t EndCount{0};

	/** Preserves the latest per-Component timing context for delta assertions. */
	FTickContext LastTickContext{};

private:
	/** Records startup at the point consumer behavior becomes observable. */
	void BeginPlay() override
	{
		++BeginCount;
		Events.Add(BeginEvent);
	}

	/** Records schedule context and order without reading scheduler internals. */
	void TickComponent(const FTickContext& Context) override
	{
		++TickCount;
		LastTickContext = Context;
		Events.Add(TickEvent);
	}

	/** Records shutdown at the point consumer behavior becomes observable. */
	void EndPlay() override
	{
		++EndCount;
		Events.Add(EndEvent);
	}

	/** Shares the caller-owned trace without changing production ownership rules. */
	FEventLog& Events;

	/** Distinguishes this Component's startup in multi-registration traces. */
	ERecordedEvent BeginEvent;

	/** Distinguishes this Component's update in multi-registration traces. */
	ERecordedEvent TickEvent;

	/** Distinguishes this Component's shutdown in multi-registration traces. */
	ERecordedEvent EndEvent;
};

template<std::size_t MaxComponents>
/** Exposes Actor hook calls while retaining the requested fixed capacity. */
class TRecordingActor final : public TActor<MaxComponents>
{
public:
	/** Shares the trace while preserving the capacity under test. */
	explicit TRecordingActor(FEventLog& InEvents, const FTickConfiguration TickConfiguration = {}) noexcept
		: TActor<MaxComponents>(TickConfiguration), Events(InEvents)
	{
	}

	/** Proves Actor startup executes exactly once after Components. */
	std::size_t BeginCount{0};

	/** Proves Actor scheduling remains independent from Components. */
	std::size_t TickCount{0};

	/** Proves Actor shutdown executes exactly once before Components. */
	std::size_t EndCount{0};

	/** Preserves the latest per-Actor timing context for delta assertions. */
	FTickContext LastTickContext{};

private:
	/** Records Actor startup without coupling tests to dispatcher internals. */
	void BeginPlay() override
	{
		++BeginCount;
		Events.Add(ERecordedEvent::ActorBegin);
	}

	/** Records Actor schedule context after Component updates. */
	void Tick(const FTickContext& Context) override
	{
		++TickCount;
		LastTickContext = Context;
		Events.Add(ERecordedEvent::ActorTick);
	}

	/** Records Actor shutdown before reverse Component cleanup. */
	void EndPlay() override
	{
		++EndCount;
		Events.Add(ERecordedEvent::ActorEnd);
	}

	/** Shares the caller-owned trace without changing production ownership rules. */
	FEventLog& Events;
};

/** Removes cadence delay when a test needs every accepted update to execute. */
constexpr FTickConfiguration EnabledEveryUpdate{
	true,
	true,
	DurationMilliseconds{0},
};

static_assert(!std::is_copy_constructible<FActorComponent>::value);
static_assert(!std::is_copy_assignable<FActorComponent>::value);
static_assert(!std::is_move_constructible<FActorComponent>::value);
static_assert(!std::is_move_assignable<FActorComponent>::value);
static_assert(!std::is_copy_constructible<TRecordingActor<1>>::value);
static_assert(!std::is_copy_assignable<TRecordingActor<1>>::value);
static_assert(!std::is_move_constructible<TRecordingActor<1>>::value);
static_assert(!std::is_move_assignable<TRecordingActor<1>>::value);
static_assert(!std::is_copy_constructible<TWorld<1>>::value);
static_assert(!std::is_copy_assignable<TWorld<1>>::value);
static_assert(!std::is_move_constructible<TWorld<1>>::value);
static_assert(!std::is_move_assignable<TWorld<1>>::value);

/** Proves hierarchical startup and reverse shutdown preserve dependency order. */
MW_TEST_CASE(ComponentsBeginBeforeActorAndEndAfterActor)
{
	FEventLog Events;
	TRecordingActor<2> Actor(Events);
	FRecordingComponent FirstComponent(
		Events, ERecordedEvent::FirstComponentBegin, ERecordedEvent::FirstComponentTick, ERecordedEvent::FirstComponentEnd);
	FRecordingComponent SecondComponent(
		Events, ERecordedEvent::SecondComponentBegin, ERecordedEvent::SecondComponentTick, ERecordedEvent::SecondComponentEnd);
	TWorld<1> World;
	const ERuntimeResult FirstComponentResult = Actor.AddComponent(FirstComponent);
	const ERuntimeResult SecondComponentResult = Actor.AddComponent(SecondComponent);
	const ERuntimeResult AddActorResult = World.AddActor(Actor);

	const ERuntimeResult BeginResult = World.BeginPlay(10);
	const ERuntimeResult EndResult = World.EndPlay();

	const std::size_t EventCount = Events.Num();
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, FirstComponentResult, "First Component registration succeeds before play");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, SecondComponentResult, "Second Component registration succeeds before play");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, AddActorResult, "Actor registration succeeds before play");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, BeginResult, "World begins its registered lifecycle exactly once");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, EndResult, "World ends its registered lifecycle exactly once");
	MW_EXPECT_EQ(Test, std::size_t{6}, EventCount, "Lifecycle dispatch records all Component and Actor hooks");

	constexpr std::array<ERecordedEvent, 6> ExpectedEvents{
		ERecordedEvent::FirstComponentBegin,
		ERecordedEvent::SecondComponentBegin,
		ERecordedEvent::ActorBegin,
		ERecordedEvent::ActorEnd,
		ERecordedEvent::SecondComponentEnd,
		ERecordedEvent::FirstComponentEnd,
	};
	for (std::size_t Index = 0; Index < ExpectedEvents.size(); ++Index)
	{
		const ERecordedEvent ActualEvent = Events.At(Index);
		const ERecordedEvent ExpectedEvent = ExpectedEvents[Index];
		MW_EXPECT_EQ(Test, ExpectedEvent, ActualEvent, "Lifecycle hook follows the documented deterministic order");
	}
}

/** Proves deterministic registration order and Component-before-Actor updates. */
MW_TEST_CASE(ComponentsTickBeforeActorInRegistrationOrder)
{
	FEventLog Events;
	TRecordingActor<2> Actor(Events, EnabledEveryUpdate);
	FRecordingComponent FirstComponent(
		Events, ERecordedEvent::FirstComponentBegin, ERecordedEvent::FirstComponentTick, ERecordedEvent::FirstComponentEnd, EnabledEveryUpdate);
	FRecordingComponent SecondComponent(
		Events, ERecordedEvent::SecondComponentBegin, ERecordedEvent::SecondComponentTick, ERecordedEvent::SecondComponentEnd, EnabledEveryUpdate);
	TWorld<1> World;
	const ERuntimeResult FirstComponentResult = Actor.AddComponent(FirstComponent);
	const ERuntimeResult SecondComponentResult = Actor.AddComponent(SecondComponent);
	const ERuntimeResult AddActorResult = World.AddActor(Actor);
	const ERuntimeResult BeginResult = World.BeginPlay(100);
	Events.Clear();

	const ERuntimeResult AdvanceResult = World.Advance(101);

	const std::size_t EventCount = Events.Num();
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, FirstComponentResult, "First ticking Component registration succeeds");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, SecondComponentResult, "Second ticking Component registration succeeds");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, AddActorResult, "Ticking Actor registration succeeds");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, BeginResult, "World begins before dispatching an update");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, AdvanceResult, "Monotonic World update dispatches registered objects");
	MW_EXPECT_EQ(Test, std::size_t{3}, EventCount, "One World update dispatches both Components and their Actor");

	constexpr std::array<ERecordedEvent, 3> ExpectedEvents{
		ERecordedEvent::FirstComponentTick,
		ERecordedEvent::SecondComponentTick,
		ERecordedEvent::ActorTick,
	};
	for (std::size_t Index = 0; Index < ExpectedEvents.size(); ++Index)
	{
		const ERecordedEvent ActualEvent = Events.At(Index);
		const ERecordedEvent ExpectedEvent = ExpectedEvents[Index];
		MW_EXPECT_EQ(Test, ExpectedEvent, ActualEvent, "World update ticks Components in registration order before Actor");
	}
}

/** Proves Actor enablement cannot suppress independently configured Components. */
MW_TEST_CASE(DisabledActorDoesNotDisableComponentTicks)
{
	FEventLog Events;
	TRecordingActor<1> Actor(Events);
	FRecordingComponent Component(
		Events, ERecordedEvent::FirstComponentBegin, ERecordedEvent::FirstComponentTick, ERecordedEvent::FirstComponentEnd, EnabledEveryUpdate);
	TWorld<1> World;
	const ERuntimeResult AddComponentResult = Actor.AddComponent(Component);
	const ERuntimeResult AddActorResult = World.AddActor(Actor);
	const ERuntimeResult BeginResult = World.BeginPlay(0);

	const ERuntimeResult AdvanceResult = World.Advance(1);

	const std::size_t ComponentTickCount = Component.TickCount;
	const std::size_t ActorTickCount = Actor.TickCount;
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, AddComponentResult, "Enabled Component registers with disabled Actor");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, AddActorResult, "Disabled Actor registers with World");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, BeginResult, "World begins independent tick lifecycles");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, AdvanceResult, "World update succeeds with disabled Actor tick");
	MW_EXPECT_EQ(Test, std::size_t{1}, ComponentTickCount, "Enabled Component ticks independently of disabled Actor");
	MW_EXPECT_EQ(Test, std::size_t{0}, ActorTickCount, "Disabled Actor does not execute its own tick");
}

/** Proves Component enablement cannot suppress its owning Actor schedule. */
MW_TEST_CASE(DisabledComponentDoesNotDisableActorTick)
{
	FEventLog Events;
	TRecordingActor<1> Actor(Events, EnabledEveryUpdate);
	FRecordingComponent Component(Events, ERecordedEvent::FirstComponentBegin, ERecordedEvent::FirstComponentTick, ERecordedEvent::FirstComponentEnd);
	TWorld<1> World;
	const ERuntimeResult AddComponentResult = Actor.AddComponent(Component);
	const ERuntimeResult AddActorResult = World.AddActor(Actor);
	const ERuntimeResult BeginResult = World.BeginPlay(0);

	const ERuntimeResult AdvanceResult = World.Advance(1);

	const std::size_t ComponentTickCount = Component.TickCount;
	const std::size_t ActorTickCount = Actor.TickCount;
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, AddComponentResult, "Disabled Component registers with enabled Actor");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, AddActorResult, "Enabled Actor registers with World");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, BeginResult, "World begins independent tick lifecycles");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, AdvanceResult, "World update succeeds with disabled Component tick");
	MW_EXPECT_EQ(Test, std::size_t{0}, ComponentTickCount, "Disabled Component does not execute its own tick");
	MW_EXPECT_EQ(Test, std::size_t{1}, ActorTickCount, "Enabled Actor ticks independently of disabled Component");
}

/** Proves rejected registration leaves ownership and bounded storage unchanged. */
MW_TEST_CASE(DuplicateAndOverCapacityRegistrationAreAtomic)
{
	FEventLog Events;
	TRecordingActor<1> FirstActor(Events);
	TRecordingActor<1> RejectedActor(Events);
	FRecordingComponent FirstComponent(
		Events, ERecordedEvent::FirstComponentBegin, ERecordedEvent::FirstComponentTick, ERecordedEvent::FirstComponentEnd);
	FRecordingComponent RejectedComponent(
		Events, ERecordedEvent::SecondComponentBegin, ERecordedEvent::SecondComponentTick, ERecordedEvent::SecondComponentEnd);
	TWorld<1> World;
	const ERuntimeResult AddComponentResult = FirstActor.AddComponent(FirstComponent);
	const ERuntimeResult DuplicateComponentResult = FirstActor.AddComponent(FirstComponent);
	const ERuntimeResult FullComponentResult = FirstActor.AddComponent(RejectedComponent);
	const ERuntimeResult AddActorResult = World.AddActor(FirstActor);
	const ERuntimeResult DuplicateActorResult = World.AddActor(FirstActor);

	const ERuntimeResult FullActorResult = World.AddActor(RejectedActor);

	FActorBase* const FirstComponentOwner = FirstComponent.GetOwner();
	FActorBase* const RejectedComponentOwner = RejectedComponent.GetOwner();
	FWorldBase* const FirstActorWorld = FirstActor.GetWorld();
	FWorldBase* const RejectedActorWorld = RejectedActor.GetWorld();
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, AddComponentResult, "First Component fills the Actor registration slot");
	MW_EXPECT_EQ(Test, ERuntimeResult::Duplicate, DuplicateComponentResult, "Duplicate Component registration is rejected");
	MW_EXPECT_EQ(Test, ERuntimeResult::CapacityExceeded, FullComponentResult, "Component registration beyond capacity is rejected");
	MW_EXPECT_EQ(Test, static_cast<FActorBase*>(&FirstActor), FirstComponentOwner, "Accepted Component keeps its original Actor owner");
	MW_EXPECT_EQ(Test, static_cast<FActorBase*>(nullptr), RejectedComponentOwner, "Capacity rejection leaves Component ownership unchanged");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, AddActorResult, "First Actor fills the World registration slot");
	MW_EXPECT_EQ(Test, ERuntimeResult::Duplicate, DuplicateActorResult, "Duplicate Actor registration is rejected");
	MW_EXPECT_EQ(Test, ERuntimeResult::CapacityExceeded, FullActorResult, "Actor registration beyond capacity is rejected");
	MW_EXPECT_EQ(Test, static_cast<FWorldBase*>(&World), FirstActorWorld, "Accepted Actor keeps its original World owner");
	MW_EXPECT_EQ(Test, static_cast<FWorldBase*>(nullptr), RejectedActorWorld, "Capacity rejection leaves Actor ownership unchanged");
}

/** Proves a zero-capacity aggregate cannot capture ownership on failure. */
MW_TEST_CASE(ZeroCapacityRegistrationRejectsWithoutAssigningOwnership)
{
	FEventLog Events;
	TRecordingActor<0> Actor(Events);
	FRecordingComponent Component(Events, ERecordedEvent::FirstComponentBegin, ERecordedEvent::FirstComponentTick, ERecordedEvent::FirstComponentEnd);
	TWorld<0> World;

	const ERuntimeResult ComponentResult = Actor.AddComponent(Component);
	const ERuntimeResult ActorResult = World.AddActor(Actor);

	FActorBase* const ComponentOwner = Component.GetOwner();
	FWorldBase* const ActorWorld = Actor.GetWorld();
	MW_EXPECT_EQ(Test, ERuntimeResult::CapacityExceeded, ComponentResult, "Zero-capacity Actor rejects its first Component");
	MW_EXPECT_EQ(Test, static_cast<FActorBase*>(nullptr), ComponentOwner, "Zero-capacity rejection does not assign Component owner");
	MW_EXPECT_EQ(Test, ERuntimeResult::CapacityExceeded, ActorResult, "Zero-capacity World rejects its first Actor");
	MW_EXPECT_EQ(Test, static_cast<FWorldBase*>(nullptr), ActorWorld, "Zero-capacity rejection does not assign Actor World");
}

/** Proves dispatch structure cannot mutate after lifecycle observers can run. */
MW_TEST_CASE(RegistrationIsLockedAfterBeginPlay)
{
	FEventLog Events;
	TRecordingActor<1> PlayingActor(Events);
	TRecordingActor<1> LateActor(Events);
	FRecordingComponent LateComponent(
		Events, ERecordedEvent::FirstComponentBegin, ERecordedEvent::FirstComponentTick, ERecordedEvent::FirstComponentEnd);
	TWorld<2> World;
	const ERuntimeResult AddActorResult = World.AddActor(PlayingActor);
	const ERuntimeResult BeginResult = World.BeginPlay(5);

	const ERuntimeResult ComponentResult = PlayingActor.AddComponent(LateComponent);
	const ERuntimeResult ActorResult = World.AddActor(LateActor);

	FActorBase* const ComponentOwner = LateComponent.GetOwner();
	FWorldBase* const ActorWorld = LateActor.GetWorld();
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, AddActorResult, "Initial Actor registration succeeds before play");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, BeginResult, "World begins before late registration attempts");
	MW_EXPECT_EQ(Test, ERuntimeResult::LifecycleLocked, ComponentResult, "Playing Actor rejects late Component registration");
	MW_EXPECT_EQ(Test, static_cast<FActorBase*>(nullptr), ComponentOwner, "Late Component remains unowned after lifecycle rejection");
	MW_EXPECT_EQ(Test, ERuntimeResult::LifecycleLocked, ActorResult, "Playing World rejects late Actor registration");
	MW_EXPECT_EQ(Test, static_cast<FWorldBase*>(nullptr), ActorWorld, "Late Actor remains unowned after lifecycle rejection");
}

/** Proves non-owning registrations still enforce one parent for pointer safety. */
MW_TEST_CASE(ActorAndComponentRejectSecondOwners)
{
	FEventLog Events;
	TRecordingActor<1> OwnedActor(Events);
	TRecordingActor<1> FirstComponentOwner(Events);
	TRecordingActor<1> SecondComponentOwner(Events);
	FRecordingComponent Component(Events, ERecordedEvent::FirstComponentBegin, ERecordedEvent::FirstComponentTick, ERecordedEvent::FirstComponentEnd);
	TWorld<1> FirstWorld;
	TWorld<1> SecondWorld;
	const ERuntimeResult FirstActorOwnerResult = FirstWorld.AddActor(OwnedActor);
	const ERuntimeResult FirstComponentOwnerResult = FirstComponentOwner.AddComponent(Component);

	const ERuntimeResult SecondActorOwnerResult = SecondWorld.AddActor(OwnedActor);
	const ERuntimeResult SecondComponentOwnerResult = SecondComponentOwner.AddComponent(Component);

	FWorldBase* const ActualWorld = OwnedActor.GetWorld();
	FActorBase* const ActualOwner = Component.GetOwner();
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, FirstActorOwnerResult, "Actor accepts its first World owner");
	MW_EXPECT_EQ(Test, ERuntimeResult::AlreadyOwned, SecondActorOwnerResult, "Actor rejects a second World owner");
	MW_EXPECT_EQ(Test, static_cast<FWorldBase*>(&FirstWorld), ActualWorld, "Rejected second World does not replace Actor ownership");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, FirstComponentOwnerResult, "Component accepts its first Actor owner");
	MW_EXPECT_EQ(Test, ERuntimeResult::AlreadyOwned, SecondComponentOwnerResult, "Component rejects a second Actor owner");
	MW_EXPECT_EQ(Test, static_cast<FActorBase*>(&FirstComponentOwner), ActualOwner, "Rejected second Actor does not replace Component ownership");
}

/** Proves World time rollback is rejected before any child observes it. */
MW_TEST_CASE(NonMonotonicWorldUpdateDispatchesNothing)
{
	FEventLog Events;
	TRecordingActor<1> Actor(Events, EnabledEveryUpdate);
	FRecordingComponent Component(
		Events, ERecordedEvent::FirstComponentBegin, ERecordedEvent::FirstComponentTick, ERecordedEvent::FirstComponentEnd, EnabledEveryUpdate);
	TWorld<1> World;
	const ERuntimeResult ComponentResult = Actor.AddComponent(Component);
	const ERuntimeResult ActorResult = World.AddActor(Actor);
	const ERuntimeResult BeginResult = World.BeginPlay(10);
	const ERuntimeResult FirstAdvanceResult = World.Advance(20);
	const std::size_t ComponentTicksBeforeRejection = Component.TickCount;
	const std::size_t ActorTicksBeforeRejection = Actor.TickCount;

	const ERuntimeResult BackwardResult = World.Advance(19);

	const std::size_t ComponentTicksAfterRejection = Component.TickCount;
	const std::size_t ActorTicksAfterRejection = Actor.TickCount;
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, ComponentResult, "Ticking Component registers before monotonic updates");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, ActorResult, "Ticking Actor registers before monotonic updates");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, BeginResult, "World begins before monotonic updates");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, FirstAdvanceResult, "First monotonic World update succeeds");
	MW_EXPECT_EQ(Test, ERuntimeResult::NonMonotonicTime, BackwardResult, "Backward World time is rejected");
	MW_EXPECT_EQ(Test, ComponentTicksBeforeRejection, ComponentTicksAfterRejection, "Backward World time dispatches no Component tick");
	MW_EXPECT_EQ(Test, ActorTicksBeforeRejection, ActorTicksAfterRejection, "Backward World time dispatches no Actor tick");
}

/** Proves invalid transitions and repeated end cannot duplicate consumer side effects. */
MW_TEST_CASE(RepeatedInvalidLifecycleCallsDoNotDispatchHooksTwice)
{
	FEventLog Events;
	TRecordingActor<0> Actor(Events);
	TWorld<1> World;
	const ERuntimeResult AddResult = World.AddActor(Actor);
	const ERuntimeResult AdvanceBeforeBeginResult = World.Advance(0);
	const ERuntimeResult EndBeforeBeginResult = World.EndPlay();
	const std::size_t BeginCountBeforeBegin = Actor.BeginCount;
	const std::size_t EndCountBeforeBegin = Actor.EndCount;
	const ERuntimeResult BeginResult = World.BeginPlay(1);

	const ERuntimeResult RepeatedBeginResult = World.BeginPlay(1);
	const ERuntimeResult EndResult = World.EndPlay();
	const ERuntimeResult RepeatedEndResult = World.EndPlay();

	const std::size_t BeginCount = Actor.BeginCount;
	const std::size_t EndCount = Actor.EndCount;
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, AddResult, "Actor registers before lifecycle validation");
	MW_EXPECT_EQ(Test, ERuntimeResult::InvalidLifecycle, AdvanceBeforeBeginResult, "World rejects advance before BeginPlay");
	MW_EXPECT_EQ(Test, ERuntimeResult::InvalidLifecycle, EndBeforeBeginResult, "World rejects EndPlay before BeginPlay");
	MW_EXPECT_EQ(Test, std::size_t{0}, BeginCountBeforeBegin, "Invalid pre-begin calls dispatch no Actor begin hook");
	MW_EXPECT_EQ(Test, std::size_t{0}, EndCountBeforeBegin, "Invalid pre-begin calls dispatch no Actor end hook");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, BeginResult, "First World BeginPlay succeeds");
	MW_EXPECT_EQ(Test, ERuntimeResult::InvalidLifecycle, RepeatedBeginResult, "Repeated World BeginPlay is rejected");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, EndResult, "First World EndPlay succeeds");
	MW_EXPECT_EQ(Test, ERuntimeResult::Success, RepeatedEndResult, "Repeated World EndPlay remains idempotent");
	MW_EXPECT_EQ(Test, std::size_t{1}, BeginCount, "Repeated BeginPlay does not dispatch Actor begin twice");
	MW_EXPECT_EQ(Test, std::size_t{1}, EndCount, "Repeated EndPlay does not dispatch Actor end twice");
}

} // namespace
