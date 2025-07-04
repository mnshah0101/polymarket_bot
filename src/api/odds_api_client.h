#pragma once

#include "../config/config_manager.h"
#include "../config/config_types.h"
#include "../common/types.h"
#include <string>
#include <vector>
#include <chrono>

namespace polymarket_bot {
namespace api {

class OddsApiClient {
public:
    OddsApiClient();
    ~OddsApiClient();

    // Main API methods
    std::vector<polymarket_bot::common::RawOddsGame> fetchOdds(const std::vector<std::string> &sports);

    // Configuration methods
    void setRateLimit(int requestsPerMinute);
    bool isHealthy() const;
    
    // For testing purposes
    void setConfigManager(polymarket_bot::config::ConfigManager& manager);

private:
    int rateLimit;
    int rateLimitRemaining;
    int rateLimitResetTime;

    polymarket_bot::config::ConfigManager& configManager;

    // Callback function for libcurl
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);

    // Helper methods
    std::string makeApiRequest(const std::string& sport, const std::string& apiKey, 
                              const std::chrono::system_clock::time_point& from, 
                              const std::chrono::system_clock::time_point& to);
    
    std::vector<polymarket_bot::common::RawOddsGame> parseResponse(const std::string& jsonResponse);
};

} // namespace api
} // namespace polymarket_bot
