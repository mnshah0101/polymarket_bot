#include "../src/config/config_manager.h"
#include <iostream>

int main() {
    std::cout << "=== Updated Config Manager Example ===\n\n";
    
    // Get the config manager instance
    auto& configManager = polymarket_bot::config::ConfigManager::getInstance();
    
    // Load configuration from file
    // Environment variables will be substituted from Docker environment
    if (configManager.loadConfig("config/config.json")) {
        std::cout << "✓ Successfully loaded configuration\n";
    } else {
        std::cout << "✗ Failed to load configuration: " << configManager.getLastError() << "\n";
        return 1;
    }
    
    // Validate configuration
    if (configManager.validateConfig()) {
        std::cout << "✓ Configuration is valid\n";
    } else {
        std::cout << "✗ Configuration validation failed: " << configManager.getLastError() << "\n";
        return 1;
    }
    
    // Access configuration values
    std::cout << "\nConfiguration Values:\n";
    std::cout << "-------------------\n";
    
    // API credentials (substituted from environment variables)
    std::cout << "Odds API Key: " << (configManager.getOddsApiKey().empty() ? "Not set" : "Set") << "\n";
    std::cout << "Polymarket Address: " << (configManager.getPolymarketAddress().empty() ? "Not set" : "Set") << "\n";
    std::cout << "Polymarket Signature: " << (configManager.getPolymarketSignature().empty() ? "Not set" : "Set") << "\n";
    std::cout << "Polymarket Timestamp: " << (configManager.getPolymarketTimestamp().empty() ? "Not set" : "Set") << "\n";
    std::cout << "Polymarket API Key: " << (configManager.getPolymarketApiKey().empty() ? "Not set" : "Set") << "\n";
    std::cout << "Polymarket Passphrase: " << (configManager.getPolymarketPassphrase().empty() ? "Not set" : "Set") << "\n";
    std::cout << "Valid API Credentials: " << (configManager.hasValidApiCredentials() ? "Yes" : "No") << "\n";
    
    // Trading parameters
    std::cout << "Kelly Fraction: " << configManager.getKellyFraction() << "\n";
    std::cout << "Min Edge: " << configManager.getMinEdge() << "\n";
    std::cout << "Max Position Size: " << configManager.getMaxPositionSize() << "\n";
    
    // Risk management
    std::cout << "Max Drawdown: " << configManager.getMaxDrawdown() << "\n";
    std::cout << "Max Daily Trades: " << configManager.getMaxDailyTrades() << "\n";
    std::cout << "Max Daily Volume: " << configManager.getMaxDailyVolume() << "\n";
    std::cout << "Circuit Breaker Enabled: " << (configManager.isCircuitBreakerEnabled() ? "Yes" : "No") << "\n";
    
    // Market matching
    std::cout << "Min Confidence Score: " << configManager.getMinConfidenceScore() << "\n";
    std::cout << "Max Time Difference: " << configManager.getMaxTimeDifference() << "\n";
    
    // Sync intervals
    std::cout << "Position Sync Interval: " << configManager.getPositionSyncInterval() << " seconds\n";
    std::cout << "Account Sync Interval: " << configManager.getAccountSyncInterval() << " seconds\n";
    std::cout << "Price Update Interval: " << configManager.getPriceUpdateInterval() << " seconds\n";
    
    // Sharp books
    std::cout << "\nSharp Books:\n";
    for (const auto& book : configManager.getSharpBooks()) {
        std::cout << "  - " << book << "\n";
    }
    
    std::cout << "\n=== Example completed ===\n";
    return 0;
} 