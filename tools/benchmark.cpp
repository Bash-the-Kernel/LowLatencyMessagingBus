/**
 * @file benchmark.cpp
 * @brief Performance benchmark tool for latency and throughput measurement
 * 
 * This tool measures:
 * - Round-trip latency (P50, P99, P99.9 percentiles)
 * - Message throughput (messages per second)
 * 
 * Results help validate performance characteristics and identify bottlenecks.
 */

#include "publisher.hpp"
#include "subscriber.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <numeric>
#include <algorithm>

/// Number of messages for benchmark (excluding warmup)
constexpr size_t NUM_MESSAGES = 100000;

/// Warmup messages to stabilize CPU frequency and caches
constexpr size_t WARMUP = 1000;

/**
 * @brief Benchmark round-trip latency
 * 
 * Measures time from publish() call to receive() completion.
 * Includes:
 * - Serialization overhead
 * - Ring buffer write
 * - Memory synchronization
 * - Ring buffer read
 * - Deserialization overhead
 * 
 * Results are sorted to calculate percentiles (P50, P99, P99.9).
 */
void benchmark_latency() {
    std::cout << "\n=== Latency Benchmark ===\n";
    std::cout << "Measuring round-trip latency...\n\n";
    
    // Create dedicated bus for latency test
    Publisher pub("bench_bus", true);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    Subscriber sub("bench_bus");
    
    // Reserve space for latency measurements
    std::vector<uint64_t> latencies;
    latencies.reserve(NUM_MESSAGES);
    
    // Run benchmark with warmup period
    for (size_t i = 0; i < NUM_MESSAGES + WARMUP; ++i) {
        uint64_t data = i;
        
        // Start timing
        auto start = std::chrono::high_resolution_clock::now();
        
        // Publish message (retry until successful)
        while (!pub.publish(&data, sizeof(data))) {
            std::this_thread::yield();
        }
        
        // Receive message (retry until successful)
        char buffer[256];
        uint32_t size;
        uint64_t timestamp;
        
        while (!sub.receive(buffer, size, timestamp)) {
            std::this_thread::yield();
        }
        
        // End timing
        auto end = std::chrono::high_resolution_clock::now();
        
        // Record latency (skip warmup period)
        if (i >= WARMUP) {
            uint64_t latency = std::chrono::duration_cast<std::chrono::nanoseconds>(
                end - start).count();
            latencies.push_back(latency);
        }
    }
    
    // Sort for percentile calculation
    std::sort(latencies.begin(), latencies.end());
    
    // Calculate statistics
    uint64_t sum = std::accumulate(latencies.begin(), latencies.end(), 0ULL);
    double avg = static_cast<double>(sum) / latencies.size();
    uint64_t p50 = latencies[latencies.size() / 2];
    uint64_t p99 = latencies[latencies.size() * 99 / 100];
    uint64_t p999 = latencies[latencies.size() * 999 / 1000];
    uint64_t min = latencies.front();
    uint64_t max = latencies.back();
    
    // Print results
    std::cout << "Messages: " << NUM_MESSAGES << "\n";
    std::cout << "Average latency: " << avg << " ns (" 
              << (avg / 1000.0) << " μs)\n";
    std::cout << "P50 latency: " << p50 << " ns (" 
              << (p50 / 1000.0) << " μs)\n";
    std::cout << "P99 latency: " << p99 << " ns (" 
              << (p99 / 1000.0) << " μs)\n";
    std::cout << "P99.9 latency: " << p999 << " ns (" 
              << (p999 / 1000.0) << " μs)\n";
    std::cout << "Min latency: " << min << " ns (" 
              << (min / 1000.0) << " μs)\n";
    std::cout << "Max latency: " << max << " ns (" 
              << (max / 1000.0) << " μs)\n";
    
    // Cleanup
    SharedMemory::unlink("bench_bus");
}

/**
 * @brief Benchmark message throughput
 * 
 * Measures maximum messages per second with concurrent producer/consumer.
 * Producer and consumer run in separate threads to simulate real workload.
 * 
 * Throughput is limited by:
 * - CPU speed
 * - Memory bandwidth
 * - Cache contention
 * - Context switching overhead
 */
void benchmark_throughput() {
    std::cout << "\n=== Throughput Benchmark ===\n";
    std::cout << "Measuring concurrent throughput...\n\n";
    
    // Create dedicated bus for throughput test
    Publisher pub("throughput_bus", true);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    Subscriber sub("throughput_bus");
    
    // Start timing
    auto start = std::chrono::high_resolution_clock::now();
    
    // Producer thread: publish messages as fast as possible
    std::thread producer([&pub]() {
        for (size_t i = 0; i < NUM_MESSAGES; ++i) {
            uint64_t data = i;
            // Retry until successful (backpressure handling)
            while (!pub.publish(&data, sizeof(data))) {
                std::this_thread::yield();
            }
        }
    });
    
    // Consumer thread: receive messages as fast as possible
    std::thread consumer([&sub]() {
        size_t count = 0;
        while (count < NUM_MESSAGES) {
            char buffer[256];
            uint32_t size;
            uint64_t timestamp;
            
            if (sub.receive(buffer, size, timestamp)) {
                ++count;
            }
            // Note: No sleep here - busy-wait for maximum throughput
        }
    });
    
    // Wait for both threads to complete
    producer.join();
    consumer.join();
    
    // End timing
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
    
    // Calculate throughput
    double throughput = (NUM_MESSAGES * 1000.0) / duration;
    double throughput_millions = throughput / 1000000.0;
    
    // Print results
    std::cout << "Messages: " << NUM_MESSAGES << "\n";
    std::cout << "Duration: " << duration << " ms\n";
    std::cout << "Throughput: " << static_cast<uint64_t>(throughput) 
              << " msg/sec\n";
    std::cout << "Throughput: " << throughput_millions << " M msg/sec\n";
    
    // Cleanup
    SharedMemory::unlink("throughput_bus");
}

/**
 * @brief Main entry point for benchmark tool
 * 
 * Runs both latency and throughput benchmarks sequentially.
 * Results can be compared against expected performance characteristics.
 */
int main() {
    std::cout << "Low Latency Messaging Bus - Benchmark Tool\n";
    std::cout << "==========================================\n";
    std::cout << "\nConfiguration:\n";
    std::cout << "  Messages: " << NUM_MESSAGES << "\n";
    std::cout << "  Warmup: " << WARMUP << "\n";
    std::cout << "  Message size: 8 bytes\n";
    std::cout << "  Buffer capacity: 1024 messages\n\n";
    
    try {
        // Run benchmarks
        benchmark_latency();
        benchmark_throughput();
        
        // Print interpretation guide
        std::cout << "\n========================================\n";
        std::cout << "Performance Interpretation\n";
        std::cout << "========================================\n";
        std::cout << "Expected Latency:\n";
        std::cout << "  P50: 200-500 ns (good)\n";
        std::cout << "  P99: 1-2 μs (good)\n";
        std::cout << "  P99.9: 2-5 μs (good)\n\n";
        std::cout << "Expected Throughput:\n";
        std::cout << "  Single-threaded: 5-10M msg/sec\n";
        std::cout << "  Multi-threaded: 2-5M msg/sec\n\n";
        std::cout << "Note: Results vary by CPU, memory, and system load\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\nBenchmark failed: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
