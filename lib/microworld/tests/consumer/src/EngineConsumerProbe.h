#pragma once

#include "ObjectConsumerProbe.h"

#include <MicroWorld/Engine/Actor.h>
#include <MicroWorld/Engine/ActorComponent.h>
#include <MicroWorld/Engine/EngineClassIds.h>
#include <MicroWorld/Engine/EngineStorage.h>
#include <MicroWorld/Engine/World.h>
#include <MicroWorld/Object/GarbageCollector.h>
#include <MicroWorld/Object/ObjectStore.h>
#include <MicroWorld/Version.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

static_assert(__cplusplus >= 201703L);

#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
#error "The MicroWorld Engine consumer must compile with exceptions disabled."
#endif

#if defined(__GXX_RTTI) || defined(_CPPRTTI)
#error "The MicroWorld Engine consumer must compile with RTTI disabled."
#endif

namespace MicroWorldConsumer
{

/** Stable process exit codes that identify the exact public-API probe failure. */
enum class EEngineConsumerExitCode : int
{
	Success = 0,
	ComponentBaseRegistrationFailed = 1,
	ActorBaseRegistrationFailed = 2,
	WorldBaseRegistrationFailed = 3,
	DerivedRegistrationFailed = 4,
	StoreConfigurationFailed = 5,
	ObjectCreationFailed = 6,
	ComponentRegistrationFailed = 7,
	ActorRegistrationFailed = 8,
	WorldRootFailed = 9,
	BeginPlayFailed = 10,
	AdvanceFailed = 11,
	EndPlayFailed = 12,
	RootedCollectionFailed = 13,
	UnrootedCollectionFailed = 14,
	ObjectProfileFailureOffset = 100,
};

/** A concrete component proving the engine component base is constructible. */
class FConsumerComponent final : public MicroWorld::UActorComponent
{
public:
	/** Keeps exact descriptor-driven destruction publicly instantiable. */
	~FConsumerComponent() noexcept override = default;
};

/** A concrete actor proving the engine actor base is constructible. */
class FConsumerActor final : public MicroWorld::AActor
{
public:
	/** Forwards store and component storage to the managed actor base. */
	explicit FConsumerActor(MicroWorld::FActorComponentRegistryBase Components) noexcept : AActor(std::move(Components)) {}

	/** Keeps exact descriptor-driven destruction publicly instantiable. */
	~FConsumerActor() noexcept override = default;
};

} // namespace MicroWorldConsumer

/** Exercises representative Core+Memory+Object+Engine public APIs without platform I/O. */
inline int RunEngineConsumerProbe() noexcept
{
	using namespace MicroWorld;
	using MicroWorldConsumer::EEngineConsumerExitCode;
	using MicroWorldConsumer::FConsumerActor;
	using MicroWorldConsumer::FConsumerComponent;

	const int ObjectProfileResult = RunObjectConsumerProbe();
	if (ObjectProfileResult != 0)
	{
		return static_cast<int>(EEngineConsumerExitCode::ObjectProfileFailureOffset) + ObjectProfileResult;
	}

	constexpr std::uint32_t SlotCount = 4;
	constexpr std::uint32_t RootCapacity = 2;
	constexpr std::size_t SlotSizeBytes = 256;
	constexpr std::size_t SlotAlignmentBytes = 16;
	constexpr FTypeId ConsumerActorTypeId{0x00040001u};
	constexpr FTypeId ConsumerComponentTypeId{0x00040002u};

	TClassRegistry<8> Registry;
	if (Registry.Register(UActorComponent::StaticClassDescriptor()) != EObjectResult::Success)
	{
		return static_cast<int>(EEngineConsumerExitCode::ComponentBaseRegistrationFailed);
	}
	if (Registry.Register(AActor::StaticClassDescriptor()) != EObjectResult::Success)
	{
		return static_cast<int>(EEngineConsumerExitCode::ActorBaseRegistrationFailed);
	}
	if (Registry.Register(UWorld::StaticClassDescriptor()) != EObjectResult::Success)
	{
		return static_cast<int>(EEngineConsumerExitCode::WorldBaseRegistrationFailed);
	}
	const FClassDescriptor ActorDescriptor =
		MakeClassDescriptor<FConsumerActor>(ConsumerActorTypeId, "ConsumerActor", Registry.Find(AActorClassId), &TraceManagedObjectReferences);
	const FClassDescriptor ComponentDescriptor = MakeClassDescriptor<FConsumerComponent>(
		ConsumerComponentTypeId, "ConsumerComponent", Registry.Find(UActorComponentClassId), &TraceManagedObjectReferences);
	if (Registry.Register(ActorDescriptor) != EObjectResult::Success || Registry.Register(ComponentDescriptor) != EObjectResult::Success)
	{
		return static_cast<int>(EEngineConsumerExitCode::DerivedRegistrationFailed);
	}

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
		return static_cast<int>(EEngineConsumerExitCode::StoreConfigurationFailed);
	}

	FActorComponentRegistry<1> ActorComponents;
	FWorldActorRegistry<1> WorldActors;
	const TObjectPtr<UWorld> World = Store.NewObject<UWorld>(*Registry.Find(UWorldClassId), WorldActors.MakeView()).Object;
	const TObjectPtr<FConsumerActor> Actor = Store.NewObject<FConsumerActor>(*Registry.Find(ConsumerActorTypeId), ActorComponents.MakeView()).Object;
	const TObjectPtr<FConsumerComponent> Component = Store.NewObject<FConsumerComponent>(*Registry.Find(ConsumerComponentTypeId)).Object;
	if (World.Get() == nullptr || Actor.Get() == nullptr || Component.Get() == nullptr)
	{
		return static_cast<int>(EEngineConsumerExitCode::ObjectCreationFailed);
	}

	if (Actor.Get()->RegisterComponent(Component) != EEngineResult::Success)
	{
		return static_cast<int>(EEngineConsumerExitCode::ComponentRegistrationFailed);
	}
	if (World.Get()->RegisterActor(TObjectPtr<AActor>{Actor}) != EEngineResult::Success)
	{
		return static_cast<int>(EEngineConsumerExitCode::ActorRegistrationFailed);
	}

	TStrongObjectPointerResult<UWorld> WorldRoot = Store.MakeStrongObjectPtr(World);
	if (WorldRoot.Result != EObjectResult::Success)
	{
		return static_cast<int>(EEngineConsumerExitCode::WorldRootFailed);
	}
	if (World.Get()->BeginPlay(0) != ERuntimeResult::Success)
	{
		return static_cast<int>(EEngineConsumerExitCode::BeginPlayFailed);
	}
	if (World.Get()->Advance(1) != ERuntimeResult::Success)
	{
		return static_cast<int>(EEngineConsumerExitCode::AdvanceFailed);
	}
	if (World.Get()->EndPlay() != ERuntimeResult::Success)
	{
		return static_cast<int>(EEngineConsumerExitCode::EndPlayFailed);
	}

	std::array<FObjectHandle, SlotCount> Worklist{};
	FGarbageCollector Collector(Store, FGarbageCollectorStorage{Worklist.data(), SlotCount});
	const FGarbageCollectionResult RootedCollection = Collector.CollectFull();
	if (RootedCollection.Result != ERuntimeResult::Success || RootedCollection.ObjectsReclaimed != 0)
	{
		return static_cast<int>(EEngineConsumerExitCode::RootedCollectionFailed);
	}

	WorldRoot.Pointer.Reset();
	const FGarbageCollectionResult UnrootedCollection = Collector.CollectFull();
	const FObjectStoreStats FinalStats = Store.Stats();
	return UnrootedCollection.Result == ERuntimeResult::Success && UnrootedCollection.ObjectsReclaimed == 3 && FinalStats.OccupiedSlots == 0
		? static_cast<int>(EEngineConsumerExitCode::Success)
		: static_cast<int>(EEngineConsumerExitCode::UnrootedCollectionFailed);
}
