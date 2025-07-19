#include "trade_executor.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>

using namespace polymarket_bot::trading;
using namespace polymarket_bot::api;
using namespace polymarket_bot::config;
using namespace polymarket_bot::common;

TradeExecutor::TradeExecutor(std::shared_ptr<PolymarketApiClient> polyClient,
                             const ConfigManager& configManager)
    : polyClient(polyClient), configManager(configManager) {
    
    // Set default limits from config or reasonable defaults
    maxStakePerTrade = 100.0;  // $100 per trade
    maxDailyStake = 1000.0;    // $1000 per day
    minEdgeThreshold = 0.03;   // 3% minimum edge
    
    // Try to load from config if available
    // Note: These could be added to the config file later
}

std::string TradeExecutor::generateTradeId(const TradeRequest& request) {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    
    std::stringstream ss;
    ss << "trade_" << timestamp << "_" 
       << request.polymarketMarketId.substr(0, 8) << "_" 
       << request.outcome;
    
    return ss.str();
}

bool TradeExecutor::validateTradeRequest(const TradeRequest& request) {
    // Basic validation
    if (request.polymarketMarketId.empty() || request.oddsGameId.empty()) {
        return false;
    }
    
    if (request.stakeAmount <= 0 || request.stakeAmount > maxStakePerTrade) {
        return false;
    }
    
    if (request.edge < minEdgeThreshold) {
        return false;
    }
    
    if (request.polymarketPrice <= 0 || request.oddsPrice <= 0) {
        return false;
    }
    
    return true;
}

bool TradeExecutor::checkDailyLimits(double proposedStake) {
    // This would normally check against the SQL database
    // For now, we'll implement a simple check
    // TODO: Integrate with SQL trade manager to get actual daily totals
    return proposedStake <= 10000000;
}

double TradeExecutor::calculateOptimalStake(double edge, double maxStake) {
    if (edge <= 0.0) {
        return 0.0; // No edge, no bet
    }
    
    // Use same logic as MarketMatcher for consistency
    double kellyFraction;
    if (edge > 0.01) { // True arbitrage (1%+ guaranteed profit)
        kellyFraction = edge;
        kellyFraction = std::min(kellyFraction, 0.10); // Cap at 10%
    } else {
        // Value betting: use fractional Kelly for safety
        kellyFraction = edge * 0.25; // 25% of full Kelly
        kellyFraction = std::min(kellyFraction, 0.02); // Cap at 2%
    }
    
    double optimalStake = maxStake * kellyFraction;
    
    // Ensure we don't exceed per-trade limits
    return std::min(optimalStake, maxStakePerTrade);
}

std::string TradeExecutor::getTokenIdForOutcome(const std::string& marketId, const std::string& outcome) {
    // This would need to be implemented based on Polymarket's market structure
    // For now, return a placeholder that would need to be resolved from market data
    // TODO: Implement proper token ID resolution from market data
    return marketId + "_" + outcome;
}

std::optional<PolymarketOpenOrder> TradeExecutor::createPolymarketOrder(const TradeRequest& request) {
    try {
        PolymarketOpenOrder order;
        
        // Set basic order parameters
        order.id = generateTradeId(request);
        order.asset_id = getTokenIdForOutcome(request.polymarketMarketId, request.outcome);
        order.maker_address = configManager.getPolymarketAddress();
        order.owner = configManager.getPolymarketAddress();
        
        // Determine order side based on recommended action
        if (request.recommendedAction == "BUY_POLYMARKET") {
            order.side = "BUY";
            order.price = std::to_string(request.polymarketPrice);
        } else {
            order.side = "SELL";
            order.price = std::to_string(1.0 - request.polymarketPrice); // Sell the opposite
        }
        
        // Calculate sizes
        order.original_size = std::to_string(request.stakeAmount);
        order.size_matched = "0"; // Initially no size matched
        
        // Set expiration (24 hours from now)
        auto now = std::chrono::system_clock::now();
        auto expiration = now + std::chrono::hours(24);
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(expiration.time_since_epoch()).count();
        order.expiration = std::to_string(timestamp);
        
        order.type = "LIMIT"; // Use limit orders for better control
        
        return order;
        
    } catch (const std::exception& e) {
        std::cerr << "Error creating Polymarket order: " << e.what() << std::endl;
        return std::nullopt;
    }
}

TradeResult TradeExecutor::executeTrade(const TradeRequest& request) {
    TradeResult result;
    result.success = false;
    result.tradeId = generateTradeId(request);
    
    std::cout << "[TradeExecutor] Starting trade execution for ID: " << result.tradeId << std::endl;
    std::cout << "[TradeExecutor] Market: " << request.polymarketMarketId << std::endl;
    std::cout << "[TradeExecutor] Outcome: " << request.outcome << std::endl;
    std::cout << "[TradeExecutor] Stake: $" << request.stakeAmount << std::endl;
    std::cout << "[TradeExecutor] Action: " << request.recommendedAction << std::endl;
    
    try {
        // Validate the trade request
        std::cout << "[TradeExecutor] Validating trade request..." << std::endl;
        if (!validateTradeRequest(request)) {
            std::cout << "[TradeExecutor] ERROR: Trade request validation failed" << std::endl;
            result.errorMessage = "Trade request validation failed";
            result.status = "FAILED";
            return result;
        }
        std::cout << "[TradeExecutor] Trade request validation passed" << std::endl;
        
        // Check daily limits
        std::cout << "[TradeExecutor] Checking daily limits..." << std::endl;
        if (!checkDailyLimits(request.stakeAmount)) {
            std::cout << "[TradeExecutor] ERROR: Daily stake limit exceeded" << std::endl;
            result.errorMessage = "Daily stake limit exceeded";
            result.status = "FAILED";
            return result;
        }
        std::cout << "[TradeExecutor] Daily limits check passed" << std::endl;
        
        // Execute order via Lambda
        std::cout << "[TradeExecutor] Executing order via Lambda..." << std::endl;
        std::string side = (request.recommendedAction == "BUY_POLYMARKET") ? "BUY" : "SELL";
        double price = (side == "BUY") ? request.polymarketPrice : (1.0 - request.polymarketPrice);
        
        auto orderResponse = polyClient->executeLambdaOrder(
            request.polymarketSlug, 
            price, 
            request.stakeAmount, 
            request.outcome, 
            side
        );
        
        if (orderResponse.success) {
            result.success = true;
            result.polymarketOrderId = orderResponse.orderId;
            result.executedStake = request.stakeAmount;
            result.expectedProfit = request.expectedProfit;
            result.status = "EXECUTED";
            
            std::cout << "[TradeExecutor] SUCCESS: Trade executed successfully" << std::endl;
            std::cout << "[TradeExecutor]   Trade ID: " << result.tradeId << std::endl;
            std::cout << "[TradeExecutor]   Polymarket Order ID: " << result.polymarketOrderId << std::endl;
            std::cout << "[TradeExecutor]   Stake: $" << result.executedStake << std::endl;
            std::cout << "[TradeExecutor]   Expected Profit: $" << result.expectedProfit << std::endl;
            
        } else {
            std::cout << "[TradeExecutor] ERROR: Polymarket order execution failed" << std::endl;
            std::cout << "[TradeExecutor] Error message: " << orderResponse.errorMsg << std::endl;
            result.errorMessage = "Polymarket order execution failed: " + orderResponse.errorMsg;
            result.status = "FAILED";
        }
        
    } catch (const std::exception& e) {
        std::cout << "[TradeExecutor] EXCEPTION: " << e.what() << std::endl;
        result.errorMessage = "Exception during trade execution: " + std::string(e.what());
        result.status = "FAILED";
    }
    
    std::cout << "[TradeExecutor] Trade execution completed with status: " << result.status << std::endl;
    return result;
}

TradeResult TradeExecutor::executeArbitrageOpportunity(const ArbitrageOpportunity& opportunity) {
    // Convert ArbitrageOpportunity to TradeRequest
    TradeRequest request;
    request.polymarketMarketId = opportunity.polymarketId;
    request.polymarketSlug = opportunity.polymarketSlug;
    request.oddsGameId = opportunity.oddsId;
    request.outcome = opportunity.outcome;
    request.polymarketPrice = opportunity.polymarketPrice;
    request.oddsPrice = opportunity.oddsPrice;
    request.edge = opportunity.edge;
    request.recommendedAction = opportunity.recommendedAction;
    request.stakeAmount = opportunity.recommendedStake;
    request.expectedProfit = opportunity.recommendedStake * opportunity.edge;
    
    return executeTrade(request);
}

std::vector<TradeResult> TradeExecutor::executeMultipleTrades(const std::vector<TradeRequest>& requests) {
    std::vector<TradeResult> results;
    results.reserve(requests.size());
    
    double totalStakeForBatch = 0;
    for (const auto& request : requests) {
        totalStakeForBatch += request.stakeAmount;
    }
    
    // Check if total batch stake exceeds daily limit
    if (!checkDailyLimits(totalStakeForBatch)) {
        // Return failed results for all trades
        for (const auto& request : requests) {
            TradeResult result;
            result.success = false;
            result.tradeId = generateTradeId(request);
            result.errorMessage = "Batch exceeds daily stake limit";
            result.status = "FAILED";
            results.push_back(result);
        }
        return results;
    }
    
    // Execute trades one by one
    for (const auto& request : requests) {
        results.push_back(executeTrade(request));
        
        // Add small delay between trades to respect rate limits
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    return results;
}

std::vector<TradeResult> TradeExecutor::executeArbitrageOpportunities(const std::vector<ArbitrageOpportunity>& opportunities) {
    std::vector<TradeRequest> requests;
    requests.reserve(opportunities.size());
    
    // Convert opportunities to requests
    for (const auto& opp : opportunities) {
        TradeRequest request;
        request.polymarketMarketId = opp.polymarketId;
        request.polymarketSlug = opp.polymarketSlug;
        request.oddsGameId = opp.oddsId;
        request.outcome = opp.outcome;
        request.polymarketPrice = opp.polymarketPrice;
        request.oddsPrice = opp.oddsPrice;
        request.edge = opp.edge;
        request.recommendedAction = opp.recommendedAction;
        request.stakeAmount = opp.recommendedStake;
        request.expectedProfit = opp.recommendedStake * opp.edge;
        
        requests.push_back(request);
    }
    
    return executeMultipleTrades(requests);
}

bool TradeExecutor::isHealthy() const {
    // Check if Polymarket client is available and responsive
    // This could include checking API connectivity, rate limits, etc.
    return polyClient != nullptr;
}

std::string TradeExecutor::getStatus() const {
    std::stringstream status;
    status << "TradeExecutor Status:" << std::endl;
    status << "  Max Stake Per Trade: $" << maxStakePerTrade << std::endl;
    status << "  Max Daily Stake: $" << maxDailyStake << std::endl;
    status << "  Min Edge Threshold: " << (minEdgeThreshold * 100) << "%" << std::endl;
    status << "  Healthy: " << (isHealthy() ? "Yes" : "No") << std::endl;
    
    return status.str();
}
