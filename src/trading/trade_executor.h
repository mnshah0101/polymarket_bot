#pragma once

#include <vector>
#include <string>
#include <memory>
#include <optional>
#include "../api/polymarket_api_client.h"
#include "../config/config_manager.h"
#include "../market/market_matcher.h"

namespace polymarket_bot {
namespace trading {

struct TradeResult {
    bool success;
    std::string errorMessage;
    std::string tradeId;
    std::string polymarketOrderId;
    double executedStake;
    double expectedProfit;
    std::string status; // PENDING, EXECUTED, FAILED
};

struct TradeRequest {
    std::string polymarketMarketId;
    std::string polymarketSlug;
    std::string oddsGameId;
    std::string outcome;
    double polymarketPrice;
    double oddsPrice;
    double edge;
    std::string recommendedAction;
    double stakeAmount;
    double expectedProfit;
};

class TradeExecutor {
private:
    std::shared_ptr<api::PolymarketApiClient> polyClient;
    const config::ConfigManager& configManager;
    double maxStakePerTrade;
    double maxDailyStake;
    double minEdgeThreshold;
    
    // Helper methods
    std::string generateTradeId(const TradeRequest& request);
    bool validateTradeRequest(const TradeRequest& request);
    bool checkDailyLimits(double proposedStake);
    std::optional<common::PolymarketOpenOrder> createPolymarketOrder(const TradeRequest& request);
    double calculateOptimalStake(double edge, double maxStake);
    std::string getTokenIdForOutcome(const std::string& marketId, const std::string& outcome);

public:
    TradeExecutor(std::shared_ptr<api::PolymarketApiClient> polyClient,
                  const config::ConfigManager& configManager);
    
    // Main trading methods
    TradeResult executeTrade(const TradeRequest& request);
    TradeResult executeArbitrageOpportunity(const ArbitrageOpportunity& opportunity);
    
    // Batch processing
    std::vector<TradeResult> executeMultipleTrades(const std::vector<TradeRequest>& requests);
    std::vector<TradeResult> executeArbitrageOpportunities(const std::vector<ArbitrageOpportunity>& opportunities);
    
    // Configuration and limits
    void setMaxStakePerTrade(double maxStake) { maxStakePerTrade = maxStake; }
    void setMaxDailyStake(double maxDaily) { maxDailyStake = maxDaily; }
    void setMinEdgeThreshold(double minEdge) { minEdgeThreshold = minEdge; }
    
    double getMaxStakePerTrade() const { return maxStakePerTrade; }
    double getMaxDailyStake() const { return maxDailyStake; }
    double getMinEdgeThreshold() const { return minEdgeThreshold; }
    
    // Status and monitoring
    bool isHealthy() const;
    std::string getStatus() const;
};

} // namespace trading
} // namespace polymarket_bot