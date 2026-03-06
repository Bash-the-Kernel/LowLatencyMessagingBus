#pragma once
#include "shared_memory.hpp"
#include "ring_buffer.hpp"
#include <chrono>
#include <thread>

/**
 * @brief Publisher API for sending messages to the bus
 * 
 * Provides high-level interface for publishing messages with automatic
 * timestamping and backpressure handling.
 * 
 * Design Principles:
 * - Single Responsibility: Handles message publishing only
 * - Dependency Inversion: Depends on SharedMemory and RingBuffer abstractions
 * - Interface Segregation: Minimal, focused API
 * 
 * Thread Safety: Safe for single producer. Multiple producers supported
 * but may experience contention on atomic operations.
 */
class Publisher {
public:
    /**
     * @brief Construct publisher and connect to message bus
     * 
     * @param bus_name Unique identifier for the message bus
     * @param create If true, creates new bus; if false, connects to existing
     * @throws std::runtime_error if bus creation/connection fails
     */
    Publisher(const std::string& bus_name, bool create = false)
        : shm_(bus_name, RingBuffer::BUFFER_SIZE, create),
          ring_(shm_.data()) {
        // Initialize ring buffer if creating new bus
        if (create) {
            ring_.init();
        }
        // Register this publisher for monitoring
        ring_.register_publisher();
    }
    
    /**
     * @brief Publish message to bus (non-blocking)
     * 
     * @param data Pointer to message data
     * @param size Size of message in bytes (max 252)
     * @return true if published successfully, false if buffer full
     * 
     * Note: Automatically adds nanosecond timestamp to message
     */
    bool publish(const void* data, uint32_t size) {
        // Validate message size
        if (size > sizeof(Message::data)) {
            return false;  // Message too large
        }
        
        // Prepare message with timestamp
        Message msg;
        msg.timestamp = now_ns();  // Capture current time
        msg.size = size;
        std::memcpy(msg.data, data, size);
        
        // Attempt to write to ring buffer
        return ring_.try_write(msg);
    }
    
    /**
     * @brief Publish message with backpressure handling (blocking)
     * 
     * Retries publishing if buffer is full, yielding CPU between attempts.
     * Useful for ensuring message delivery when buffer contention is expected.
     * 
     * @param data Pointer to message data
     * @param size Size of message in bytes (max 252)
     * @param max_retries Maximum number of retry attempts (default: 100)
     * @return true if published, false if all retries exhausted
     * 
     * Performance Note: Each retry adds ~1-10 μs latency depending on
     * system load. For lowest latency, use publish() and handle failures.
     */
    bool publish_with_backpressure(const void* data, uint32_t size, 
                                   int max_retries = 100) {
        // Retry loop with yield
        for (int i = 0; i < max_retries; ++i) {
            if (publish(data, size)) {
                return true;  // Success
            }
            // Yield CPU to allow consumer to drain buffer
            std::this_thread::yield();
        }
        return false;  // All retries exhausted
    }
    
    /**
     * @brief Get number of free slots in buffer
     * @return Count of available write slots
     */
    size_t free_space() const { 
        return ring_.free_space(); 
    }

private:
    SharedMemory shm_;   ///< Shared memory region
    RingBuffer ring_;    ///< Ring buffer interface
    
    /**
     * @brief Get current time in nanoseconds since epoch
     * @return Timestamp in nanoseconds
     */
    static uint64_t now_ns() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    }
};
