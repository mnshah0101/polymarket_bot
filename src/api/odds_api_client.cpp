#include "odds_api_client.h"
#include <iostream> 
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <curl/curl.h>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;

namespace polymarket_bot {
namespace api {

OddsApiClient::OddsApiClient() 
    : rateLimit(10), rateLimitRemaining(10), rateLimitResetTime(10),
      configManager(polymarket_bot::config::ConfigManager::getInstance()) {
}

OddsApiClient::~OddsApiClient() {
}

void OddsApiClient::setConfigManager(polymarket_bot::config::ConfigManager& manager) {
    // Note: This is a reference, so we can't actually change it
    // In a real implementation, you might want to use a pointer or different approach
    (void)manager; // Suppress unused parameter warning
}

// Callback function for libcurl
size_t OddsApiClient::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string OddsApiClient::makeApiRequest(const std::string& sport, const std::string& apiKey, 
                                         const std::chrono::system_clock::time_point& from, 
                                         const std::chrono::system_clock::time_point& to) {
    CURL* curl = curl_easy_init();
    std::string response;
    
    if (curl) {
        // Convert time points to ISO 8601 strings
        auto from_time_t = std::chrono::system_clock::to_time_t(from);
        auto to_time_t = std::chrono::system_clock::to_time_t(to);
        
        std::stringstream ss;
        ss << "https://api.the-odds-api.com/v4/sports/" << sport << "/odds"
           << "?apiKey=" << apiKey
           << "&regions=us,uk"
           << "&commenceTimeFrom=" << std::put_time(std::gmtime(&from_time_t), "%Y-%m-%dT%H:%M:%SZ")
           << "&commenceTimeTo=" << std::put_time(std::gmtime(&to_time_t), "%Y-%m-%dT%H:%M:%SZ");
        
        curl_easy_setopt(curl, CURLOPT_URL, ss.str().c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "polymarket-bot/1.0");
        
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) {
            return "";
        }
    }
    
    return response;
}

std::vector<polymarket_bot::common::RawOddsGame> OddsApiClient::parseResponse(const std::string& jsonResponse) {
    try {
        std::cout << "Parsing response: " << jsonResponse << std::endl;
        json j = json::parse(jsonResponse);

        std::vector<polymarket_bot::common::RawOddsGame> games;
        for (const auto& game : j) {
            games.push_back(game.get<polymarket_bot::common::RawOddsGame>());
        }
        return games;
    } catch (const std::exception& e) {
        return {};
    }
}

std::vector<polymarket_bot::common::RawOddsGame> OddsApiClient::fetchOdds(const std::vector<std::string> &sports)
{
    // lets check if the rate limit is exceeded
    if (rateLimitRemaining <= 0) {
        // lets wait for the rate limit to reset
        std::this_thread::sleep_for(std::chrono::seconds(rateLimitResetTime));
        // lets reset the rate limit
        rateLimitRemaining = rateLimit;
    }
    // lets decrement the rate limit
    rateLimitRemaining--;

    // lets load the .env for the odds api key from the config manager
    auto oddsApiKey = configManager.getOddsApiKey();
    // get todays date and the next week date - save as commenceTimeFrom and commenceTimeTo, ISO 8601 format
    auto commenceTimeFrom = std::chrono::system_clock::now();
    auto commenceTimeTo = commenceTimeFrom + std::chrono::hours(7 * 24);

    // this should return a vector of RawOddsData
    // lets make the api request - this is what it looks like: https://api.the-odds-api.com/v4/sports/?apiKey=YOUR_API_KEY
    // we need to make a request for each sport

    std::vector<polymarket_bot::common::RawOddsGame> odds;

    // Use the passed sports parameter instead of configManager.getSports()
    for (const auto &sport : sports) {
        std::cout << "Fetching odds for sport: " << sport << std::endl;
        // make the api request
        auto response = makeApiRequest(sport, oddsApiKey, commenceTimeFrom, commenceTimeTo);
        // parse the response
        auto parsedResponse = parseResponse(response);
        std::cout << "Parsed response: " << parsedResponse.size() << std::endl;
        // add the parsed response to the odds vector
        odds.insert(odds.end(), parsedResponse.begin(), parsedResponse.end());
    }

    return odds;
}

void OddsApiClient::setRateLimit(int requestsPerMinute) {
    //lets set the rate limit
    rateLimit = requestsPerMinute;
    rateLimitRemaining = requestsPerMinute;
}

bool OddsApiClient::isHealthy() const {
    //lets return a boolean
    return true;
}

} // namespace api
} // namespace polymarket_bot


