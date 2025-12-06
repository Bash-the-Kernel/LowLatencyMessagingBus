#pragma once
#include <cstring>
#include <cstdint>

class Serializer {
public:
    Serializer(void* buffer, size_t size) : buf_(static_cast<char*>(buffer)), 
                                             size_(size), pos_(0) {}
    
    template<typename T>
    bool write(const T& value) {
        if (pos_ + sizeof(T) > size_) return false;
        std::memcpy(buf_ + pos_, &value, sizeof(T));
        pos_ += sizeof(T);
        return true;
    }
    
    bool write_bytes(const void* data, size_t len) {
        if (pos_ + len > size_) return false;
        std::memcpy(buf_ + pos_, data, len);
        pos_ += len;
        return true;
    }
    
    size_t position() const { return pos_; }
    void reset() { pos_ = 0; }

private:
    char* buf_;
    size_t size_;
    size_t pos_;
};

class Deserializer {
public:
    Deserializer(const void* buffer, size_t size) : buf_(static_cast<const char*>(buffer)),
                                                     size_(size), pos_(0) {}
    
    template<typename T>
    bool read(T& value) {
        if (pos_ + sizeof(T) > size_) return false;
        std::memcpy(&value, buf_ + pos_, sizeof(T));
        pos_ += sizeof(T);
        return true;
    }
    
    bool read_bytes(void* data, size_t len) {
        if (pos_ + len > size_) return false;
        std::memcpy(data, buf_ + pos_, len);
        pos_ += len;
        return true;
    }
    
    size_t position() const { return pos_; }

private:
    const char* buf_;
    size_t size_;
    size_t pos_;
};
