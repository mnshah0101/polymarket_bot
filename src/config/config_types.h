#pragma once

#include <string>
#include <vector>
#include <memory>
#include "../common/types.h"

namespace polymarket_bot {
namespace config {

// API Configuration
struct OddsApiConfig {
    std::string baseUrl;
    std::string apiKey;
    int rateLimitPerMinute;
};

struct PolymarketConfig {
    std::string baseUrl;
    std::string gammaBaseUrl;  // Gamma Markets API base URL
    std::string address;
    std::string signature;
    std::string timestamp;
    std::string apiKey;
    std::string passphrase;
    int chainId;
};

struct ApiConfig {
    OddsApiConfig oddsApi;
    PolymarketConfig polymarket;
};

// Database Configuration
struct DatabaseConfig {
    std::string path;
    bool backupEnabled;
    int backupInterval;
};

// Kelly Criterion Configuration
struct KellyConfig {
    double fractionOfKelly;
    double minEdge;
    double maxPositionSize;
};

// Risk Management Configuration
struct RiskConfig {
    double maxDrawdown;
    int maxDailyTrades;
    double maxDailyVolume;
    bool circuitBreakerEnabled;
};

// Market Matching Configuration
struct MatchingConfig {
    double minConfidenceScore;
    int maxTimeDifference;
};

// Synchronization Configuration
struct SyncConfig {
    int positionSyncInterval;
    int accountSyncInterval;
    int priceUpdateInterval;
};

// Main Configuration Structure
struct Config {
    ApiConfig apis;
    DatabaseConfig database;
    std::vector<std::string> sharpBooks;
    std::vector<std::string> sports;
    KellyConfig kelly;
    RiskConfig risk;
    MatchingConfig matching;
    SyncConfig sync;
};

} // namespace config
} // namespace polymarket_bot

