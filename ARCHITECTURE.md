# Architecture Overview

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Application Layer                        │
├─────────────────────────────────────────────────────────────┤
│  Publisher API          │          Subscriber API            │
│  - publish()            │          - receive()               │
│  - backpressure()       │          - poll()                  │
└──────────┬──────────────┴──────────────┬────────────────────┘
           │                              │
           ▼                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Serializer/Deserializer                   │
│  - Type-safe serialization                                   │
│  - Zero-copy byte operations                                 │
└──────────┬──────────────────────────────┬───────────────────┘
           │                              │
           ▼                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Lock-Free Ring Buffer                     │
│  ┌─────────────────────────────────────────────────────┐    │
│  │ Metadata (16 bytes)                                 │    │
│  │  - atomic<uint64_t> write_pos                       │    │
│  │  - atomic<uint64_t> read_pos                        │    │
│  │  - uint32_t num_publishers                          │    │
│  │  - uint32_t num_subscribers                         │    │
│  └─────────────────────────────────────────────────────┘    │
│  ┌─────────────────────────────────────────────────────┐    │
│  │ Message Buffer (1024 × 264 bytes = 270,336 bytes)  │    │
│  │  [Msg 0][Msg 1][Msg 2]...[Msg 1023]                │    │
│  └─────────────────────────────────────────────────────┘    │
└──────────┬──────────────────────────────┬───────────────────┘
           │                              │
           ▼                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Shared Memory Layer                       │
│  Windows: CreateFileMapping + MapViewOfFile                  │
│  Linux/macOS: shm_open + mmap                                │
└─────────────────────────────────────────────────────────────┘
```

## Message Flow

### Write Path (Publisher)

```
1. Application Data
   ↓
2. Serializer.write<T>()
   ↓
3. Publisher.publish(data, size)
   ↓
4. Timestamp capture (now_ns)
   ↓
5. Check buffer space (write_pos - read_pos < CAPACITY)
   ↓
6. Copy to ring buffer slot (write_pos % CAPACITY)
   ↓
7. Atomic increment write_pos (memory_order_release)
   ↓
8. Return success
```

**Latency**: ~100-200 ns (no contention)

### Read Path (Subscriber)

```
1. Subscriber.receive(buffer, size, timestamp)
   ↓
2. Check data available (read_pos < write_pos)
   ↓
3. Copy from ring buffer slot (read_pos % CAPACITY)
   ↓
4. Atomic increment read_pos (memory_order_release)
   ↓
5. Deserializer.read<T>()
   ↓
6. Application processes data
```

**Latency**: ~100-200 ns (no contention)

## Memory Layout

```
Shared Memory Region (270,352 bytes total)
┌────────────────────────────────────────────────────────┐
│ Offset 0: Metadata (16 bytes)                          │
│ ┌────────────────────────────────────────────────────┐ │
│ │ +0:  write_pos (8 bytes, atomic)                   │ │
│ │ +8:  read_pos (8 bytes, atomic)                    │ │
│ │ +16: num_publishers (4 bytes)                      │ │
│ │ +20: num_subscribers (4 bytes)                     │ │
│ └────────────────────────────────────────────────────┘ │
├────────────────────────────────────────────────────────┤
│ Offset 16: Message Array (270,336 bytes)               │
│ ┌────────────────────────────────────────────────────┐ │
│ │ Message 0 (264 bytes)                              │ │
│ │  +0:  timestamp (8 bytes)                          │ │
│ │  +8:  size (4 bytes)                               │ │
│ │  +12: data[252] (252 bytes)                        │ │
│ ├────────────────────────────────────────────────────┤ │
│ │ Message 1 (264 bytes)                              │ │
│ │  ...                                               │ │
│ ├────────────────────────────────────────────────────┤ │
│ │ ...                                                │ │
│ ├────────────────────────────────────────────────────┤ │
│ │ Message 1023 (264 bytes)                           │ │
│ └────────────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────────┘
```

## Synchronization Model

### Memory Ordering

```cpp
// Publisher (Writer)
write_pos.load(memory_order_relaxed)     // Local read, no sync needed
read_pos.load(memory_order_acquire)      // Sync with consumer
memcpy(buffer, data, size)               // Data write
write_pos.store(+1, memory_order_release) // Publish to consumer

// Subscriber (Reader)
read_pos.load(memory_order_relaxed)      // Local read, no sync needed
write_pos.load(memory_order_acquire)     // Sync with producer
memcpy(data, buffer, size)               // Data read
read_pos.store(+1, memory_order_release) // Publish to producer
```

### Happens-Before Relationships

```
Publisher Thread          Subscriber Thread
─────────────────         ─────────────────
write data to buffer
       │
       ▼
write_pos.store(release) ──────────────┐
                                       │
                                       │ synchronizes-with
                                       │
                                       ▼
                         write_pos.load(acquire)
                                       │
                                       ▼
                         read data from buffer
                                       │
                                       ▼
                         read_pos.store(release) ──────────┐
                                                            │
                                                            │ synchronizes-with
                                                            │
read_pos.load(acquire) ◄────────────────────────────────────┘
```

## Concurrency Patterns

### Single Producer, Single Consumer (SPSC)

```
┌───────────┐                    ┌────────────┐
│ Publisher │ ──── write_pos ───▶│ Ring Buffer│
│           │                    │            │
│           │ ◄─── read_pos ────│            │
└───────────┘                    └────────────┘
                                       │
                                       │
                                       ▼
                                 ┌────────────┐
                                 │ Subscriber │
                                 └────────────┘
```

**Characteristics**:
- No contention
- Optimal performance
- Predictable latency

### Multiple Producers, Single Consumer (MPSC)

```
┌─────────────┐
│ Publisher 1 │─┐
└─────────────┘ │
                ├──▶ write_pos ──▶┌────────────┐
┌─────────────┐ │                 │ Ring Buffer│──▶┌────────────┐
│ Publisher 2 │─┘                 │            │   │ Subscriber │
└─────────────┘                   └────────────┘   └────────────┘
```

**Characteristics**:
- Atomic contention on write_pos
- Potential message reordering
- Reduced throughput

### Single Producer, Multiple Consumers (SPMC)

```
                                  ┌────────────┐
                                  │Subscriber 1│
                                  └────────────┘
                                       ▲
                                       │
┌───────────┐    ┌────────────┐       │
│ Publisher │───▶│ Ring Buffer│───────┤
└───────────┘    └────────────┘       │
                                       │
                                       ▼
                                  ┌────────────┐
                                  │Subscriber 2│
                                  └────────────┘
```

**Characteristics**:
- Broadcast: All subscribers see all messages
- Slowest subscriber determines buffer pressure
- Requires per-subscriber read indices (not implemented)

## Cache Optimization

### Cache Line Alignment

```
CPU Cache Line (64 bytes typical)
┌────────────────────────────────────────────────────────┐
│ Message (264 bytes) spans 4.125 cache lines            │
├────────────────────────────────────────────────────────┤
│ Line 0: timestamp, size, data[0..51]                   │
│ Line 1: data[52..115]                                  │
│ Line 2: data[116..179]                                 │
│ Line 3: data[180..243]                                 │
│ Line 4: data[244..251]                                 │
└────────────────────────────────────────────────────────┘
```

### False Sharing Prevention

```
Metadata Layout (separate cache lines)
┌────────────────────────────────────────────────────────┐
│ Cache Line 0: write_pos (8 bytes) + padding            │
├────────────────────────────────────────────────────────┤
│ Cache Line 1: read_pos (8 bytes) + padding             │
└────────────────────────────────────────────────────────┘
```

Prevents false sharing between producer and consumer.

## Component Dependencies

```
┌─────────────────────────────────────────────────────────┐
│                      Application                         │
└───────────────┬─────────────────────┬───────────────────┘
                │                     │
                ▼                     ▼
         ┌─────────────┐       ┌─────────────┐
         │  Publisher  │       │ Subscriber  │
         └──────┬──────┘       └──────┬──────┘
                │                     │
                └──────────┬──────────┘
                           │
                           ▼
                    ┌─────────────┐
                    │ RingBuffer  │
                    └──────┬──────┘
                           │
                           ▼
                  ┌──────────────────┐
                  │  SharedMemory    │
                  └──────────────────┘
                           │
                           ▼
                  ┌──────────────────┐
                  │   OS (mmap/Win)  │
                  └──────────────────┘
```

## Deployment Topology

### Same Process (Threads)

```
┌─────────────────────────────────────┐
│          Process                     │
│  ┌──────────┐      ┌──────────┐    │
│  │ Thread 1 │      │ Thread 2 │    │
│  │Publisher │      │Subscriber│    │
│  └────┬─────┘      └─────┬────┘    │
│       │                  │          │
│       └────────┬─────────┘          │
│                ▼                     │
│         ┌─────────────┐             │
│         │Shared Memory│             │
│         └─────────────┘             │
└─────────────────────────────────────┘
```

### Different Processes (IPC)

```
┌──────────────────┐         ┌──────────────────┐
│   Process A      │         │   Process B      │
│  ┌──────────┐   │         │  ┌──────────┐   │
│  │Publisher │   │         │  │Subscriber│   │
│  └────┬─────┘   │         │  └─────┬────┘   │
│       │         │         │        │        │
└───────┼─────────┘         └────────┼────────┘
        │                            │
        └──────────┬─────────────────┘
                   ▼
            ┌─────────────┐
            │Shared Memory│
            │  (OS Kernel)│
            └─────────────┘
```

## Performance Characteristics

### Latency Distribution

```
Latency (ns)
    │
5000│                                        ╱
    │                                      ╱
4000│                                    ╱
    │                                  ╱
3000│                               ╱
    │                            ╱
2000│                        ╱╱╱
    │                   ╱╱╱╱
1000│             ╱╱╱╱╱
    │      ╱╱╱╱╱╱
 500│╱╱╱╱╱
    └────────────────────────────────────▶
     P50  P90  P99  P99.9  P99.99  Max
```

### Throughput vs. Latency

```
Throughput
(msg/sec)
    │
10M │ ●
    │   ●
 8M │     ●
    │       ●
 6M │         ●
    │           ●
 4M │             ●
    │               ●
 2M │                 ●
    │                   ●
  0 └─────────────────────────────▶
    0   500  1000  1500  2000  2500
              Latency (ns)
```

As throughput increases, latency increases due to:
- Buffer contention
- Cache pressure
- CPU saturation

---

This architecture achieves ultra-low latency through:
1. **Zero-copy** shared memory
2. **Lock-free** atomic operations
3. **Cache-optimized** data structures
4. **Minimal** abstraction layers
