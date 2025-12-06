#pragma once
#include "shared_memory.hpp"
#include "ring_buffer.hpp"
#include <functional>

class Subscriber {
public:
    using MessageHandler = std::function<void(const void*, uint32_t, uint64_t)>;
    
    Subscriber(const std::string& bus_name)
        : shm_(bus_name, RingBuffer::BUFFER_SIZE, false),
          ring_(shm_.data()) {
        ring_.register_subscriber();
    }
    
    bool receive(void* data, uint32_t& size, uint64_t& timestamp) {
        Message msg;
        if (!ring_.try_read(msg)) return false;
        
        size = msg.size;
        timestamp = msg.timestamp;
        std::memcpy(data, msg.data, msg.size);
        return true;
    }
    
    void poll(MessageHandler handler) {
        Message msg;
        while (ring_.try_read(msg)) {
            handler(msg.data, msg.size, msg.timestamp);
        }
    }
    
    size_t available() const { return ring_.available(); }

private:
    SharedMemory shm_;
    RingBuffer ring_;
};
