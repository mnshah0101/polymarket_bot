#pragma once

#include <vector>
#include <string>
#include <utility>
#include <cstdlib>
#include <cmath>
#include <curl/curl.h>
#include <thread>
#include <mutex>
#include <future>
#include <queue>
#include <map>
#include <unordered_map>
#include "nlohmann/json.hpp"

#include "api/odds_api_client.h"
#include "api/polymarket_api_client.h"
#include "config/config_manager.h"
#include "common/types.h"

// Trading opportunity structure
struct ArbitrageOpportunity {
    std::string polymarketId;
    std::string polymarketSlug;
    std::string oddsId;
    std::string oddsGame;
    std::string outcome;
    double polymarketPrice;  // Decimal odds from Polymarket
    double oddsPrice;        // Decimal odds from sportsbook
    double edge;             // Percentage edge (e.g., 0.05 for 5%)
    double impliedProbability; // Combined implied probability
    std::string recommendedAction; // "BUY_POLYMARKET_YES", "BUY_POLYMARKET_NO", or "NO_TRADE"
    double recommendedStake;  // Recommended stake amount
};

// Team mapping structure for slug generation
struct TeamMapping {
    std::string oddsTeamName;      // Team name from odds API
    std::string polymarketCode;    // 3-letter code for Polymarket slug
    std::string fullName;          // Full team name for reference
};

class MarketMatcher
{
private:
    polymarket_bot::api::PolymarketApiClient polyClient;
    polymarket_bot::api::OddsApiClient oddsClient;
    const polymarket_bot::config::ConfigManager &configManager;

    std::vector<polymarket_bot::common::RawOddsGame> oddsGames;
    std::vector<polymarket_bot::common::GammaMarket> gammaMarkets;

    // Threading support
    std::mutex coutMutex;
    int maxConcurrentRequests;
    
    // Team mappings for slug generation
    std::unordered_map<std::string, TeamMapping> nbaTeams;
    std::unordered_map<std::string, TeamMapping> nhlTeams;
    std::unordered_map<std::string, TeamMapping> mlbTeams;
    
    // Date helpers
    static std::string dateOnly(const std::string &iso);
    static int parseDate(const std::string &iso);
    static std::string formatDateForSlug(const std::string &iso);
    
    // Text normalization
    static std::string normalizeText(const std::string &text);
    static std::string slugToQuestion(const std::string &slug);

    // libcurl write callback
    static size_t writeCallback(void *contents,
                                size_t size,
                                size_t nmemb,
                                void *userp);

    // Embedding & similarity
    std::vector<double> getEmbedding(const std::string &text);
    std::vector<double> getEmbeddingWithLogging(const std::string &text, int index, int total);
    std::vector<std::vector<double>> getBatchEmbeddings(const std::vector<std::string> &texts);
    std::vector<std::vector<double>> getBatchEmbeddingsSingle(const std::vector<std::string> &texts);
    static double cosineSimilarity(const std::vector<double> &A,
                                   const std::vector<double> &B);

    // Arbitrage calculation helpers
    static double calculateImpliedProbability(double decimalOdds);
    static double calculatePolymarketProbability(double polymarketPrice);
    static double calculateEdge(double prob1, double prob2);
    static std::string determineRecommendedAction(double polymarketProb, double oddsProb);
    static double calculateOptimalStake(double edge, double totalStake = 1000.0);

    // New slug-based matching methods
    void initializeTeamMappings();
    std::string generateSlugForGame(const polymarket_bot::common::RawOddsGame& game);
    std::string findPolymarketMarketBySlug(const std::string& slug);
    std::vector<std::pair<std::string, std::string>> matchMarketsBySlug();
    
    // New method to fetch market by slug directly from API
    std::optional<polymarket_bot::common::GammaMarket> fetchMarketBySlug(const std::string& slug);

public:
    MarketMatcher(polymarket_bot::api::PolymarketApiClient polyClient,
                  polymarket_bot::api::OddsApiClient oddsClient,
                  const polymarket_bot::config::ConfigManager &configManager);

    void loadAll();
    std::vector<std::pair<std::string, std::string>> matchMarkets();
    
    // Find Polymarket trading opportunities (value betting)
    std::vector<ArbitrageOpportunity> findArbitrageOpportunities(double minEdge = 0.03);
    
    // Public access to slug-based matching for testing
    std::vector<std::pair<std::string, std::string>> testMatchMarketsBySlug() { return matchMarketsBySlug(); }
    
    // Public access to slug generation for testing
    std::string testGenerateSlugForGame(const polymarket_bot::common::RawOddsGame& game) { return generateSlugForGame(game); }
    
    // Getter for odds games
    const std::vector<polymarket_bot::common::RawOddsGame>& getOddsGames() const { return oddsGames; }
};
