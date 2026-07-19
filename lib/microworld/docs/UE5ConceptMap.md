# UE-Style Concept Map

MicroWorld borrows a few useful C++ concepts from UE. It is not source, binary,
editor, or asset compatible with UE.

| Familiar concept | MicroWorld | State | Difference |
| --- | --- | --- | --- |
| Application root | `FApplication` | Core | Consumer owns and orders concrete services; no global engine |
| World / Actor dispatch | `TWorld<N>` / `TActor<N>` | Core | Fixed-capacity, non-owning registrations |
| Component | `FActorComponent` | Core | Consumer-owned, non-GC component |
| Primary tick | `FTickFunction` | Core | Caller supplies time; no tick groups or catch-up bursts |
| Managed object | `UObject`, handles, roots, GC | Object candidate | Fixed caller-owned storage and explicit tracing |
| Managed World / Actor / Component | `UWorld`, `AActor`, `UActorComponent` | Engine candidate | Application roots World; World/Actor trace children; parent references are weak |
| Timers | `TTimerManager<MaxTimers, InlineCallbackBytes>` | Engine candidate | Fixed capacity, caller time, explicit OneShot/Looping mode allowlist, single-pass post-dispatch compaction, deterministic insertion-order dispatch, no catch-up bursts |
| Network byte I/O | `INetDriver` and `FNetManager` | Next | One non-blocking driver, fixed-capacity manager, bounded bytes, and host loopback |

`TObjectPtr` is a traced managed reference, `TWeakObjectPtr` observes without
retaining, and `TStrongObjectPtr` is an explicit external root. They are not
general-purpose replacements for normal ownership.

`F`, `T`, `E`, `I`, and `b` follow the local naming style. `U` and `A` are
reserved for real MicroWorld managed types; they do not claim Unreal inheritance
or compatibility.

Not part of the first engine: dynamic spawn/destroy, reflection generation,
replication/RPC, background tasks, universal hardware APIs, editor tooling,
rendering, physics, audio, navigation, or asset systems.
