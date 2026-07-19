# UE5 to MicroWorld Concept Map

MicroWorld transfers selected UE5 C++ mental models to bounded embedded
applications. It does not provide UE5 source, binary, module, editor, or asset
compatibility. A familiar name is used only when MicroWorld implements the
ownership and lifecycle semantics that make that name honest.

This source-anchored map covers released Core 0.1 and candidate contracts at
`e1e7b75`. [PROGRESS.md](../PROGRESS.md) owns live gate and promotion state.

## Maturity labels

- **Released** — available in the current public headers and covered by
  behavior tests.
- **Candidate contract at `e1e7b75`** — source-anchored candidate semantics,
  not a released API or accepted target support.
- **Approved direction** — accepted architecture, but not a released API.
- **Deferred** — intentionally excluded until a measured application need
  justifies a new decision.
- **Excluded** — outside the planned lightweight engine.

## Runtime concepts

| UE5 concept | MicroWorld equivalent | Maturity | Important semantic difference |
| --- | --- | --- | --- |
| Engine/application composition root | `FApplication` | Released in 0.1 | Consumer derives and explicitly orders subsystems; there is no global `GEngine` |
| `UWorld` lifecycle and Actor dispatch | `TWorld<MaxActors>` | Released static form in 0.1 | Compile-time-bounded non-owning registrations; no dynamic spawning or GC |
| `AActor` | `TActor<MaxComponents>` | Released static form in 0.1 | `T` prefix is honest because the object is a consumer-owned template, not a managed Actor |
| `UActorComponent` | `FActorComponent` | Released static form in 0.1 | Consumer-owned, non-GC behavior registered with one Actor |
| Primary Actor/Component tick | `FTickFunction` through `FTickable` | Released in 0.1 | Caller supplies monotonic milliseconds; no tick groups, prerequisites, time dilation, or catch-up bursts |
| Engine subsystem | `FNetwork` or a consumer-owned lifecycle boundary | Released minimal form in 0.1 | `FNetwork` only schedules a policy-free hook; it is not `FNetManager` or a transport |
| Managed `UObject` identity | `UObject` plus `FObjectHandle` | Candidate contract at `e1e7b75` | Fixed caller-owned store, explicit descriptors, no UE reflection/code generation |
| Dynamic managed World | `UWorld` | Approved direction | Bounded mutation queues and explicit barriers; no seamless travel or level streaming |
| Managed Actor | `AActor` | Approved direction | Exists only after object identity, tracing, lifecycle, and destruction gates pass |
| Managed Actor Component | `UActorComponent` | Approved direction | World/Actor own children strongly; reverse owner links are weak |
| `NewObject` | Object-store construction | Candidate contract at `e1e7b75` | Capacity/OOM is explicit through the fixed object store |
| `SpawnActor` | Deferred World attachment | Approved direction | Bounded mutation queues prevent active iteration from changing |
| Garbage collection | `FGarbageCollector` | Candidate contract at `e1e7b75` | Optional per build, fixed-capacity, non-moving, explicit-root, iterative, and operation-budgeted |
| `TObjectPtr` | `TObjectPtr` | Candidate contract at `e1e7b75` | Generation-checked local identity; a local pointer variable is not an implicit root |
| `TWeakObjectPtr` | `TWeakObjectPtr` | Candidate contract at `e1e7b75` | Resolves through store index and generation; stale reuse cannot alias a new object |
| `TStrongObjectPtr` | `TStrongObjectPtr` | Candidate contract at `e1e7b75` | Explicit bounded root registration with observable capacity failure |
| `FTimerManager` | Bounded timer manager | Approved direction | At most budgeted callbacks; late looping timers do not catch up in bursts |
| Delegates | Fixed-inline single/multicast delegates | Candidate contract at `e1e7b75` | Callable and binding capacities are explicit; no hidden heap spill |

## Ownership concepts

| UE5/C++ concept | MicroWorld rule | Maturity |
| --- | --- | --- |
| Stack/value ownership | Preferred for deterministic services and small state | Released Core practice |
| `TUniquePtr` | Exclusive non-`UObject` ownership backed by an injected memory resource | Candidate contract at `e1e7b75` |
| `TSharedPtr` / `TWeakPtr` | Reference-counted non-`UObject` ownership; single-threaded portable baseline | Candidate contract at `e1e7b75` |
| `TObjectPtr` | Managed reference traced only through an object descriptor/reference visitor | Candidate contract at `e1e7b75` |
| `TWeakObjectPtr` | Non-owning managed observation | Candidate contract at `e1e7b75` |
| `TStrongObjectPtr` | Explicit external GC root | Candidate contract at `e1e7b75` |
| Raw pointer/reference | Short documented scope where the owner is known and pointer stability is guaranteed | Released Core practice |
| Hardware/ISR/watchdog/safety state | Never GC-owned; use values, fixed storage, or deterministic ownership | Approved invariant |

The pointer-foundation decision is recorded in
[0002a-smart-pointer-foundation.md](decisions/0002a-smart-pointer-foundation.md).
Managed ownership is recorded in
[0002-managed-memory.md](decisions/0002-managed-memory.md).

## Networking concepts

| UE5 concept | Planned MicroWorld equivalent | Difference |
| --- | --- | --- |
| Net driver | `INetDriver` | One non-blocking bounded byte transport operation per call; no session or gameplay policy |
| Network manager | `FNetManager` | Explicit packet/byte budgets, validation, sessions, queues, handlers, and counters |
| Engine network scheduling | Optional Engine-Net adapter | Net does not depend on Engine; Core applications can schedule it directly |
| Net object identity | `FNetObjectId` | Session-qualified wire identity; never a pointer or `FObjectHandle` |
| Replication | Explicit descriptor-driven snapshot preview | No automatic property scan, RPC, prediction, rollback, or relevancy graph |
| Packet integrity/authentication | Injected packet policy | CRC and authentication remain distinct; no primitive or security claim without a threat model |

The module split is recorded in
[0001-modular-runtime.md](decisions/0001-modular-runtime.md).

## Naming rules

- `F`, `T`, `E`, `I`, and `b` follow an embedded adaptation of UE naming.
- `A` is reserved for a real managed Actor with World lifecycle.
- `U` is reserved for a real managed object traced by the Object module.
- MicroWorld does not define `UCLASS`, `UPROPERTY`, `GENERATED_BODY`, or other
  UE reflection macros.
- Diagnostic class names are not stable wire identifiers.

## Deferred or excluded systems

| System | Status | Reason |
| --- | --- | --- |
| Generated reflection and property editor metadata | Deferred | Explicit descriptors are simpler until repetition proves tooling value |
| Thread-safe shared pointers and background tasks | Deferred | Need a measured concurrent consumer and toolchain/atomic evidence |
| GPIO/UART/SPI/I2C universal HAL | Deferred | Shared semantics must be proven by at least two ports and one consumer |
| Input/display/simple 2D helpers | Deferred | Enter only through a concrete application concept |
| Full replication, RPC, prediction, rollback | Deferred | Authority and bandwidth policy require a real networked application |
| Blueprint/editor, rendering, physics, audio, navigation, GAS, asset cooking | Excluded | They conflict with the lightweight microcontroller scope |

## Released truth versus roadmap

The public headers and tests remain authoritative for released behavior.
Approved-direction rows describe the reviewed roadmap and must not be reported
as implemented, measured, or supported until their release gates pass.
