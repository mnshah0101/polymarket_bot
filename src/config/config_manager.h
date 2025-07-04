#pragma once

#include "config_types.h"
#include <string>
#include <memory>
#include <optional>
#include <functional>

namespace polymarket_bot {
namespace config {

class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();

    // Singleton pattern
    static ConfigManager& getInstance();

    // Load and validate configuration
    bool loadConfig(const std::string& configPath = "config/config.json");
    bool validateConfig() const;
    
    // Get configuration
    const Config& getConfig() const;
    
    // API credentials management
    std::string getOddsApiKey() const;
    std::string getPolymarketPrivateKey() const;
    bool hasValidApiCredentials() const;
    
    // Sharp sportsbook preferences
    const std::vector<std::string>& getSharpBooks() const;
    bool isSharpBook(const std::string& bookName) const;
    
    // Trading parameters
    double getKellyFraction() const;
    double getMinEdge() const;
    double getMaxPositionSize() const;

    // Sports
    const std::vector<std::string>& getSports() const;


    
    // Risk limits
    double getMaxDrawdown() const;
    int getMaxDailyTrades() const;
    double getMaxDailyVolume() const;
    bool isCircuitBreakerEnabled() const;
    
    // Market matching parameters
    double getMinConfidenceScore() const;
    int getMaxTimeDifference() const;
    
    // Sync intervals
    int getPositionSyncInterval() const;
    int getAccountSyncInterval() const;
    int getPriceUpdateInterval() const;
    
    // Configuration validation callbacks
    using ValidationCallback = std::function<bool(const Config&)>;
    void addValidationCallback(ValidationCallback callback);
    
    // Reload configuration
    bool reloadConfig();
    
    // Error handling
    std::string getLastError() const;
    void clearError();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
    
    // Disable copy and assignment
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
};

} // namespace config
} // namespace polymarket_bot

