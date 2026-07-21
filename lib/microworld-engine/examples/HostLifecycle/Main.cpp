#include <MicroWorld/Engine/Actor.h>
#include <MicroWorld/Engine/ActorComponent.h>
#include <MicroWorld/Engine/EngineClassIds.h>
#include <MicroWorld/Engine/EngineResult.h>
#include <MicroWorld/Engine/InlineTypes.h>
#include <MicroWorld/Engine/World.h>
#include <MicroWorld/Object/ClassDescriptor.h>
#include <MicroWorld/Object/Object.h>
#include <MicroWorld/Object/ObjectPtr.h>
#include <MicroWorld/Object/ObjectStore.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace
{

/** Samples a host value at its own 100 ms cadence in the managed lifecycle example. */
class FSensorComponent final : public MicroWorld::UActorComponent
{
public:
	/** Selects a 100 ms schedule so the trace includes due and not-due updates. */
	FSensorComponent() noexcept : UActorComponent({true, true, 100}) {}

protected:
	/** Marks Component startup so its order relative to the Actor is visible. */
	void BeginPlay() noexcept override { std::printf("sensor begin\n"); }

	/** Prints canonical time and per-Component delta to demonstrate schedule ownership. */
	void TickComponent(const MicroWorld::FTickContext& Context) noexcept override
	{
		std::printf(
			"sensor tick now=%llu delta=%u\n",
			static_cast<unsigned long long>(Context.NowMilliseconds),
			static_cast<unsigned>(Context.DeltaMilliseconds));
	}

	/** Marks Component shutdown so reverse lifecycle order is visible. */
	void EndPlay() noexcept override { std::printf("sensor end\n"); }
};

/** Aggregates Component state while owning its component registry inline. */
class FDeviceActor final : public MicroWorld::TInlineActor<1>
{
public:
	/** Disables only the Actor schedule so Component independence is observable. */
	FDeviceActor() noexcept : TInlineActor<1>({true, false, 0}) {}

protected:
	/** Marks Actor startup after the Component begin hook. */
	void BeginPlay() noexcept override { std::printf("actor begin (primary tick disabled)\n"); }

	/** Would expose an incorrect Actor execution if disabled scheduling regressed. */
	void Tick(const MicroWorld::FTickContext&) noexcept override { std::printf("actor tick\n"); }

	/** Marks Actor shutdown before the Component end hook. */
	void EndPlay() noexcept override { std::printf("actor end\n"); }
};

/** The example's world owns its single-actor registry inline. */
using FDeviceWorld = MicroWorld::TInlineWorld<1>;

/** Stable type id for the example's inline world descriptor. */
constexpr MicroWorld::FTypeId DeviceWorldTypeId{0x00010003u};

/** Stable type id for the example's user-derived managed actor descriptor. */
constexpr MicroWorld::FTypeId DeviceActorTypeId{0x00010001u};

/** Stable type id for the example's user-derived managed component descriptor. */
constexpr MicroWorld::FTypeId SensorComponentTypeId{0x00010002u};

} // namespace

/** Builds a managed composition from inline world/actor types and prints deterministic lifecycle evidence. */
int main()
{
	using namespace MicroWorld;

	constexpr std::uint32_t SlotCount = 4;
	constexpr std::uint32_t RootCapacity = 2;
	// Inline world/actor types embed their registries, so slots are wider than the
	// lease-composed variant used before the Phase 2 ergonomics work.
	constexpr std::size_t SlotSizeBytes = 512;
	constexpr std::size_t SlotAlignmentBytes = 16;

	// Each concrete managed type needs its own descriptor: the three engine base
	// descriptors plus the inline DeviceWorld and derived DeviceActor/SensorComponent.
	TClassRegistry<8> Registry;
	if (Registry.Register(UActorComponent::StaticClassDescriptor()) != EObjectResult::Success
		|| Registry.Register(AActor::StaticClassDescriptor()) != EObjectResult::Success
		|| Registry.Register(UWorld::StaticClassDescriptor()) != EObjectResult::Success)
	{
		return 1;
	}

	const FClassDescriptor DeviceWorldDescriptor =
		MakeClassDescriptor<FDeviceWorld>(DeviceWorldTypeId, "DeviceWorld", Registry.Find(UWorldClassId), &TraceManagedObjectReferences);
	const FClassDescriptor DeviceDescriptor =
		MakeClassDescriptor<FDeviceActor>(DeviceActorTypeId, "DeviceActor", Registry.Find(AActorClassId), &TraceManagedObjectReferences);
	const FClassDescriptor SensorDescriptor = MakeClassDescriptor<FSensorComponent>(
		SensorComponentTypeId, "SensorComponent", Registry.Find(UActorComponentClassId), &TraceManagedObjectReferences);
	if (Registry.Register(DeviceWorldDescriptor) != EObjectResult::Success || Registry.Register(DeviceDescriptor) != EObjectResult::Success
		|| Registry.Register(SensorDescriptor) != EObjectResult::Success)
	{
		return 1;
	}

	// The store owns every managed lifetime; all of its storage is caller-owned.
	alignas(SlotAlignmentBytes) std::array<std::byte, SlotSizeBytes * SlotCount> SlotBytes{};
	std::array<FObjectSlotMetadata, SlotCount> Slots{};
	std::array<FObjectRootEntry, RootCapacity> Roots{};
	FObjectStore Store(
		FObjectStoreStorage{
			SlotBytes.data(),
			SlotBytes.size(),
			Slots.data(),
			SlotCount,
			SlotSizeBytes,
			SlotAlignmentBytes,
			Roots.data(),
			RootCapacity,
		},
		MakeClassRegistryView(Registry));
	if (Store.ConfigurationResult() != EObjectResult::Success)
	{
		return 1;
	}

	// Inline types carry their own registries, so no FWorldActorRegistry or
	// FActorComponentRegistry object is composed here.
	const TObjectPtr<FDeviceWorld> World = Store.NewObject<FDeviceWorld>(*Registry.Find(DeviceWorldTypeId)).Object;
	const TObjectPtr<FDeviceActor> Device = Store.NewObject<FDeviceActor>(*Registry.Find(DeviceActorTypeId)).Object;
	const TObjectPtr<FSensorComponent> Sensor = Store.NewObject<FSensorComponent>(*Registry.Find(SensorComponentTypeId)).Object;
	if (World.Get() == nullptr || Device.Get() == nullptr || Sensor.Get() == nullptr)
	{
		return 1;
	}

	if (Device.Get()->RegisterComponent(Sensor) != EEngineResult::Success
		|| World.Get()->RegisterActor(TObjectPtr<AActor>{Device}) != EEngineResult::Success
		|| World.Get()->BeginPlay(0) != ERuntimeResult::Success)
	{
		return 1;
	}

	// Early, exact-deadline, and late updates for the 100 ms Component schedule.
	constexpr TimePointMilliseconds Updates[] = {0, 50, 100, 175, 200};
	for (const TimePointMilliseconds Now : Updates)
	{
		if (World.Get()->Advance(Now) != ERuntimeResult::Success)
		{
			return 1;
		}
	}
	return World.Get()->EndPlay() == ERuntimeResult::Success ? 0 : 1;
}
