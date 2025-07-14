#include "../src/api/polymarket_api_client.h"
#include "../src/config/config_manager.h"
#include <iostream>
#include <string>
#include <ctime>

using namespace polymarket_bot::api;
using namespace polymarket_bot::common;
using namespace polymarket_bot::config;

int main() {
    try {
        // Load configuration
        ConfigManager configManager;
        if (!configManager.loadConfig("config/config.json")) {
            std::cerr << "Failed to load configuration" << std::endl;
            return 1;
        }
        
        // Get API configuration
        auto apiConfig = configManager.getApiConfig();
        
        // Create API client
        PolymarketApiClient client(
            apiConfig.polymarket.baseUrl,
            apiConfig.polymarket.gammaBaseUrl,
            apiConfig.polymarket.dataBaseUrl,
            apiConfig.polymarket.address,
            apiConfig.polymarket.signature,
            apiConfig.polymarket.timestamp,
            apiConfig.polymarket.apiKey,
            apiConfig.polymarket.passphrase,
            apiConfig.polymarket.chainId
        );
        
        // Example 1: Execute order using PolymarketOpenOrder structure
        std::cout << "=== Example 1: Execute order using PolymarketOpenOrder ===" << std::endl;
        
        PolymarketOpenOrder order;
        order.id = "12345";
        order.maker_address = apiConfig.polymarket.address;
        order.owner = apiConfig.polymarket.apiKey;
        order.asset_id = "0x1234567890abcdef"; // Example token ID
        order.original_size = "1000000"; // 1 USDC in wei
        order.size_matched = "0";
        order.expiration = std::to_string(std::time(nullptr) + 3600); // 1 hour from now
        order.side = "buy";
        order.type = "GTC"; // Good Till Cancelled
        
        PolymarketOrderResponse response1 = client.executeOrder(order);
        
        if (response1.success) {
            std::cout << "Order executed successfully!" << std::endl;
            std::cout << "Order ID: " << response1.orderId << std::endl;
            for (const auto& hash : response1.orderHashes) {
                std::cout << "Transaction hash: " << hash << std::endl;
            }
        } else {
            std::cout << "Order execution failed: " << response1.errorMsg << std::endl;
        }
        
        // Example 2: Create order using individual parameters
        std::cout << "\n=== Example 2: Create order using individual parameters ===" << std::endl;
        
        PolymarketOrderResponse response2 = client.createOrder(
            apiConfig.polymarket.address,           // maker
            apiConfig.polymarket.address,           // signer
            apiConfig.polymarket.apiKey,            // taker
            "0x1234567890abcdef",       // tokenId
            "500000",                   // makerAmount (0.5 USDC)
            "1000000",                  // takerAmount (1 USDC)
            std::to_string(std::time(nullptr) + 1800), // expiration (30 min)
            "67890",                    // nonce
            "0",                        // feeRateBps
            "sell",                     // side
            0,                          // signatureType
            "",                         // signature (empty for this example)
            apiConfig.polymarket.apiKey,           // owner
            "FOK"                       // orderType (Fill or Kill)
        );
        
        if (response2.success) {
            std::cout << "Order created successfully!" << std::endl;
            std::cout << "Order ID: " << response2.orderId << std::endl;
            for (const auto& hash : response2.orderHashes) {
                std::cout << "Transaction hash: " << hash << std::endl;
            }
        } else {
            std::cout << "Order creation failed: " << response2.errorMsg << std::endl;
        }
        
        // Example 3: Check balance before placing orders
        std::cout << "\n=== Example 3: Check balance ===" << std::endl;
        double balance = client.getBalance(apiConfig.polymarket.address);
        std::cout << "Current balance: " << balance << " USDC" << std::endl;
        
        // Example 4: Get current positions
        std::cout << "\n=== Example 4: Get current positions ===" << std::endl;
        auto positions = client.getPositions(apiConfig.polymarket.address, "", 1.0, false, false);
        std::cout << "Number of positions: " << positions.size() << std::endl;
        
        for (const auto& position : positions) {
            std::cout << "Position - Asset: " << position.asset 
                      << ", Size: " << position.size 
                      << ", Avg Price: " << position.avgPrice << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 