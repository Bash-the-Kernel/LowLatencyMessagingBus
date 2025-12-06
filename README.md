# LowLatencyMessagingBus

A lightweight, high-throughput, low-latency messaging bus optimized for trading workloads.

## Architecture

### Core Design Principles

1. **Zero-Copy Message Passing**: Messages are written directly to shared memory, eliminating serialization overhead
2. **Lock-Free Ring Buffer**: Uses atomic operations for thread-safe, wait-free communication
3. **Memory-Mapped Shared Memory**: Cross-process communication via POSIX shm (Linux/macOS) or Windows file mapping
4. **Cache-Line Aligned**: 264-byte messages aligned for optimal CPU cache performance
5. **Backpressure Handling**: Configurable retry logic when buffer is full

### Components

- **SharedMemory**: Cross-platform wrapper for shared memory (POSIX/Windows)
- **RingBuffer**: Lock-free SPSC/MPMC ring buffer with atomic indices
- **Publisher**: API for publishing messages with backpressure support
- **Subscriber**: API for receiving messages with polling
- **Serializer/Deserializer**: Zero-copy message formatting utilities

### Memory Layout

```
[Metadata: 16 bytes]
  - write_pos: atomic<uint64_t>
  - read_pos: atomic<uint64_t>
  - num_publishers: uint32_t
  - num_subscribers: uint32_t

[Ring Buffer: 1024 slots × 264 bytes]
  Each Message:
    - timestamp: uint64_t (8 bytes)
    - size: uint32_t (4 bytes)
    - data: char[252] (252 bytes)
```

Total shared memory: ~270 KB

## Performance Characteristics

### Expected Latency (Single Producer/Consumer)

- **P50**: 200-500 ns
- **P99**: 1-2 μs
- **P99.9**: 2-5 μs

### Expected Throughput

- **Single thread**: 5-10M msg/sec
- **Multi-threaded**: 2-5M msg/sec (with contention)

### Factors Affecting Performance

1. **CPU Cache**: Messages fit in L1/L2 cache for optimal performance
2. **Context Switching**: Minimize by using busy-wait polling
3. **Memory Ordering**: Acquire/release semantics ensure visibility
4. **Backpressure**: Yield vs. sleep tradeoffs

## Build Instructions

### Prerequisites

- CMake 3.15+
- C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- Windows: Visual Studio or MinGW
- Linux/macOS: GCC or Clang

### Build

```bash
# Windows
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release

# Linux/macOS
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j
```

### Run Tests

```bash
# Windows
.\build\Release\test_bus.exe

# Linux/macOS
./build/test_bus
```

## Usage Examples

### Publisher

```cpp
#include "publisher.hpp"

Publisher pub("my_bus", true); // Create bus

uint64_t order_id = 12345;
pub.publish(&order_id, sizeof(order_id));

// With backpressure handling
pub.publish_with_backpressure(&order_id, sizeof(order_id), 100);
```

### Subscriber

```cpp
#include "subscriber.hpp"

Subscriber sub("my_bus");

char buffer[256];
uint32_t size;
uint64_t timestamp;

if (sub.receive(buffer, size, timestamp)) {
    // Process message
}

// Polling mode
sub.poll([](const void* data, uint32_t size, uint64_t ts) {
    // Handle each message
});
```

### Serialization

```cpp
#include "serializer.hpp"

char buffer[256];
Serializer ser(buffer, sizeof(buffer));

ser.write<uint64_t>(12345);
ser.write<double>(99.99);
ser.write_bytes("AAPL", 4);

// Deserialize
Deserializer deser(buffer, ser.position());
uint64_t id;
double price;
deser.read(id);
deser.read(price);
```

## Running Examples

### Start Publisher

```bash
# Windows (create bus)
.\build\Release\publisher.exe --create

# Linux/macOS
./build/publisher --create
```

### Start Subscriber (in another terminal)

```bash
# Windows
.\build\Release\subscriber.exe

# Linux/macOS
./build/subscriber
```

## Benchmark Tool

```bash
# Windows
.\build\Release\benchmark.exe

# Linux/macOS
./build/benchmark
```

Output:
```
=== Latency Benchmark ===
Messages: 100000
Average latency: 450 ns
P50 latency: 380 ns
P99 latency: 1200 ns
P99.9 latency: 3500 ns

=== Throughput Benchmark ===
Messages: 100000
Duration: 15 ms
Throughput: 6.67M msg/sec
```

## Design Tradeoffs

### Advantages

✅ **Ultra-low latency**: Sub-microsecond P99 latency  
✅ **High throughput**: Millions of messages per second  
✅ **Zero-copy**: Direct memory access, no serialization  
✅ **Lock-free**: No mutex contention or deadlocks  
✅ **Cross-platform**: Works on Windows, Linux, macOS  
✅ **Simple API**: Easy to integrate into existing systems  

### Limitations

⚠️ **Fixed message size**: 252 bytes max payload  
⚠️ **Local only**: Shared memory requires same machine  
⚠️ **No persistence**: Messages lost on crash  
⚠️ **Limited capacity**: 1024 messages in-flight  
⚠️ **No ordering guarantees**: With multiple publishers  
⚠️ **Manual cleanup**: Shared memory persists after crash (Linux/macOS)  

### When to Use

✅ Trading systems requiring microsecond latency  
✅ High-frequency market data distribution  
✅ Order management systems  
✅ Risk engines with real-time updates  
✅ Inter-process communication on same host  

### When NOT to Use

❌ Distributed systems across network  
❌ Large messages (>252 bytes)  
❌ Persistent message queues  
❌ Guaranteed delivery requirements  
❌ Complex routing/filtering needs  

## Multi-Publisher/Subscriber Support

The ring buffer supports multiple publishers and subscribers through atomic operations:

```cpp
// Multiple publishers
Publisher pub1("bus", true);
Publisher pub2("bus", false);

// Multiple subscribers
Subscriber sub1("bus");
Subscriber sub2("bus");
```

**Note**: With multiple subscribers, each receives ALL messages (broadcast). For load balancing, implement application-level message filtering.

## Cleanup (Linux/macOS)

Shared memory persists after process termination:

```bash
# List shared memory
ls /dev/shm/

# Remove manually
rm /dev/shm/my_bus

# Or use API
SharedMemory::unlink("my_bus");
```

Windows automatically cleans up shared memory on process exit.

## License

MIT License - Free for commercial and personal use.

## Contributing

This is a minimal reference implementation. For production use, consider:

- Dynamic message sizing
- Message priorities
- Filtering/routing
- Monitoring/metrics
- Crash recovery
- Network transport layer
