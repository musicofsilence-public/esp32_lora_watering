# MicroWorld Resource Budgets

MicroWorld accepts a target/profile only when its resource limits and evidence
are explicit. Phase 0 defines invariants and measurement ownership; it does not
invent absolute limits for hardware that has not been selected or run.

## Evidence states

| State | Meaning |
| --- | --- |
| Unresolved | Target, value, or measurement method still needs owner approval |
| Provisional | Design rule accepted; implementation or target evidence pending |
| Compile-measured | Linker/build output recorded; no runtime claim |
| Runtime-measured | Observed on named hardware with explicit authorization |
| Accepted | Owner accepted the measured value and remaining margin |

Compile success never establishes cycle cost, heap behavior, stack margin,
radio behavior, or physical hardware operation.

## Cross-profile invariants

| Metric | Owner | Status | Required budget | Evidence |
| --- | --- | --- | --- | --- |
| Core steady-state allocation | MicroWorld Core | Accepted invariant; host measured | Zero framework allocations in lifecycle/tick paths | [Host baseline](../benchmarks/Results/Host.md) |
| Hidden allocator fallback | Every portable module | Accepted invariant; Memory host-measured | Zero; every allocation is attributed to an injected resource | [Memory host benchmark](../../microworld-memory/benchmarks/Results/Host.md) |
| Per-object tick work | MicroWorld Core | Accepted invariant; host measured | At most once per caller update; no catch-up burst | Existing 31 behavior tests |
| Registration/container growth | MicroWorld Core | Accepted invariant | Fixed declared capacity; overflow returns typed failure | Existing capacity tests |
| Failure-path work | Owning module | Provisional | Bounded by declared capacities and per-update budgets | Required before each module gate |
| Stack recursion in runtime paths | Owning module | Accepted invariant | None in dispatch, parsing, or GC | Static review and maximum-graph tests |
| Same-target/profile regression | Release owner | Accepted gate | No unexplained regression over 10% outside recorded noise | Fixed benchmark before/after record |

## Core profile

| Metric | Owner | Status | Required or provisional value | Evidence / next action |
| --- | --- | --- | --- | --- |
| Flash delta | Project owner | Unresolved per target | Select after clean profile map | Gate B linker-map comparison |
| Static RAM delta | Project owner | Unresolved per target | Select after clean profile map | Gate B linker-map comparison |
| Task/thread stack margin | Project owner | Unresolved per target | Must leave accepted application margin | Authorized target high-water mark |
| Dispatch complexity | MicroWorld Core | Accepted invariant | `O(Actors + Components)` single pass | Source review and workload scaling |
| Dispatch allocation delta | MicroWorld Core | Runtime-measured on host only | Zero | [Host baseline](../benchmarks/Results/Host.md) |
| ESP32 cycles/update | Project owner | Unresolved | No claim until authorized execution | Existing benchmark firmware is compile-ready |

## Memory profile

Memory is an experimental Core overlay. The owner accepted Gate C for roadmap
progression on 2026-07-19, but absolute target budgets and remaining target
margins remain unresolved; measured layout and allocation facts must not be
converted into invented limits.

| Metric | Owner | Status | Required or measured value | Evidence / next action |
| --- | --- | --- | --- | --- |
| Fixed-resource hidden allocation | Memory module | Runtime-measured on host | Zero for unique/shared/container/delegate workloads | [Host Memory benchmark](../../microworld-memory/benchmarks/Results/Host.md) |
| Thin unique owner size | Memory module | Runtime-measured on MSVC x64 | 32 bytes; equal to direct standard resource-deleter owner | Owner review at Gate C |
| Custom shared/weak handle size | Memory module | Runtime-measured on MSVC x64 | 8 bytes each | Re-measure on authorized targets |
| Custom combined allocation | Application-selected resource | Runtime-measured on MSVC x64 | 56 bytes for the 16-byte benchmark value | Value/control-block layout benchmark |
| Fixed-arena object overhead | Memory module | Runtime-measured on MSVC x64 | 88 bytes for 256-byte payload/alignment-8 instance | Re-measure representative capacities |
| Container/delegate work | Memory module | Runtime-measured on host | Fixed 100,000-operation workloads; semantic counters exact; zero global allocations | Same-target comparisons only |
| ESP32 whole-image RAM | Project owner | Compile-measured | 20,156 / 327,680 bytes | [ESP compile evidence](../../microworld-memory/benchmarks/Results/Esp32S3N16R8.md) |
| ESP32 whole-image flash | Project owner | Compile-measured | 194,457 / 4,194,304 bytes | Complete image, not isolated Memory delta |
| Target heap/stack/timing | Project owner | Unresolved | No claim without authorized execution | Hardware run remains blocked |

## Managed ownership tier

Managed is optional per build. These budgets must pass before managed types are
released.

| Metric | Owner | Status | Required or provisional value | Evidence / next action |
| --- | --- | --- | --- | --- |
| Object arena bytes | Application | Provisional | Exact caller-provided capacity; no fallback | Object-store tests and profile configuration |
| Maximum object count | Application | Provisional | Exact slot count; capacity failure observable | Object-store boundary tests |
| Maximum object size/alignment | Application | Provisional | Exact arena layout; unsupported layout rejected | Layout boundary tests |
| Metadata bytes/object | Object module | Unresolved | Measure index, generation, flags, descriptor, and GC state | Host and target `sizeof`/map report |
| Fixed-slot internal fragmentation | Object module | Unresolved | Record workload distribution before adding size classes | Object benchmark |
| Root capacity | Application | Provisional | Fixed and explicit; root creation may fail | Root-capacity tests |
| GC work/update | Application | Provisional | No more than caller-provided root/mark/sweep operations | Incremental budget tests |
| GC stack growth | Object module | Accepted invariant | Fixed iterative worklist; no graph recursion | Maximum-depth/cycle tests |
| Hidden full collection | Object module | Accepted invariant | Forbidden on allocation failure | OOM behavior tests |
| Managed steady-state heap delta | Managed profile | Provisional | Zero when fixed resources are selected | Authorized target heap measurement |

## Networking overlay

Net may be linked with Core or Managed.

| Metric | Owner | Status | Required or provisional value | Evidence / next action |
| --- | --- | --- | --- | --- |
| Maximum packet bytes | Application/protocol | Unresolved | Explicit compile/runtime configuration | First accepted protocol |
| Receive packets/update | Application | Provisional | Caller-provided bound | Fake-driver budget tests |
| Receive bytes/update | Application | Provisional | Caller-provided bound | Fake-driver budget tests |
| Transmit packets/update | Application | Provisional | Caller-provided bound | Backpressure tests |
| Transmit bytes/update | Application | Provisional | Caller-provided bound | Backpressure tests |
| Peer/session/handler capacity | Application | Provisional | Fixed declared capacities | Capacity plus one tests |
| Queue allocation | Net module | Accepted invariant | Fixed storage or injected resource only | Allocation probes |
| Parser work on invalid input | Net module | Provisional | Bounded by packet length and manager budget | Fuzz and maximum-invalid workloads |
| Bandwidth | Application/transport | Unresolved | Measured for named message mix and link | First real driver/application |
| Authentication cost | Security owner | Unresolved | No primitive selected without threat model | Separate security decision |

## Target evidence matrix

| Target | Core compile | Core runtime | Managed | Net | Current status |
| --- | --- | --- | --- | --- | --- |
| Windows x64 / MSVC | Measured | Measured host-relative | Memory measured; Managed not implemented | Not implemented | Development host only |
| ESP32-S3-WROOM-1-N16R8 | Compile-measured | Unresolved; run not authorized | Memory compile-measured; Managed not implemented | Not implemented | Core+Memory compile probe |
| STM32 reference target | Unresolved | Unresolved | Unresolved | Unresolved | Exact board/SDK/toolchain not selected |
| RP2040/RP2350 reference target | Unresolved | Unresolved | Unresolved | Unresolved | Exact board/SDK/toolchain not selected |

Current host and ESP32 measurements are recorded in
[benchmarks/Results](../benchmarks/Results). Target selection and measurement
obligations are defined in [Porting.md](Porting.md).

## Promotion rules

- A profile is **experimental** while absolute target budgets are unresolved.
- Compile evidence changes only the compile-support status.
- Runtime support requires explicit hardware execution and recorded stack,
  allocation, timing, and semantic evidence.
- A measured value becomes **accepted** only after the application owner
  approves its remaining margin.
- A later feature may spend resources only through an updated budget record and
  explicit trade-off.
- Failure to meet a budget triggers simplification, a narrower profile, or a
  revised decision; it does not justify hidden allocation or unbounded work.
