#pragma once
#include <string>
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#endif

class SharedMemory {
public:
    SharedMemory(const std::string& name, size_t size, bool create = false)
        : name_(name), size_(size), ptr_(nullptr) {
#ifdef _WIN32
        DWORD access = create ? PAGE_READWRITE : PAGE_READWRITE;
        DWORD flags = create ? CREATE_ALWAYS : OPEN_EXISTING;
        
        handle_ = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr,
                                      access, 0, static_cast<DWORD>(size), name.c_str());
        if (!handle_) throw std::runtime_error("CreateFileMapping failed");
        
        ptr_ = MapViewOfFile(handle_, FILE_MAP_ALL_ACCESS, 0, 0, size);
        if (!ptr_) {
            CloseHandle(handle_);
            throw std::runtime_error("MapViewOfFile failed");
        }
        
        if (create) std::memset(ptr_, 0, size);
#else
        int flags = create ? (O_CREAT | O_RDWR) : O_RDWR;
        fd_ = shm_open(name.c_str(), flags, 0666);
        if (fd_ < 0) throw std::runtime_error("shm_open failed");
        
        if (create && ftruncate(fd_, size) < 0) {
            close(fd_);
            throw std::runtime_error("ftruncate failed");
        }
        
        ptr_ = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        if (ptr_ == MAP_FAILED) {
            close(fd_);
            throw std::runtime_error("mmap failed");
        }
        
        if (create) std::memset(ptr_, 0, size);
#endif
    }
    
    ~SharedMemory() {
#ifdef _WIN32
        if (ptr_) UnmapViewOfFile(ptr_);
        if (handle_) CloseHandle(handle_);
#else
        if (ptr_) munmap(ptr_, size_);
        if (fd_ >= 0) close(fd_);
#endif
    }
    
    void* data() { return ptr_; }
    size_t size() const { return size_; }
    
    static void unlink(const std::string& name) {
#ifndef _WIN32
        shm_unlink(name.c_str());
#endif
    }

private:
    std::string name_;
    size_t size_;
    void* ptr_;
#ifdef _WIN32
    HANDLE handle_;
#else
    int fd_;
#endif
};
