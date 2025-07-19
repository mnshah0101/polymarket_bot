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
        if (!std::getenv("POLY_TIMESTAMP")) {
            std::cout << "Warning: POLY_TIMESTAMP not set. Using test configuration." << std::endl;
        }
        if (!std::getenv("POLY_API_KEY")) {
            std::cout << "Warning: POLY_API_KEY not set. Using test configuration." << std::endl;
        }
        if (!std::getenv("POLY_PASSPHRASE")) {
            std::cout << "Warning: POLY_PASSPHRASE not set. Using test configuration." << std::endl;
        }
        if (!std::getenv("BANKROLL")) {
            std::cout << "Warning: BANKROLL not set. Using default bankroll of $10,000." << std::endl;
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

        // Test Polymarket API
        std::cout << "\n=== Testing Polymarket API ===" << std::endl;
        try {
            auto gammaMarkets = polyClient.getGammaMarkets(1, 20);
            std::cout << "Retrieved " << gammaMarkets.markets.size() << " gamma markets" << std::endl;
            
            // Print first few markets with their slugs
            for (size_t i = 0; i < std::min(gammaMarkets.markets.size(), size_t(10)); ++i) {
                const auto& market = gammaMarkets.markets[i];
                std::cout << "Market " << (i+1) << ":" << std::endl;
                if (market.slug) {
                    std::cout << "  Slug: " << *market.slug << std::endl;
                }
                if (market.question) {
                    std::cout << "  Question: " << *market.question << std::endl;
                }
                if (market.endDateIso) {
                    std::cout << "  End Date: " << *market.endDateIso << std::endl;
                }
                std::cout << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "Error fetching Polymarket data: " << e.what() << std::endl;
        }

        // Test MarketMatcher with new slug-based approach
        std::cout << "\n=== Testing MarketMatcher (Slug-Based) ===" << std::endl;
        
        MarketMatcher matcher(polyClient, client, configManager);
        std::cout << "MarketMatcher created successfully" << std::endl;
        
        // Load odds data only
        std::cout << "Loading odds data..." << std::endl;
        matcher.loadAll();
        
        // Debug: Show what games we actually got from the odds API
        std::cout << "\n=== Debug: Odds API Results ===" << std::endl;
        auto oddsGames = matcher.getOddsGames(); // We'll need to add this method
        std::cout << "Total games from odds API: " << oddsGames.size() << std::endl;
        
        for (size_t i = 0; i < std::min(oddsGames.size(), size_t(5)); ++i) {
            const auto& game = oddsGames[i];
            std::cout << "Game " << (i+1) << ":" << std::endl;
            std::cout << "  Sport Key: '" << game.sport_key << "'" << std::endl;
            std::cout << "  Away Team: '" << game.away_team << "'" << std::endl;
            std::cout << "  Home Team: '" << game.home_team << "'" << std::endl;
            std::cout << "  Commence Time: " << game.commence_time << std::endl;
            std::cout << std::endl;
        }
        
        // Test slug-based matching (fetches Polymarket markets individually)
        std::cout << "\n=== Testing Slug-Based Matching ===" << std::endl;
        std::cout << "This will generate slugs for each game and fetch corresponding Polymarket markets..." << std::endl;
        auto matchedMarkets = matcher.testMatchMarketsBySlug();
        std::cout << "Found " << matchedMarkets.size() << " matched markets using slug-based matching" << std::endl;
        
        // Test slug generation with sample data
        std::cout << "\n=== Testing Slug Generation ===" << std::endl;
        std::cout << "Demonstrating slug generation with sample data:" << std::endl;
        
        // Create sample games to test slug generation
        std::vector<polymarket_bot::common::RawOddsGame> sampleGames = {
            {"game1", "basketball_nba", "2025-01-15T19:30:00Z", "Phoenix Suns", "Sacramento Kings", {}},
            {"game2", "icehockey_nhl", "2025-01-16T20:00:00Z", "Boston Bruins", "Toronto Maple Leafs", {}},
            {"game3", "baseball_mlb", "2025-04-15T19:05:00Z", "New York Yankees", "Boston Red Sox", {}}
        };
        
        for (const auto& game : sampleGames) {
            std::string slug = matcher.testGenerateSlugForGame(game);
            std::cout << "Game: " << game.away_team << " vs " << game.home_team << std::endl;
            std::cout << "Generated slug: " << slug << std::endl;
            std::cout << "Would call: https://gamma-api.polymarket.com/markets?slug=" << slug << std::endl;
            std::cout << std::endl;
        }
        
        // Display matches
        for (size_t i = 0; i < std::min(matchedMarkets.size(), size_t(5)); ++i) {
            const auto& match = matchedMarkets[i];
            std::cout << "Match " << (i+1) << ":" << std::endl;
            std::cout << "  Polymarket ID: " << match.first << std::endl;
            std::cout << "  Odds ID: " << match.second << std::endl;
            std::cout << std::endl;
        }
        
        // Test arbitrage finder with all opportunities
        std::cout << "\n=== Testing Arbitrage Finder ===" << std::endl;
        std::cout << "Analyzing all matched markets for arbitrage opportunities..." << std::endl;
        auto arbitrageOpportunities = matcher.findArbitrageOpportunities(0.0); // 0% minimum edge to show all
        std::cout << "Found " << arbitrageOpportunities.size() << " arbitrage opportunities (all edges)" << std::endl;
        
        // Display all opportunities sorted by edge
        std::sort(arbitrageOpportunities.begin(), arbitrageOpportunities.end(), 
                  [](const ArbitrageOpportunity& a, const ArbitrageOpportunity& b) {
                      return a.edge > b.edge;
                  });
        
        for (size_t i = 0; i < arbitrageOpportunities.size(); ++i) {
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
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 