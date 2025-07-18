#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <signal.h>

#include "api/odds_api_client.h"
#include "api/polymarket_api_client.h"
#include "config/config_manager.h"
#include "market/market_matcher.h"
#include "trading/trade_executor.h"
#include "trading/trade_manager.h"
#include "cli/trade_dashboard.h"

// Global flag for graceful shutdown
volatile bool g_running = true;

void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ". Shutting down gracefully..." << std::endl;
    g_running = false;
}

int main(int argc, char* argv[]) {
    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    std::cout << "Polymarket Arbitrage Trading Bot v2.0.0" << std::endl;
    std::cout << "=======================================" << std::endl;
    
    try {
        // Check environment variables
        if (!std::getenv("ODDS_API_KEY") || !std::getenv("POLY_ADDRESS") || 
            !std::getenv("POLY_API_KEY") || !std::getenv("BANKROLL")) {
            std::cerr << "Error: Required environment variables not set." << std::endl;
            std::cerr << "Please set: ODDS_API_KEY, POLY_ADDRESS, POLY_API_KEY, BANKROLL" << std::endl;
            return 1;
        }
        
        // Initialize configuration
        auto& configManager = polymarket_bot::config::ConfigManager::getInstance();
        
        std::vector<std::string> configPaths = {
            "config/config_test.json",
            "config/config.json"
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
            std::cerr << "Failed to load configuration" << std::endl;
            return 1;
        }
        
        if (!configManager.validateConfig()) {
            std::cerr << "Configuration validation failed: " << configManager.getLastError() << std::endl;
            return 1;
        }
        
        // Initialize API clients
        std::cout << "Initializing API clients..." << std::endl;
        
        polymarket_bot::api::OddsApiClient oddsClient;
        if (!oddsClient.isHealthy()) {
            std::cerr << "Odds API client is not healthy" << std::endl;
            return 1;
        }
        oddsClient.setRateLimit(10);
        
        auto polyClient = std::make_shared<polymarket_bot::api::PolymarketApiClient>(
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
        
        std::cout << "API clients initialized successfully" << std::endl;
        
        // Initialize trading components
        std::cout << "Initializing trading system..." << std::endl;
        
        // Create trade executor
        auto tradeExecutor = std::make_unique<polymarket_bot::trading::TradeExecutor>(
            polyClient, configManager
        );
        
        // Set trading limits from environment or config
        double bankroll = std::stod(std::getenv("BANKROLL"));
        tradeExecutor->setMaxStakePerTrade(bankroll * 0.05);  // 5% per trade
        tradeExecutor->setMaxDailyStake(bankroll * 0.20);     // 20% per day
        tradeExecutor->setMinEdgeThreshold(0.03);             // 3% minimum edge
        
        // Create trade manager with database
        auto tradeManager = std::make_shared<polymarket_bot::trading::TradeManager>(
            "data/trades.db", std::move(tradeExecutor)
        );
        
        // Create market matcher
        MarketMatcher matcher(*polyClient, oddsClient, configManager);
        
        // Create dashboard
        auto dashboard = std::make_unique<polymarket_bot::cli::TradeDashboard>(
            tradeManager, polyClient
        );
        
        std::cout << "Trading system initialized successfully" << std::endl;
        std::cout << "Bankroll: $" << bankroll << std::endl;
        std::cout << "Max stake per trade: $" << (bankroll * 0.05) << std::endl;
        std::cout << "Max daily stake: $" << (bankroll * 0.20) << std::endl;
        
        // Parse command line arguments
        bool interactiveMode = false;
        bool dryRun = false;
        int scanInterval = 300; // 5 minutes default
        
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--interactive" || arg == "-i") {
                interactiveMode = true;
            } else if (arg == "--dry-run" || arg == "-d") {
                dryRun = true;
            } else if (arg == "--interval" && i + 1 < argc) {
                scanInterval = std::stoi(argv[++i]);
            } else if (arg == "--help" || arg == "-h") {
                std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
                std::cout << "Options:" << std::endl;
                std::cout << "  -i, --interactive    Run in interactive mode" << std::endl;
                std::cout << "  -d, --dry-run       Scan for opportunities but don't execute trades" << std::endl;
                std::cout << "  --interval SECONDS  Set scan interval (default: 300)" << std::endl;
                std::cout << "  -h, --help          Show this help message" << std::endl;
                return 0;
            }
        }
        
        if (dryRun) {
            std::cout << "Running in DRY RUN mode - no trades will be executed" << std::endl;
        }
        
        if (interactiveMode) {
            std::cout << "Starting interactive dashboard..." << std::endl;
            dashboard->runInteractiveMode();
        } else {
            // Automated trading loop
            std::cout << "Starting automated trading loop..." << std::endl;
            std::cout << "Scan interval: " << scanInterval << " seconds" << std::endl;
            std::cout << "Press Ctrl+C to stop" << std::endl;
            
            int loopCount = 0;
            while (g_running) {
                loopCount++;
                std::cout << "\n--- Scan #" << loopCount << " ---" << std::endl;
                
                try {
                    // Load market data
                    std::cout << "Loading market data..." << std::endl;
                    matcher.loadAll();
                    
                    // Find arbitrage opportunities
                    std::cout << "Scanning for arbitrage opportunities..." << std::endl;
                    auto opportunities = matcher.findArbitrageOpportunities(0.02); // 2% minimum edge
                    
                    std::cout << "Found " << opportunities.size() << " potential opportunities" << std::endl;
                    
                    if (!opportunities.empty()) {
                        // Display opportunities
                        dashboard->displayOpportunities(opportunities);
                        
                        if (!dryRun) {
                            // Execute profitable opportunities
                            std::cout << "Executing trades..." << std::endl;
                            auto results = tradeManager->executeOpportunities(opportunities);
                            
                            int successful = 0;
                            int blocked = 0;
                            int failed = 0;
                            
                            for (const auto& result : results) {
                                if (result.success) {
                                    successful++;
                                    std::cout << "✓ Trade executed: " << result.tradeId 
                                             << " (Stake: $" << result.executedStake << ")" << std::endl;
                                } else if (result.status == "BLOCKED") {
                                    blocked++;
                                } else {
                                    failed++;
                                    std::cout << "✗ Trade failed: " << result.errorMessage << std::endl;
                                }
                            }
                            
                            std::cout << "Execution summary: " << successful << " successful, " 
                                     << blocked << " blocked, " << failed << " failed" << std::endl;
                        } else {
                            std::cout << "DRY RUN: Would have attempted to execute " 
                                     << opportunities.size() << " trades" << std::endl;
                        }
                    }
                    
                    
                } catch (const std::exception& e) {
                    std::cerr << "Error in trading loop: " << e.what() << std::endl;
                }
                
                // Wait for next scan
                for (int i = 0; i < scanInterval && g_running; ++i) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }
        }
        
        std::cout << "Trading bot shutdown complete." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
