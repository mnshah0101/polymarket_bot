#include <iostream>
#include <cstdlib>
#include "api/odds_api_client.h"
#include "api/polymarket_api_client.h"
#include "config/config_manager.h"

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
            configManager.getPolymarketAddress(),
            configManager.getPolymarketSignature(),
            configManager.getPolymarketTimestamp(),
            configManager.getPolymarketApiKey(),
            configManager.getPolymarketPassphrase(),
            configManager.getPolymarketChainId()
        );

        std::cout << "Polymarket API client initialized successfully" << std::endl;

        // Test Gamma Markets API
        std::cout << "Testing Gamma Markets API..." << std::endl;
        auto gammaMarkets = polyClient.getGammaMarkets(1, 5);
        std::cout << "Found " << gammaMarkets.markets.size() << " Gamma markets" << std::endl;
        
        for (const auto& market : gammaMarkets.markets) {
            std::cout << "Gamma Market ID: " << market.id << ", Slug: " << market.slug << std::endl;
        }

        // Test Polymarket CLOB API
        std::cout << "Testing Polymarket CLOB API..." << std::endl;
        auto polyMarkets = polyClient.getCurrentMarkets();
        std::cout << "Found " << polyMarkets.size() << " Polymarket markets" << std::endl;

        auto odds = client.fetchOdds(sports);

        std::cout << "Odds: " << odds.size() << std::endl;

        for (const auto& odd : odds) {
            std::cout << "Odd: " << odd.id << std::endl;
        }
        
        // For now, just demonstrate that everything is working
        // In a real implementation, you would start the main loop here
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 