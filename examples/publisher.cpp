#include "publisher.hpp"
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

int main(int argc, char** argv) {
    try {
        bool create = (argc > 1 && std::string(argv[1]) == "--create");
        Publisher pub("trading_bus", create);
        
        std::cout << "Publisher started. Publishing trades...\n";
        
        for (uint64_t i = 0; i < 1000; ++i) {
            TradeData trade{i, 100.5 + i * 0.1, 100 + i, "AAPL"};
            
            char buffer[256];
            Serializer ser(buffer, sizeof(buffer));
            ser.write(trade.order_id);
            ser.write(trade.price);
            ser.write(trade.quantity);
            ser.write_bytes(trade.symbol, sizeof(trade.symbol));
            
            if (pub.publish_with_backpressure(buffer, ser.position())) {
                if (i % 100 == 0) {
                    std::cout << "Published trade " << i << " - OrderID: " << trade.order_id 
                              << " Price: " << trade.price << "\n";
                }
            } else {
                std::cerr << "Failed to publish trade " << i << "\n";
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        std::cout << "Publishing complete.\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
