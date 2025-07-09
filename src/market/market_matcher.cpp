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
        if (scores.size() > 5)
            scores.resize(5);

        // prompt
        std::cout << "\nGame: " << game.away_team << " vs " << game.home_team
                  << " on " << dateOnly(game.commence_time) << "\n";
        std::cout << " 0) [No match]\n";
        for (size_t j = 0; j < scores.size(); ++j)
            std::cout << " " << (j + 1) << ") " << *scores[j].second->gm->slug
                      << " (score=" << scores[j].first << ")\n";
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
