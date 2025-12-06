#pragma once
#include "shared_memory.hpp"
#include "ring_buffer.hpp"
#include <chrono>
#include <thread>

class Publisher {
public:
    Publisher(const std::string& bus_name, bool create = false)
        : shm_(bus_name, RingBuffer::BUFFER_SIZE, create),
          ring_(shm_.data()) {
        if (create) ring_.init();
        ring_.register_publisher();
    }
    
    bool publish(const void* data, uint32_t size) {
        if (size > sizeof(Message::data)) return false;
        
        Message msg;
        msg.timestamp = now_ns();
        msg.size = size;
        std::memcpy(msg.data, data, size);
        
        return ring_.try_write(msg);
    }
    
    bool publish_with_backpressure(const void* data, uint32_t size, int max_retries = 100) {
        for (int i = 0; i < max_retries; ++i) {
            if (publish(data, size)) return true;
            std::this_thread::yield();
        }
        return false;
    }
    
    size_t free_space() const { return ring_.free_space(); }

private:
    SharedMemory shm_;
    RingBuffer ring_;
    
    static uint64_t now_ns() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }
};
