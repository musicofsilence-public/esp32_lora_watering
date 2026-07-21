# TNetHost — Networking with Roles (Phase 4.3)

## Problem
Phase 4.1 gave us addressed transport (`INetDriver` v2, multi-endpoint loopback);
4.2 gave us the wire framing + session-control messages (`NetProtocol.h`). What's
missing is the *session layer*: the UE5 concept of dedicated server / listen
server / client, delivered as a bounded peer table with admission, heartbeats,
and timeout eviction. Without it there is no notion of "who is connected" — only
raw packets. 4.3 delivers `TNetHost`, the piece 4.4 wires into `TEngineHost`.

## Proposed Approach
A header-only `Net/NetHost.h` with `TNetHost<MaxPeers, MaxPacketBytes>` that owns a
bounded peer table and drives the protocol through explicit tick calls
(`PumpReceive(Now)` / `PumpSend(Now)`) — no hidden clock, no hidden allocation. It
sits on 4.2's framing and 4.1's driver/manager.

Four roles (`ENetMode`): `DedicatedServer`/`ListenServer` admit up to `MaxPeers`
clients via Hello→Welcome; `Client` holds one server peer and (re)sends Hello until
Welcome; `Standalone` runs no traffic and returns `Unavailable` on send.
`ListenServer` additionally owns a *local peer* whose messages dispatch straight to
the handler without the driver. Application messages (channel ≥1) fan out to one
bounded `TMulticastDelegate` handler; channel 0 is handled internally. Peer
identity is generation-checked (`FPeerId{Index, Generation}`); eviction bumps the
slot generation so stale ids fail safely — the same pattern as `FObjectHandle`.

### Design decisions (made; listed for the record)
- **Reuse `FNetManager` for the outbound FIFO** rather than re-implementing queue
  mechanics (DRY). `FNetManager` binds its driver by reference at construction.
- **Driver injection point (NEEDS YOUR CALL):** the spec writes
  `Configure(ENetMode, INetDriver&, FNetHostConfig)`. To reuse `FNetManager` as a
  plain member — the clearest option, no deferred-construction machinery (which is
  exactly the "cleverness" Kernighan warns against, and the codebase has no
  `std::optional` precedent) — I recommend **constructor injection**:
  `explicit TNetHost(INetDriver&)` + `Configure(ENetMode, const FNetHostConfig&)`.
  This matches how every other component here binds collaborators (driver/storage
  bound once at construction). Alternative: keep the spec literal and hand-roll
  deferred construction (`aligned_storage` + placement-new like `TDelegate`) — more
  code, less clarity, for no functional gain.
- **Internal capacities are documented constants, not new template params** (honors
  the spec's two-arg `TNetHost<MaxPeers, MaxPacketBytes>`): send FIFO depth =
  `2*MaxPeers + 4` (one full broadcast + pending heartbeats + slack); handler
  delegate `MaxHandlers = 4`, `InlineBytes = 32`.
- **Local peer** = sentinel `FPeerId{Index=0xFE}`, dispatched synchronously to the
  handler on `SendTo`/`Broadcast` in `ListenServer` mode; kept *outside* the
  `MaxPeers` remote table so all `MaxPeers` slots stay available to clients.
- **Client** = single server peer in `Peers[0]`; `ENetHostState`
  (`Idle`/`Connecting`/`Connected`/`Listening`) + a `LastHelloSend` timestamp drive
  the Connecting→Connected→(timeout)Connecting machine.
- **`Stop()` sends `Bye`** to active peers (best-effort) then evicts all — completes
  the protocol and makes the Bye send-path testable.
- **`Broadcast` is best-effort per peer** (not atomic): returns `Success` if every
  active peer queued, else the first failure code.
- **Logging:** version-mismatch `Hello` and unknown/malformed control →
  `MW_LOG(Warning/Log, "NetHost", ...)`, no reply, no state change (per spec).

## Open Questions
- (resolved) Driver injection point — see Decisions Log.

## Decisions Log
- 2026-07-21: **Driver injected at construction** (`explicit TNetHost(INetDriver&)`); `Configure(ENetMode, const FNetHostConfig&)` sets mode+config. User-approved deviation from the spec's `Configure(driver)` — lets `TNetHost` own `FNetManager` as a plain member (DRY + Kernighan-clean, no deferred-construction machinery).
- 2026-07-21: Reuse `FNetManager` (DRY) → drove the constructor-injection choice.
- 2026-07-21: Local peer is a sentinel id outside the remote table — keeps `MaxPeers` fully available to clients and satisfies the spec's "not a naming gimmick".
- 2026-07-21: Internal capacities are documented constants, not template params — honors the spec's two-arg `TNetHost<MaxPeers, MaxPacketBytes>`.
