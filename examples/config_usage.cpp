#include "../src/config/config_manager.h"
#include <iostream>

int main() {
    // Get the config manager instance
    auto& configManager = polymarket_bot::config::ConfigManager::getInstance();
    
    // Load configuration (environment variables should be set in Docker)
    if (!configManager.loadConfig()) {
        std::cerr << "Failed to load config: " << configManager.getLastError() << std::endl;
        return 1;
    }
    
    // Check if API credentials are valid
    if (!configManager.hasValidApiCredentials()) {
        std::cerr << "Invalid API credentials" << std::endl;
        return 1;
    }
    
    // Print some configuration values
    std::cout << "Configuration loaded successfully!" << std::endl;
    std::cout << "Odds API Base URL: " << configManager.getConfig().apis.oddsApi.baseUrl << std::endl;
    std::cout << "Polymarket Chain ID: " << configManager.getConfig().apis.polymarket.chainId << std::endl;
    std::cout << "Kelly Fraction: " << configManager.getKellyFraction() << std::endl;
    std::cout << "Max Daily Trades: " << configManager.getMaxDailyTrades() << std::endl;
    
    // Check sharp books
    std::cout << "Sharp Books: ";
    for (const auto& book : configManager.getSharpBooks()) {
        std::cout << book << " ";
    }
    std::cout << std::endl;
    
    return 0;
} 