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
                                         const std::string& dataBaseUrl,
                                         const std::string& address,
                                         const std::string& signature,
                                         const std::string& timestamp,
                                         const std::string& apiKey,
                                         const std::string& passphrase,
                                         int chainId)
    : baseUrl(baseUrl), gammaBaseUrl(gammaBaseUrl), dataBaseUrl(dataBaseUrl), address(address), signature(signature),
      timestamp(timestamp), apiKey(apiKey), passphrase(passphrase), chainId(chainId) {
}

PolymarketApiClient::~PolymarketApiClient() {
}

std::string PolymarketApiClient::makeAuthenticatedRequest(const std::string& endpoint, const std::string& method, const std::string& body) {
    CURL* curl = curl_easy_init();
    std::string response;

    std::cout << "[PolymarketApiClient] HTTP Request Details:" << std::endl;
    std::cout << "[PolymarketApiClient]   Method: " << method << std::endl;
    std::cout << "[PolymarketApiClient]   Endpoint: " << endpoint << std::endl;
    std::cout << "[PolymarketApiClient]   Base URL: " << baseUrl << std::endl;

    if (curl) {
        std::string url = baseUrl + endpoint;
        std::cout << "[PolymarketApiClient]   Full URL: " << url << std::endl;
        
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36");
        headers = curl_slist_append(headers, ("X-POLYMARKET-ADDRESS: " + address).c_str());
        headers = curl_slist_append(headers, ("X-POLYMARKET-SIGNATURE: " + signature).c_str());
        headers = curl_slist_append(headers, ("X-POLYMARKET-TIMESTAMP: " + timestamp).c_str());
        headers = curl_slist_append(headers, ("X-POLYMARKET-API-KEY: " + apiKey).c_str());
        headers = curl_slist_append(headers, ("X-POLYMARKET-PASSPHRASE: " + passphrase).c_str());
        
        std::cout << "[PolymarketApiClient] API Credentials Being Used:" << std::endl;
        std::cout << "[PolymarketApiClient]   Address: " << address << std::endl;
        std::cout << "[PolymarketApiClient]   API Key: " << apiKey << std::endl;
        std::cout << "[PolymarketApiClient]   Passphrase: " << passphrase << std::endl;
        std::cout << "[PolymarketApiClient]   Timestamp: " << timestamp << std::endl;
        std::cout << "[PolymarketApiClient]   Signature (first 20 chars): " << signature.substr(0, 20) << "..." << std::endl;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
        
        if (!body.empty()) {
            std::cout << "[PolymarketApiClient]   Body length: " << body.length() << " characters" << std::endl;
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        }

        std::cout << "[PolymarketApiClient] Executing HTTP request..." << std::endl;
        CURLcode res = curl_easy_perform(curl);
        
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        std::cout << "[PolymarketApiClient] HTTP Response Code: " << response_code << std::endl;
        
        if (res != CURLE_OK) {
            std::cout << "[PolymarketApiClient] CURL ERROR: " << curl_easy_strerror(res) << std::endl;
        } else {
            std::cout << "[PolymarketApiClient] HTTP request completed successfully" << std::endl;
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
    // Create the order object according to Polymarket CLOB API specification
    nlohmann::json orderObj;
    
    // Generate a random salt for the order (using timestamp + random component)
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    int salt = static_cast<int>(timestamp % 1000000); // Use last 6 digits of timestamp
    
    // Required fields for the order object as per API specification
    orderObj["salt"] = salt;
    orderObj["maker"] = order.maker_address;
    orderObj["signer"] = order.maker_address; // Assuming signer is same as maker
    orderObj["taker"] = order.owner; // Using owner as taker
    orderObj["tokenId"] = order.asset_id;
    orderObj["makerAmount"] = order.original_size;
    orderObj["takerAmount"] = order.size_matched;
    orderObj["expiration"] = order.expiration;
    orderObj["nonce"] = order.id; // Using order ID as nonce
    orderObj["feeRateBps"] = "0"; // Default fee rate, adjust as needed
    orderObj["side"] = order.side;
    orderObj["signatureType"] = 0; // Default signature type
    orderObj["signature"] = ""; // This should be provided by the caller or generated
    
    // Create the request payload
    nlohmann::json requestPayload;
    requestPayload["order"] = orderObj;
    requestPayload["owner"] = apiKey; // Using API key as owner for this examplec
        requestPayload["orderType"] = order.type;

    // Make the POST request to /order endpoint
    std::string endpoint = "/order";
    std::cout << "[PolymarketApiClient] Making POST request to " << endpoint << std::endl;
    std::cout << "[PolymarketApiClient] Request payload: " << requestPayload.dump(2) << std::endl;
    
    std::string response = makeAuthenticatedRequest(endpoint, "POST", requestPayload.dump());
    
    std::cout << "[PolymarketApiClient] Received response length: " << response.length() << " characters" << std::endl;
    std::cout << "[PolymarketApiClient] Response preview (first 200 chars): " << response.substr(0, 200) << std::endl;
    
    // Parse the response
    PolymarketOrderResponse result;
    try {
        nlohmann::json j = nlohmann::json::parse(response);
        
        result.success = j.value("success", false);
        result.errorMsg = j.value("errorMsg", "");
        result.orderId = j.value("orderId", "");
        
        if (j.contains("orderHashes") && j["orderHashes"].is_array()) {
            for (const auto& hash : j["orderHashes"]) {
                result.orderHashes.push_back(hash.get<std::string>());
            }
        }
        
        // Log the response for debugging
        if (!result.success) {
            std::cerr << "Order placement failed: " << result.errorMsg << std::endl;
        } else {
            std::cout << "Order placed successfully. Order ID: " << result.orderId << std::endl;
            if (!result.orderHashes.empty()) {
                std::cout << "Order hashes: ";
                for (const auto& hash : result.orderHashes) {
                    std::cout << hash << " ";
                }
                std::cout << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing order response: " << e.what() << std::endl;
        std::cerr << "Response: " << response << std::endl;
        result.success = false;
        result.errorMsg = "Failed to parse response: " + std::string(e.what());
    }
    
    return result;
}

polymarket_bot::common::PolymarketOrderResponse PolymarketApiClient::createOrder(
    const std::string& maker,
    const std::string& signer,
    const std::string& taker,
    const std::string& tokenId,
    const std::string& makerAmount,
    const std::string& takerAmount,
    const std::string& expiration,
    const std::string& nonce,
    const std::string& feeRateBps,
    const std::string& side,
    int signatureType,
    const std::string& signature,
    const std::string& owner,
    const std::string& orderType) {
    
    // Generate a random salt for the order
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    int salt = static_cast<int>(timestamp % 1000000);
    
    // Create the order object according to Polymarket CLOB API specification
    nlohmann::json orderObj;
    orderObj["salt"] = salt;
    orderObj["maker"] = maker;
    orderObj["signer"] = signer;
    orderObj["taker"] = taker;
    orderObj["tokenId"] = tokenId;
    orderObj["makerAmount"] = makerAmount;
    orderObj["takerAmount"] = takerAmount;
    orderObj["expiration"] = expiration;
    orderObj["nonce"] = nonce;
    orderObj["feeRateBps"] = feeRateBps;
    orderObj["side"] = side;
    orderObj["signatureType"] = signatureType;
    orderObj["signature"] = signature;
    
    // Create the request payload
    nlohmann::json requestPayload;
    requestPayload["order"] = orderObj;
    requestPayload["owner"] = owner;
    requestPayload["orderType"] = orderType;
    
    // Make the POST request to /order endpoint
    std::string endpoint = "/order";
    std::string response = makeAuthenticatedRequest(endpoint, "POST", requestPayload.dump());
    
    // Parse the response
    PolymarketOrderResponse result;
    try {
        nlohmann::json j = nlohmann::json::parse(response);
        
        result.success = j.value("success", false);
        result.errorMsg = j.value("errorMsg", "");
        result.orderId = j.value("orderId", "");
        
        if (j.contains("orderHashes") && j["orderHashes"].is_array()) {
            for (const auto& hash : j["orderHashes"]) {
                result.orderHashes.push_back(hash.get<std::string>());
            }
        }
        
        // Log the response for debugging
        if (!result.success) {
            std::cerr << "Order placement failed: " << result.errorMsg << std::endl;
        } else {
            std::cout << "Order placed successfully. Order ID: " << result.orderId << std::endl;
            if (!result.orderHashes.empty()) {
                std::cout << "Order hashes: ";
                for (const auto& hash : result.orderHashes) {
                    std::cout << hash << " ";
                }
                std::cout << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing order response: " << e.what() << std::endl;
        std::cerr << "Response: " << response << std::endl;
        result.success = false;
        result.errorMsg = "Failed to parse response: " + std::string(e.what());
    }
    
    return result;
}

std::string PolymarketApiClient::makeDataRequest(const std::string &endpoint, const std::string &method, const std::string &body)
{
    CURL *curl = curl_easy_init();
    std::string response;

    if (curl)
    {
        std::string url = dataBaseUrl + endpoint;

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());

        if (!body.empty())
        {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        }

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    return response;
}

double PolymarketApiClient::getBalance(const std::string& user) {
    // call https://data-api.polymarket.com/value?user={user_address} - not a gamma request a data request - since we only use this for the data api, we can do all o
    std::string endpoint = "/value?user=" + user;
    std::string response = makeDataRequest(endpoint);
    
    try {
        nlohmann::json j = nlohmann::json::parse(response);
        
        // Check if response is an object with a "value" field
        if (j.is_object() && j.contains("value")) {
            return j["value"].get<double>();
        }
        // Check if response is an array (some APIs return arrays)
        else if (j.is_array() && j.size() > 0) {
            // Try to get the first element if it's an array
            return j[0]["value"].get<double>();
        }
        else {
            std::cerr << "Unexpected balance response structure: " << response << std::endl;
            return 0.0;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing balance response: " << e.what() << std::endl;
        std::cerr << "Response: " << response << std::endl;
        return 0.0;
    }
}

std::vector<polymarket_bot::common::PolymarketPosition> PolymarketApiClient::getPositions(const std::string& user,
                                                                                           const std::string& market,
                                                                                           double sizeThreshold,
                                                                                           bool redeemable,
                                                                                           bool mergeable,
                                                                                           const std::string& title,
                                                                                           const std::string& eventId,
                                                                                           int limit,
                                                                                           int offset,
                                                                                           const std::string& sortBy,
                                                                                           const std::string& sortDirection) {
    // Build query parameters
    std::string endpoint = "/positions?user=" + user;
    
    if (!market.empty()) {
        endpoint += "&market=" + market;
    }
    
    endpoint += "&sizeThreshold=" + std::to_string(sizeThreshold);
    
    if (redeemable) {
        endpoint += "&redeemable=true";
    }
    
    if (mergeable) {
        endpoint += "&mergeable=true";
    }
    
    if (!title.empty()) {
        endpoint += "&title=" + title;
    }
    
    if (!eventId.empty()) {
        endpoint += "&eventId=" + eventId;
    }
    
    endpoint += "&limit=" + std::to_string(limit);
    endpoint += "&offset=" + std::to_string(offset);
    endpoint += "&sortBy=" + sortBy;
    endpoint += "&sortDirection=" + sortDirection;
    
    std::string response = makeDataRequest(endpoint);
    
    std::vector<polymarket_bot::common::PolymarketPosition> positions;
    
    try {
        nlohmann::json j = nlohmann::json::parse(response);
        
        if (j.is_array()) {
            positions = j.get<std::vector<polymarket_bot::common::PolymarketPosition>>();
        } else {
            std::cerr << "Unexpected positions response structure - expected array" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing positions response: " << e.what() << std::endl;
        std::cerr << "Response: " << response << std::endl;
    }
    
    return positions;
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
    auto twoWeeksFromNow = now + std::chrono::hours(24 * 7);
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

// Lambda order execution
polymarket_bot::common::PolymarketOrderResponse PolymarketApiClient::executeLambdaOrder(const std::string& slug, double price, double size, const std::string& outcome, const std::string& side, const std::string& orderType) {
    CURL* curl = curl_easy_init();
    std::string response;
    polymarket_bot::common::PolymarketOrderResponse result;

    if (curl) {
        std::string url = "https://s7raz3kdkgbqtk5eej6hzsbogq0vjvrh.lambda-url.ca-central-1.on.aws/";
        
        // Create request payload
        nlohmann::json payload;
        payload["slug"] = slug;
        payload["price"] = price;
        payload["size"] = size;
        payload["outcome"] = outcome;
        payload["side"] = side;
        payload["order_type"] = orderType;
        
        std::string jsonPayload = payload.dump();
        
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());
        
        CURLcode res = curl_easy_perform(curl);
        
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        
        if (res == CURLE_OK && response_code == 200) {
            try {
                nlohmann::json j = nlohmann::json::parse(response);
                result.success = true;
                result.orderId = j.value("order_id", "");
                result.errorMsg = "";
                if (j.contains("transaction_hash")) {
                    result.orderHashes.push_back(j["transaction_hash"].get<std::string>());
                }
            } catch (const std::exception& e) {
                result.success = false;
                result.errorMsg = "Failed to parse lambda response: " + std::string(e.what());
            }
        } else {
            result.success = false;
            result.errorMsg = "Lambda request failed with code: " + std::to_string(response_code);
        }
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    } else {
        result.success = false;
        result.errorMsg = "Failed to initialize CURL";
    }
    
    return result;
}
