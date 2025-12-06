#pragma once
#include <atomic>
#include <cstring>
#include <cstdint>

struct Message {
    uint64_t timestamp;
    uint32_t size;
    char data[252]; // Total 264 bytes (cache-line friendly)
};

class RingBuffer {
public:
    static constexpr size_t CAPACITY = 1024;
    static constexpr size_t BUFFER_SIZE = sizeof(Message) * CAPACITY + sizeof(Metadata);
    
    struct Metadata {
        std::atomic<uint64_t> write_pos;
        std::atomic<uint64_t> read_pos;
        uint32_t num_publishers;
        uint32_t num_subscribers;
    };
    
    RingBuffer(void* mem) : meta_(static_cast<Metadata*>(mem)),
                            buffer_(static_cast<char*>(mem) + sizeof(Metadata)) {}
    
    void init() {
        meta_->write_pos.store(0, std::memory_order_relaxed);
        meta_->read_pos.store(0, std::memory_order_relaxed);
        meta_->num_publishers = 0;
        meta_->num_subscribers = 0;
    }
    
    bool try_write(const Message& msg) {
        uint64_t write = meta_->write_pos.load(std::memory_order_relaxed);
        uint64_t read = meta_->read_pos.load(std::memory_order_acquire);
        
        if (write - read >= CAPACITY) return false; // Full
        
        uint64_t idx = write % CAPACITY;
        std::memcpy(buffer_ + idx * sizeof(Message), &msg, sizeof(Message));
        
        meta_->write_pos.store(write + 1, std::memory_order_release);
        return true;
    }
    
    bool try_read(Message& msg) {
        uint64_t read = meta_->read_pos.load(std::memory_order_relaxed);
        uint64_t write = meta_->write_pos.load(std::memory_order_acquire);
        
        if (read >= write) return false; // Empty
        
        uint64_t idx = read % CAPACITY;
        std::memcpy(&msg, buffer_ + idx * sizeof(Message), sizeof(Message));
        
        meta_->read_pos.store(read + 1, std::memory_order_release);
        return true;
    }
    
    size_t available() const {
        return meta_->write_pos.load(std::memory_order_acquire) - 
               meta_->read_pos.load(std::memory_order_acquire);
    }
    
    size_t free_space() const {
        return CAPACITY - available();
    }
    
    void register_publisher() { ++meta_->num_publishers; }
    void register_subscriber() { ++meta_->num_subscribers; }

private:
    Metadata* meta_;
    char* buffer_;
};
