# microworld-platform-host: time source + UDP driver (Phase 5.1)

## Problem
Phase 4 delivered the whole portable networking stack — addressing, wire framing,
roles/sessions (`TNetHost`), and `TEngineHost` composition — but every test proves
it only over the in-process `FHostLoopback`. Nothing has yet crossed a real socket.
Phase 5.1 delivers the first **platform adapter**: a `microworld-platform-host`
package with a real-clock time source and a UDP `INetDriver`, so `TNetHost` traffic
travels over UDP on localhost. It is also the *template* every later adapter (ESP32
UDP in 5.2, E32 LoRa in 5.3) copies: time source + net driver = one platform.

## Proposed Approach
A new **non-portable** package `lib/microworld-platform-host/` (CMake + `include/`
+ `src/` + `tests/`). It depends outward on Net (`INetDriver`, `FNetAddress`,
`ENetResult`) and Core (`Time.h`), and — unlike the five portable packages — it may
include OS socket headers. It is therefore **not** passed to
`CheckDependencyBoundaries.py` (that checker governs only Core/Memory/Object/Engine/
Net); adding it there would wrongly reject `<winsock2.h>`/`<sys/socket.h>`.

- **`FHostTimeSource`** — wraps `std::chrono::steady_clock`, captures a baseline at
  construction, and returns `TimePointMilliseconds Now()` as milliseconds since that
  baseline. Concrete, allocation-free; the app calls `Now()` and passes it to
  `TEngineHost::Tick`, so the engine keeps its no-hidden-clock contract.

- **`FHostUdpDriver final : INetDriver`** — owns one non-blocking UDP socket bound to
  a local IPv4:port. Implements the v2 contract:
  - `TrySend(To, Packet)` → `sendto` to the IPv4+port decoded from `To`; maps
    would-block to `Full`, oversize/null-span/unroutable to `Invalid`, else `Success`.
  - `TryReceive(OutFrom, Dest, Out)` → non-blocking `recvfrom`; no datagram →
    `Unavailable`; delivered → `Success` (writes bytes, count, and the sender encoded
    into `OutFrom`); datagram larger than `Dest` → `Full` **without consuming it**
    (see decision below).
  - `MaxPacketBytes()` → a fixed safe UDP payload bound (e.g. 1200 bytes).
  - `FNetAddress` UDP encoding = 6 bytes: `Bytes[0..3]` = IPv4 octets `a.b.c.d`,
    `Bytes[4..5]` = port big-endian (network order). A `MakeUdpAddress(a,b,c,d,port)`
    helper (and inverse accessors) lives in this package, since the encoding is this
    driver's, not Net's (`FNetAddress` is deliberately opaque).

- **Cross-platform socket glue** centralized in one internal header/TU behind
  `#ifdef _WIN32`: a socket-handle alias, close, set-non-blocking (`ioctlsocket`
  FIONBIO vs `fcntl` O_NONBLOCK), and last-error → `ENetResult` mapping
  (`WSAGetLastError`/`WSAEWOULDBLOCK` vs `errno`/`EWOULDBLOCK`).

- **WinSock lifecycle** — a reference-counted RAII `FWinSockScope` so N drivers share
  one `WSAStartup`/`WSACleanup`; a no-op on POSIX. Held as a driver member.

- **Tests** (`tests/`, host-only, real UDP on `127.0.0.1`): two drivers on
  OS-assigned ephemeral ports exchange a packet (A→B, wait for readability, `Success`
  with matching bytes and `OutFrom` == A); `Unavailable` on an empty receive;
  `Invalid` on null-span-nonzero and oversize send; `Full` receive mapping where the
  OS lets us reproduce it; `MaxPacketBytes` reported. A small end-to-end check drives
  `TNetHost` Hello→Welcome over two UDP drivers to satisfy the Done-when.

**Done when:** a host test/demo sends `TNetHost` traffic over real UDP localhost;
the package builds and its tests pass under the strict host toolchain.

## Open Questions
- **Cross-platform scope now, or Windows-first? (NEEDS YOUR CALL.)** The roadmap says
  "BSD/WinSock UDP". I can only *build and run* on this Windows host, so POSIX code
  would ship compile-guarded but unverified this phase. Options: (a) write both
  Windows + POSIX now (Windows verified, POSIX behind `#ifdef`, unverified until a
  Linux/ESP32 build); (b) Windows-first, add + verify POSIX when 5.2/ESP32 lands on a
  POSIX-like build. Recommend (a) — the `#ifdef` seams are cheap now and 5.2 reuses
  the same encoding, but honestly note the POSIX path is unverified until then.
- (resolved-by-recommendation) Oversized datagram → `MSG_PEEK` to size first and
  leave the datagram queued, so `Full` stays transactional per the `INetDriver`
  contract (rather than consuming into an internal buffer).
- (resolved-by-recommendation) Test readiness wait → `select()` with a bounded
  timeout (sleep-free, deterministic), not a sleep-poll loop.
- (resolved-by-recommendation) Test ports → bind port 0 (ephemeral) and read the
  actual port via `getsockname`, avoiding fixed-port collisions on CI.

## Decisions Log
- 2026-07-21: `microworld-platform-host` is a **non-portable platform package**;
  excluded from the portable dependency-boundary checker; may use OS socket headers.
- 2026-07-21: UDP `FNetAddress` = 6 bytes (4 IPv4 octets + 2 port bytes big-endian),
  encoded/decoded by helpers owned by this package.
- 2026-07-21: `Full` receive stays transactional via `MSG_PEEK`; tests wait with
  `select()` timeout and bind ephemeral ports — all to keep host socket tests
  deterministic and CI-safe.
