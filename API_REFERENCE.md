# API Reference

## Publisher

### Constructor
```cpp
Publisher(const std::string& bus_name, bool create = false)
```
- `bus_name`: Unique identifier for the message bus
- `create`: If true, creates new shared memory; if false, opens existing

**Example**:
```cpp
Publisher pub("my_bus", true);  // Create new bus
Publisher pub2("my_bus", false); // Connect to existing
```

### publish
```cpp
bool publish(const void* data, uint32_t size)
```
Non-blocking message publish.

**Parameters**:
- `data`: Pointer to message data
- `size`: Size in bytes (max 252)

**Returns**: `true` if published, `false` if buffer full

**Example**:
```cpp
uint64_t order_id = 12345;
if (pub.publish(&order_id, sizeof(order_id))) {
    // Success
}
```

### publish_with_backpressure
```cpp
bool publish_with_backpressure(const void* data, uint32_t size, int max_retries = 100)
```
Blocking publish with retry logic.

**Parameters**:
- `data`: Pointer to message data
- `size`: Size in bytes (max 252)
- `max_retries`: Maximum retry attempts (default 100)

**Returns**: `true` if published, `false` if all retries exhausted

**Example**:
```cpp
pub.publish_with_backpressure(&order_id, sizeof(order_id), 1000);
```

### free_space
```cpp
size_t free_space() const
```
**Returns**: Number of available slots in ring buffer

---

## Subscriber

### Constructor
```cpp
Subscriber(const std::string& bus_name)
```
- `bus_name`: Name of existing message bus

**Example**:
```cpp
Subscriber sub("my_bus");
```

### receive
```cpp
bool receive(void* data, uint32_t& size, uint64_t& timestamp)
```
Non-blocking message receive.

**Parameters**:
- `data`: Buffer to receive message (min 252 bytes)
- `size`: [out] Actual message size received
- `timestamp`: [out] Publisher timestamp (nanoseconds)

**Returns**: `true` if message received, `false` if buffer empty

**Example**:
```cpp
char buffer[256];
uint32_t size;
uint64_t timestamp;

if (sub.receive(buffer, size, timestamp)) {
    // Process message
}
```

### poll
```cpp
void poll(MessageHandler handler)
```
Process all available messages with callback.

**Parameters**:
- `handler`: Callback function `void(const void* data, uint32_t size, uint64_t timestamp)`

**Example**:
```cpp
sub.poll([](const void* data, uint32_t size, uint64_t ts) {
    std::cout << "Received " << size << " bytes\n";
});
```

### available
```cpp
size_t available() const
```
**Returns**: Number of messages waiting to be read

---

## Serializer

### Constructor
```cpp
Serializer(void* buffer, size_t size)
```
- `buffer`: Target buffer for serialization
- `size`: Buffer size

### write
```cpp
template<typename T>
bool write(const T& value)
```
Write POD type to buffer.

**Returns**: `true` if successful, `false` if buffer full

**Example**:
```cpp
char buffer[256];
Serializer ser(buffer, sizeof(buffer));

ser.write<uint64_t>(12345);
ser.write<double>(99.99);
ser.write<uint32_t>(100);
```

### write_bytes
```cpp
bool write_bytes(const void* data, size_t len)
```
Write raw bytes to buffer.

**Example**:
```cpp
const char* symbol = "AAPL";
ser.write_bytes(symbol, 4);
```

### position
```cpp
size_t position() const
```
**Returns**: Current write position (total bytes written)

### reset
```cpp
void reset()
```
Reset write position to beginning.

---

## Deserializer

### Constructor
```cpp
Deserializer(const void* buffer, size_t size)
```
- `buffer`: Source buffer for deserialization
- `size`: Buffer size

### read
```cpp
template<typename T>
bool read(T& value)
```
Read POD type from buffer.

**Returns**: `true` if successful, `false` if buffer exhausted

**Example**:
```cpp
Deserializer deser(buffer, size);

uint64_t id;
double price;
uint32_t qty;

deser.read(id);
deser.read(price);
deser.read(qty);
```

### read_bytes
```cpp
bool read_bytes(void* data, size_t len)
```
Read raw bytes from buffer.

**Example**:
```cpp
char symbol[16];
deser.read_bytes(symbol, 16);
```

### position
```cpp
size_t position() const
```
**Returns**: Current read position

---

## Message Structure

```cpp
struct Message {
    uint64_t timestamp;  // Nanoseconds since epoch
    uint32_t size;       // Payload size (0-252)
    char data[252];      // Payload data
};
```

Total size: 264 bytes (cache-friendly)

---

## RingBuffer (Advanced)

### Constants
```cpp
static constexpr size_t CAPACITY = 1024;
static constexpr size_t BUFFER_SIZE = sizeof(Message) * CAPACITY + sizeof(Metadata);
```

### try_write
```cpp
bool try_write(const Message& msg)
```
Low-level write operation.

### try_read
```cpp
bool try_read(Message& msg)
```
Low-level read operation.

---

## SharedMemory (Advanced)

### Constructor
```cpp
SharedMemory(const std::string& name, size_t size, bool create = false)
```

### data
```cpp
void* data()
```
**Returns**: Pointer to shared memory region

### size
```cpp
size_t size() const
```
**Returns**: Size of shared memory region

### unlink (static)
```cpp
static void unlink(const std::string& name)
```
Remove shared memory (Linux/macOS only).

**Example**:
```cpp
SharedMemory::unlink("my_bus");
```

---

## Complete Example

```cpp
#include "publisher.hpp"
#include "subscriber.hpp"
#include "serializer.hpp"
#include <iostream>

struct Order {
    uint64_t id;
    double price;
    uint32_t quantity;
};

int main() {
    // Publisher
    Publisher pub("orders", true);
    
    Order order{12345, 150.75, 100};
    
    char buffer[256];
    Serializer ser(buffer, sizeof(buffer));
    ser.write(order.id);
    ser.write(order.price);
    ser.write(order.quantity);
    
    pub.publish(buffer, ser.position());
    
    // Subscriber
    Subscriber sub("orders");
    
    char recv_buffer[256];
    uint32_t size;
    uint64_t timestamp;
    
    if (sub.receive(recv_buffer, size, timestamp)) {
        Deserializer deser(recv_buffer, size);
        
        Order recv_order;
        deser.read(recv_order.id);
        deser.read(recv_order.price);
        deser.read(recv_order.quantity);
        
        std::cout << "Order ID: " << recv_order.id << "\n";
        std::cout << "Price: " << recv_order.price << "\n";
        std::cout << "Quantity: " << recv_order.quantity << "\n";
    }
    
    return 0;
}
```

---

## Error Handling

All constructors throw `std::runtime_error` on failure:
- Shared memory creation/opening fails
- Memory mapping fails
- Invalid parameters

**Example**:
```cpp
try {
    Publisher pub("my_bus", true);
} catch (const std::runtime_error& e) {
    std::cerr << "Error: " << e.what() << "\n";
}
```

---

## Thread Safety

- **Publisher**: Thread-safe for single writer
- **Subscriber**: Thread-safe for single reader
- **Multiple publishers**: Supported but may have contention
- **Multiple subscribers**: Each sees all messages (broadcast)

---

## Performance Tips

1. **Reuse buffers**: Avoid allocation in hot path
2. **Batch operations**: Use `poll()` for multiple messages
3. **CPU pinning**: Pin threads to specific cores
4. **Disable frequency scaling**: Lock CPU frequency
5. **Isolate cores**: Use `isolcpus` kernel parameter
6. **Huge pages**: Enable for shared memory
7. **NUMA awareness**: Allocate on same NUMA node
