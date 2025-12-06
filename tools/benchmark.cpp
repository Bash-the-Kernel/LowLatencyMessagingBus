#include "publisher.hpp"
#include "subscriber.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <numeric>
#include <algorithm>

constexpr size_t NUM_MESSAGES = 100000;
constexpr size_t WARMUP = 1000;

void benchmark_latency() {
    std::cout << "\n=== Latency Benchmark ===\n";
    
    Publisher pub("bench_bus", true);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    Subscriber sub("bench_bus");
    
    std::vector<uint64_t> latencies;
    latencies.reserve(NUM_MESSAGES);
    
    for (size_t i = 0; i < NUM_MESSAGES + WARMUP; ++i) {
        uint64_t data = i;
        auto start = std::chrono::high_resolution_clock::now();
        
        while (!pub.publish(&data, sizeof(data))) {
            std::this_thread::yield();
        }
        
        char buffer[256];
        uint32_t size;
        uint64_t timestamp;
        
        while (!sub.receive(buffer, size, timestamp)) {
            std::this_thread::yield();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        
        if (i >= WARMUP) {
            uint64_t latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            latencies.push_back(latency);
        }
    }
    
    std::sort(latencies.begin(), latencies.end());
    
    uint64_t sum = std::accumulate(latencies.begin(), latencies.end(), 0ULL);
    double avg = static_cast<double>(sum) / latencies.size();
    uint64_t p50 = latencies[latencies.size() / 2];
    uint64_t p99 = latencies[latencies.size() * 99 / 100];
    uint64_t p999 = latencies[latencies.size() * 999 / 1000];
    
    std::cout << "Messages: " << NUM_MESSAGES << "\n";
    std::cout << "Average latency: " << avg << " ns\n";
    std::cout << "P50 latency: " << p50 << " ns\n";
    std::cout << "P99 latency: " << p99 << " ns\n";
    std::cout << "P99.9 latency: " << p999 << " ns\n";
    std::cout << "Min latency: " << latencies.front() << " ns\n";
    std::cout << "Max latency: " << latencies.back() << " ns\n";
    
    SharedMemory::unlink("bench_bus");
}

void benchmark_throughput() {
    std::cout << "\n=== Throughput Benchmark ===\n";
    
    Publisher pub("throughput_bus", true);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    Subscriber sub("throughput_bus");
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::thread producer([&pub]() {
        for (size_t i = 0; i < NUM_MESSAGES; ++i) {
            uint64_t data = i;
            while (!pub.publish(&data, sizeof(data))) {
                std::this_thread::yield();
            }
        }
    });
    
    std::thread consumer([&sub]() {
        size_t count = 0;
        while (count < NUM_MESSAGES) {
            char buffer[256];
            uint32_t size;
            uint64_t timestamp;
            
            if (sub.receive(buffer, size, timestamp)) {
                ++count;
            }
        }
    });
    
    producer.join();
    consumer.join();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    double throughput = (NUM_MESSAGES * 1000.0) / duration;
    
    std::cout << "Messages: " << NUM_MESSAGES << "\n";
    std::cout << "Duration: " << duration << " ms\n";
    std::cout << "Throughput: " << throughput << " msg/sec\n";
    std::cout << "Throughput: " << (throughput / 1000000.0) << " M msg/sec\n";
    
    SharedMemory::unlink("throughput_bus");
}

int main() {
    std::cout << "Low Latency Messaging Bus - Benchmark Tool\n";
    std::cout << "==========================================\n";
    
    benchmark_latency();
    benchmark_throughput();
    
    return 0;
}
