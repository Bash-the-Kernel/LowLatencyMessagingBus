#include "publisher.hpp"
#include "subscriber.hpp"
#include "serializer.hpp"
#include <iostream>
#include <thread>
#include <cassert>

void test_single_pub_sub() {
    std::cout << "Test: Single Publisher/Subscriber... ";
    
    Publisher pub("test_bus1", true);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    Subscriber sub("test_bus1");
    
    const char* msg = "Hello, World!";
    assert(pub.publish(msg, strlen(msg) + 1));
    
    char buffer[256];
    uint32_t size;
    uint64_t timestamp;
    assert(sub.receive(buffer, size, timestamp));
    assert(strcmp(buffer, msg) == 0);
    
    SharedMemory::unlink("test_bus1");
    std::cout << "PASSED\n";
}

void test_multi_messages() {
    std::cout << "Test: Multiple Messages... ";
    
    Publisher pub("test_bus2", true);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    Subscriber sub("test_bus2");
    
    constexpr int N = 100;
    for (int i = 0; i < N; ++i) {
        assert(pub.publish(&i, sizeof(i)));
    }
    
    for (int i = 0; i < N; ++i) {
        char buffer[256];
        uint32_t size;
        uint64_t timestamp;
        assert(sub.receive(buffer, size, timestamp));
        int value;
        memcpy(&value, buffer, sizeof(value));
        assert(value == i);
    }
    
    SharedMemory::unlink("test_bus2");
    std::cout << "PASSED\n";
}

void test_backpressure() {
    std::cout << "Test: Backpressure Handling... ";
    
    Publisher pub("test_bus3", true);
    
    // Fill the buffer
    int count = 0;
    while (pub.publish(&count, sizeof(count))) {
        ++count;
    }
    
    assert(count > 0);
    assert(pub.free_space() == 0);
    
    // Should fail without backpressure
    int dummy = 999;
    assert(!pub.publish(&dummy, sizeof(dummy)));
    
    // Create subscriber and drain
    Subscriber sub("test_bus3");
    char buffer[256];
    uint32_t size;
    uint64_t timestamp;
    
    int received = 0;
    while (sub.receive(buffer, size, timestamp)) {
        ++received;
    }
    
    assert(received == count);
    
    // Now should succeed
    assert(pub.publish(&dummy, sizeof(dummy)));
    
    SharedMemory::unlink("test_bus3");
    std::cout << "PASSED\n";
}

void test_serialization() {
    std::cout << "Test: Serialization... ";
    
    char buffer[256];
    Serializer ser(buffer, sizeof(buffer));
    
    uint64_t id = 12345;
    double price = 99.99;
    uint32_t qty = 100;
    
    assert(ser.write(id));
    assert(ser.write(price));
    assert(ser.write(qty));
    
    Deserializer deser(buffer, ser.position());
    
    uint64_t id2;
    double price2;
    uint32_t qty2;
    
    assert(deser.read(id2));
    assert(deser.read(price2));
    assert(deser.read(qty2));
    
    assert(id == id2);
    assert(price == price2);
    assert(qty == qty2);
    
    std::cout << "PASSED\n";
}

void test_concurrent() {
    std::cout << "Test: Concurrent Pub/Sub... ";
    
    Publisher pub("test_bus4", true);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    Subscriber sub("test_bus4");
    
    constexpr int N = 1000;
    std::atomic<int> received{0};
    
    std::thread producer([&pub]() {
        for (int i = 0; i < N; ++i) {
            while (!pub.publish(&i, sizeof(i))) {
                std::this_thread::yield();
            }
        }
    });
    
    std::thread consumer([&sub, &received]() {
        while (received < N) {
            char buffer[256];
            uint32_t size;
            uint64_t timestamp;
            if (sub.receive(buffer, size, timestamp)) {
                ++received;
            }
        }
    });
    
    producer.join();
    consumer.join();
    
    assert(received == N);
    
    SharedMemory::unlink("test_bus4");
    std::cout << "PASSED\n";
}

int main() {
    std::cout << "Running Integration Tests\n";
    std::cout << "==========================\n";
    
    try {
        test_single_pub_sub();
        test_multi_messages();
        test_backpressure();
        test_serialization();
        test_concurrent();
        
        std::cout << "\nAll tests PASSED!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\nTest FAILED: " << e.what() << "\n";
        return 1;
    }
}
