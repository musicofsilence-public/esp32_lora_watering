# MicroWorld Net

MicroWorld Net is the bounded non-blocking byte-I/O package above Memory. It
provides a byte reader/writer, one non-blocking `INetDriver`, one fixed-
capacity `FNetManager`, explicit `ENetResult` outcomes, and a deterministic
host loopback driver for embedded applications.

Current status and recorded evidence live in
[PROGRESS.md](../microworld/PROGRESS.md).

## What Net provides

- `ENetResult` reports `Success`, `Full`, `Invalid`, and `Unavailable` with one
  normalized meaning per value: `Success` (complete operation), `Full` (valid
  operation lacks destination/queue/transport capacity), `Invalid` (invalid
  span/configuration, oversized packet, or truncated byte-reader request), and
  `Unavailable` (a valid non-blocking driver/manager operation has no work or
  cannot progress now).
- `FByteWriter` appends single bytes and byte spans into a caller-owned
  `TSpan<std::uint8_t>` and reports position, remaining, and capacity. A
  backing span bound to `{nullptr, nonzero}` is an invalid configuration that
  every mutating operation rejects as `Invalid` without dereferencing null. A
  failed write does not advance the cursor or alter accepted bytes.
- `FByteReader` reads single bytes and byte spans from a caller-owned
  `TSpan<const std::uint8_t>` and reports position and remaining. An invalid
  backing span is rejected as `Invalid` without dereferencing null. A failed
  read does not advance the cursor or modify output parameters.
- `INetDriver` exposes one bounded non-blocking `TrySend` and one bounded
  non-blocking `TryReceive` over caller-owned byte spans. Every receive is
  transactional: on `Full`, `Invalid`, or `Unavailable` the destination and
  `FNetReceiveResult::BytesReceived` are unchanged.
- `FNetPacketStorage<MaxPackets, MaxPacketBytes>` is the smallest fixed
  storage type the manager needs; both capacities must be nonzero. The caller
  constructs one instance and lends it to the manager by reference.
- `FNetManager<MaxPackets, MaxPacketBytes>` holds one externally referenced
  `INetDriver` and one externally referenced `FNetPacketStorage`, queues
  complete packets into that storage, attempts at most the FIFO head per send
  advance (retaining the head on any driver failure), and performs at most one
  direct driver receive.
- `FHostLoopback<CapacityPackets, PacketBytes>` is a deterministic fixed-
  capacity loopback driver for host tests.

The application owns the driver, the packet storage, the manager value, and
all fixed buffers.

## Build

```sh
cmake -S lib/microworld-net -B <build-directory>
cmake --build <build-directory>
ctest --test-dir <build-directory> --output-on-failure
```

CMake consumers link `MicroWorld::Net`. A successful compile or host test
does not establish target runtime margins or hardware behavior.

Net does not provide wire framing, sessions, retries, reliability,
authentication, real transports, replication, platform abstraction, or
hardware APIs.
