/**
 * @file publisher.cpp
 * @brief Example publisher demonstrating trading data distribution
 * 
 * This example shows how to:
 * - Create a message bus
 * - Serialize complex data structures
 * - Publish messages with backpressure handling
 * - Monitor publishing progress
 */

#include "publisher.hpp"
#include "serializer.hpp"
#include <iostream>
#include <thread>
#include <chrono>

/**
 * @brief Trading data structure
 * 
 * Represents a single trade with order details.
 * Total size: 32 bytes (fits easily in 252-byte message payload)
 */
struct TradeData {
    uint64_t order_id;    ///< Unique order identifier
    double price;         ///< Trade price
    uint32_t quantity;    ///< Number of shares/contracts
    char symbol[16];      ///< Trading symbol (e.g., "AAPL")
};

/**
 * @brief Main entry point for publisher example
 * 
 * Usage:
 *   publisher.exe --create    (Create new bus and publish)
 *   publisher.exe             (Connect to existing bus)
 */
int main(int argc, char** argv) {
    try {
        // Parse command line arguments
        bool create = (argc > 1 && std::string(argv[1]) == "--create");
        
        // Connect to message bus (create if requested)
        Publisher pub("trading_bus", create);
        
        std::cout << "Publisher started. Publishing trades...\n";
        std::cout << "Bus: trading_bus\n";
        std::cout << "Mode: " << (create ? "CREATE" : "CONNECT") << "\n\n";
        
        // Publish 1000 simulated trades
        constexpr size_t NUM_TRADES = 1000;
        size_t published = 0;
        size_t failed = 0;
        
        for (uint64_t i = 0; i < NUM_TRADES; ++i) {
            // Create sample trade data
            TradeData trade{
                i,                      // order_id
                100.5 + i * 0.1,       // price (incrementing)
                100 + static_cast<uint32_t>(i),  // quantity
                "AAPL"                 // symbol
            };
            
            // Serialize trade data into buffer
            char buffer[256];
            Serializer ser(buffer, sizeof(buffer));
            ser.write(trade.order_id);
            ser.write(trade.price);
            ser.write(trade.quantity);
            ser.write_bytes(trade.symbol, sizeof(trade.symbol));
            
            // Publish with backpressure handling (retry up to 100 times)
            if (pub.publish_with_backpressure(buffer, ser.position())) {
                ++published;
                
                // Print progress every 100 trades
                if (i % 100 == 0) {
                    std::cout << "Published trade " << i 
                              << " - OrderID: " << trade.order_id 
                              << " Price: $" << trade.price 
                              << " Qty: " << trade.quantity
                              << "\n";
                }
            } else {
                ++failed;
                std::cerr << "Failed to publish trade " << i 
                          << " (buffer full after retries)\n";
            }
            
            // Simulate realistic publishing rate (~1ms between trades)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        // Print summary
        std::cout << "\n========================================\n";
        std::cout << "Publishing complete\n";
        std::cout << "========================================\n";
        std::cout << "Total trades: " << NUM_TRADES << "\n";
        std::cout << "Published: " << published << "\n";
        std::cout << "Failed: " << failed << "\n";
        std::cout << "Success rate: " 
                  << (100.0 * published / NUM_TRADES) << "%\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::cerr << "\nTroubleshooting:\n";
        std::cerr << "- If 'shm_open failed': Run with --create flag\n";
        std::cerr << "- If 'CreateFileMapping failed': Check permissions\n";
        return 1;
    }
    
    return 0;
}
