#pragma once

#include <string>
#include <vector>
#include <memory>
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
    int price;  // American odds format (e.g., 240, -303)
    double point;  // Optional, for spreads/totals
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

// JSON serialization support
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(OddsOutcome, name, price, point)
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

} // namespace common
} // namespace polymarket_bot 