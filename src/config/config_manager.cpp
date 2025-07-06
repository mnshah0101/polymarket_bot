#include "config_manager.h"
#include <fstream>
#include <iostream>
#include <regex>
#include <cstdlib>
#include <algorithm>
#include <sstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace polymarket_bot {
namespace config {

class ConfigManager::Impl {
public:
    Config config;
    std::string lastError;
    std::string configPath;
    std::vector<ValidationCallback> validationCallbacks;
    static ConfigManager* instance;

    Impl() = default;
    ~Impl() = default;

    bool loadConfigFromFile(const std::string& path) {
        try {
            std::ifstream file(path);
            if (!file.is_open()) {
                lastError = "Failed to open config file: " + path;
                return false;
            }

            json j;
            file >> j;
            
            // Parse configuration
            if (!parseConfig(j)) {
                return false;
            }

            configPath = path;
            return true;
        } catch (const std::exception& e) {
            lastError = "Error parsing config file: " + std::string(e.what());
            return false;
        }
    }

    bool parseConfig(const json& j) {
        try {
            // Parse APIs
            if (j.contains("apis")) {
                auto& apis = j["apis"];
                
                if (apis.contains("oddsApi")) {
                    auto& oddsApi = apis["oddsApi"];
                    config.apis.oddsApi.baseUrl = oddsApi["baseUrl"];
                    const char* oddsApiKey = std::getenv("ODDS_API_KEY");
                    config.apis.oddsApi.apiKey = oddsApiKey ? oddsApiKey : "";
                    config.apis.oddsApi.rateLimitPerMinute = oddsApi["rateLimitPerMinute"];
                }
                
                if (apis.contains("polymarket")) {
                    auto& polymarket = apis["polymarket"];
                    config.apis.polymarket.baseUrl = polymarket["baseUrl"];
                    config.apis.polymarket.gammaBaseUrl = polymarket["gammaBaseUrl"];
                    
                    // Read Polymarket headers from environment variables
                    const char* polyAddress = std::getenv("POLY_ADDRESS");
                    config.apis.polymarket.address = polyAddress ? polyAddress : "";
                    
                    const char* polySignature = std::getenv("POLY_SIGNATURE");
                    config.apis.polymarket.signature = polySignature ? polySignature : "";
                    
                    const char* polyTimestamp = std::getenv("POLY_TIMESTAMP");
                    config.apis.polymarket.timestamp = polyTimestamp ? polyTimestamp : "";
                    
                    const char* polyApiKey = std::getenv("POLY_API_KEY");
                    config.apis.polymarket.apiKey = polyApiKey ? polyApiKey : "";
                    
                    const char* polyPassphrase = std::getenv("POLY_PASSPHRASE");
                    config.apis.polymarket.passphrase = polyPassphrase ? polyPassphrase : "";
                    
                    config.apis.polymarket.chainId = polymarket["chainId"];
                }
            }

            // Parse database
            if (j.contains("database")) {
                auto& db = j["database"];
                config.database.path = db["path"];
                config.database.backupEnabled = db["backupEnabled"];
                config.database.backupInterval = db["backupInterval"];
            }

            // Parse sharp books
            if (j.contains("sharpBooks")) {
                config.sharpBooks.clear();
                for (const auto& book : j["sharpBooks"]) {
                    config.sharpBooks.push_back(book);
                }
            }

            // Parse sports
            if (j.contains("sports")) {
                config.sports.clear();
                for (const auto& sport : j["sports"]) {
                    config.sports.push_back(sport);
                }
            }

            // Parse Kelly criterion
            if (j.contains("kelly")) {
                auto& kelly = j["kelly"];
                config.kelly.fractionOfKelly = kelly["fractionOfKelly"];
                config.kelly.minEdge = kelly["minEdge"];
                config.kelly.maxPositionSize = kelly["maxPositionSize"];
            }

            // Parse risk management
            if (j.contains("risk")) {
                auto& risk = j["risk"];
                config.risk.maxDrawdown = risk["maxDrawdown"];
                config.risk.maxDailyTrades = risk["maxDailyTrades"];
                config.risk.maxDailyVolume = risk["maxDailyVolume"];
                config.risk.circuitBreakerEnabled = risk["circuitBreakerEnabled"];
            }

            // Parse matching
            if (j.contains("matching")) {
                auto& matching = j["matching"];
                config.matching.minConfidenceScore = matching["minConfidenceScore"];
                config.matching.maxTimeDifference = matching["maxTimeDifference"];
            }

            // Parse sync
            if (j.contains("sync")) {
                auto& sync = j["sync"];
                config.sync.positionSyncInterval = sync["positionSyncInterval"];
                config.sync.accountSyncInterval = sync["accountSyncInterval"];
                config.sync.priceUpdateInterval = sync["priceUpdateInterval"];
            }

            return true;
        } catch (const std::exception& e) {
            lastError = "Error parsing config: " + std::string(e.what());
            return false;
        }
    }

    bool validateConfig()  {
        // Validate API configuration
        if (config.apis.oddsApi.baseUrl.empty()) {
            lastError = "Odds API base URL is required";
            return false;
        }
        
        if (config.apis.oddsApi.apiKey.empty()) {
            lastError = "Odds API key is required";
            return false;
        }
        
        if (config.apis.oddsApi.rateLimitPerMinute <= 0) {
            lastError = "Odds API rate limit must be positive";
            return false;
        }
        
        if (config.apis.polymarket.baseUrl.empty()) {
            lastError = "Polymarket base URL is required";
            return false;
        }
        
        if (config.apis.polymarket.address.empty()) {
            lastError = "Polymarket address (POLY_ADDRESS) is required";
            return false;
        }
        
        if (config.apis.polymarket.signature.empty()) {
            lastError = "Polymarket signature (POLY_SIGNATURE) is required";
            return false;
        }
        
        if (config.apis.polymarket.timestamp.empty()) {
            lastError = "Polymarket timestamp (POLY_TIMESTAMP) is required";
            return false;
        }
        
        if (config.apis.polymarket.apiKey.empty()) {
            lastError = "Polymarket API key (POLY_API_KEY) is required";
            return false;
        }
        
        if (config.apis.polymarket.passphrase.empty()) {
            lastError = "Polymarket passphrase (POLY_PASSPHRASE) is required";
            return false;
        }
        
        if (config.apis.polymarket.chainId <= 0) {
            lastError = "Polymarket chain ID must be positive";
            return false;
        }

        // Validate database configuration
        if (config.database.path.empty()) {
            lastError = "Database path is required";
            return false;
        }
        
        if (config.database.backupInterval <= 0) {
            lastError = "Database backup interval must be positive";
            return false;
        }

        // Validate Kelly criterion
        if (config.kelly.fractionOfKelly <= 0.0 || config.kelly.fractionOfKelly > 1.0) {
            lastError = "Kelly fraction must be between 0 and 1";
            return false;
        }
        
        if (config.kelly.minEdge < 0.0) {
            lastError = "Minimum edge must be non-negative";
            return false;
        }
        
        if (config.kelly.maxPositionSize <= 0.0 || config.kelly.maxPositionSize > 1.0) {
            lastError = "Maximum position size must be between 0 and 1";
            return false;
        }

        // Validate risk management
        if (config.risk.maxDrawdown <= 0.0 || config.risk.maxDrawdown > 1.0) {
            lastError = "Maximum drawdown must be between 0 and 1";
            return false;
        }
        
        if (config.risk.maxDailyTrades <= 0) {
            lastError = "Maximum daily trades must be positive";
            return false;
        }
        
        if (config.risk.maxDailyVolume <= 0.0) {
            lastError = "Maximum daily volume must be positive";
            return false;
        }

        // Validate matching
        if (config.matching.minConfidenceScore < 0.0 || config.matching.minConfidenceScore > 1.0) {
            lastError = "Minimum confidence score must be between 0 and 1";
            return false;
        }
        
        if (config.matching.maxTimeDifference <= 0) {
            lastError = "Maximum time difference must be positive";
            return false;
        }

        // Validate sync intervals
        if (config.sync.positionSyncInterval <= 0) {
            lastError = "Position sync interval must be positive";
            return false;
        }
        
        if (config.sync.accountSyncInterval <= 0) {
            lastError = "Account sync interval must be positive";
            return false;
        }
        
        if (config.sync.priceUpdateInterval <= 0) {
            lastError = "Price update interval must be positive";
            return false;
        }

        // Validate sharp books
        if (config.sharpBooks.empty()) {
            lastError = "At least one sharp book must be specified";
            return false;
        }

        // Validate sports
        if (config.sports.empty()) {
            lastError = "At least one sport must be specified";
            return false;
        }

        // Run custom validation callbacks
        for (const auto& callback : validationCallbacks) {
            if (!callback(config)) {
                lastError = "Custom validation failed";
                return false;
            }
        }

        return true;
    }
};

// Static member initialization
ConfigManager* ConfigManager::Impl::instance = nullptr;

// ConfigManager implementation
ConfigManager::ConfigManager() : pImpl(std::make_unique<Impl>()) {
    Impl::instance = this;
}

ConfigManager::~ConfigManager() = default;

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadConfig(const std::string& configPath) {
    if (!pImpl->loadConfigFromFile(configPath)) {
        return false;
    }
    
    if (!pImpl->validateConfig()) {
        return false;
    }
    
    return true;
}

bool ConfigManager::validateConfig() const {
    return pImpl->validateConfig();
}

const Config& ConfigManager::getConfig() const {
    return pImpl->config;
}

std::string ConfigManager::getOddsApiKey() const {
    return pImpl->config.apis.oddsApi.apiKey;
}

// Polymarket credentials getters
std::string ConfigManager::getPolymarketAddress() const {
    return pImpl->config.apis.polymarket.address;
}

std::string ConfigManager::getPolymarketSignature() const {
    return pImpl->config.apis.polymarket.signature;
}

std::string ConfigManager::getPolymarketTimestamp() const {
    return pImpl->config.apis.polymarket.timestamp;
}

std::string ConfigManager::getPolymarketApiKey() const {
    return pImpl->config.apis.polymarket.apiKey;
}

std::string ConfigManager::getPolymarketPassphrase() const {
    return pImpl->config.apis.polymarket.passphrase;
}

std::string ConfigManager::getPolymarketBaseUrl() const {
    return pImpl->config.apis.polymarket.baseUrl;
}

std::string ConfigManager::getPolymarketGammaBaseUrl() const {
    return pImpl->config.apis.polymarket.gammaBaseUrl;
}

int ConfigManager::getPolymarketChainId() const {
    return pImpl->config.apis.polymarket.chainId;
}

const std::vector<std::string>& ConfigManager::getSports() const {
    return pImpl->config.sports;
}

bool ConfigManager::hasValidApiCredentials() const {
    return !pImpl->config.apis.oddsApi.apiKey.empty() && 
           !pImpl->config.apis.polymarket.address.empty() &&
           !pImpl->config.apis.polymarket.signature.empty() &&
           !pImpl->config.apis.polymarket.timestamp.empty() &&
           !pImpl->config.apis.polymarket.apiKey.empty() &&
           !pImpl->config.apis.polymarket.passphrase.empty();
}

const std::vector<std::string>& ConfigManager::getSharpBooks() const {
    return pImpl->config.sharpBooks;
}

bool ConfigManager::isSharpBook(const std::string& bookName) const {
    return std::find(pImpl->config.sharpBooks.begin(), 
                    pImpl->config.sharpBooks.end(), 
                    bookName) != pImpl->config.sharpBooks.end();
}

double ConfigManager::getKellyFraction() const {
    return pImpl->config.kelly.fractionOfKelly;
}

double ConfigManager::getMinEdge() const {
    return pImpl->config.kelly.minEdge;
}

double ConfigManager::getMaxPositionSize() const {
    return pImpl->config.kelly.maxPositionSize;
}

double ConfigManager::getMaxDrawdown() const {
    return pImpl->config.risk.maxDrawdown;
}

int ConfigManager::getMaxDailyTrades() const {
    return pImpl->config.risk.maxDailyTrades;
}

double ConfigManager::getMaxDailyVolume() const {
    return pImpl->config.risk.maxDailyVolume;
}

bool ConfigManager::isCircuitBreakerEnabled() const {
    return pImpl->config.risk.circuitBreakerEnabled;
}

double ConfigManager::getMinConfidenceScore() const {
    return pImpl->config.matching.minConfidenceScore;
}

int ConfigManager::getMaxTimeDifference() const {
    return pImpl->config.matching.maxTimeDifference;
}

int ConfigManager::getPositionSyncInterval() const {
    return pImpl->config.sync.positionSyncInterval;
}

int ConfigManager::getAccountSyncInterval() const {
    return pImpl->config.sync.accountSyncInterval;
}

int ConfigManager::getPriceUpdateInterval() const {
    return pImpl->config.sync.priceUpdateInterval;
}

void ConfigManager::addValidationCallback(ValidationCallback callback) {
    pImpl->validationCallbacks.push_back(callback);
}

bool ConfigManager::reloadConfig() {
    if (pImpl->configPath.empty()) {
        pImpl->lastError = "No config file path available for reload";
        return false;
    }
    
    return loadConfig(pImpl->configPath);
}

std::string ConfigManager::getLastError() const {
    return pImpl->lastError;
}

void ConfigManager::clearError() {
    pImpl->lastError.clear();
}

} // namespace config
} // namespace polymarket_bot

