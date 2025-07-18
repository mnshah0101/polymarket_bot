#pragma once

#include <string>
#include <vector>
#include <memory>
#include "../trading/trade_manager.h"
#include "../api/polymarket_api_client.h"

namespace polymarket_bot {
namespace cli {

class TradeDashboard {
private:
    std::shared_ptr<trading::TradeManager> tradeManager;
    std::shared_ptr<api::PolymarketApiClient> polyClient;
    
    // Display formatting
    void printHeader(const std::string& title);
    void printSeparator(char ch = '-', int length = 80);
    void printTableHeader(const std::vector<std::string>& headers, const std::vector<int>& widths);
    void printTableRow(const std::vector<std::string>& values, const std::vector<int>& widths);
    std::string formatCurrency(double amount);
    std::string formatPercentage(double percentage);
    std::string formatDate(const std::string& isoDate);
    std::string truncateString(const std::string& str, int maxLength);
    std::string colorText(const std::string& text, const std::string& color);
    
    // Color codes
    static const std::string COLOR_GREEN;
    static const std::string COLOR_RED;
    static const std::string COLOR_YELLOW;
    static const std::string COLOR_BLUE;
    static const std::string COLOR_RESET;
    
    // Dashboard sections
    void showPortfolioSummary();
    void showRecentTrades(int limit = 10);
    void showDailyPerformance(int days = 7);
    void showActivePositions();
    void showTradeStatistics();
    void showArbitrageOpportunities(const std::vector<ArbitrageOpportunity>& opportunities);

public:
    TradeDashboard(std::shared_ptr<trading::TradeManager> tradeManager,
                   std::shared_ptr<api::PolymarketApiClient> polyClient);
    
    // Main dashboard display
    void displayFullDashboard();
    void displaySummaryDashboard();
    
    // Individual sections
    void displayPortfolioOverview();
    void displayTradeHistory(int limit = 20);
    void displayPerformanceMetrics(int days = 30);
    void displayPositions();
    void displayOpportunities(const std::vector<ArbitrageOpportunity>& opportunities);
    
    // Interactive features
    void runInteractiveMode();
    void showTradeDetails(const std::string& tradeId);
    void showMarketDetails(const std::string& marketId);
    
    // Refresh and update
    void refreshData();
    void clearScreen();
    void waitForKeyPress();
    
    // Configuration
    void setColorEnabled(bool enabled);
    void setRefreshInterval(int seconds);
};

} // namespace cli
} // namespace polymarket_bot