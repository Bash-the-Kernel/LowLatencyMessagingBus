#pragma once
#include "shared_memory.hpp"
#include "ring_buffer.hpp"
#include <functional>

/**
 * @brief Subscriber API for receiving messages from the bus
 * 
 * Provides high-level interface for consuming messages with support for
 * both single-message polling and batch processing.
 * 
 * Design Principles:
 * - Single Responsibility: Handles message consumption only
 * - Dependency Inversion: Depends on SharedMemory and RingBuffer abstractions
 * - Open/Closed: Extensible via MessageHandler callback
 * 
 * Thread Safety: Safe for single consumer. Multiple consumers will each
 * receive all messages (broadcast pattern).
 */
class Subscriber {
public:
    /**
     * @brief Callback function type for message handling
     * @param data Pointer to message payload
     * @param size Size of payload in bytes
     * @param timestamp Message creation timestamp (nanoseconds)
     */
    using MessageHandler = std::function<void(const void*, uint32_t, uint64_t)>;
    
    /**
     * @brief Construct subscriber and connect to message bus
     * 
     * @param bus_name Unique identifier for the message bus
     * @throws std::runtime_error if bus doesn't exist or connection fails
     */
    explicit Subscriber(const std::string& bus_name)
        : shm_(bus_name, RingBuffer::BUFFER_SIZE, false),  // Open existing
          ring_(shm_.data()) {
        // Register this subscriber for monitoring
        ring_.register_subscriber();
    }
    
    /**
     * @brief Receive single message from bus (non-blocking)
     * 
     * @param data Buffer to receive message payload (must be >= 252 bytes)
     * @param size Output parameter for actual message size
     * @param timestamp Output parameter for message timestamp
     * @return true if message received, false if buffer empty
     * 
     * Usage Pattern:
     * @code
     * char buffer[256];
     * uint32_t size;
     * uint64_t timestamp;
     * if (sub.receive(buffer, size, timestamp)) {
     *     // Process message
     * }
     * @endcode
     */
    bool receive(void* data, uint32_t& size, uint64_t& timestamp) {
        Message msg;
        
        // Attempt to read from ring buffer
        if (!ring_.try_read(msg)) {
            return false;  // No messages available
        }
        
        // Extract message data
        size = msg.size;
        timestamp = msg.timestamp;
        std::memcpy(data, msg.data, msg.size);
        
        return true;
    }
    
    /**
     * @brief Process all available messages with callback (non-blocking)
     * 
     * Drains all messages currently in buffer and invokes handler for each.
     * Returns immediately if buffer is empty.
     * 
     * @param handler Callback function to process each message
     * 
     * Usage Pattern:
     * @code
     * sub.poll([](const void* data, uint32_t size, uint64_t ts) {
     *     // Process each message
     *     auto latency = now() - ts;
     *     std::cout << "Latency: " << latency << " ns\n";
     * });
     * @endcode
     * 
     * Performance Note: Batch processing reduces per-message overhead
     * compared to calling receive() in a loop.
     */
    void poll(MessageHandler handler) {
        Message msg;
        
        // Process all available messages
        while (ring_.try_read(msg)) {
            handler(msg.data, msg.size, msg.timestamp);
        }
    }
    
    /**
     * @brief Get number of messages available to read
     * @return Count of unread messages
     */
    size_t available() const { 
        return ring_.available(); 
    }

private:
    SharedMemory shm_;   ///< Shared memory region
    RingBuffer ring_;    ///< Ring buffer interface
};
