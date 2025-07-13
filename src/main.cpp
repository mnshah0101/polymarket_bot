#include <iostream>
#include <cstdlib>
#include <algorithm>
#include "api/odds_api_client.h"
#include "api/polymarket_api_client.h"
#include "config/config_manager.h"
#include "market/market_matcher.h"
#include <fstream>

int main(int argc, char* argv[]) {
    std::cout << "Polymarket Bot v1.0.0" << std::endl;
    std::cout << "=====================" << std::endl;
    
    try {
        // Environment variables are set by CMake
        if (!std::getenv("ODDS_API_KEY")) {
            std::cout << "Warning: ODDS_API_KEY not set. Using test configuration." << std::endl;
        }
        
        if (!std::getenv("POLY_ADDRESS")) {
            std::cout << "Warning: POLY_ADDRESS not set. Using test configuration." << std::endl;
        }
        if (!std::getenv("POLY_SIGNATURE")) {
            std::cout << "Warning: POLY_SIGNATURE not set. Using test configuration." << std::endl;
        }
        if (!std::getenv("POLY_TIMESTAMP")) {
            std::cout << "Warning: POLY_TIMESTAMP not set. Using test configuration." << std::endl;
        }
        if (!std::getenv("POLY_API_KEY")) {
            std::cout << "Warning: POLY_API_KEY not set. Using test configuration." << std::endl;
        }
        if (!std::getenv("POLY_PASSPHRASE")) {
            std::cout << "Warning: POLY_PASSPHRASE not set. Using test configuration." << std::endl;
        }
        
        // Initialize the config manager
        auto& configManager = polymarket_bot::config::ConfigManager::getInstance();
        
        // Load configuration (try multiple locations)
        std::vector<std::string> configPaths = {
            "config/config_test.json",
            "config/config.json",
            "bin/config/config_test.json",
            "bin/config/config.json"
        };
        
        bool configLoaded = false;
        for (const auto& path : configPaths) {
            if (configManager.loadConfig(path)) {
                std::cout << "Configuration loaded from: " << path << std::endl;
                configLoaded = true;
                break;
            }
        }
        
        if (!configLoaded) {
            std::cerr << "Failed to load configuration from any location" << std::endl;
            std::cerr << "Tried paths:" << std::endl;
            for (const auto& path : configPaths) {
                std::cerr << "  - " << path << std::endl;
            }
            std::cerr << "Last error: " << configManager.getLastError() << std::endl;
            return 1;
        }
        
        // Validate configuration
        if (!configManager.validateConfig()) {
            std::cerr << "Configuration validation failed: " << configManager.getLastError() << std::endl;
            return 1;
        }
        
        std::cout << "Configuration loaded successfully!" << std::endl;
        
        // Create the odds API client
        polymarket_bot::api::OddsApiClient client;
        
        // Check if the client is healthy
        if (!client.isHealthy()) {
            std::cerr << "Odds API client is not healthy" << std::endl;
            return 1;
        }
        
        std::cout << "Odds API client initialized successfully" << std::endl;
        
        // Get sports from config
        auto sports = configManager.getSports();
        std::cout << "Configured sports: ";
        for (const auto& sport : sports) {
            std::cout << sport << " ";
        }
        std::cout << std::endl;
        
        // Set rate limit
        client.setRateLimit(10); // 10 requests per minute
        
        std::cout << "Bot is ready to fetch odds data" << std::endl;

        // Create the Polymarket API client
        polymarket_bot::api::PolymarketApiClient polyClient(
            configManager.getPolymarketBaseUrl(),
            configManager.getPolymarketGammaBaseUrl(),
            configManager.getPolymarketDataBaseUrl(),
            configManager.getPolymarketAddress(),
            configManager.getPolymarketSignature(),
            configManager.getPolymarketTimestamp(),
            configManager.getPolymarketApiKey(),
            configManager.getPolymarketPassphrase(),
            configManager.getPolymarketChainId()
        );

        std::cout << "Polymarket API client initialized successfully" << std::endl;

        double balance = polyClient.getBalance(std::getenv("POLYMARKET_ADDRESS"));
        std::cout << "Balance: " << balance << std::endl;

        // Test getPositions functionality
        std::cout << "\n=== Testing getPositions ===" << std::endl;
        const char* userAddress = std::getenv("POLYMARKET_ADDRESS");
        if (userAddress) {
            try {
                auto positions = polyClient.getPositions(userAddress, "", 1.0, false, false, "", "", 10, 0, "TOKENS", "DESC");
                std::cout << "Retrieved " << positions.size() << " positions" << std::endl;
                
                for (size_t i = 0; i < std::min(positions.size(), size_t(3)); ++i) {
                    const auto& pos = positions[i];
                    std::cout << "Position " << (i+1) << ": " << pos.title 
                              << " (Size: " << pos.size << ", PnL: " << pos.cashPnl << ")" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error getting positions: " << e.what() << std::endl;
            }
        } else {
            std::cout << "POLYMARKET_ADDRESS not set, skipping positions test" << std::endl;
        }

         // Test MarketMatcher
        std::cout << "\n=== Testing MarketMatcher ===" << std::endl;
        
        MarketMatcher matcher(polyClient, client, configManager);
        std::cout << "MarketMatcher created successfully" << std::endl;
        
        // Load all data
        matcher.loadAll();
        
     
        // Test arbitrage finder
        std::cout << "\n=== Testing Arbitrage Finder ===" << std::endl;
        auto arbitrageOpportunities = matcher.findArbitrageOpportunities(0.03); // 3% minimum edge
        std::cout << "Found " << arbitrageOpportunities.size() << " arbitrage opportunities with >= 3% edge" << std::endl;
        
        // Display top opportunities
        for (size_t i = 0; i < std::min(arbitrageOpportunities.size(), size_t(5)); ++i) {
            const auto& opp = arbitrageOpportunities[i];
            std::cout << "\nOpportunity " << (i+1) << ":" << std::endl;
            std::cout << "  Market: " << opp.polymarketSlug << std::endl;
            std::cout << "  Game: " << opp.oddsGame << std::endl;
            std::cout << "  Outcome: " << opp.outcome << std::endl;
            std::cout << "  Polymarket Price: " << opp.polymarketPrice << std::endl;
            std::cout << "  Odds Price: " << opp.oddsPrice << std::endl;
            std::cout << "  Edge: " << (opp.edge * 100) << "%" << std::endl;
            std::cout << "  Recommended Action: " << opp.recommendedAction << std::endl;
            std::cout << "  Recommended Stake: $" << opp.recommendedStake << std::endl;
        }
        
        // For now, just demonstrate that everything is working
        // In a real implementation, you would start the main loop here

        // Get balance
        
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 