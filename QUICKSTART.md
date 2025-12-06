# Quick Start Guide

## 1. Build the Project

### Windows

**Option A: Simple Build (No CMake)**
```bash
.\build_simple.bat
```
Requires: Visual Studio 2017+ with C++ tools

**Option B: CMake Build**
```bash
.\build.bat
```
Requires: CMake + Visual Studio or MinGW

**Option C: Developer Command Prompt**
```bash
# Open "Developer Command Prompt for VS"
cd C:\Users\jackw\Documents\GitHub\LowLatencyMessagingBus
.\build_simple.bat
```

### Linux/macOS
```bash
chmod +x build.sh
./build.sh
```

## 2. Run Integration Tests

```bash
# Windows
.\build\test_bus.exe

# Linux/macOS
./build/test_bus
```

Expected output:
```
Running Integration Tests
==========================
Test: Single Publisher/Subscriber... PASSED
Test: Multiple Messages... PASSED
Test: Backpressure Handling... PASSED
Test: Serialization... PASSED
Test: Concurrent Pub/Sub... PASSED

All tests PASSED!
```

## 3. Run Benchmark

```bash
# Windows
.\build\benchmark.exe

# Linux/macOS
./build/benchmark
```

## 4. Test Publisher/Subscriber

### Terminal 1 - Start Publisher
```bash
# Windows
.\build\publisher.exe --create

# Linux/macOS
./build/publisher --create
```

### Terminal 2 - Start Subscriber
```bash
# Windows
.\build\subscriber.exe

# Linux/macOS
./build/subscriber
```

## 5. Integrate into Your Project

### Copy headers to your project:
```
include/shared_memory.hpp
include/ring_buffer.hpp
include/publisher.hpp
include/subscriber.hpp
include/serializer.hpp
```

### Basic usage:
```cpp
#include "publisher.hpp"
#include "subscriber.hpp"

// Publisher
Publisher pub("my_bus", true);
uint64_t data = 12345;
pub.publish(&data, sizeof(data));

// Subscriber
Subscriber sub("my_bus");
char buffer[256];
uint32_t size;
uint64_t timestamp;
if (sub.receive(buffer, size, timestamp)) {
    // Process message
}
```

## Troubleshooting

### Windows: No compiler found

**Solution 1: Install Visual Studio**
1. Download Visual Studio Community (free): https://visualstudio.microsoft.com/
2. During install, select "Desktop development with C++"
3. Run `.\build_simple.bat`

**Solution 2: Use Developer Command Prompt**
1. Search for "Developer Command Prompt for VS" in Start Menu
2. Navigate to project folder
3. Run `.\build_simple.bat`

**Solution 3: Install MinGW**
1. Download MinGW-w64: https://www.mingw-w64.org/
2. Add to PATH
3. Run `.\build.bat`

### Linux/macOS: Permission denied
```bash
sudo chmod 777 /dev/shm
```

### Shared memory not cleaned up (Linux/macOS)
```bash
rm /dev/shm/trading_bus
rm /dev/shm/bench_bus
rm /dev/shm/throughput_bus
```
