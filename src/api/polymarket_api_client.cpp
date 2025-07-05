#include "../common/types.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

using namespace polymarket_bot::common;



class PolymarketApiClient {



    public:
        PolymarketApiClient();
        ~PolymarketApiClient();

        std::vector<PolymarketMarket> getCurrentMarkets();
        PolymarketOrderResponse executeOrder(const PolymarketOpenOrder& order);
        double getBalance(const std::string& user);
        std::vector<PolymarketOpenOrder> getPositions();
        std::vector<PolymarketUserActivity> getUserActivity(const std::string& user, 
                                                           int limit = 100, 
                                                           int offset = 0,
                                                           const std::string& market = "",
                                                           const std::string& type = "",
                                                           int start = 0,
                                                           int end = 0,
                                                           const std::string& side = "",
                                                           const std::string& sortBy = "TIMESTAMP",
                                                           const std::string& sortDirection = "DESC");
        PolymarketMarket getMarketInfo(const std::string &marketId);
};