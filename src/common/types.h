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

// JSON serialization support
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(OddsOutcome, name, price, point)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(OddsMarket, key, outcomes)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(OddsBookmaker, key, title, last_update, markets)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RawOddsGame, id, sport_key, commence_time, home_team, away_team, bookmakers)

} // namespace common
} // namespace polymarket_bot 