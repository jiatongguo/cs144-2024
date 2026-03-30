# Minimal TCP/IP Stack in C++

This repository contains a user-space implementation of a minimal TCP/IP stack built on top of the Stanford CS144 `minnow` framework. The code focuses on protocol mechanics rather than kernel integration: bounded byte-stream transport, out-of-order TCP reassembly, sender/receiver control logic, ARP-based next-hop resolution, and IPv4 forwarding.

The main implementation work lives in `src/`. The surrounding course harness, utilities, and test infrastructure are retained to validate behavior, but the protocol logic below is the core of the project.

## Implemented Scope

| Module | What it does | Implementation notes |
| --- | --- | --- |
| `ByteStream` | Bounded in-memory stream between a writer and a reader | Capacity-limited writes, `std::string_view`-based peek path, byte counters, close/error state tracking |
| `Reassembler` | Reconstructs ordered stream data from indexed TCP payload fragments | Stores out-of-order ranges, merges overlaps, trims bytes outside the current acceptance window, flushes contiguous data into the output stream |
| `TCPReceiver` | Converts inbound TCP segments into ordered stream data and receiver feedback | Initializes receiver state on `SYN`, unwraps 32-bit sequence numbers, maps sequence space to stream indices, generates `ACK` and advertised window |
| `TCPSender` | Drives outbound transmission and retransmission logic | Segments outbound data under the receiver window, tracks outstanding segments, performs timeout-driven retransmission with RTO backoff, and handles `SYN` / `FIN` |
| `NetworkInterface` | Connects IPv4 datagrams to Ethernet frames | Resolves next-hop MAC addresses via ARP, maintains a bounded-lifetime ARP cache, suppresses duplicate ARP requests, queues datagrams until resolution completes |
| `Router` | Forwards IPv4 datagrams across interfaces | Maintains static routes, performs longest-prefix match, supports direct and next-hop forwarding, decrements TTL and refreshes the IPv4 checksum |

Primary source files:

- `src/byte_stream.*`
- `src/reassembler.*`
- `src/wrapping_integers.*`
- `src/tcp_receiver.*`
- `src/tcp_sender.*`
- `src/network_interface.*`
- `src/router.*`

## Engineering Characteristics

- Built with C++20 and CMake
- Uses modern C++ interfaces such as `std::string_view` and `std::optional` where they simplify ownership and state representation
- Compiles under strict warning settings
- Supports AddressSanitizer and UndefinedBehaviorSanitizer builds through the provided CMake configuration
- Structured as a compact, verification-oriented protocol stack rather than a production networking system

## Build and Verify

Requirements:

- CMake `>= 3.24.2`
- C++20 toolchain
- Linux or another Unix-like environment

Build:

```bash
cmake -S . -B build
cmake --build build
```

Run the main test suites:

```bash
cmake --build build --target test
cmake --build build --target speed
```

Checkpoint-oriented targets:

```bash
cmake --build build --target check0
cmake --build build --target check1
cmake --build build --target check2
cmake --build build --target check3
cmake --build build --target check5
cmake --build build --target check6
```

Run a single test directly:

```bash
cmake --build build --target recv_connect
./build/tests/recv_connect
```

Representative subsystem coverage includes:

- `byte_stream_*`
- `reassembler_*`
- `wrapping_integers_*`
- `recv_*`
- `send_*`
- `net_interface`
- `router`

## Repository Layout

```text
.
├── src/        Core protocol-stack implementation
├── util/       Packet/header abstractions and networking utilities
├── tests/      Functionality tests and speed benchmarks
├── apps/       Small demo applications built on the stack
├── scripts/    Build and experiment helper scripts
└── etc/        CMake configuration fragments
```

## Scope Boundary

This is a user-space protocol implementation intended for correctness and systems practice. It does not aim to be a production-ready TCP/IP stack, and it intentionally relies on the existing CS144 harness instead of replacing the surrounding framework.

## Acknowledgement

This project is based on the Stanford CS144 networking labs. The course framework, tests, and utilities are retained; the protocol implementations under `src/` are the primary implementation work presented here.
