#pragma once

#include "../common/types.h"
#include <string>
#include <vector>

namespace polymarket_bot {
namespace api {

class PolymarketApiClient {
private:
    std::string baseUrl;
    std::string gammaBaseUrl;
    std::string dataBaseUrl;
    std::string address;
    std::string signature;
    std::string timestamp;
    std::string apiKey;
    std::string passphrase;
    int chainId;

    // Helper methods
    std::string makeAuthenticatedRequest(const std::string& endpoint, const std::string& method = "GET", const std::string& body = "");
    std::string makeGammaRequest(const std::string& endpoint, const std::string& method = "GET", const std::string& body = "");
    std::string makeDataRequest(const std::string& endpoint, const std::string& method = "GET", const std::string& body = "");

public:
    PolymarketApiClient(const std::string& baseUrl, 
                       const std::string& gammaBaseUrl,
                       const std::string& dataBaseUrl,
                       const std::string& address,
                       const std::string& signature,
                       const std::string& timestamp,
                       const std::string& apiKey,
                       const std::string& passphrase,
                       int chainId);
    ~PolymarketApiClient();

    // Polymarket CLOB API methods
    std::vector<common::PolymarketMarket> getCurrentMarkets();
    common::PolymarketOrderResponse executeOrder(const common::PolymarketOpenOrder& order);
    
    // Alternative method for creating orders with individual parameters
    common::PolymarketOrderResponse createOrder(
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
        const std::string& orderType
    );
    double getBalance(const std::string& user);
    std::vector<common::PolymarketPosition> getPositions(const std::string& user,
                                                         const std::string& market = "",
                                                         double sizeThreshold = 1.0,
                                                         bool redeemable = false,
                                                         bool mergeable = false,
                                                         const std::string& title = "",
                                                         const std::string& eventId = "",
                                                         int limit = 50,
                                                         int offset = 0,
                                                         const std::string& sortBy = "TOKENS",
                                                         const std::string& sortDirection = "DESC");
    std::vector<common::PolymarketUserActivity> getUserActivity(const std::string& user, 
                                                       int limit = 100, 
                                                       int offset = 0,
                                                       const std::string& market = "",
                                                       const std::string& type = "",
                                                       int start = 0,
                                                       int end = 0,
                                                       const std::string& side = "",
                                                       const std::string& sortBy = "TIMESTAMP",
                                                       const std::string& sortDirection = "DESC");
    common::PolymarketMarket getMarketInfo(const std::string& marketId);

    // Gamma Markets API methods
    common::GammaMarketsResponse getGammaMarkets(int page = 1, int limit = 20);
    common::GammaMarket getGammaMarket(const std::string& marketId);
};

} // namespace api
} // namespace polymarket_bot
