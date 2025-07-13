// MarketMatcher.cpp

#include <iostream>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <limits>
#include <mutex>
#include <curl/curl.h>
#include "market/market_matcher.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

// Helper for libcurl to accumulate response
size_t MarketMatcher::writeCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

// Extract YYYY-MM-DD from ISO timestamp
std::string MarketMatcher::dateOnly(const std::string &iso)
{
    size_t tPos = iso.find('T');
    if (tPos != std::string::npos)
        return iso.substr(0, tPos);
    return iso;
}

// Parse date string like "2024-01-15" to day number since epoch
int MarketMatcher::parseDate(const std::string &iso)
{
    std::string dateStr = dateOnly(iso);
    if (dateStr.length() >= 10)
    {
        int year = std::stoi(dateStr.substr(0, 4));
        int month = std::stoi(dateStr.substr(5, 2));
        int day = std::stoi(dateStr.substr(8, 2));
        int days = (year - 1970) * 365 + (month - 1) * 30 + day;
        return days;
    }
    return 0;
}

// Normalize text for matching
std::string MarketMatcher::normalizeText(const std::string &text)
{
    std::string normalized = text;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    std::replace(normalized.begin(), normalized.end(), '-', ' ');
    std::replace(normalized.begin(), normalized.end(), '_', ' ');
    auto new_end = std::unique(normalized.begin(), normalized.end(),
                               [](char a, char b)
                               { return std::isspace(a) && std::isspace(b); });
    normalized.erase(new_end, normalized.end());
    normalized.erase(0, normalized.find_first_not_of(" \t\n\r\f\v"));
    normalized.erase(normalized.find_last_not_of(" \t\n\r\f\v") + 1);
    return normalized;
}

// Convert slug to question
std::string MarketMatcher::slugToQuestion(const std::string &slug)
{
    std::string normalized = normalizeText(slug);
    if (normalized.find("will ") == 0)
        return normalized;
    if (normalized.find("up or down") != std::string::npos)
        return "will " + normalized + "?";
    if (normalized.find("vs") != std::string::npos || normalized.find("versus") != std::string::npos)
        return "will " + normalized + " win?";
    if (normalized.find("beat") != std::string::npos)
        return "will " + normalized + "?";
    if (normalized.find("between") != std::string::npos && normalized.find("and") != std::string::npos)
        return "will " + normalized + "?";
    if (normalized.find("greater than") != std::string::npos || normalized.find("less than") != std::string::npos)
        return "will " + normalized + "?";
    if (normalized.find("on ") != std::string::npos && normalized.find("et") != std::string::npos)
        return "will " + normalized + "?";
    return "will " + normalized + " happen?";
}

// Fetch single embedding
std::vector<double> MarketMatcher::getEmbedding(const std::string &text)
{
    std::cout << "[Embedding] Starting embedding request for text: "
              << (text.size() > 50 ? text.substr(0, 50) + "..." : text) << std::endl;
    const char *api_key = std::getenv("OPENAI_API_KEY");
    if (!api_key)
        throw std::runtime_error("OPENAI_API_KEY not set");
    CURL *curl = curl_easy_init();
    if (!curl)
        throw std::runtime_error("Failed to init curl");
    std::string readBuffer;
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/embeddings");
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    json body = {{"model", "text-embedding-ada-002"}, {"input", text}};
    std::string bodyStr = body.dump();
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + std::string(api_key)).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bodyStr.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK)
        throw std::runtime_error("curl_easy_perform() failed");
    auto resp = json::parse(readBuffer);
    auto arr = resp["data"][0]["embedding"];
    return {arr.begin(), arr.end()};
}

// Fetch embedding with logging (thread-safe)
std::vector<double> MarketMatcher::getEmbeddingWithLogging(const std::string &text, int index, int total)
{
    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "[Embedding] [" << index << "/" << total << "] "
                  << "requesting: " << (text.size() > 50 ? text.substr(0, 50) + "..." : text)
                  << std::endl;
    }
    return getEmbedding(text);
}

// Batch embeddings
std::vector<std::vector<double>> MarketMatcher::getBatchEmbeddings(const std::vector<std::string> &texts)
{
    if (texts.empty())
        return {};
    constexpr size_t MAX_BATCH = 100;
    std::vector<std::vector<double>> all;
    for (size_t i = 0; i < texts.size(); i += MAX_BATCH)
    {
        auto begin = texts.begin() + i;
        auto end = (i + MAX_BATCH < texts.size() ? begin + MAX_BATCH : texts.end());
        std::vector<std::string> batch(begin, end);
        auto sub = getBatchEmbeddingsSingle(batch);
        all.insert(all.end(), sub.begin(), sub.end());
    }
    return all;
}

// Single batch call
std::vector<std::vector<double>> MarketMatcher::getBatchEmbeddingsSingle(const std::vector<std::string> &texts)
{
    const char *api_key = std::getenv("OPENAI_API_KEY");
    if (!api_key)
        throw std::runtime_error("OPENAI_API_KEY not set");
    CURL *curl = curl_easy_init();
    if (!curl)
        throw std::runtime_error("Failed to init curl");
    std::string readBuffer;
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/embeddings");
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    json body = {{"model", "text-embedding-ada-002"}, {"input", texts}};
    std::string bodyStr = body.dump();
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + std::string(api_key)).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bodyStr.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK)
        throw std::runtime_error("curl_easy_perform() failed");
    auto resp = json::parse(readBuffer);
    std::vector<std::vector<double>> embs;
    for (auto &item : resp["data"])
    {
        auto arr = item["embedding"];
        embs.emplace_back(arr.begin(), arr.end());
    }
    return embs;
}

// Cosine similarity
double MarketMatcher::cosineSimilarity(const std::vector<double> &A,
                                       const std::vector<double> &B)
{
    double dot = 0, nA = 0, nB = 0;
    for (size_t i = 0; i < A.size() && i < B.size(); ++i)
    {
        dot += A[i] * B[i];
        nA += A[i] * A[i];
        nB += B[i] * B[i];
    }
    return (nA && nB) ? dot / (std::sqrt(nA) * std::sqrt(nB)) : 0.0;
}

// Constructor
MarketMatcher::MarketMatcher(polymarket_bot::api::PolymarketApiClient polyClient,
                             polymarket_bot::api::OddsApiClient oddsClient,
                             const polymarket_bot::config::ConfigManager &configManager)
    : polyClient(polyClient), oddsClient(oddsClient), configManager(configManager), maxConcurrentRequests(5)
{
}


void MarketMatcher::loadAll()
{
    bool more = true;
    int page = 1;
    while (more)
    {
        auto resp = polyClient.getGammaMarkets(page, 100);
        if (resp.markets.empty())
            more = false;
        else
        {
            gammaMarkets.insert(gammaMarkets.end(), resp.markets.begin(), resp.markets.end());
            ++page;
        }
    }
    std::cout << "Loaded " << gammaMarkets.size() << " Γ markets\n";
    oddsGames = oddsClient.fetchOdds(configManager.getSports());
    std::cout << "Loaded " << oddsGames.size() << " odds games\n";
}

// Interactive matching with top-5 choices
std::vector<std::pair<std::string, std::string>> MarketMatcher::matchMarkets()
{
    std::cout << "[MarketMatcher] Starting interactive matching...\n";

    // Prepare embeddings (batch) as before...
    struct Prep
    {
        const polymarket_bot::common::GammaMarket *gm;
        std::vector<double> emb;
        int endDay;
    };
    std::vector<Prep> prepared;
    std::vector<std::string> texts;
    std::vector<const polymarket_bot::common::GammaMarket *> ptrs;
    for (auto &g : gammaMarkets)
        if (g.slug && g.endDateIso)
        {
            texts.push_back(slugToQuestion(*g.slug));
            ptrs.push_back(&g);
        }
    auto batch = getBatchEmbeddings(texts);
    for (size_t i = 0; i < batch.size(); ++i)
        prepared.push_back({ptrs[i], std::move(batch[i]), parseDate(*ptrs[i]->endDateIso)});

    // Embed games
    std::vector<std::vector<double>> gameEmbs;
    std::vector<int> gameDays;
    for (auto &gm : oddsGames)
    {
        gameEmbs.push_back(getBatchEmbeddings({normalizeText(
            gm.away_team + " vs " + gm.home_team + " on " + dateOnly(gm.commence_time))})[0]);
        gameDays.push_back(parseDate(gm.commence_time));
    }

    std::vector<std::pair<std::string, std::string>> results;
    for (size_t i = 0; i < oddsGames.size(); ++i)
    {
        auto &game = oddsGames[i];
        auto &gev = gameEmbs[i];
        int gDay = gameDays[i];
        // score all
        std::vector<std::pair<double, const Prep *>> scores;
        for (auto &p : prepared)
            if (std::abs(gDay - p.endDay) <= 2)
                scores.emplace_back(cosineSimilarity(p.emb, gev), &p);
        std::sort(scores.begin(), scores.end(), [](auto &a, auto &b)
                  { return a.first > b.first; });
        
        // Filter to show top 5 and any with score > 0.80
        std::vector<std::pair<double, const Prep *>> filteredScores;
        
        // Add top 5
        size_t topCount = std::min(scores.size(), size_t(5));
        for (size_t i = 0; i < topCount; ++i) {
            filteredScores.push_back(scores[i]);
        }
        
        // Add any with score > 0.80 that aren't already in top 5
        for (const auto& score : scores) {
            if (score.first > 0.80) {
                bool alreadyIncluded = false;
                for (const auto& filtered : filteredScores) {
                    if (filtered.second == score.second) {
                        alreadyIncluded = true;
                        break;
                    }
                }
                if (!alreadyIncluded) {
                    filteredScores.push_back(score);
                }
            }
        }
        
        // Sort by score again
        std::sort(filteredScores.begin(), filteredScores.end(), [](auto &a, auto &b)
                  { return a.first > b.first; });
        
        scores = std::move(filteredScores);

        // prompt
        std::cout << "\nGame: " << game.away_team << " vs " << game.home_team
                  << " on " << dateOnly(game.commence_time) << "\n";
        std::cout << "Showing top 5 matches and any with score > 0.80\n";
        std::cout << " 0) [No match]\n";
        for (size_t j = 0; j < scores.size(); ++j) {
            std::string scoreNote = "";
            if (scores[j].first > 0.80) {
                scoreNote = " (HIGH SCORE)";
            }
            std::cout << " " << (j + 1) << ") " << *scores[j].second->gm->slug
                      << " (score=" << scores[j].first << ")" << scoreNote << "\n";
        }
        std::cout << "Select (0-" << scores.size() << "): ";
        int choice;
        if (!(std::cin >> choice))
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            choice = 0;
        }
        if (choice >= 1 && choice <= (int)scores.size())
        {
            auto sel = scores[choice - 1].second;
            std::cout << "Selected: " << *sel->gm->slug << "\n";
            results.emplace_back(*sel->gm->id, game.id);
        }
        else
        {
            std::cout << "Skipped.\n";
        }
    }

    std::cout << "Matching done. Total: " << results.size() << "\n";
    return results;
}

// Arbitrage calculation helpers

double MarketMatcher::calculateImpliedProbability(double decimalOdds) {
    if (decimalOdds <= 1.0) return 0.0;
    return 1.0 / decimalOdds;
}

double MarketMatcher::calculatePolymarketProbability(double polymarketPrice) {
    // Polymarket prices are already in probability format (0.615 = 61.5%)
    return polymarketPrice;
}

double MarketMatcher::calculateEdge(double prob1, double prob2) {
    if (prob1 <= 0.0 || prob2 <= 0.0) return 0.0;
    return std::abs(prob1 - prob2) / std::min(prob1, prob2);
}

std::string MarketMatcher::determineRecommendedAction(double polymarketProb, double oddsProb) {
    if (polymarketProb > oddsProb) {
        return "BUY_POLYMARKET";  // Polymarket odds are higher, so buy there
    } else {
        return "BUY_ODDS";        // Sportsbook odds are higher, so buy there
    }
}

double MarketMatcher::calculateOptimalStake(double edge, double totalStake) {
    // Simple proportional stake based on edge
    // In a real implementation, you might use Kelly Criterion or other methods
    return totalStake * edge;
}

std::vector<ArbitrageOpportunity> MarketMatcher::findArbitrageOpportunities(double minEdge) {
    std::cout << "[ArbitrageFinder] Starting arbitrage analysis with minimum edge: " 
              << (minEdge * 100) << "%" << std::endl;
    
    std::vector<ArbitrageOpportunity> opportunities;
    
    // First, get matched markets
    auto matchedMarkets = matchMarkets();
    std::cout << "[ArbitrageFinder] Found " << matchedMarkets.size() << " matched markets" << std::endl;
    
    for (const auto& match : matchedMarkets) {
        const std::string& polymarketId = match.first;
        const std::string& oddsId = match.second;
        
        // Find the corresponding market data
        const polymarket_bot::common::GammaMarket* polyMarket = nullptr;
        const polymarket_bot::common::RawOddsGame* oddsGame = nullptr;
        
        // Find Polymarket market
        for (const auto& gm : gammaMarkets) {
            if (gm.id && *gm.id == polymarketId) {
                polyMarket = &gm;
                break;
            }
        }
        
        // Find Odds game
        for (const auto& og : oddsGames) {
            if (og.id == oddsId) {
                oddsGame = &og;
                break;
            }
        }
        
        if (!polyMarket || !oddsGame) {
            std::cout << "[ArbitrageFinder] Warning: Could not find market data for match " 
                      << polymarketId << " <-> " << oddsId << std::endl;
            continue;
        }
        
        std::cout << "[ArbitrageFinder] Analyzing match: " << polymarketId << " <-> " << oddsId << std::endl;
        if (polyMarket->slug) {
            std::cout << "[ArbitrageFinder] Polymarket market: " << *polyMarket->slug << std::endl;
        }
        std::cout << "[ArbitrageFinder] Odds game: " << oddsGame->away_team << " vs " << oddsGame->home_team << std::endl;
        
        // Parse Polymarket outcomes and prices
        std::vector<std::pair<std::string, double>> polyOutcomes;
        if (polyMarket->outcomes && polyMarket->outcomePrices) {
            std::cout << "[ArbitrageFinder] Raw Polymarket outcomes: " << *polyMarket->outcomes << std::endl;
            std::cout << "[ArbitrageFinder] Raw Polymarket prices: " << *polyMarket->outcomePrices << std::endl;
            
            try {
                auto outcomesJson = nlohmann::json::parse(*polyMarket->outcomes);
                auto pricesJson = nlohmann::json::parse(*polyMarket->outcomePrices);
                
                if (outcomesJson.is_array() && pricesJson.is_array() && 
                    outcomesJson.size() == pricesJson.size()) {
                    for (size_t i = 0; i < outcomesJson.size(); ++i) {
                        std::string outcome = outcomesJson[i];
                        // Handle both string and number price formats
                        double price;
                        if (pricesJson[i].is_string()) {
                            price = std::stod(pricesJson[i].get<std::string>());
                        } else {
                            price = pricesJson[i].get<double>();
                        }
                        polyOutcomes.emplace_back(outcome, price);
                    }
                }
            } catch (const std::exception& e) {
                std::cout << "[ArbitrageFinder] Error parsing Polymarket data: " << e.what() << std::endl;
                continue;
            }
        } else {
            std::cout << "[ArbitrageFinder] Warning: Missing Polymarket outcomes or prices data" << std::endl;
        }
        
        // Parse Odds outcomes - prefer Pinnacle, fallback to others
        std::vector<std::pair<std::string, double>> oddsOutcomes;
        bool foundPinnacle = false;
        
        // First pass: look for Pinnacle
        for (const auto& bookmaker : oddsGame->bookmakers) {
            if (bookmaker.key == "pinnacle") {
                for (const auto& market : bookmaker.markets) {
                    if (market.key == "h2h") {  // Head-to-head markets
                        for (const auto& outcome : market.outcomes) {
                            oddsOutcomes.emplace_back(outcome.name, outcome.price);
                        }
                        foundPinnacle = true;
                        break;
                    }
                }
                if (foundPinnacle) break;
            }
        }
        
        // Second pass: if no Pinnacle, use first available bookmaker
        if (!foundPinnacle) {
            for (const auto& bookmaker : oddsGame->bookmakers) {
                for (const auto& market : bookmaker.markets) {
                    if (market.key == "h2h") {  // Head-to-head markets
                        for (const auto& outcome : market.outcomes) {
                            oddsOutcomes.emplace_back(outcome.name, outcome.price);
                        }
                        std::cout << "[ArbitrageFinder] Using " << bookmaker.key 
                                  << " odds (Pinnacle not available)" << std::endl;
                        break;
                    }
                }
                if (!oddsOutcomes.empty()) break;
            }
        } else {
            std::cout << "[ArbitrageFinder] Using Pinnacle odds" << std::endl;
        }
        
        if (oddsOutcomes.empty()) {
            std::cout << "[ArbitrageFinder] Warning: No odds outcomes found" << std::endl;
            std::cout << "[ArbitrageFinder] Available bookmakers:" << std::endl;
            for (const auto& bookmaker : oddsGame->bookmakers) {
                std::cout << "  - " << bookmaker.key << " (" << bookmaker.title << ")" << std::endl;
                for (const auto& market : bookmaker.markets) {
                    std::cout << "    Market: " << market.key << " with " << market.outcomes.size() << " outcomes" << std::endl;
                }
            }
        }
        
        std::cout << "[ArbitrageFinder] Found " << polyOutcomes.size() << " Polymarket outcomes and " 
                  << oddsOutcomes.size() << " Odds outcomes" << std::endl;
        
        // Interactive outcome matching for arbitrage
        std::cout << "[ArbitrageFinder] Interactive outcome matching:" << std::endl;
        std::cout << "Polymarket outcomes:" << std::endl;
        for (size_t i = 0; i < polyOutcomes.size(); ++i) {
            std::cout << "  " << (i + 1) << ") " << polyOutcomes[i].first << " @ " << polyOutcomes[i].second << std::endl;
        }
        std::cout << "Odds outcomes:" << std::endl;
        for (size_t i = 0; i < oddsOutcomes.size(); ++i) {
            std::cout << "  " << (i + 1) << ") " << oddsOutcomes[i].first << " @ " << oddsOutcomes[i].second << std::endl;
        }
        std::cout << std::endl;
        
        // Store manual matches (single and combined)
        std::vector<std::pair<size_t, std::vector<size_t>>> manualMatches;
        
        std::cout << "Enter outcome matches:" << std::endl;
        std::cout << "Format: poly_index odds_indices (comma-separated)" << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  1 2 (matches Polymarket 1 with Odds 2)" << std::endl;
        std::cout << "  1 2,3 (matches Polymarket 1 with combined Odds 2+3)" << std::endl;
        std::cout << "  0 0 (skip)" << std::endl;
        
        while (true) {
            std::cout << "Match (poly odds): ";
            int polyIndex;
            if (!(std::cin >> polyIndex)) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                break;
            }
            
            if (polyIndex == 0) {
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                break;
            }
            
            std::string oddsInput;
            std::getline(std::cin >> std::ws, oddsInput);
            
            if (oddsInput == "0") {
                break;
            }
            
            // Parse comma-separated odds indices
            std::vector<size_t> oddsIndices;
            std::stringstream ss(oddsInput);
            std::string token;
            while (std::getline(ss, token, ',')) {
                try {
                    int index = std::stoi(token);
                    if (index > 0 && index <= (int)oddsOutcomes.size()) {
                        oddsIndices.push_back(index - 1);
                    } else {
                        std::cout << "Invalid odds index: " << index << std::endl;
                        continue;
                    }
                } catch (const std::exception& e) {
                    std::cout << "Invalid input: " << token << std::endl;
                    continue;
                }
            }
            
            if (polyIndex > 0 && polyIndex <= (int)polyOutcomes.size() && !oddsIndices.empty()) {
                manualMatches.emplace_back(polyIndex - 1, oddsIndices);
                
                std::cout << "Matched: '" << polyOutcomes[polyIndex - 1].first << "' with: ";
                for (size_t i = 0; i < oddsIndices.size(); ++i) {
                    if (i > 0) std::cout << " + ";
                    std::cout << "'" << oddsOutcomes[oddsIndices[i]].first << "'";
                }
                std::cout << std::endl;
            } else {
                std::cout << "Invalid indices. Poly: 1-" << polyOutcomes.size() 
                          << ", Odds: 1-" << oddsOutcomes.size() << std::endl;
            }
        }
        
        // Process manual matches
        for (const auto& match : manualMatches) {
            size_t polyIdx = match.first;
            const std::vector<size_t>& oddsIndices = match.second;
            
            const auto& polyOutcome = polyOutcomes[polyIdx];
            
            // Calculate combined odds probability for multiple outcomes
            double combinedOddsProb = 0.0;
            std::string combinedOutcomeName = "";
            
            for (size_t oddsIdx : oddsIndices) {
                const auto& oddsOutcome = oddsOutcomes[oddsIdx];
                double oddsProb = calculateImpliedProbability(oddsOutcome.second);
                combinedOddsProb += oddsProb;
                
                if (!combinedOutcomeName.empty()) combinedOutcomeName += " + ";
                combinedOutcomeName += oddsOutcome.first;
            }
            
            double polyProb = calculatePolymarketProbability(polyOutcome.second);
            double edge = calculateEdge(polyProb, combinedOddsProb);
            
            std::cout << "[ArbitrageFinder] MANUAL MATCH:" << std::endl;
            std::cout << "  Polymarket: " << polyOutcome.first << " @ " << polyOutcome.second 
                      << " (implied prob: " << (polyProb * 100) << "%)" << std::endl;
            std::cout << "  Odds (combined): " << combinedOutcomeName 
                      << " (implied prob: " << (combinedOddsProb * 100) << "%)" << std::endl;
            std::cout << "  Edge: " << (edge * 100) << "%" << std::endl;
            
            if (edge >= minEdge) {
                ArbitrageOpportunity opp;
                opp.polymarketId = polymarketId;
                opp.polymarketSlug = polyMarket->slug ? *polyMarket->slug : "Unknown";
                opp.oddsId = oddsId;
                opp.oddsGame = oddsGame->away_team + " vs " + oddsGame->home_team;
                opp.outcome = polyOutcome.first;
                opp.polymarketPrice = polyOutcome.second;
                // For combined outcomes, use the first odds price as representative
                opp.oddsPrice = oddsOutcomes[oddsIndices[0]].second;
                opp.edge = edge;
                opp.impliedProbability = polyProb + combinedOddsProb;
                opp.recommendedAction = determineRecommendedAction(polyProb, combinedOddsProb);
                opp.recommendedStake = calculateOptimalStake(edge);
                
                opportunities.push_back(opp);
                
                std::cout << "  *** ARBITRAGE OPPORTUNITY DETECTED ***" << std::endl;
                std::cout << "  Market: " << opp.polymarketSlug << std::endl;
                std::cout << "  Game: " << opp.oddsGame << std::endl;
                std::cout << "  Recommended Action: " << opp.recommendedAction << std::endl;
                std::cout << "  Recommended Stake: $" << opp.recommendedStake << std::endl;
            } else {
                std::cout << "  Edge too small (min required: " << (minEdge * 100) << "%)" << std::endl;
            }
            std::cout << "  ---" << std::endl;
        }
        
        // Also try automatic matching for comparison
        std::cout << "[ArbitrageFinder] Automatic matching results:" << std::endl;
        for (const auto& polyOutcome : polyOutcomes) {
            for (const auto& oddsOutcome : oddsOutcomes) {
                // Simple text matching for outcomes
                std::string polyOutcomeLower = polyOutcome.first;
                std::string oddsOutcomeLower = oddsOutcome.first;
                std::transform(polyOutcomeLower.begin(), polyOutcomeLower.end(), 
                              polyOutcomeLower.begin(), ::tolower);
                std::transform(oddsOutcomeLower.begin(), oddsOutcomeLower.end(), 
                              oddsOutcomeLower.begin(), ::tolower);
                
                // Check if outcomes match (simple contains check)
                bool outcomesMatch = false;
                if (polyOutcomeLower.find(oddsOutcomeLower) != std::string::npos ||
                    oddsOutcomeLower.find(polyOutcomeLower) != std::string::npos) {
                    outcomesMatch = true;
                }
                
                if (outcomesMatch) {
                    double polyProb = calculatePolymarketProbability(polyOutcome.second);
                    double oddsProb = calculateImpliedProbability(oddsOutcome.second);
                    double edge = calculateEdge(polyProb, oddsProb);
                    
                    std::cout << "[ArbitrageFinder] AUTO MATCH FOUND:" << std::endl;
                    std::cout << "  Polymarket: " << polyOutcome.first << " @ " << polyOutcome.second 
                              << " (implied prob: " << (polyProb * 100) << "%)" << std::endl;
                    std::cout << "  Odds: " << oddsOutcome.first << " @ " << oddsOutcome.second 
                              << " (implied prob: " << (oddsProb * 100) << "%)" << std::endl;
                    std::cout << "  Edge: " << (edge * 100) << "%" << std::endl;
                    
                    if (edge >= minEdge) {
                        ArbitrageOpportunity opp;
                        opp.polymarketId = polymarketId;
                        opp.polymarketSlug = polyMarket->slug ? *polyMarket->slug : "Unknown";
                        opp.oddsId = oddsId;
                        opp.oddsGame = oddsGame->away_team + " vs " + oddsGame->home_team;
                        opp.outcome = polyOutcome.first;
                        opp.polymarketPrice = polyOutcome.second;
                        opp.oddsPrice = oddsOutcome.second;
                        opp.edge = edge;
                        opp.impliedProbability = polyProb + oddsProb;
                        opp.recommendedAction = determineRecommendedAction(polyProb, oddsProb);
                        opp.recommendedStake = calculateOptimalStake(edge);
                        
                        opportunities.push_back(opp);
                        
                        std::cout << "  *** ARBITRAGE OPPORTUNITY DETECTED ***" << std::endl;
                        std::cout << "  Market: " << opp.polymarketSlug << std::endl;
                        std::cout << "  Game: " << opp.oddsGame << std::endl;
                        std::cout << "  Recommended Action: " << opp.recommendedAction << std::endl;
                        std::cout << "  Recommended Stake: $" << opp.recommendedStake << std::endl;
                    } else {
                        std::cout << "  Edge too small (min required: " << (minEdge * 100) << "%)" << std::endl;
                    }
                    std::cout << "  ---" << std::endl;
                }
            }
        }
    }
    
    std::cout << "[ArbitrageFinder] Analysis complete. Found " << opportunities.size() 
              << " arbitrage opportunities with edge >= " << (minEdge * 100) << "%" << std::endl;
    
    return opportunities;
}
