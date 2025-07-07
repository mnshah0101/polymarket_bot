#include "polymarket_api_client.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <optional>

using namespace polymarket_bot::api;
using namespace polymarket_bot::common;

// Callback function for CURL
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

PolymarketApiClient::PolymarketApiClient(const std::string& baseUrl, 
                                         const std::string& gammaBaseUrl,
                                         const std::string& address,
                                         const std::string& signature,
                                         const std::string& timestamp,
                                         const std::string& apiKey,
                                         const std::string& passphrase,
                                         int chainId)
    : baseUrl(baseUrl), gammaBaseUrl(gammaBaseUrl), address(address), signature(signature),
      timestamp(timestamp), apiKey(apiKey), passphrase(passphrase), chainId(chainId) {
}

PolymarketApiClient::~PolymarketApiClient() {
}

std::string PolymarketApiClient::makeAuthenticatedRequest(const std::string& endpoint, const std::string& method, const std::string& body) {
    CURL* curl = curl_easy_init();
    std::string response;

    if (curl) {
        std::string url = baseUrl + endpoint;
        
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, ("X-POLYMARKET-ADDRESS: " + address).c_str());
        headers = curl_slist_append(headers, ("X-POLYMARKET-SIGNATURE: " + signature).c_str());
        headers = curl_slist_append(headers, ("X-POLYMARKET-TIMESTAMP: " + timestamp).c_str());
        headers = curl_slist_append(headers, ("X-POLYMARKET-API-KEY: " + apiKey).c_str());
        headers = curl_slist_append(headers, ("X-POLYMARKET-PASSPHRASE: " + passphrase).c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
        
        if (!body.empty()) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        }

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    return response;
}

std::string PolymarketApiClient::makeGammaRequest(const std::string& endpoint, const std::string& method, const std::string& body) {
    CURL* curl = curl_easy_init();
    std::string response;

    if (curl) {
        std::string url = gammaBaseUrl + endpoint;
        
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
        
        if (!body.empty()) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        }

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    return response;
}



polymarket_bot::common::PolymarketOrderResponse PolymarketApiClient::executeOrder(const polymarket_bot::common::PolymarketOpenOrder& order) {
    return polymarket_bot::common::PolymarketOrderResponse();
}

double PolymarketApiClient::getBalance(const std::string& user) {
    return 0.0;
}

std::vector<polymarket_bot::common::PolymarketOpenOrder> PolymarketApiClient::getPositions() {
    return std::vector<polymarket_bot::common::PolymarketOpenOrder>();
}

std::vector<polymarket_bot::common::PolymarketUserActivity> PolymarketApiClient::getUserActivity(const std::string& user, 
                                                                         int limit, 
                                                                         int offset,
                                                                         const std::string& market,
                                                                         const std::string& type,
                                                                         int start,
                                                                         int end,
                                                                         const std::string& side,
                                                                         const std::string& sortBy,
                                                                         const std::string& sortDirection) {
    return std::vector<polymarket_bot::common::PolymarketUserActivity>();
}
std::vector<polymarket_bot::common::PolymarketMarket> PolymarketApiClient::getCurrentMarkets() {
    return std::vector<polymarket_bot::common::PolymarketMarket>();
}

polymarket_bot::common::PolymarketMarket PolymarketApiClient::getMarketInfo(const std::string& marketId) {
   return polymarket_bot::common::PolymarketMarket();
}

// Gamma Markets API methods
polymarket_bot::common::GammaMarketsResponse PolymarketApiClient::getGammaMarkets(int page, int limit) {
    if(limit > 500) {
        limit = 500;
    }
    // Get yesterday's date in ISO format
    auto now = std::chrono::system_clock::now();
    auto yesterday = now - std::chrono::hours(24);
    auto time_t = std::chrono::system_clock::to_time_t(yesterday);
    std::tm* tm = std::gmtime(&time_t);
    
    char date_str[32];
    std::strftime(date_str, sizeof(date_str), "%Y-%m-%dT%H:%M:%SZ", tm);
    
    // Get two weeks from now in ISO format
    auto twoWeeksFromNow = now + std::chrono::hours(24 * 14);
    auto time_t_future = std::chrono::system_clock::to_time_t(twoWeeksFromNow);
    std::tm* tm_future = std::gmtime(&time_t_future);
    
    char date_str_future[32];
    std::strftime(date_str_future, sizeof(date_str_future), "%Y-%m-%dT%H:%M:%SZ", tm_future);

    //lets paginage with offset

    int offset = (page - 1) * limit;
    
    std::string endpoint = "/markets?active=true&closed=false&end_date_min=" + std::string(date_str) + "&start_date_max=" + std::string(date_str_future) + "&offset=" + std::to_string(offset) + "&limit=" + std::to_string(limit) + "&offset=" + std::to_string(offset);
    std::string response = makeGammaRequest(endpoint);
    
    polymarket_bot::common::GammaMarketsResponse result;
    try {
        nlohmann::json j = nlohmann::json::parse(response);
        
        // The response is an array of markets, not an object with a markets field
        if (j.is_array()) {
            result.markets = j.get<std::vector<polymarket_bot::common::GammaMarket>>();
            result.total = result.markets.size();
            result.page = page;
            result.limit = limit;
        } else {
            std::cerr << "Unexpected Gamma markets response structure - expected array" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing Gamma markets response: " << e.what() << std::endl;
    }
    
    return result;
}

polymarket_bot::common::GammaMarket PolymarketApiClient::getGammaMarket(const std::string& marketId) {
    std::string endpoint = "/markets/" + marketId;
    std::string response = makeGammaRequest(endpoint);
    
    polymarket_bot::common::GammaMarket market;
    try {
        nlohmann::json j = nlohmann::json::parse(response);
        market = j.get<polymarket_bot::common::GammaMarket>();
    } catch (const std::exception& e) {
        std::cerr << "Error parsing Gamma market response: " << e.what() << std::endl;
    }
    
    return market;
}