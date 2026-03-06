#pragma once
#include <cstring>
#include <cstdint>

/**
 * @brief Type-safe message serializer
 * 
 * Provides efficient serialization of POD types and raw bytes into a buffer.
 * Uses memcpy for zero-copy performance.
 * 
 * Design Principles:
 * - Single Responsibility: Handles serialization only
 * - Open/Closed: Extensible via template specialization
 * 
 * Usage:
 * @code
 * char buffer[256];
 * Serializer ser(buffer, sizeof(buffer));
 * ser.write<uint64_t>(12345);
 * ser.write<double>(99.99);
 * ser.write_bytes("AAPL", 4);
 * @endcode
 */
class Serializer {
public:
    /**
     * @brief Construct serializer for target buffer
     * @param buffer Target buffer for serialized data
     * @param size Size of buffer in bytes
     */
    Serializer(void* buffer, size_t size) 
        : buf_(static_cast<char*>(buffer)), 
          size_(size), 
          pos_(0) {}
    
    /**
     * @brief Write POD type to buffer
     * 
     * @tparam T POD type (e.g., int, double, struct)
     * @param value Value to serialize
     * @return true if written, false if insufficient space
     * 
     * Note: Only use with POD types. For complex types, serialize members individually.
     */
    template<typename T>
    bool write(const T& value) {
        // Check if enough space available
        if (pos_ + sizeof(T) > size_) {
            return false;  // Buffer overflow
        }
        
        // Copy value to buffer (zero-copy from caller's perspective)
        std::memcpy(buf_ + pos_, &value, sizeof(T));
        pos_ += sizeof(T);
        
        return true;
    }
    
    /**
     * @brief Write raw bytes to buffer
     * 
     * @param data Pointer to source data
     * @param len Number of bytes to write
     * @return true if written, false if insufficient space
     * 
     * Use for strings, arrays, or any raw binary data.
     */
    bool write_bytes(const void* data, size_t len) {
        // Check if enough space available
        if (pos_ + len > size_) {
            return false;  // Buffer overflow
        }
        
        // Copy bytes to buffer
        std::memcpy(buf_ + pos_, data, len);
        pos_ += len;
        
        return true;
    }
    
    /**
     * @brief Get current write position (total bytes written)
     * @return Number of bytes written to buffer
     */
    size_t position() const { 
        return pos_; 
    }
    
    /**
     * @brief Reset write position to beginning
     * 
     * Allows reusing serializer for multiple messages.
     */
    void reset() { 
        pos_ = 0; 
    }

private:
    char* buf_;      ///< Target buffer
    size_t size_;    ///< Buffer size
    size_t pos_;     ///< Current write position
};

/**
 * @brief Type-safe message deserializer
 * 
 * Provides efficient deserialization of POD types and raw bytes from a buffer.
 * Uses memcpy for zero-copy performance.
 * 
 * Design Principles:
 * - Single Responsibility: Handles deserialization only
 * - Open/Closed: Extensible via template specialization
 * 
 * Usage:
 * @code
 * Deserializer deser(buffer, size);
 * uint64_t id;
 * double price;
 * deser.read(id);
 * deser.read(price);
 * @endcode
 */
class Deserializer {
public:
    /**
     * @brief Construct deserializer for source buffer
     * @param buffer Source buffer containing serialized data
     * @param size Size of buffer in bytes
     */
    Deserializer(const void* buffer, size_t size) 
        : buf_(static_cast<const char*>(buffer)),
          size_(size), 
          pos_(0) {}
    
    /**
     * @brief Read POD type from buffer
     * 
     * @tparam T POD type (e.g., int, double, struct)
     * @param value Output parameter to receive deserialized value
     * @return true if read, false if insufficient data
     * 
     * Note: Only use with POD types. For complex types, deserialize members individually.
     */
    template<typename T>
    bool read(T& value) {
        // Check if enough data available
        if (pos_ + sizeof(T) > size_) {
            return false;  // Buffer underflow
        }
        
        // Copy value from buffer
        std::memcpy(&value, buf_ + pos_, sizeof(T));
        pos_ += sizeof(T);
        
        return true;
    }
    
    /**
     * @brief Read raw bytes from buffer
     * 
     * @param data Pointer to destination buffer
     * @param len Number of bytes to read
     * @return true if read, false if insufficient data
     * 
     * Use for strings, arrays, or any raw binary data.
     */
    bool read_bytes(void* data, size_t len) {
        // Check if enough data available
        if (pos_ + len > size_) {
            return false;  // Buffer underflow
        }
        
        // Copy bytes from buffer
        std::memcpy(data, buf_ + pos_, len);
        pos_ += len;
        
        return true;
    }
    
    /**
     * @brief Get current read position (total bytes read)
     * @return Number of bytes read from buffer
     */
    size_t position() const { 
        return pos_; 
    }

private:
    const char* buf_;  ///< Source buffer
    size_t size_;      ///< Buffer size
    size_t pos_;       ///< Current read position
};
