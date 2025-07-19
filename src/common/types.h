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

// CLOB Market Response Structures
struct ClobMarketToken {
    std::string token_id;
    std::string outcome;
    double price;
    bool winner;
};

struct ClobMarketRewards {
    std::optional<std::string> rates;
    double min_size;
    double max_spread;
};

struct ClobMarket {
    bool enable_order_book;
    bool active;
    bool closed;
    bool archived;
    bool accepting_orders;
    std::string accepting_order_timestamp;
    double minimum_order_size;
    double minimum_tick_size;
    std::string condition_id;
    std::string question_id;
    std::string question;
    std::string description;
    std::string market_slug;
    std::string end_date_iso;
    std::string game_start_time;
    int seconds_delay;
    std::string fpmm;
    double maker_base_fee;
    double taker_base_fee;
    bool notifications_enabled;
    bool neg_risk;
    std::string neg_risk_market_id;
    std::string neg_risk_request_id;
    std::string icon;
    std::string image;
    ClobMarketRewards rewards;
    bool is_50_50_outcome;
    std::vector<ClobMarketToken> tokens;
    std::vector<std::string> tags;
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

// Polymarket Position Structure (Data API)
struct PolymarketPosition {
    std::string proxyWallet;
    std::string asset;
    std::string conditionId;
    double size;
    double avgPrice;
    double initialValue;
    double currentValue;
    double cashPnl;
    double percentPnl;
    double totalBought;
    double realizedPnl;
    double percentRealizedPnl;
    double curPrice;
    bool redeemable;
    std::string title;
    std::string slug;
    std::string icon;
    std::string eventSlug;
    std::string outcome;
    int outcomeIndex;
    std::string oppositeOutcome;
    std::string oppositeAsset;
    std::string endDate;
    bool negativeRisk;
};

// Gamma Markets API Response Structures
struct GammaMarket {
    std::optional<std::string> id;                    // The unique identifier of the market
    std::optional<std::string> question;              // The market question
    std::optional<std::string> conditionId;           // The condition ID
    std::optional<std::string> slug;                  // The slug of the market
    std::optional<std::string> resolutionSource;      // Resolution source
    std::optional<std::string> endDate;               // The end date of the market
    std::optional<std::string> liquidity;             // The liquidity of the market
    std::optional<std::string> startDate;             // The start date of the market
    std::optional<std::string> image;                 // The market image URL
    std::optional<std::string> icon;                  // The market icon URL
    std::optional<std::string> description;           // The market description
    std::optional<std::string> outcomes;              // The market outcomes (JSON string)
    std::optional<std::string> outcomePrices;         // The outcome prices (JSON string)
    std::optional<std::string> volume;                // The trading volume
    std::optional<bool> active;                       // Whether the market is active
    std::optional<bool> closed;                       // Whether the market is closed
    std::optional<std::string> marketMakerAddress;    // The market maker address
    std::optional<std::string> createdAt;             // Creation timestamp
    std::optional<std::string> updatedAt;             // Last update timestamp
    std::optional<bool> new_market;                   // Whether this is a new market
    std::optional<bool> featured;                     // Whether the market is featured
    std::optional<std::string> submitted_by;          // Who submitted the market
    std::optional<bool> archived;                     // Whether the market is archived
    std::optional<std::string> resolvedBy;            // Who resolved the market
    std::optional<bool> restricted;                   // Whether the market is restricted
    std::optional<std::string> groupItemTitle;        // Group item title
    std::optional<std::string> groupItemThreshold;    // Group item threshold
    std::optional<std::string> questionID;            // Question ID
    std::optional<bool> enableOrderBook;              // Whether order book is enabled
    std::optional<double> orderPriceMinTickSize;      // Minimum tick size for orders
    std::optional<double> orderMinSize;               // Minimum order size
    std::optional<double> volumeNum;                  // Volume as number
    std::optional<double> liquidityNum;               // Liquidity as number
    std::optional<std::string> endDateIso;            // End date in ISO format
    std::optional<std::string> startDateIso;          // Start date in ISO format
    std::optional<bool> hasReviewedDates;             // Whether dates have been reviewed
    std::optional<double> volume24hr;                 // 24-hour volume
    std::optional<double> volume1wk;                  // 1-week volume
    std::optional<double> volume1mo;                  // 1-month volume
    std::optional<double> volume1yr;                  // 1-year volume
    std::optional<std::string> clobTokenIds;          // CLOB token IDs (JSON string)
    std::optional<std::string> umaBond;               // UMA bond
    std::optional<std::string> umaReward;             // UMA reward
    std::optional<double> volume24hrClob;             // 24-hour CLOB volume
    std::optional<double> volume1wkClob;              // 1-week CLOB volume
    std::optional<double> volume1moClob;              // 1-month CLOB volume
    std::optional<double> volume1yrClob;              // 1-year CLOB volume
    std::optional<double> volumeClob;                 // Total CLOB volume
    std::optional<double> liquidityClob;              // CLOB liquidity
    std::optional<bool> acceptingOrders;              // Whether accepting orders
    std::optional<bool> negRisk;                      // Negative risk
    std::optional<bool> ready;                        // Whether market is ready
    std::optional<bool> funded;                       // Whether market is funded
    std::optional<std::string> acceptingOrdersTimestamp; // When orders started being accepted
    std::optional<bool> cyom;                         // Create your own market
    std::optional<double> competitive;                // Competitive status
    std::optional<bool> pagerDutyNotificationEnabled; // PagerDuty notifications enabled
    std::optional<bool> approved;                     // Whether market is approved
    std::optional<double> rewardsMinSize;             // Minimum reward size
    std::optional<double> rewardsMaxSpread;           // Maximum reward spread
    std::optional<double> spread;                     // Market spread
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
    std::optional<std::string> umaResolutionStatuses; // UMA resolution statuses (JSON string)
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
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ClobMarketToken, token_id, outcome, price, winner)
// Custom serialization for ClobMarketRewards due to optional field
inline void to_json(nlohmann::json& j, const ClobMarketRewards& r) {
    j = nlohmann::json{{"min_size", r.min_size}, {"max_spread", r.max_spread}};
    if (r.rates.has_value()) {
        j["rates"] = r.rates.value();
    }
}

inline void from_json(const nlohmann::json& j, ClobMarketRewards& r) {
    j.at("min_size").get_to(r.min_size);
    j.at("max_spread").get_to(r.max_spread);
    r.rates = j.contains("rates") && !j["rates"].is_null() ? 
              std::optional<std::string>(j["rates"].get<std::string>()) : std::nullopt;
}
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ClobMarket, enable_order_book, active, closed, archived, accepting_orders, accepting_order_timestamp, minimum_order_size, minimum_tick_size, condition_id, question_id, question, description, market_slug, end_date_iso, game_start_time, seconds_delay, fpmm, maker_base_fee, taker_base_fee, notifications_enabled, neg_risk, neg_risk_market_id, neg_risk_request_id, icon, image, rewards, is_50_50_outcome, tokens, tags)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PolymarketUserActivity, proxyWallet, timestamp, conditionId, type, size, usdcSize, transactionHash, price, asset, side, outcomeIndex, title, slug, icon, eventSlug, outcome, name, pseudonym, bio, profileImage, profileImageOptimized)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PolymarketPosition, proxyWallet, asset, conditionId, size, avgPrice, initialValue, currentValue, cashPnl, percentPnl, totalBought, realizedPnl, percentRealizedPnl, curPrice, redeemable, title, slug, icon, eventSlug, outcome, outcomeIndex, oppositeOutcome, oppositeAsset, endDate, negativeRisk)

// Custom serialization for GammaMarket to handle the actual API response structure
inline void to_json(nlohmann::json& j, const GammaMarket& m) {
    j = nlohmann::json{};
    
    // Add all fields only if they have values
    if (m.id.has_value()) j["id"] = m.id.value();
    if (m.question.has_value()) j["question"] = m.question.value();
    if (m.conditionId.has_value()) j["conditionId"] = m.conditionId.value();
    if (m.slug.has_value()) j["slug"] = m.slug.value();
    if (m.resolutionSource.has_value()) j["resolutionSource"] = m.resolutionSource.value();
    if (m.endDate.has_value()) j["endDate"] = m.endDate.value();
    if (m.liquidity.has_value()) j["liquidity"] = m.liquidity.value();
    if (m.startDate.has_value()) j["startDate"] = m.startDate.value();
    if (m.image.has_value()) j["image"] = m.image.value();
    if (m.icon.has_value()) j["icon"] = m.icon.value();
    if (m.description.has_value()) j["description"] = m.description.value();
    if (m.outcomes.has_value()) j["outcomes"] = m.outcomes.value();
    if (m.outcomePrices.has_value()) j["outcomePrices"] = m.outcomePrices.value();
    if (m.volume.has_value()) j["volume"] = m.volume.value();
    if (m.active.has_value()) j["active"] = m.active.value();
    if (m.closed.has_value()) j["closed"] = m.closed.value();
    if (m.marketMakerAddress.has_value()) j["marketMakerAddress"] = m.marketMakerAddress.value();
    if (m.createdAt.has_value()) j["createdAt"] = m.createdAt.value();
    if (m.updatedAt.has_value()) j["updatedAt"] = m.updatedAt.value();
    if (m.new_market.has_value()) j["new"] = m.new_market.value();
    if (m.featured.has_value()) j["featured"] = m.featured.value();
    if (m.submitted_by.has_value()) j["submitted_by"] = m.submitted_by.value();
    if (m.archived.has_value()) j["archived"] = m.archived.value();
    if (m.resolvedBy.has_value()) j["resolvedBy"] = m.resolvedBy.value();
    if (m.restricted.has_value()) j["restricted"] = m.restricted.value();
    if (m.groupItemTitle.has_value()) j["groupItemTitle"] = m.groupItemTitle.value();
    if (m.groupItemThreshold.has_value()) j["groupItemThreshold"] = m.groupItemThreshold.value();
    if (m.questionID.has_value()) j["questionID"] = m.questionID.value();
    if (m.enableOrderBook.has_value()) j["enableOrderBook"] = m.enableOrderBook.value();
    if (m.orderPriceMinTickSize.has_value()) j["orderPriceMinTickSize"] = m.orderPriceMinTickSize.value();
    if (m.orderMinSize.has_value()) j["orderMinSize"] = m.orderMinSize.value();
    if (m.volumeNum.has_value()) j["volumeNum"] = m.volumeNum.value();
    if (m.liquidityNum.has_value()) j["liquidityNum"] = m.liquidityNum.value();
    if (m.endDateIso.has_value()) j["endDateIso"] = m.endDateIso.value();
    if (m.startDateIso.has_value()) j["startDateIso"] = m.startDateIso.value();
    if (m.hasReviewedDates.has_value()) j["hasReviewedDates"] = m.hasReviewedDates.value();
    if (m.volume24hr.has_value()) j["volume24hr"] = m.volume24hr.value();
    if (m.volume1wk.has_value()) j["volume1wk"] = m.volume1wk.value();
    if (m.volume1mo.has_value()) j["volume1mo"] = m.volume1mo.value();
    if (m.volume1yr.has_value()) j["volume1yr"] = m.volume1yr.value();
    if (m.clobTokenIds.has_value()) j["clobTokenIds"] = m.clobTokenIds.value();
    if (m.umaBond.has_value()) j["umaBond"] = m.umaBond.value();
    if (m.umaReward.has_value()) j["umaReward"] = m.umaReward.value();
    if (m.volume24hrClob.has_value()) j["volume24hrClob"] = m.volume24hrClob.value();
    if (m.volume1wkClob.has_value()) j["volume1wkClob"] = m.volume1wkClob.value();
    if (m.volume1moClob.has_value()) j["volume1moClob"] = m.volume1moClob.value();
    if (m.volume1yrClob.has_value()) j["volume1yrClob"] = m.volume1yrClob.value();
    if (m.volumeClob.has_value()) j["volumeClob"] = m.volumeClob.value();
    if (m.liquidityClob.has_value()) j["liquidityClob"] = m.liquidityClob.value();
    if (m.acceptingOrders.has_value()) j["acceptingOrders"] = m.acceptingOrders.value();
    if (m.negRisk.has_value()) j["negRisk"] = m.negRisk.value();
    if (m.ready.has_value()) j["ready"] = m.ready.value();
    if (m.funded.has_value()) j["funded"] = m.funded.value();
    if (m.acceptingOrdersTimestamp.has_value()) j["acceptingOrdersTimestamp"] = m.acceptingOrdersTimestamp.value();
    if (m.cyom.has_value()) j["cyom"] = m.cyom.value();
    if (m.competitive.has_value()) j["competitive"] = m.competitive.value();
    if (m.pagerDutyNotificationEnabled.has_value()) j["pagerDutyNotificationEnabled"] = m.pagerDutyNotificationEnabled.value();
    if (m.approved.has_value()) j["approved"] = m.approved.value();
    if (m.rewardsMinSize.has_value()) j["rewardsMinSize"] = m.rewardsMinSize.value();
    if (m.rewardsMaxSpread.has_value()) j["rewardsMaxSpread"] = m.rewardsMaxSpread.value();
    if (m.spread.has_value()) j["spread"] = m.spread.value();
    if (m.umaResolutionStatuses.has_value()) j["umaResolutionStatuses"] = m.umaResolutionStatuses.value();
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
    m.id = j.contains("id") ? std::optional<std::string>(j["id"].get<std::string>()) : std::nullopt;
    m.question = j.contains("question") ? std::optional<std::string>(j["question"].get<std::string>()) : std::nullopt;
    m.conditionId = j.contains("conditionId") ? std::optional<std::string>(j["conditionId"].get<std::string>()) : std::nullopt;
    m.slug = j.contains("slug") ? std::optional<std::string>(j["slug"].get<std::string>()) : std::nullopt;
    m.resolutionSource = j.contains("resolutionSource") ? std::optional<std::string>(j["resolutionSource"].get<std::string>()) : std::nullopt;
    m.endDate = j.contains("endDate") ? std::optional<std::string>(j["endDate"].get<std::string>()) : std::nullopt;
    m.liquidity = j.contains("liquidity") ? std::optional<std::string>(j["liquidity"].get<std::string>()) : std::nullopt;
    m.startDate = j.contains("startDate") ? std::optional<std::string>(j["startDate"].get<std::string>()) : std::nullopt;
    m.image = j.contains("image") ? std::optional<std::string>(j["image"].get<std::string>()) : std::nullopt;
    m.icon = j.contains("icon") ? std::optional<std::string>(j["icon"].get<std::string>()) : std::nullopt;
    m.description = j.contains("description") ? std::optional<std::string>(j["description"].get<std::string>()) : std::nullopt;
    m.outcomes = j.contains("outcomes") ? std::optional<std::string>(j["outcomes"].get<std::string>()) : std::nullopt;
    m.outcomePrices = j.contains("outcomePrices") ? std::optional<std::string>(j["outcomePrices"].get<std::string>()) : std::nullopt;
    m.volume = j.contains("volume") ? std::optional<std::string>(j["volume"].get<std::string>()) : std::nullopt;
    m.active = j.contains("active") ? std::optional<bool>(j["active"].get<bool>()) : std::nullopt;
    m.closed = j.contains("closed") ? std::optional<bool>(j["closed"].get<bool>()) : std::nullopt;
    m.marketMakerAddress = j.contains("marketMakerAddress") ? std::optional<std::string>(j["marketMakerAddress"].get<std::string>()) : std::nullopt;
    m.createdAt = j.contains("createdAt") ? std::optional<std::string>(j["createdAt"].get<std::string>()) : std::nullopt;
    m.updatedAt = j.contains("updatedAt") ? std::optional<std::string>(j["updatedAt"].get<std::string>()) : std::nullopt;
    m.new_market = j.contains("new") ? std::optional<bool>(j["new"].get<bool>()) : std::nullopt;
    m.featured = j.contains("featured") ? std::optional<bool>(j["featured"].get<bool>()) : std::nullopt;
    m.submitted_by = j.contains("submitted_by") ? std::optional<std::string>(j["submitted_by"].get<std::string>()) : std::nullopt;
    m.archived = j.contains("archived") ? std::optional<bool>(j["archived"].get<bool>()) : std::nullopt;
    m.resolvedBy = j.contains("resolvedBy") ? std::optional<std::string>(j["resolvedBy"].get<std::string>()) : std::nullopt;
    m.restricted = j.contains("restricted") ? std::optional<bool>(j["restricted"].get<bool>()) : std::nullopt;
    m.groupItemTitle = j.contains("groupItemTitle") ? std::optional<std::string>(j["groupItemTitle"].get<std::string>()) : std::nullopt;
    m.groupItemThreshold = j.contains("groupItemThreshold") ? std::optional<std::string>(j["groupItemThreshold"].get<std::string>()) : std::nullopt;
    m.questionID = j.contains("questionID") ? std::optional<std::string>(j["questionID"].get<std::string>()) : std::nullopt;
    m.enableOrderBook = j.contains("enableOrderBook") ? std::optional<bool>(j["enableOrderBook"].get<bool>()) : std::nullopt;
    m.orderPriceMinTickSize = j.contains("orderPriceMinTickSize") ? std::optional<double>(j["orderPriceMinTickSize"].get<double>()) : std::nullopt;
    m.orderMinSize = j.contains("orderMinSize") ? std::optional<double>(j["orderMinSize"].get<double>()) : std::nullopt;
    m.volumeNum = j.contains("volumeNum") ? std::optional<double>(j["volumeNum"].get<double>()) : std::nullopt;
    m.liquidityNum = j.contains("liquidityNum") ? std::optional<double>(j["liquidityNum"].get<double>()) : std::nullopt;
    m.endDateIso = j.contains("endDateIso") ? std::optional<std::string>(j["endDateIso"].get<std::string>()) : std::nullopt;
    m.startDateIso = j.contains("startDateIso") ? std::optional<std::string>(j["startDateIso"].get<std::string>()) : std::nullopt;
    m.hasReviewedDates = j.contains("hasReviewedDates") ? std::optional<bool>(j["hasReviewedDates"].get<bool>()) : std::nullopt;
    m.volume24hr = j.contains("volume24hr") ? std::optional<double>(j["volume24hr"].get<double>()) : std::nullopt;
    m.volume1wk = j.contains("volume1wk") ? std::optional<double>(j["volume1wk"].get<double>()) : std::nullopt;
    m.volume1mo = j.contains("volume1mo") ? std::optional<double>(j["volume1mo"].get<double>()) : std::nullopt;
    m.volume1yr = j.contains("volume1yr") ? std::optional<double>(j["volume1yr"].get<double>()) : std::nullopt;
    m.clobTokenIds = j.contains("clobTokenIds") ? std::optional<std::string>(j["clobTokenIds"].get<std::string>()) : std::nullopt;
    m.umaBond = j.contains("umaBond") ? std::optional<std::string>(j["umaBond"].get<std::string>()) : std::nullopt;
    m.umaReward = j.contains("umaReward") ? std::optional<std::string>(j["umaReward"].get<std::string>()) : std::nullopt;
    m.volume24hrClob = j.contains("volume24hrClob") ? std::optional<double>(j["volume24hrClob"].get<double>()) : std::nullopt;
    m.volume1wkClob = j.contains("volume1wkClob") ? std::optional<double>(j["volume1wkClob"].get<double>()) : std::nullopt;
    m.volume1moClob = j.contains("volume1moClob") ? std::optional<double>(j["volume1moClob"].get<double>()) : std::nullopt;
    m.volume1yrClob = j.contains("volume1yrClob") ? std::optional<double>(j["volume1yrClob"].get<double>()) : std::nullopt;
    m.volumeClob = j.contains("volumeClob") ? std::optional<double>(j["volumeClob"].get<double>()) : std::nullopt;
    m.liquidityClob = j.contains("liquidityClob") ? std::optional<double>(j["liquidityClob"].get<double>()) : std::nullopt;
    m.acceptingOrders = j.contains("acceptingOrders") ? std::optional<bool>(j["acceptingOrders"].get<bool>()) : std::nullopt;
    m.negRisk = j.contains("negRisk") ? std::optional<bool>(j["negRisk"].get<bool>()) : std::nullopt;
    m.ready = j.contains("ready") ? std::optional<bool>(j["ready"].get<bool>()) : std::nullopt;
    m.funded = j.contains("funded") ? std::optional<bool>(j["funded"].get<bool>()) : std::nullopt;
    m.acceptingOrdersTimestamp = j.contains("acceptingOrdersTimestamp") ? std::optional<std::string>(j["acceptingOrdersTimestamp"].get<std::string>()) : std::nullopt;
    m.cyom = j.contains("cyom") ? std::optional<bool>(j["cyom"].get<bool>()) : std::nullopt;
    m.competitive = j.contains("competitive") ? std::optional<double>(j["competitive"].get<double>()) : std::nullopt;
    m.pagerDutyNotificationEnabled = j.contains("pagerDutyNotificationEnabled") ? std::optional<bool>(j["pagerDutyNotificationEnabled"].get<bool>()) : std::nullopt;
    m.approved = j.contains("approved") ? std::optional<bool>(j["approved"].get<bool>()) : std::nullopt;
    m.rewardsMinSize = j.contains("rewardsMinSize") ? std::optional<double>(j["rewardsMinSize"].get<double>()) : std::nullopt;
    m.rewardsMaxSpread = j.contains("rewardsMaxSpread") ? std::optional<double>(j["rewardsMaxSpread"].get<double>()) : std::nullopt;
    m.spread = j.contains("spread") ? std::optional<double>(j["spread"].get<double>()) : std::nullopt;
    m.umaResolutionStatuses = j.contains("umaResolutionStatuses") ? std::optional<std::string>(j["umaResolutionStatuses"].get<std::string>()) : std::nullopt;
    
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