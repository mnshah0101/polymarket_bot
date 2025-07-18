#include "trade_manager.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <openssl/sha.h>

using namespace polymarket_bot::trading;

TradeManager::TradeManager(const std::string& dbPath, std::unique_ptr<TradeExecutor> executor)
    : dbPath(dbPath), tradeExecutor(std::move(executor)), db(nullptr) {
    
    if (!initializeDatabase()) {
        throw std::runtime_error("Failed to initialize database at: " + dbPath);
    }
}

TradeManager::~TradeManager() {
    if (db) {
        sqlite3_close(db);
    }
}

bool TradeManager::initializeDatabase() {
    // Open database
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    
    // Read and execute schema from file
    std::ifstream schemaFile("sql/schema.sql");
    if (!schemaFile.is_open()) {
        std::cerr << "Cannot open schema file: sql/schema.sql" << std::endl;
        return false;
    }
    
    std::string schema((std::istreambuf_iterator<char>(schemaFile)),
                       std::istreambuf_iterator<char>());
    schemaFile.close();
    
    // Execute schema
    char* errMsg = nullptr;
    rc = sqlite3_exec(db, schema.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    std::cout << "Database initialized successfully at: " << dbPath << std::endl;
    return true;
}

bool TradeManager::executeSQL(const std::string& sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

std::string TradeManager::generateOpportunityHash(const ArbitrageOpportunity& opportunity) {
    // Create hash from key identifying information
    auto now = std::chrono::system_clock::now();
    auto today = std::chrono::floor<std::chrono::days>(now);
    auto dateStr = std::format("{:%F}", today);
    
    std::string input = opportunity.polymarketId + "|" + 
                       opportunity.oddsId + "|" + 
                       opportunity.outcome + "|" + 
                       dateStr;
    
    // Simple hash using std::hash (for production, consider using SHA256)
    std::hash<std::string> hasher;
    size_t hashValue = hasher(input);
    
    std::stringstream ss;
    ss << std::hex << hashValue;
    return ss.str();
}

bool TradeManager::isDuplicateOpportunity(const ArbitrageOpportunity& opportunity) {
    std::string hash = generateOpportunityHash(opportunity);
    
    sqlite3_stmt* stmt;
    const char* sql = "SELECT COUNT(*) FROM trade_opportunities WHERE opportunity_hash = ? AND status IN ('ACTIVE', 'TRADED')";
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(db) << std::endl;
        return true; // Err on the side of caution
    }
    
    sqlite3_bind_text(stmt, 1, hash.c_str(), -1, SQLITE_STATIC);
    
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return count > 0;
}

bool TradeManager::hasRecentTradeForMarket(const std::string& marketId, const std::string& outcome, int hoursWindow) {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT COUNT(*) FROM executed_trades WHERE polymarket_market_id = ? AND outcome = ? AND created_at > datetime('now', '-' || ? || ' hours')";
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(db) << std::endl;
        return true; // Err on the side of caution
    }
    
    sqlite3_bind_text(stmt, 1, marketId.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, outcome.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, hoursWindow);
    
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return count > 0;
}

bool TradeManager::exceedsDailyLimits(double proposedStake) {
    double dailyUsed = getDailyStakeUsed();
    double maxDaily = tradeExecutor->getMaxDailyStake();
    
    return (dailyUsed + proposedStake) > maxDaily;
}

bool TradeManager::canExecuteTrade(const ArbitrageOpportunity& opportunity) {
    // Check if it's a duplicate opportunity
    if (isDuplicateOpportunity(opportunity)) {
        return true; // Allow duplicates for now, handled in executeOpportunity
    }
    
    // Check if we've traded this market/outcome recently
    if (hasRecentTradeForMarket(opportunity.polymarketId, opportunity.outcome, 24)) {
        return false;
    }
    
    // Check daily stake limits
    if (exceedsDailyLimits(opportunity.recommendedStake)) {
        return false;
    }
    
    // Check minimum edge threshold
    if (opportunity.edge < tradeExecutor->getMinEdgeThreshold()) {
        return false;
    }
    
    // Check stake per trade limit
    if (opportunity.recommendedStake > tradeExecutor->getMaxStakePerTrade()) {
        return false;
    }
    
    return true;
}

TradeResult TradeManager::executeOpportunity(const ArbitrageOpportunity& opportunity) {
    TradeResult result;
    result.success = false;
    
    // Check if we can execute this trade
    if (!canExecuteTrade(opportunity)) {
        result.errorMessage = "Trade execution blocked by risk management rules";
        result.status = "BLOCKED";
        return result;
    }
    
    // Mark opportunity as seen
    markOpportunityAsSeen(opportunity);
    
    // Execute the trade
    result = tradeExecutor->executeArbitrageOpportunity(opportunity);
    
    // Record the trade result
    if (result.success) {
        markOpportunityAsTraded(opportunity);
        recordTrade(result, opportunity);
    }
    
    return result;
}

std::vector<TradeResult> TradeManager::executeOpportunities(const std::vector<ArbitrageOpportunity>& opportunities) {
    std::vector<TradeResult> results;
    results.reserve(opportunities.size());
    
    // Filter opportunities we can actually trade
    std::vector<ArbitrageOpportunity> validOpportunities;
    for (const auto& opp : opportunities) {
        if (canExecuteTrade(opp)) {
            std::cout << "[TradeManager] Executing opportunity: " 
                      << opp.polymarketSlug << " - " << opp.outcome 
                      << " (Edge: " << (opp.edge * 100) << "%)" << std::endl;
            validOpportunities.push_back(opp);
        } else {
            std::cout << "[TradeManager] Skipping opportunity due to risk management rules: " 
                      << opp.polymarketSlug << " - " << opp.outcome << std::endl;
            TradeResult blocked;
            blocked.success = false;
            blocked.errorMessage = "Blocked by risk management";
            blocked.status = "BLOCKED";
            results.push_back(blocked);
        }
    }
    
    // Execute valid opportunities
    auto tradeResults = tradeExecutor->executeArbitrageOpportunities(validOpportunities);
    
    // Record results and update opportunity tracking
    for (size_t i = 0; i < validOpportunities.size(); ++i) {
        const auto& opp = validOpportunities[i];
        const auto& result = tradeResults[i];
        
        markOpportunityAsSeen(opp);
        if (result.success) {
            markOpportunityAsTraded(opp);
            recordTrade(result, opp);
        }
        
        results.push_back(result);
    }
    
    return results;
}

bool TradeManager::recordTrade(const TradeResult& result, const ArbitrageOpportunity& opportunity) {
    const char* sql = R"(
        INSERT INTO executed_trades (
            trade_id, polymarket_market_id, polymarket_slug, odds_game_id, outcome,
            polymarket_price, odds_price, edge_percentage, recommended_action,
            stake_amount, expected_profit, polymarket_order_id, polymarket_order_status
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, result.tradeId.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, opportunity.polymarketId.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, opportunity.polymarketSlug.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, opportunity.oddsId.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, opportunity.outcome.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 6, opportunity.polymarketPrice);
    sqlite3_bind_double(stmt, 7, opportunity.oddsPrice);
    sqlite3_bind_double(stmt, 8, opportunity.edge);
    sqlite3_bind_text(stmt, 9, opportunity.recommendedAction.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 10, result.executedStake);
    sqlite3_bind_double(stmt, 11, result.expectedProfit);
    sqlite3_bind_text(stmt, 12, result.polymarketOrderId.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 13, result.status.c_str(), -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

bool TradeManager::markOpportunityAsSeen(const ArbitrageOpportunity& opportunity) {
    std::string hash = generateOpportunityHash(opportunity);
    
    const char* sql = R"(
        INSERT OR REPLACE INTO trade_opportunities (
            opportunity_hash, polymarket_market_id, odds_game_id, outcome,
            first_seen, last_seen, times_seen, status
        ) VALUES (
            ?, ?, ?, ?,
            COALESCE((SELECT first_seen FROM trade_opportunities WHERE opportunity_hash = ?), CURRENT_TIMESTAMP),
            CURRENT_TIMESTAMP,
            COALESCE((SELECT times_seen FROM trade_opportunities WHERE opportunity_hash = ?), 0) + 1,
            'ACTIVE'
        )
    )";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, opportunity.polymarketId.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, opportunity.oddsId.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, opportunity.outcome.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, hash.c_str(), -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

bool TradeManager::markOpportunityAsTraded(const ArbitrageOpportunity& opportunity) {
    std::string hash = generateOpportunityHash(opportunity);
    
    const char* sql = "UPDATE trade_opportunities SET status = 'TRADED' WHERE opportunity_hash = ?";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, hash.c_str(), -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

double TradeManager::getDailyStakeUsed(const std::string& date) {
    std::string targetDate = date;
    if (targetDate.empty()) {
        auto now = std::chrono::system_clock::now();
        auto today = std::chrono::floor<std::chrono::days>(now);
        targetDate = std::format("{:%F}", today);
    }
    
    const char* sql = "SELECT COALESCE(SUM(stake_amount), 0) FROM executed_trades WHERE DATE(created_at) = ?";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(db) << std::endl;
        return 0.0;
    }
    
    sqlite3_bind_text(stmt, 1, targetDate.c_str(), -1, SQLITE_STATIC);
    
    double total = 0.0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        total = sqlite3_column_double(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return total;
}

std::vector<DailyPerformance> TradeManager::getDailyPerformance(int days) {
    const char* sql = R"(
        SELECT * FROM daily_performance 
        WHERE trade_date >= date('now', '-' || ? || ' days')
        ORDER BY trade_date DESC
    )";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(db) << std::endl;
        return {};
    }
    
    sqlite3_bind_int(stmt, 1, days);
    
    std::vector<DailyPerformance> performance;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        DailyPerformance perf;
        perf.date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        perf.tradesCount = sqlite3_column_int(stmt, 1);
        perf.totalStake = sqlite3_column_double(stmt, 2);
        perf.winningTrades = sqlite3_column_int(stmt, 3);
        perf.totalProfit = sqlite3_column_double(stmt, 4);
        perf.avgEdge = sqlite3_column_double(stmt, 5);
        perf.winRate = sqlite3_column_double(stmt, 6);
        
        performance.push_back(perf);
    }
    
    sqlite3_finalize(stmt);
    return performance;
}

void TradeManager::setMaxStakePerTrade(double maxStake) {
    tradeExecutor->setMaxStakePerTrade(maxStake);
}

void TradeManager::setMaxDailyStake(double maxDaily) {
    tradeExecutor->setMaxDailyStake(maxDaily);
}

void TradeManager::setMinEdgeThreshold(double minEdge) {
    tradeExecutor->setMinEdgeThreshold(minEdge);
}

std::vector<TradeRecord> TradeManager::getTradeHistory(int limit, int offset) {
    std::vector<TradeRecord> trades;
    if (!db) return trades;
    
    sqlite3_stmt* stmt;
    const char* sql = "SELECT polymarket_market_id, outcome, stake_amount, polymarket_price, created_at, actual_profit, status, edge_percentage FROM executed_trades ORDER BY created_at DESC LIMIT ? OFFSET ?";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, limit);
        sqlite3_bind_int(stmt, 2, offset);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            TradeRecord trade;
            trade.polymarketMarketId = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            trade.outcome = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            trade.stakeAmount = sqlite3_column_double(stmt, 2);
            trade.polymarketPrice = sqlite3_column_double(stmt, 3);
            trade.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            trade.actualProfit = sqlite3_column_double(stmt, 5);
            trade.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            trade.edgePercentage = sqlite3_column_double(stmt, 7);
            trades.push_back(trade);
        }
        sqlite3_finalize(stmt);
    }
    
    return trades;
}

std::vector<TradeRecord> TradeManager::getActivetrades() {
    std::vector<TradeRecord> trades;
    if (!db) return trades;
    
    sqlite3_stmt* stmt;
    const char* sql = "SELECT polymarket_market_id, outcome, stake_amount, polymarket_price, created_at, actual_profit, status, edge_percentage FROM executed_trades WHERE status = 'active'";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            TradeRecord trade;
            trade.polymarketMarketId = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            trade.outcome = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            trade.stakeAmount = sqlite3_column_double(stmt, 2);
            trade.polymarketPrice = sqlite3_column_double(stmt, 3);
            trade.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            trade.actualProfit = sqlite3_column_double(stmt, 5);
            trade.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            trade.edgePercentage = sqlite3_column_double(stmt, 7);
            trades.push_back(trade);
        }
        sqlite3_finalize(stmt);
    }
    
    return trades;
}

double TradeManager::getTotalProfit() {
    if (!db) return 0.0;
    
    sqlite3_stmt* stmt;
    const char* sql = "SELECT SUM(profit_loss) FROM executed_trades WHERE status = 'settled'";
    double totalProfit = 0.0;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            totalProfit = sqlite3_column_double(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    
    return totalProfit;
}

double TradeManager::getWinRate(int days) {
    if (!db) return 0.0;
    
    sqlite3_stmt* stmt;
    const char* sql = "SELECT COUNT(CASE WHEN profit_loss > 0 THEN 1 END) * 100.0 / COUNT(*) FROM executed_trades WHERE status = 'settled' AND datetime(timestamp) >= datetime('now', '-' || ? || ' days')";
    double winRate = 0.0;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, days);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            winRate = sqlite3_column_double(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    
    return winRate;
}

double TradeManager::getROI(int days) {
    if (!db) return 0.0;
    
    sqlite3_stmt* stmt;
    const char* sql = "SELECT (SUM(profit_loss) * 100.0 / SUM(stake)) FROM executed_trades WHERE status = 'settled' AND datetime(timestamp) >= datetime('now', '-' || ? || ' days')";
    double roi = 0.0;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, days);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            roi = sqlite3_column_double(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    
    return roi;
}

std::string TradeManager::getDBStatus() {
    std::stringstream status;
    status << "Database Status:" << std::endl;
    status << "  Path: " << dbPath << std::endl;
    status << "  Connected: " << (db ? "Yes" : "No") << std::endl;
    
    if (db) {
        // Get trade counts
        sqlite3_stmt* stmt;
        const char* sql = "SELECT COUNT(*) FROM executed_trades";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                int count = sqlite3_column_int(stmt, 0);
                status << "  Total Trades: " << count << std::endl;
            }
            sqlite3_finalize(stmt);
        }
        
        // Get today's stake usage
        double dailyUsed = getDailyStakeUsed();
        status << "  Today's Stake Used: $" << dailyUsed << std::endl;
    }
    
    return status.str();
}
