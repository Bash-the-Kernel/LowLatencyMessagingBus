#include "subscriber.hpp"
#include "serializer.hpp"
#include <iostream>
#include <thread>
#include <chrono>

struct TradeData {
    uint64_t order_id;
    double price;
    uint32_t quantity;
    char symbol[16];
};

int main() {
    try {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        Subscriber sub("trading_bus");
        
        std::cout << "Subscriber started. Waiting for messages...\n";
        
        int count = 0;
        while (count < 1000) {
            char buffer[256];
            uint32_t size;
            uint64_t timestamp;
            
            if (sub.receive(buffer, size, timestamp)) {
                Deserializer deser(buffer, size);
                TradeData trade;
                deser.read(trade.order_id);
                deser.read(trade.price);
                deser.read(trade.quantity);
                deser.read_bytes(trade.symbol, sizeof(trade.symbol));
                
                if (count % 100 == 0) {
                    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
                    uint64_t now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
                    uint64_t latency = now_ns - timestamp;
                    
                    std::cout << "Received trade " << count << " - OrderID: " << trade.order_id
                              << " Price: " << trade.price << " Latency: " << latency << " ns\n";
                }
                ++count;
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        }
        
        std::cout << "Received all messages.\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
