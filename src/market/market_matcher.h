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
#include "nlohmann/json.hpp"

#include "api/odds_api_client.h"
#include "api/polymarket_api_client.h"
#include "config/config_manager.h"
#include "common/types.h"

// Arbitrage opportunity structure
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
    std::string recommendedAction; // "BUY_POLYMARKET" or "BUY_ODDS"
    double recommendedStake;  // Recommended stake amount
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
    
    // Date helpers
    static std::string dateOnly(const std::string &iso);
    static int parseDate(const std::string &iso);
    
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

public:
    MarketMatcher(polymarket_bot::api::PolymarketApiClient polyClient,
                  polymarket_bot::api::OddsApiClient oddsClient,
                  const polymarket_bot::config::ConfigManager &configManager);

    void loadAll();
    std::vector<std::pair<std::string, std::string>> matchMarkets();
    
    // New arbitrage finder method
    std::vector<ArbitrageOpportunity> findArbitrageOpportunities(double minEdge = 0.03);
};
