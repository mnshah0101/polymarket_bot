#include <iostream>
#include <vector>
#include <string>
#include "src/api/odds_api_client.h"
#include "src/config/config_manager.h"

int main() {
    std::cout << "Testing Odds API Client..." << std::endl;
    
    try {
        // Initialize config manager
        auto& configManager = polymarket_bot::config::ConfigManager::getInstance();
        
        // Load test config
        if (!configManager.loadConfig("config/config_test.json")) {
            std::cerr << "Failed to load config: " << configManager.getLastError() << std::endl;
            return 1;
        }
        
        std::cout << "Config loaded successfully" << std::endl;
        
        // Create odds client
        polymarket_bot::api::OddsApiClient client;
        
        // Get sports from config
        auto sports = configManager.getSports();
        std::cout << "Configured sports: ";
        for (const auto& sport : sports) {
            std::cout << sport << " ";
        }
        std::cout << std::endl;
        
        // Try to fetch odds (this will likely fail with test credentials, but we can see the error)
        try {
            auto oddsGames = client.fetchOdds(sports);
            std::cout << "Fetched " << oddsGames.size() << " games" << std::endl;
            
            // Print details of first few games
            for (size_t i = 0; i < std::min(oddsGames.size(), size_t(5)); ++i) {
                const auto& game = oddsGames[i];
                std::cout << "Game " << (i+1) << ":" << std::endl;
                std::cout << "  Sport Key: " << game.sport_key << std::endl;
                std::cout << "  Away Team: " << game.away_team << std::endl;
                std::cout << "  Home Team: " << game.home_team << std::endl;
                std::cout << "  Commence Time: " << game.commence_time << std::endl;
                std::cout << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "Error fetching odds: " << e.what() << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 