/**
 * @file subscriber.cpp
 * @brief Example subscriber demonstrating trading data consumption
 * 
 * This example shows how to:
 * - Connect to an existing message bus
 * - Receive and deserialize messages
 * - Measure end-to-end latency
 * - Handle message processing
 */

#include "subscriber.hpp"
#include "serializer.hpp"
#include <iostream>
#include <thread>
#include <chrono>

/**
 * @brief Trading data structure (must match publisher)
 */
struct TradeData {
    uint64_t order_id;    ///< Unique order identifier
    double price;         ///< Trade price
    uint32_t quantity;    ///< Number of shares/contracts
    char symbol[16];      ///< Trading symbol
};

/**
 * @brief Get current time in nanoseconds
 * @return Timestamp in nanoseconds since epoch
 */
uint64_t now_ns() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

/**
 * @brief Main entry point for subscriber example
 * 
 * Usage:
 *   subscriber.exe    (Connect to existing bus and receive)
 * 
 * Note: Publisher must be started first with --create flag
 */
int main() {
    try {
        // Wait briefly for publisher to create bus
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Connect to message bus
        Subscriber sub("trading_bus");
        
        std::cout << "Subscriber started. Waiting for messages...\n";
        std::cout << "Bus: trading_bus\n\n";
        
        // Statistics tracking
        constexpr size_t EXPECTED_TRADES = 1000;
        size_t received = 0;
        uint64_t total_latency = 0;
        uint64_t min_latency = UINT64_MAX;
        uint64_t max_latency = 0;
        
        // Receive loop
        while (received < EXPECTED_TRADES) {
            char buffer[256];
            uint32_t size;
            uint64_t timestamp;
            
            // Try to receive message (non-blocking)
            if (sub.receive(buffer, size, timestamp)) {
                // Calculate end-to-end latency
                uint64_t now = now_ns();
                uint64_t latency = now - timestamp;
                
                // Update statistics
                total_latency += latency;
                min_latency = std::min(min_latency, latency);
                max_latency = std::max(max_latency, latency);
                
                // Deserialize trade data
                Deserializer deser(buffer, size);
                TradeData trade;
                deser.read(trade.order_id);
                deser.read(trade.price);
                deser.read(trade.quantity);
                deser.read_bytes(trade.symbol, sizeof(trade.symbol));
                
                // Print progress every 100 trades
                if (received % 100 == 0) {
                    std::cout << "Received trade " << received 
                              << " - OrderID: " << trade.order_id
                              << " Price: $" << trade.price 
                              << " Qty: " << trade.quantity
                              << " Latency: " << latency << " ns"
                              << " (" << (latency / 1000.0) << " μs)\n";
                }
                
                ++received;
            } else {
                // No messages available, sleep briefly to avoid busy-wait
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        }
        
        // Print summary statistics
        std::cout << "\n========================================\n";
        std::cout << "Receiving complete\n";
        std::cout << "========================================\n";
        std::cout << "Total trades: " << received << "\n";
        std::cout << "\nLatency Statistics:\n";
        std::cout << "  Average: " << (total_latency / received) << " ns "
                  << "(" << (total_latency / received / 1000.0) << " μs)\n";
        std::cout << "  Min: " << min_latency << " ns "
                  << "(" << (min_latency / 1000.0) << " μs)\n";
        std::cout << "  Max: " << max_latency << " ns "
                  << "(" << (max_latency / 1000.0) << " μs)\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::cerr << "\nTroubleshooting:\n";
        std::cerr << "- Make sure publisher is running with --create flag\n";
        std::cerr << "- Check that bus name matches (trading_bus)\n";
        return 1;
    }
    
    return 0;
}
