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

/**
 * @brief Cross-platform shared memory wrapper
 * 
 * Provides RAII-based management of shared memory regions for inter-process
 * communication. Uses Windows file mapping on Windows and POSIX shared memory
 * on Linux/macOS.
 * 
 * Single Responsibility: Manages lifecycle of shared memory regions
 * Open/Closed: Closed for modification, open for extension via inheritance
 */
class SharedMemory {
public:
    /**
     * @brief Construct shared memory region
     * @param name Unique identifier for the shared memory segment
     * @param size Size in bytes of the memory region
     * @param create If true, creates new region; if false, opens existing
     * @throws std::runtime_error if creation/opening fails
     */
    SharedMemory(const std::string& name, size_t size, bool create = false)
        : name_(name), size_(size), ptr_(nullptr) {
#ifdef _WIN32
        initializeWindows(create);
#else
        initializePosix(create);
#endif
    }
    
    /**
     * @brief Destructor - automatically unmaps and closes shared memory
     */
    ~SharedMemory() {
#ifdef _WIN32
        cleanupWindows();
#else
        cleanupPosix();
#endif
    }
    
    // Prevent copying (shared memory should have single owner)
    SharedMemory(const SharedMemory&) = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;
    
    /**
     * @brief Get pointer to shared memory region
     * @return Raw pointer to memory-mapped region
     */
    void* data() { return ptr_; }
    
    /**
     * @brief Get size of shared memory region
     * @return Size in bytes
     */
    size_t size() const { return size_; }
    
    /**
     * @brief Remove shared memory segment (POSIX only)
     * @param name Name of segment to remove
     * @note Windows automatically cleans up on process exit
     */
    static void unlink(const std::string& name) {
#ifndef _WIN32
        shm_unlink(name.c_str());
#endif
    }

private:
    std::string name_;  ///< Unique identifier for shared memory
    size_t size_;       ///< Size of memory region in bytes
    void* ptr_;         ///< Pointer to mapped memory
    
#ifdef _WIN32
    HANDLE handle_;     ///< Windows file mapping handle
    
    /**
     * @brief Initialize shared memory on Windows
     */
    void initializeWindows(bool create) {
        // Create or open file mapping object
        handle_ = CreateFileMappingA(
            INVALID_HANDLE_VALUE,           // Use system paging file
            nullptr,                        // Default security
            PAGE_READWRITE,                 // Read/write access
            0,                              // High-order DWORD of size
            static_cast<DWORD>(size_),      // Low-order DWORD of size
            name_.c_str()                   // Name of mapping object
        );
        
        if (!handle_) {
            throw std::runtime_error("CreateFileMapping failed");
        }
        
        // Map view of file into address space
        ptr_ = MapViewOfFile(
            handle_,                        // Handle to mapping object
            FILE_MAP_ALL_ACCESS,            // Read/write access
            0,                              // High-order offset
            0,                              // Low-order offset
            size_                           // Number of bytes to map
        );
        
        if (!ptr_) {
            CloseHandle(handle_);
            throw std::runtime_error("MapViewOfFile failed");
        }
        
        // Zero-initialize memory if creating new region
        if (create) {
            std::memset(ptr_, 0, size_);
        }
    }
    
    /**
     * @brief Cleanup Windows shared memory resources
     */
    void cleanupWindows() {
        if (ptr_) UnmapViewOfFile(ptr_);
        if (handle_) CloseHandle(handle_);
    }
#else
    int fd_;            ///< POSIX file descriptor
    
    /**
     * @brief Initialize shared memory on POSIX systems
     */
    void initializePosix(bool create) {
        // Open or create shared memory object
        int flags = create ? (O_CREAT | O_RDWR) : O_RDWR;
        fd_ = shm_open(name_.c_str(), flags, 0666);
        
        if (fd_ < 0) {
            throw std::runtime_error("shm_open failed");
        }
        
        // Set size if creating new region
        if (create && ftruncate(fd_, size_) < 0) {
            close(fd_);
            throw std::runtime_error("ftruncate failed");
        }
        
        // Map shared memory into address space
        ptr_ = mmap(
            nullptr,                        // Let kernel choose address
            size_,                          // Size to map
            PROT_READ | PROT_WRITE,         // Read/write access
            MAP_SHARED,                     // Shared mapping
            fd_,                            // File descriptor
            0                               // Offset
        );
        
        if (ptr_ == MAP_FAILED) {
            close(fd_);
            throw std::runtime_error("mmap failed");
        }
        
        // Zero-initialize memory if creating new region
        if (create) {
            std::memset(ptr_, 0, size_);
        }
    }
    
    /**
     * @brief Cleanup POSIX shared memory resources
     */
    void cleanupPosix() {
        if (ptr_ && ptr_ != MAP_FAILED) {
            munmap(ptr_, size_);
        }
        if (fd_ >= 0) {
            close(fd_);
        }
    }
#endif
};
