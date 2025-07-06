#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <nlohmann/json.hpp>

namespace polymarket_bot {
namespace common {

// Sports Book Enumeration
enum class SportsBook {
    PINNACLE,
    BETFAIR,
    BET365,
    DRAFTKINGS,
    FANDUEL,
    FOX_BET,
    MGM,
    BETMGM,
    BETRIVIAL,
    UNIBET,
    CAESARS,
    SUGARHOUSE,
    POINTSBETUS,
    BETONLINEAG,
    BETRIVERS,
    BARSTOOL,
    BOVADA,
    WILLIAMHILL_US
};

// Raw Odds API Response Structures
struct OddsOutcome {
    std::string name;
    double price;  // Decimal odds format (e.g., 1.7, 5.0)
    std::optional<double> point;  // Optional, for spreads/totals
};

struct OddsMarket {
    std::string key;  // e.g., "h2h", "spreads", "totals"
    std::vector<OddsOutcome> outcomes;
};

struct OddsBookmaker {
    std::string key;  // e.g., "unibet", "caesars"
    std::string title;  // e.g., "Unibet", "Caesars"
    std::string last_update;  // ISO 8601 timestamp
    std::vector<OddsMarket> markets;
};

struct RawOddsGame {
    std::string id;
    std::string sport_key;  // e.g., "americanfootball_nfl"
    std::string commence_time;  // ISO 8601 timestamp
    std::string home_team;
    std::string away_team;
    std::vector<OddsBookmaker> bookmakers;
};

struct RawOddsResponse {
    std::vector<RawOddsGame> games;
};

// Polymarket Market Response Structures
struct PolymarketToken {
    std::string token_id;
    std::string outcome;
};

struct PolymarketRewards {
    double min_size;
    double max_spread;
    std::string event_start_date;
    std::string event_end_date;
    double in_game_multiplier;
    int reward_epoch;
};

struct PolymarketMarket {
    std::string condition_id;
    std::string question_id;
    std::vector<PolymarketToken> tokens;
    PolymarketRewards rewards;
    std::string minimum_order_size;
    std::string minimum_tick_size;
    std::string category;
    std::string end_date_iso;
    std::string game_start_time;
    std::string question;
    std::string market_slug;
    std::string min_incentive_size;
    std::string max_incentive_spread;
    bool active;
    bool closed;
    int seconds_delay;
    std::string icon;
    std::string fpmm;
};

// Polymarket Order Response Structure
struct PolymarketOrderResponse {
    bool success;                    // Server-side success indicator
    std::string errorMsg;            // Error message if unsuccessful
    std::string orderId;             // ID of the placed order
    std::vector<std::string> orderHashes; // Transaction hashes if order was marketable
};

// Polymarket Open Order Structure
struct PolymarketOpenOrder {
    std::vector<std::string> associate_trades; // Trade IDs the order has been partially included in
    std::string id;                  // Order ID
    std::string status;              // Order current status
    std::string market;              // Market ID (condition ID)
    std::string original_size;       // Original order size at placement
    std::string outcome;             // Human readable outcome the order is for
    std::string maker_address;       // Maker address (funder)
    std::string owner;               // API key
    std::string price;               // Price
    std::string side;                // Buy or sell
    std::string size_matched;        // Size of order that has been matched/filled
    std::string asset_id;            // Token ID
    std::string expiration;          // Unix timestamp when order expired, 0 if no expiration
    std::string type;                // Order type (GTC, FOK, GTD)
    std::string created_at;          // Unix timestamp when order was created
};

// Polymarket User Activity Structure
struct PolymarketUserActivity {
    std::string proxyWallet;
    int timestamp;
    std::string conditionId;
    std::string type;
    double size;
    double usdcSize;
    std::string transactionHash;
    double price;
    std::string asset;
    std::string side;
    int outcomeIndex;
    std::string title;
    std::string slug;
    std::string icon;
    std::string eventSlug;
    std::string outcome;
    std::string name;
    std::string pseudonym;
    std::string bio;
    std::string profileImage;
    std::string profileImageOptimized;
};

// Gamma Markets API Response Structures
struct GammaMarket {
    std::string id;                    // The unique identifier of the market
    std::string question;              // The market question
    std::string conditionId;           // The condition ID
    std::string slug;                  // The slug of the market
    std::string resolutionSource;      // Resolution source
    std::string endDate;               // The end date of the market
    std::string liquidity;             // The liquidity of the market
    std::string startDate;             // The start date of the market
    std::string image;                 // The market image URL
    std::string icon;                  // The market icon URL
    std::string description;           // The market description
    std::string outcomes;              // The market outcomes (JSON string)
    std::string outcomePrices;         // The outcome prices (JSON string)
    std::string volume;                // The trading volume
    bool active;                       // Whether the market is active
    bool closed;                       // Whether the market is closed
    std::string marketMakerAddress;    // The market maker address
    std::string createdAt;             // Creation timestamp
    std::string updatedAt;             // Last update timestamp
    bool new_market;                   // Whether this is a new market
    bool featured;                     // Whether the market is featured
    std::string submitted_by;          // Who submitted the market
    bool archived;                     // Whether the market is archived
    std::string resolvedBy;            // Who resolved the market
    bool restricted;                   // Whether the market is restricted
    std::string groupItemTitle;        // Group item title
    std::string groupItemThreshold;    // Group item threshold
    std::string questionID;            // Question ID
    bool enableOrderBook;              // Whether order book is enabled
    double orderPriceMinTickSize;      // Minimum tick size for orders
    double orderMinSize;               // Minimum order size
    double volumeNum;                  // Volume as number
    double liquidityNum;               // Liquidity as number
    std::string endDateIso;            // End date in ISO format
    std::string startDateIso;          // Start date in ISO format
    bool hasReviewedDates;             // Whether dates have been reviewed
    double volume24hr;                 // 24-hour volume
    double volume1wk;                  // 1-week volume
    double volume1mo;                  // 1-month volume
    double volume1yr;                  // 1-year volume
    std::string clobTokenIds;          // CLOB token IDs (JSON string)
    std::string umaBond;               // UMA bond
    std::string umaReward;             // UMA reward
    double volume24hrClob;             // 24-hour CLOB volume
    double volume1wkClob;              // 1-week CLOB volume
    double volume1moClob;              // 1-month CLOB volume
    double volume1yrClob;              // 1-year CLOB volume
    double volumeClob;                 // Total CLOB volume
    double liquidityClob;              // CLOB liquidity
    bool acceptingOrders;              // Whether accepting orders
    bool negRisk;                      // Negative risk
    bool ready;                        // Whether market is ready
    bool funded;                       // Whether market is funded
    std::string acceptingOrdersTimestamp; // When orders started being accepted
    bool cyom;                         // Create your own market
    double competitive;                // Competitive status
    bool pagerDutyNotificationEnabled; // PagerDuty notifications enabled
    bool approved;                     // Whether market is approved
    double rewardsMinSize;             // Minimum reward size
    double rewardsMaxSpread;           // Maximum reward spread
    double spread;                     // Market spread
    std::optional<double> oneDayPriceChange;          // 1-day price change
    std::optional<double> oneWeekPriceChange;         // 1-week price change
    std::optional<double> oneMonthPriceChange;        // 1-month price change
    std::optional<double> lastTradePrice;             // Last trade price
    std::optional<double> bestBid;                    // Best bid
    std::optional<double> bestAsk;                    // Best ask
    std::optional<bool> automaticallyActive;           // Whether automatically active
    std::optional<bool> clearBookOnStart;             // Clear book on start
    std::optional<bool> manualActivation;             // Manual activation
    std::optional<bool> negRiskOther;                 // Negative risk other
    std::string umaResolutionStatuses; // UMA resolution statuses (JSON string)
    std::optional<bool> pendingDeployment;            // Pending deployment
    std::optional<bool> deploying;                    // Currently deploying
    std::optional<bool> rfqEnabled;                   // RFQ enabled
};

struct GammaMarketsResponse {
    std::vector<GammaMarket> markets;
    int total;
    int page;
    int limit;
};

// JSON serialization support
// Custom serialization for OddsOutcome to handle optional point field
inline void to_json(nlohmann::json& j, const OddsOutcome& o) {
    j = nlohmann::json{{"name", o.name}, {"price", o.price}};
    if (o.point.has_value()) {
        j["point"] = o.point.value();
    }
}

inline void from_json(const nlohmann::json& j, OddsOutcome& o) {
    j.at("name").get_to(o.name);
    j.at("price").get_to(o.price);
    o.point = j.contains("point") ? std::optional<double>(j["point"].get<double>()) : std::nullopt;
}
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(OddsMarket, key, outcomes)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(OddsBookmaker, key, title, last_update, markets)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RawOddsGame, id, sport_key, commence_time, home_team, away_team, bookmakers)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RawOddsResponse, games)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PolymarketToken, token_id, outcome)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PolymarketRewards, min_size, max_spread, event_start_date, event_end_date, in_game_multiplier, reward_epoch)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PolymarketMarket, condition_id, question_id, tokens, rewards, minimum_order_size, minimum_tick_size, category, end_date_iso, game_start_time, question, market_slug, min_incentive_size, max_incentive_spread, active, closed, seconds_delay, icon, fpmm)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PolymarketOrderResponse, success, errorMsg, orderId, orderHashes)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PolymarketOpenOrder, associate_trades, id, status, market, original_size, outcome, maker_address, owner, price, side, size_matched, asset_id, expiration, type, created_at)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PolymarketUserActivity, proxyWallet, timestamp, conditionId, type, size, usdcSize, transactionHash, price, asset, side, outcomeIndex, title, slug, icon, eventSlug, outcome, name, pseudonym, bio, profileImage, profileImageOptimized)

// Custom serialization for GammaMarket to handle the actual API response structure
inline void to_json(nlohmann::json& j, const GammaMarket& m) {
    j = nlohmann::json{
        {"id", m.id},
        {"question", m.question},
        {"conditionId", m.conditionId},
        {"slug", m.slug},
        {"resolutionSource", m.resolutionSource},
        {"endDate", m.endDate},
        {"liquidity", m.liquidity},
        {"startDate", m.startDate},
        {"image", m.image},
        {"icon", m.icon},
        {"description", m.description},
        {"outcomes", m.outcomes},
        {"outcomePrices", m.outcomePrices},
        {"volume", m.volume},
        {"active", m.active},
        {"closed", m.closed},
        {"marketMakerAddress", m.marketMakerAddress},
        {"createdAt", m.createdAt},
        {"updatedAt", m.updatedAt},
        {"new", m.new_market},
        {"featured", m.featured},
        {"submitted_by", m.submitted_by},
        {"archived", m.archived},
        {"resolvedBy", m.resolvedBy},
        {"restricted", m.restricted},
        {"groupItemTitle", m.groupItemTitle},
        {"groupItemThreshold", m.groupItemThreshold},
        {"questionID", m.questionID},
        {"enableOrderBook", m.enableOrderBook},
        {"orderPriceMinTickSize", m.orderPriceMinTickSize},
        {"orderMinSize", m.orderMinSize},
        {"volumeNum", m.volumeNum},
        {"liquidityNum", m.liquidityNum},
        {"endDateIso", m.endDateIso},
        {"startDateIso", m.startDateIso},
        {"hasReviewedDates", m.hasReviewedDates},
        {"volume24hr", m.volume24hr},
        {"volume1wk", m.volume1wk},
        {"volume1mo", m.volume1mo},
        {"volume1yr", m.volume1yr},
        {"clobTokenIds", m.clobTokenIds},
        {"umaBond", m.umaBond},
        {"umaReward", m.umaReward},
        {"volume24hrClob", m.volume24hrClob},
        {"volume1wkClob", m.volume1wkClob},
        {"volume1moClob", m.volume1moClob},
        {"volume1yrClob", m.volume1yrClob},
        {"volumeClob", m.volumeClob},
        {"liquidityClob", m.liquidityClob},
        {"acceptingOrders", m.acceptingOrders},
        {"negRisk", m.negRisk},
        {"ready", m.ready},
        {"funded", m.funded},
        {"acceptingOrdersTimestamp", m.acceptingOrdersTimestamp},
        {"cyom", m.cyom},
        {"competitive", m.competitive},
        {"pagerDutyNotificationEnabled", m.pagerDutyNotificationEnabled},
        {"approved", m.approved},
        {"rewardsMinSize", m.rewardsMinSize},
        {"rewardsMaxSpread", m.rewardsMaxSpread},
        {"spread", m.spread},
        {"umaResolutionStatuses", m.umaResolutionStatuses}
    };
    
    // Add optional fields only if they have values
    if (m.oneDayPriceChange.has_value()) j["oneDayPriceChange"] = m.oneDayPriceChange.value();
    if (m.oneWeekPriceChange.has_value()) j["oneWeekPriceChange"] = m.oneWeekPriceChange.value();
    if (m.oneMonthPriceChange.has_value()) j["oneMonthPriceChange"] = m.oneMonthPriceChange.value();
    if (m.lastTradePrice.has_value()) j["lastTradePrice"] = m.lastTradePrice.value();
    if (m.bestBid.has_value()) j["bestBid"] = m.bestBid.value();
    if (m.bestAsk.has_value()) j["bestAsk"] = m.bestAsk.value();
    if (m.automaticallyActive.has_value()) j["automaticallyActive"] = m.automaticallyActive.value();
    if (m.clearBookOnStart.has_value()) j["clearBookOnStart"] = m.clearBookOnStart.value();
    if (m.manualActivation.has_value()) j["manualActivation"] = m.manualActivation.value();
    if (m.negRiskOther.has_value()) j["negRiskOther"] = m.negRiskOther.value();
    if (m.pendingDeployment.has_value()) j["pendingDeployment"] = m.pendingDeployment.value();
    if (m.deploying.has_value()) j["deploying"] = m.deploying.value();
    if (m.rfqEnabled.has_value()) j["rfqEnabled"] = m.rfqEnabled.value();
}

inline void from_json(const nlohmann::json& j, GammaMarket& m) {
    j.at("id").get_to(m.id);
    j.at("question").get_to(m.question);
    j.at("conditionId").get_to(m.conditionId);
    j.at("slug").get_to(m.slug);
    j.at("resolutionSource").get_to(m.resolutionSource);
    j.at("endDate").get_to(m.endDate);
    j.at("liquidity").get_to(m.liquidity);
    j.at("startDate").get_to(m.startDate);
    j.at("image").get_to(m.image);
    j.at("icon").get_to(m.icon);
    j.at("description").get_to(m.description);
    j.at("outcomes").get_to(m.outcomes);
    j.at("outcomePrices").get_to(m.outcomePrices);
    j.at("volume").get_to(m.volume);
    j.at("active").get_to(m.active);
    j.at("closed").get_to(m.closed);
    j.at("marketMakerAddress").get_to(m.marketMakerAddress);
    j.at("createdAt").get_to(m.createdAt);
    j.at("updatedAt").get_to(m.updatedAt);
    j.at("new").get_to(m.new_market);
    j.at("featured").get_to(m.featured);
    j.at("submitted_by").get_to(m.submitted_by);
    j.at("archived").get_to(m.archived);
    j.at("resolvedBy").get_to(m.resolvedBy);
    j.at("restricted").get_to(m.restricted);
    j.at("groupItemTitle").get_to(m.groupItemTitle);
    j.at("groupItemThreshold").get_to(m.groupItemThreshold);
    j.at("questionID").get_to(m.questionID);
    j.at("enableOrderBook").get_to(m.enableOrderBook);
    j.at("orderPriceMinTickSize").get_to(m.orderPriceMinTickSize);
    j.at("orderMinSize").get_to(m.orderMinSize);
    j.at("volumeNum").get_to(m.volumeNum);
    j.at("liquidityNum").get_to(m.liquidityNum);
    j.at("endDateIso").get_to(m.endDateIso);
    j.at("startDateIso").get_to(m.startDateIso);
    j.at("hasReviewedDates").get_to(m.hasReviewedDates);
    j.at("volume24hr").get_to(m.volume24hr);
    j.at("volume1wk").get_to(m.volume1wk);
    j.at("volume1mo").get_to(m.volume1mo);
    j.at("volume1yr").get_to(m.volume1yr);
    j.at("clobTokenIds").get_to(m.clobTokenIds);
    j.at("umaBond").get_to(m.umaBond);
    j.at("umaReward").get_to(m.umaReward);
    j.at("volume24hrClob").get_to(m.volume24hrClob);
    j.at("volume1wkClob").get_to(m.volume1wkClob);
    j.at("volume1moClob").get_to(m.volume1moClob);
    j.at("volume1yrClob").get_to(m.volume1yrClob);
    j.at("volumeClob").get_to(m.volumeClob);
    j.at("liquidityClob").get_to(m.liquidityClob);
    j.at("acceptingOrders").get_to(m.acceptingOrders);
    j.at("negRisk").get_to(m.negRisk);
    j.at("ready").get_to(m.ready);
    j.at("funded").get_to(m.funded);
    j.at("acceptingOrdersTimestamp").get_to(m.acceptingOrdersTimestamp);
    j.at("cyom").get_to(m.cyom);
    j.at("competitive").get_to(m.competitive);
    j.at("pagerDutyNotificationEnabled").get_to(m.pagerDutyNotificationEnabled);
    j.at("approved").get_to(m.approved);
    j.at("rewardsMinSize").get_to(m.rewardsMinSize);
    j.at("rewardsMaxSpread").get_to(m.rewardsMaxSpread);
    j.at("spread").get_to(m.spread);
    j.at("umaResolutionStatuses").get_to(m.umaResolutionStatuses);
    
    // Handle optional fields
    m.oneDayPriceChange = j.contains("oneDayPriceChange") ? std::optional<double>(j["oneDayPriceChange"].get<double>()) : std::nullopt;
    m.oneWeekPriceChange = j.contains("oneWeekPriceChange") ? std::optional<double>(j["oneWeekPriceChange"].get<double>()) : std::nullopt;
    m.oneMonthPriceChange = j.contains("oneMonthPriceChange") ? std::optional<double>(j["oneMonthPriceChange"].get<double>()) : std::nullopt;
    m.lastTradePrice = j.contains("lastTradePrice") ? std::optional<double>(j["lastTradePrice"].get<double>()) : std::nullopt;
    m.bestBid = j.contains("bestBid") ? std::optional<double>(j["bestBid"].get<double>()) : std::nullopt;
    m.bestAsk = j.contains("bestAsk") ? std::optional<double>(j["bestAsk"].get<double>()) : std::nullopt;
    m.automaticallyActive = j.contains("automaticallyActive") ? std::optional<bool>(j["automaticallyActive"].get<bool>()) : std::nullopt;
    m.clearBookOnStart = j.contains("clearBookOnStart") ? std::optional<bool>(j["clearBookOnStart"].get<bool>()) : std::nullopt;
    m.manualActivation = j.contains("manualActivation") ? std::optional<bool>(j["manualActivation"].get<bool>()) : std::nullopt;
    m.negRiskOther = j.contains("negRiskOther") ? std::optional<bool>(j["negRiskOther"].get<bool>()) : std::nullopt;
    m.pendingDeployment = j.contains("pendingDeployment") ? std::optional<bool>(j["pendingDeployment"].get<bool>()) : std::nullopt;
    m.deploying = j.contains("deploying") ? std::optional<bool>(j["deploying"].get<bool>()) : std::nullopt;
    m.rfqEnabled = j.contains("rfqEnabled") ? std::optional<bool>(j["rfqEnabled"].get<bool>()) : std::nullopt;
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GammaMarketsResponse, markets)

} // namespace common
} // namespace polymarket_bot 