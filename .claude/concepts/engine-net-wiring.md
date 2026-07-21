# TEngineHost ↔ TNetHost Wiring (Phase 4.4)

## Problem
Phase 4.3 delivered `TNetHost` (session layer, roles, peers). The engine's
canonical frame order (roadmap section 4) already reserves **step 1**
(receive/dispatch) and **step 7** (flush outbound) as inert comments in
`TEngineHost::Tick`. Phase 4.4 makes those two slots live and proves that net +
engine compose: two `TEngineHost` instances over loopback exchange a message that
spawns an actor on the server world.

The hard constraint: **`microworld-engine` must not depend on `microworld-net`.**
`CheckDependencyBoundaries.py` allows `Engine → {Core, Memory, Object}` only (Net
is a *sibling*, not an inward dependency). So `TEngineHost` cannot `#include` any
`MicroWorld/Net/…` header — the spec's literal "constructor accepts a `TNetHost&`"
is impossible to write as-is. The wiring must be done through an abstraction the
engine owns, with the concrete `TNetHost` bound by the caller (who *can* see both).

## Proposed Approach
Introduce a tiny **engine-owned** frame-facing network seam and inject it by
reference — mirroring how the codebase already injects `INetDriver`, storage, and
(in 4.3) the driver into `TNetHost`. Naming mirrors UE5's `UNetDriver`
(`TickDispatch` = process inbound, `TickFlush` = send outbound); no "pump".

New engine header `Engine/NetworkFrame.h`:
- `class INetworkFrame` — two pure virtuals `TickDispatch(TimePointMilliseconds)`
  (drain driver, dispatch messages, update peers) and
  `TickFlush(TimePointMilliseconds)` (flush outbound FIFO, heartbeats), both
  `noexcept` returning `void`, plus a virtual defaulted dtor. This is the *only*
  thing `TEngineHost` knows about networking.
- `template<typename TNet> class TNetHostFrame final : public INetworkFrame` — a
  caller-side adapter holding `TNet&` whose overrides call
  `(void)Net.PumpReceive(now)` / `(void)Net.PumpSend(now)`, discarding the
  `ENetResult` exactly as `Tick` already discards `Timers.Advance` /
  `Collector.Advance`. `TNet` is deduced at the *call site* (test/app), so the
  engine never names `TNetHost`. (The adapter bridges to `TNetHost`'s existing
  4.3 `PumpReceive`/`PumpSend` methods — those shipped in commit 8a3d769 and are
  not renamed by this task.)

`TEngineHost` changes:
- New member `INetworkFrame* Network{nullptr}` (default: no networking — the
  "optional slot" stays optional; every existing standalone `TEngineHost` is
  unaffected).
- New **non-template** constructor overload
  `TEngineHost(FGarbageCollectionBudget, INetworkFrame&, std::uint32_t Reclamation = MaxObjects)`
  that delegates to the existing constructor and binds `Network`.
- `Tick`: replace the two comment slots with
  `if (Network != nullptr) Network->TickDispatch(now);` as step 1 and
  `… TickFlush(now);` as step 7. Update the `Tick` doc comment: the net slots are
  now live; document the full 7-step order in `EngineHost.h` (Done-when).

Caller (the new engine test) owns the whole graph and wires it:
```cpp
FHostLoopback<2, MailboxCap, PacketBytes> Network;
TNetHost<MaxPeers, PacketBytes> ServerNet{Network.Port(0)};
TNetHost<MaxPeers, PacketBytes> ClientNet{Network.Port(1)};
TNetHostFrame<decltype(ServerNet)> ServerFrame{ServerNet};  // caller-owned adapter
TEngineHost<…> ServerHost{Budget, ServerFrame};             // binds INetworkFrame&
```

### Concept-proof test (`EngineNetHostTests.cpp`, engine test target)
Two `TEngineHost` + two `TNetHost` over one `FHostLoopback`. Server is
`ListenServer` (or `DedicatedServer`); client is `Client`. Server installs a
message handler that, on an app-channel message, spawns a user actor in **its**
world via `CreateObject<T>` + world spawn.
1. Configure/Start both; drive `Tick(now)` on both across a few frames until the
   client reaches `Connected` (handshake runs through the live frame slots — proof
   the seam is wired, not called directly).
2. Client `SendTo(server, appChannel, payload)`.
3. Drive `Tick` on both: client step 7 flushes; server step 1 dispatches → handler
   requests spawn; server step 4 `ApplyPending` begins-plays the actor.
4. Assert the server world's live-actor count increased by one and the client
   never spawned (message was one-directional). This is the "net + engine compose"
   acceptance.

CMake: the engine test target gains `MicroWorld::Net` (guarded `add_subdirectory`
of `../microworld-net` if the target is absent; Net re-uses the already-present
Memory target) and links it `PRIVATE` to `microworld_engine_tests` only —
production `microworld_engine` stays net-free, so the dependency checker keeps
passing.

## Open Questions
- (resolved) Storage mechanism — see Decisions Log (A′, interface-based).
- (resolved) Naming — see Decisions Log (`INetworkFrame`/`TickDispatch`/`TickFlush`).

## Decisions Log
- 2026-07-21: **Engine must not include Net** — verified against
  `CheckDependencyBoundaries.py` (`Engine → {Core, Memory, Object}`; self-test
  forbids a `Net/` path in another package). The spec's literal `TNetHost&`
  constructor parameter is therefore replaced by an engine-owned abstraction with
  the concrete host bound by the caller. Faithful to intent (ctor takes the net
  host by reference; `Tick` drives its two frame steps), compatible with the
  boundary.
- 2026-07-21: **Storage = A′ (engine-owned `INetworkFrame` interface +
  caller-owned `TNetHostFrame<TNet>` adapter).** Interface-based, matching the
  `INetDriver` idiom; no `void*` type-erasure; `TNetHost` untouched; the engine
  constructor is a plain `INetworkFrame&` overload. Rejected: (A) by-value
  type-erased `void*`+function-pointer adapter (the "cleverness" Kernighan warns
  against); (B) interface in Core (`ENetResult` is a Net type Core cannot name).
- 2026-07-21: **Naming — no "pump".** Engine seam is `INetworkFrame` with
  `TickDispatch`/`TickFlush`, mirroring UE5 `UNetDriver`; adapter `TNetHostFrame`;
  member `Network`. User directive: avoid weak/vague identifiers. `TNetHost`'s own
  `PumpReceive`/`PumpSend` (shipped 4.3) are left as-is unless separately requested.
- 2026-07-21: Concept-proof test lives in the **engine test target** (tests are not
  dependency-scanned), linking `MicroWorld::Net` PRIVATE; production engine stays
  net-free.
