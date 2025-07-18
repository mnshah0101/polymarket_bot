#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <sqlite3.h>
#include "trade_executor.h"
#include "../market/market_matcher.h"

namespace polymarket_bot {
namespace trading {

struct TradeRecord {
    int id;
    std::string tradeId;
    std::string polymarketMarketId;
    std::string polymarketSlug;
    std::string oddsGameId;
    std::string outcome;
    double polymarketPrice;
    double oddsPrice;
    double edgePercentage;
    std::string recommendedAction;
    double stakeAmount;
    double expectedProfit;
    std::string polymarketOrderId;
    std::string status;
    std::string createdAt;
    std::string executedAt;
    double actualProfit;
};

struct DailyPerformance {
    std::string date;
    int tradesCount;
    double totalStake;
    int winningTrades;
    double totalProfit;
    double avgEdge;
    double winRate;
};

class TradeManager {
private:
    sqlite3* db;
    std::string dbPath;
    std::unique_ptr<TradeExecutor> tradeExecutor;
    
    // Database operations
    bool initializeDatabase();
    bool executeSQL(const std::string& sql);
    std::string generateOpportunityHash(const ArbitrageOpportunity& opportunity);
    
    // Trade validation
    bool isDuplicateOpportunity(const ArbitrageOpportunity& opportunity);
    bool hasRecentTradeForMarket(const std::string& marketId, const std::string& outcome, int hoursWindow = 24);
    bool exceedsDailyLimits(double proposedStake);

public:
    TradeManager(const std::string& dbPath, std::unique_ptr<TradeExecutor> executor);
    ~TradeManager();
    
    // Core trading functions
    bool canExecuteTrade(const ArbitrageOpportunity& opportunity);
    TradeResult executeOpportunity(const ArbitrageOpportunity& opportunity);
    std::vector<TradeResult> executeOpportunities(const std::vector<ArbitrageOpportunity>& opportunities);
    
    // Trade recording and tracking
    bool recordTrade(const TradeResult& result, const ArbitrageOpportunity& opportunity);
    bool updateTradeStatus(const std::string& tradeId, const std::string& status);
    bool updateTradeResult(const std::string& tradeId, double actualProfit, const std::string& finalResult);
    
    // Query functions
    std::vector<TradeRecord> getTradeHistory(int limit = 100, int offset = 0);
    std::vector<TradeRecord> getActivetrades();
    std::vector<DailyPerformance> getDailyPerformance(int days = 30);
    double getTotalProfit();
    double getDailyStakeUsed(const std::string& date = "");
    int getTradeCount(const std::string& date = "");
    
    // Duplicate prevention
    bool markOpportunityAsSeen(const ArbitrageOpportunity& opportunity);
    bool markOpportunityAsTraded(const ArbitrageOpportunity& opportunity);
    std::vector<std::string> getRecentOpportunityHashes(int hours = 24);
    
    // Performance metrics
    double getWinRate(int days = 30);
    double getROI(int days = 30);
    double getAverageEdge(int days = 30);
    
    // Database maintenance
    bool cleanupOldRecords(int daysToKeep = 90);
    bool vacuum();
    std::string getDBStatus();
    
    // Configuration
    void setMaxStakePerTrade(double maxStake);
    void setMaxDailyStake(double maxDaily);
    void setMinEdgeThreshold(double minEdge);
};

} // namespace trading
} // namespace polymarket_bot