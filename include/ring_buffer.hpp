#pragma once
#include <atomic>
#include <cstring>
#include <cstdint>

/**
 * @brief Fixed-size message structure for zero-copy communication
 * 
 * Total size: 264 bytes (cache-line friendly)
 * - 8 bytes: timestamp (nanoseconds since epoch)
 * - 4 bytes: payload size
 * - 252 bytes: payload data
 */
struct Message {
    uint64_t timestamp;  ///< Message creation time in nanoseconds
    uint32_t size;       ///< Actual payload size (0-252 bytes)
    char data[252];      ///< Payload data
};

/**
 * @brief Lock-free ring buffer for inter-process communication
 * 
 * Implements a Single Producer Single Consumer (SPSC) ring buffer using
 * atomic operations for thread-safe, wait-free communication.
 * 
 * Design Principles:
 * - Single Responsibility: Manages circular buffer of messages
 * - Open/Closed: Closed for modification, open for extension
 * - Dependency Inversion: Depends on Message abstraction
 * 
 * Memory Layout:
 * [Metadata: 16 bytes][Message Array: 1024 × 264 bytes]
 * Total: ~270 KB
 */
class RingBuffer {
public:
    /**
     * @brief Metadata stored at beginning of shared memory
     * 
     * Uses atomic operations with memory ordering for lock-free synchronization:
     * - write_pos: Producer's write index (monotonically increasing)
     * - read_pos: Consumer's read index (monotonically increasing)
     */
    struct Metadata {
        std::atomic<uint64_t> write_pos;     ///< Next write position
        std::atomic<uint64_t> read_pos;      ///< Next read position
        uint32_t num_publishers;             ///< Number of registered publishers
        uint32_t num_subscribers;            ///< Number of registered subscribers
    };
    
    static constexpr size_t CAPACITY = 1024;  ///< Maximum messages in buffer
    static constexpr size_t BUFFER_SIZE = sizeof(Message) * CAPACITY + sizeof(Metadata);
    
    /**
     * @brief Construct ring buffer from existing shared memory
     * @param mem Pointer to shared memory region (must be BUFFER_SIZE bytes)
     */
    explicit RingBuffer(void* mem) 
        : meta_(static_cast<Metadata*>(mem)),
          buffer_(static_cast<char*>(mem) + sizeof(Metadata)) {}
    
    /**
     * @brief Initialize ring buffer metadata (call once on creation)
     */
    void init() {
        meta_->write_pos.store(0, std::memory_order_relaxed);
        meta_->read_pos.store(0, std::memory_order_relaxed);
        meta_->num_publishers = 0;
        meta_->num_subscribers = 0;
    }
    
    /**
     * @brief Attempt to write message to buffer (non-blocking)
     * 
     * @param msg Message to write
     * @return true if written successfully, false if buffer full
     * 
     * Memory Ordering:
     * - Load read_pos with acquire: ensures we see consumer's updates
     * - Store write_pos with release: ensures consumer sees our data
     */
    bool try_write(const Message& msg) {
        // Get current write position (relaxed: local read only)
        uint64_t write = meta_->write_pos.load(std::memory_order_relaxed);
        
        // Get current read position (acquire: sync with consumer)
        uint64_t read = meta_->read_pos.load(std::memory_order_acquire);
        
        // Check if buffer is full
        if (write - read >= CAPACITY) {
            return false;  // Buffer full, cannot write
        }
        
        // Calculate slot index (wrap around using modulo)
        uint64_t idx = write % CAPACITY;
        
        // Copy message to buffer slot (zero-copy from caller's perspective)
        std::memcpy(buffer_ + idx * sizeof(Message), &msg, sizeof(Message));
        
        // Publish write (release: make data visible to consumer)
        meta_->write_pos.store(write + 1, std::memory_order_release);
        
        return true;
    }
    
    /**
     * @brief Attempt to read message from buffer (non-blocking)
     * 
     * @param msg Output parameter to receive message
     * @return true if read successfully, false if buffer empty
     * 
     * Memory Ordering:
     * - Load write_pos with acquire: ensures we see producer's data
     * - Store read_pos with release: ensures producer sees our progress
     */
    bool try_read(Message& msg) {
        // Get current read position (relaxed: local read only)
        uint64_t read = meta_->read_pos.load(std::memory_order_relaxed);
        
        // Get current write position (acquire: sync with producer)
        uint64_t write = meta_->write_pos.load(std::memory_order_acquire);
        
        // Check if buffer is empty
        if (read >= write) {
            return false;  // Buffer empty, nothing to read
        }
        
        // Calculate slot index (wrap around using modulo)
        uint64_t idx = read % CAPACITY;
        
        // Copy message from buffer slot
        std::memcpy(&msg, buffer_ + idx * sizeof(Message), sizeof(Message));
        
        // Publish read (release: make progress visible to producer)
        meta_->read_pos.store(read + 1, std::memory_order_release);
        
        return true;
    }
    
    /**
     * @brief Get number of messages available to read
     * @return Count of unread messages
     */
    size_t available() const {
        return meta_->write_pos.load(std::memory_order_acquire) - 
               meta_->read_pos.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Get number of free slots in buffer
     * @return Count of available write slots
     */
    size_t free_space() const {
        return CAPACITY - available();
    }
    
    /**
     * @brief Register a publisher (for monitoring)
     */
    void register_publisher() { 
        ++meta_->num_publishers; 
    }
    
    /**
     * @brief Register a subscriber (for monitoring)
     */
    void register_subscriber() { 
        ++meta_->num_subscribers; 
    }

private:
    Metadata* meta_;    ///< Pointer to metadata at start of shared memory
    char* buffer_;      ///< Pointer to message array after metadata
};
