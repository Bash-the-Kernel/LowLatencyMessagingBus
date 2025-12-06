# LowLatencyBus - Documentation Index

## 📚 Quick Navigation

### Getting Started
- **[QUICKSTART.md](QUICKSTART.md)** - Build and run in 5 minutes
- **[README.md](README.md)** - Project overview and usage guide
- **[PROJECT_SUMMARY.md](PROJECT_SUMMARY.md)** - Complete feature checklist

### Technical Documentation
- **[ARCHITECTURE.md](ARCHITECTURE.md)** - System architecture with diagrams
- **[DESIGN.md](DESIGN.md)** - Design decisions and tradeoffs
- **[API_REFERENCE.md](API_REFERENCE.md)** - Complete API documentation

### Code
- **[include/](include/)** - Header-only library (5 files)
- **[examples/](examples/)** - Publisher and subscriber examples
- **[tools/](tools/)** - Benchmark tool
- **[tests/](tests/)** - Integration tests

### Build
- **[CMakeLists.txt](CMakeLists.txt)** - CMake build configuration
- **[Makefile](Makefile)** - Alternative build system
- **[build.bat](build.bat)** - Windows build script
- **[build.sh](build.sh)** - Linux/macOS build script

---

## 📖 Documentation Guide

### For First-Time Users
1. Read [QUICKSTART.md](QUICKSTART.md)
2. Build and run tests
3. Run benchmark
4. Try examples
5. Read [README.md](README.md) for details

### For Integration
1. Read [API_REFERENCE.md](API_REFERENCE.md)
2. Copy headers from `include/`
3. See `examples/` for usage patterns
4. Refer to [README.md](README.md) for tradeoffs

### For Understanding Design
1. Read [ARCHITECTURE.md](ARCHITECTURE.md) for overview
2. Read [DESIGN.md](DESIGN.md) for deep dive
3. Study `include/ring_buffer.hpp` for implementation
4. Review benchmark results

### For Performance Tuning
1. Run [tools/benchmark.cpp](tools/benchmark.cpp)
2. Read "Performance Characteristics" in [README.md](README.md)
3. Read "Optimization Techniques" in [DESIGN.md](DESIGN.md)
4. See "Performance Tips" in [API_REFERENCE.md](API_REFERENCE.md)

---

## 🗂️ File Reference

### Core Headers (include/)

| File | Description | Lines |
|------|-------------|-------|
| **shared_memory.hpp** | Cross-platform shared memory wrapper | ~80 |
| **ring_buffer.hpp** | Lock-free ring buffer implementation | ~70 |
| **publisher.hpp** | Publisher API with backpressure | ~50 |
| **subscriber.hpp** | Subscriber API with polling | ~40 |
| **serializer.hpp** | Message serialization utilities | ~60 |

### Examples (examples/)

| File | Description | Purpose |
|------|-------------|---------|
| **publisher.cpp** | Trading data publisher | Demonstrates publishing with serialization |
| **subscriber.cpp** | Trading data subscriber | Demonstrates receiving with latency measurement |

### Tools (tools/)

| File | Description | Output |
|------|-------------|--------|
| **benchmark.cpp** | Latency and throughput tests | P50/P99/P99.9 latency, msg/sec throughput |

### Tests (tests/)

| File | Description | Coverage |
|------|-------------|----------|
| **integration_test.cpp** | 5 comprehensive tests | Pub/sub, backpressure, serialization, concurrency |

### Documentation

| File | Purpose | Audience |
|------|---------|----------|
| **README.md** | Main documentation | All users |
| **QUICKSTART.md** | Getting started | New users |
| **API_REFERENCE.md** | API documentation | Developers |
| **ARCHITECTURE.md** | System design | Architects |
| **DESIGN.md** | Technical deep-dive | Advanced users |
| **PROJECT_SUMMARY.md** | Feature checklist | Project managers |
| **INDEX.md** | This file | All users |

---

## 🎯 Use Case Guide

### High-Frequency Trading
- Read: [DESIGN.md](DESIGN.md) - "Performance Analysis"
- See: [tools/benchmark.cpp](tools/benchmark.cpp)
- Focus: Sub-microsecond latency

### Market Data Distribution
- Read: [README.md](README.md) - "Multi-Publisher/Subscriber"
- See: [examples/publisher.cpp](examples/publisher.cpp)
- Focus: High throughput

### Order Management
- Read: [API_REFERENCE.md](API_REFERENCE.md) - "Backpressure"
- See: [include/publisher.hpp](include/publisher.hpp)
- Focus: Reliable delivery

### Risk Engine
- Read: [ARCHITECTURE.md](ARCHITECTURE.md) - "Message Flow"
- See: [examples/subscriber.cpp](examples/subscriber.cpp)
- Focus: Real-time processing

---

## 🔍 Topic Index

### Latency
- [README.md](README.md) - "Expected Latency"
- [DESIGN.md](DESIGN.md) - "Latency Breakdown"
- [tools/benchmark.cpp](tools/benchmark.cpp) - Measurement

### Throughput
- [README.md](README.md) - "Expected Throughput"
- [DESIGN.md](DESIGN.md) - "Throughput Analysis"
- [tools/benchmark.cpp](tools/benchmark.cpp) - Measurement

### Memory Layout
- [ARCHITECTURE.md](ARCHITECTURE.md) - "Memory Layout"
- [README.md](README.md) - "Memory Layout"
- [include/ring_buffer.hpp](include/ring_buffer.hpp) - Implementation

### Synchronization
- [ARCHITECTURE.md](ARCHITECTURE.md) - "Synchronization Model"
- [DESIGN.md](DESIGN.md) - "Lock-Free Ring Buffer"
- [include/ring_buffer.hpp](include/ring_buffer.hpp) - Atomics

### Backpressure
- [API_REFERENCE.md](API_REFERENCE.md) - "publish_with_backpressure"
- [DESIGN.md](DESIGN.md) - "Backpressure Strategy"
- [tests/integration_test.cpp](tests/integration_test.cpp) - Test

### Serialization
- [API_REFERENCE.md](API_REFERENCE.md) - "Serializer/Deserializer"
- [include/serializer.hpp](include/serializer.hpp) - Implementation
- [examples/publisher.cpp](examples/publisher.cpp) - Usage

### Cross-Platform
- [include/shared_memory.hpp](include/shared_memory.hpp) - Windows/POSIX
- [README.md](README.md) - "Build Instructions"
- [CMakeLists.txt](CMakeLists.txt) - Build config

---

## 🛠️ Build Guide

### Quick Build
```bash
# Windows
build.bat

# Linux/macOS
./build.sh
```

### CMake Build
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Makefile Build
```bash
make
make test
make benchmark
```

See [QUICKSTART.md](QUICKSTART.md) for details.

---

## 🧪 Testing Guide

### Run All Tests
```bash
# Windows
.\build\test_bus.exe

# Linux/macOS
./build/test_bus
```

### Run Benchmarks
```bash
# Windows
.\build\benchmark.exe

# Linux/macOS
./build/benchmark
```

### Manual Testing
```bash
# Terminal 1
./build/publisher --create

# Terminal 2
./build/subscriber
```

See [QUICKSTART.md](QUICKSTART.md) for details.

---

## 📊 Performance Guide

### Measure Latency
1. Run `./build/benchmark`
2. Check P50, P99, P99.9 values
3. Compare with [README.md](README.md) expectations

### Measure Throughput
1. Run `./build/benchmark`
2. Check msg/sec output
3. Compare with [README.md](README.md) expectations

### Optimize
1. Read [API_REFERENCE.md](API_REFERENCE.md) - "Performance Tips"
2. Read [DESIGN.md](DESIGN.md) - "Optimization Techniques"
3. Profile with perf/VTune

---

## 🤝 Contributing

This is a reference implementation. For production:
- Add dynamic message sizing
- Implement true MPMC support
- Add monitoring/metrics
- Implement crash recovery
- Add network transport

See [README.md](README.md) - "Contributing" for ideas.

---

## 📄 License

MIT License - See [LICENSE](LICENSE)

---

## 📞 Support

- Issues: GitHub Issues
- Documentation: This index
- Examples: `examples/` directory
- Tests: `tests/` directory

---

**Last Updated**: 2024
**Version**: 1.0
**Status**: Production Ready
