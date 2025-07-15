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
    initializeTeamMappings();
}


void MarketMatcher::loadAll()
{
    // Only load odds games - we'll fetch Polymarket markets individually by slug
    oddsGames = oddsClient.fetchOdds(configManager.getSports());
    std::cout << "Loaded " << oddsGames.size() << " odds games\n";
}

// Initialize team mappings for NBA, NHL, and MLB
void MarketMatcher::initializeTeamMappings()
{
    // NBA Teams
    nbaTeams["Atlanta Hawks"] = {"Atlanta Hawks", "atl", "Atlanta Hawks"};
    nbaTeams["Boston Celtics"] = {"Boston Celtics", "bos", "Boston Celtics"};
    nbaTeams["Brooklyn Nets"] = {"Brooklyn Nets", "bkn", "Brooklyn Nets"};
    nbaTeams["Charlotte Hornets"] = {"Charlotte Hornets", "cha", "Charlotte Hornets"};
    nbaTeams["Chicago Bulls"] = {"Chicago Bulls", "chi", "Chicago Bulls"};
    nbaTeams["Cleveland Cavaliers"] = {"Cleveland Cavaliers", "cle", "Cleveland Cavaliers"};
    nbaTeams["Dallas Mavericks"] = {"Dallas Mavericks", "dal", "Dallas Mavericks"};
    nbaTeams["Denver Nuggets"] = {"Denver Nuggets", "den", "Denver Nuggets"};
    nbaTeams["Detroit Pistons"] = {"Detroit Pistons", "det", "Detroit Pistons"};
    nbaTeams["Golden State Warriors"] = {"Golden State Warriors", "gsw", "Golden State Warriors"};
    nbaTeams["Houston Rockets"] = {"Houston Rockets", "hou", "Houston Rockets"};
    nbaTeams["Indiana Pacers"] = {"Indiana Pacers", "ind", "Indiana Pacers"};
    nbaTeams["LA Clippers"] = {"LA Clippers", "lac", "LA Clippers"};
    nbaTeams["Los Angeles Clippers"] = {"Los Angeles Clippers", "lac", "Los Angeles Clippers"};
    nbaTeams["LA Lakers"] = {"LA Lakers", "lal", "LA Lakers"};
    nbaTeams["Los Angeles Lakers"] = {"Los Angeles Lakers", "lal", "Los Angeles Lakers"};
    nbaTeams["Memphis Grizzlies"] = {"Memphis Grizzlies", "mem", "Memphis Grizzlies"};
    nbaTeams["Miami Heat"] = {"Miami Heat", "mia", "Miami Heat"};
    nbaTeams["Milwaukee Bucks"] = {"Milwaukee Bucks", "mil", "Milwaukee Bucks"};
    nbaTeams["Minnesota Timberwolves"] = {"Minnesota Timberwolves", "min", "Minnesota Timberwolves"};
    nbaTeams["New Orleans Pelicans"] = {"New Orleans Pelicans", "nop", "New Orleans Pelicans"};
    nbaTeams["New York Knicks"] = {"New York Knicks", "nyk", "New York Knicks"};
    nbaTeams["Oklahoma City Thunder"] = {"Oklahoma City Thunder", "okc", "Oklahoma City Thunder"};
    nbaTeams["Orlando Magic"] = {"Orlando Magic", "orl", "Orlando Magic"};
    nbaTeams["Philadelphia 76ers"] = {"Philadelphia 76ers", "phi", "Philadelphia 76ers"};
    nbaTeams["Phoenix Suns"] = {"Phoenix Suns", "phx", "Phoenix Suns"};
    nbaTeams["Portland Trail Blazers"] = {"Portland Trail Blazers", "por", "Portland Trail Blazers"};
    nbaTeams["Sacramento Kings"] = {"Sacramento Kings", "sac", "Sacramento Kings"};
    nbaTeams["San Antonio Spurs"] = {"San Antonio Spurs", "sas", "San Antonio Spurs"};
    nbaTeams["Toronto Raptors"] = {"Toronto Raptors", "tor", "Toronto Raptors"};
    nbaTeams["Utah Jazz"] = {"Utah Jazz", "uta", "Utah Jazz"};
    nbaTeams["Washington Wizards"] = {"Washington Wizards", "was", "Washington Wizards"};

    // NHL Teams
    nhlTeams["Anaheim Ducks"] = {"Anaheim Ducks", "ana", "Anaheim Ducks"};
    nhlTeams["Arizona Coyotes"] = {"Arizona Coyotes", "ari", "Arizona Coyotes"};
    nhlTeams["Boston Bruins"] = {"Boston Bruins", "bos", "Boston Bruins"};
    nhlTeams["Buffalo Sabres"] = {"Buffalo Sabres", "buf", "Buffalo Sabres"};
    nhlTeams["Calgary Flames"] = {"Calgary Flames", "cgy", "Calgary Flames"};
    nhlTeams["Carolina Hurricanes"] = {"Carolina Hurricanes", "car", "Carolina Hurricanes"};
    nhlTeams["Chicago Blackhawks"] = {"Chicago Blackhawks", "chi", "Chicago Blackhawks"};
    nhlTeams["Colorado Avalanche"] = {"Colorado Avalanche", "col", "Colorado Avalanche"};
    nhlTeams["Columbus Blue Jackets"] = {"Columbus Blue Jackets", "cbj", "Columbus Blue Jackets"};
    nhlTeams["Dallas Stars"] = {"Dallas Stars", "dal", "Dallas Stars"};
    nhlTeams["Detroit Red Wings"] = {"Detroit Red Wings", "det", "Detroit Red Wings"};
    nhlTeams["Edmonton Oilers"] = {"Edmonton Oilers", "edm", "Edmonton Oilers"};
    nhlTeams["Florida Panthers"] = {"Florida Panthers", "fla", "Florida Panthers"};
    nhlTeams["Los Angeles Kings"] = {"Los Angeles Kings", "lak", "Los Angeles Kings"};
    nhlTeams["Minnesota Wild"] = {"Minnesota Wild", "min", "Minnesota Wild"};
    nhlTeams["Montreal Canadiens"] = {"Montreal Canadiens", "mtl", "Montreal Canadiens"};
    nhlTeams["Nashville Predators"] = {"Nashville Predators", "nsh", "Nashville Predators"};
    nhlTeams["New Jersey Devils"] = {"New Jersey Devils", "njd", "New Jersey Devils"};
    nhlTeams["New York Islanders"] = {"New York Islanders", "nyi", "New York Islanders"};
    nhlTeams["New York Rangers"] = {"New York Rangers", "nyr", "New York Rangers"};
    nhlTeams["Ottawa Senators"] = {"Ottawa Senators", "ott", "Ottawa Senators"};
    nhlTeams["Philadelphia Flyers"] = {"Philadelphia Flyers", "phi", "Philadelphia Flyers"};
    nhlTeams["Pittsburgh Penguins"] = {"Pittsburgh Penguins", "pit", "Pittsburgh Penguins"};
    nhlTeams["San Jose Sharks"] = {"San Jose Sharks", "sjs", "San Jose Sharks"};
    nhlTeams["Seattle Kraken"] = {"Seattle Kraken", "sea", "Seattle Kraken"};
    nhlTeams["St. Louis Blues"] = {"St. Louis Blues", "stl", "St. Louis Blues"};
    nhlTeams["Tampa Bay Lightning"] = {"Tampa Bay Lightning", "tbl", "Tampa Bay Lightning"};
    nhlTeams["Toronto Maple Leafs"] = {"Toronto Maple Leafs", "tor", "Toronto Maple Leafs"};
    nhlTeams["Vancouver Canucks"] = {"Vancouver Canucks", "van", "Vancouver Canucks"};
    nhlTeams["Vegas Golden Knights"] = {"Vegas Golden Knights", "vgk", "Vegas Golden Knights"};
    nhlTeams["Washington Capitals"] = {"Washington Capitals", "was", "Washington Capitals"};
    nhlTeams["Winnipeg Jets"] = {"Winnipeg Jets", "wpg", "Winnipeg Jets"};

    // MLB Teams
    mlbTeams["Arizona Diamondbacks"] = {"Arizona Diamondbacks", "ari", "Arizona Diamondbacks"};
    mlbTeams["Atlanta Braves"] = {"Atlanta Braves", "atl", "Atlanta Braves"};
    mlbTeams["Baltimore Orioles"] = {"Baltimore Orioles", "bal", "Baltimore Orioles"};
    mlbTeams["Boston Red Sox"] = {"Boston Red Sox", "bos", "Boston Red Sox"};
    mlbTeams["Chicago Cubs"] = {"Chicago Cubs", "chc", "Chicago Cubs"};
    mlbTeams["Chicago White Sox"] = {"Chicago White Sox", "cws", "Chicago White Sox"};
    mlbTeams["Cincinnati Reds"] = {"Cincinnati Reds", "cin", "Cincinnati Reds"};
    mlbTeams["Cleveland Guardians"] = {"Cleveland Guardians", "cle", "Cleveland Guardians"};
    mlbTeams["Colorado Rockies"] = {"Colorado Rockies", "col", "Colorado Rockies"};
    mlbTeams["Detroit Tigers"] = {"Detroit Tigers", "det", "Detroit Tigers"};
    mlbTeams["Houston Astros"] = {"Houston Astros", "hou", "Houston Astros"};
    mlbTeams["Kansas City Royals"] = {"Kansas City Royals", "kan", "Kansas City Royals"};
    mlbTeams["Los Angeles Angels"] = {"Los Angeles Angels", "laa", "Los Angeles Angels"};
    mlbTeams["Los Angeles Dodgers"] = {"Los Angeles Dodgers", "lad", "Los Angeles Dodgers"};
    mlbTeams["Miami Marlins"] = {"Miami Marlins", "mia", "Miami Marlins"};
    mlbTeams["Milwaukee Brewers"] = {"Milwaukee Brewers", "mil", "Milwaukee Brewers"};
    mlbTeams["Minnesota Twins"] = {"Minnesota Twins", "min", "Minnesota Twins"};
    mlbTeams["New York Mets"] = {"New York Mets", "nym", "New York Mets"};
    mlbTeams["New York Yankees"] = {"New York Yankees", "nyy", "New York Yankees"};
    mlbTeams["Oakland Athletics"] = {"Oakland Athletics", "oak", "Oakland Athletics"};
    mlbTeams["Philadelphia Phillies"] = {"Philadelphia Phillies", "phi", "Philadelphia Phillies"};
    mlbTeams["Pittsburgh Pirates"] = {"Pittsburgh Pirates", "pit", "Pittsburgh Pirates"};
    mlbTeams["San Diego Padres"] = {"San Diego Padres", "sd", "San Diego Padres"};
    mlbTeams["San Francisco Giants"] = {"San Francisco Giants", "sf", "San Francisco Giants"};
    mlbTeams["Seattle Mariners"] = {"Seattle Mariners", "sea", "Seattle Mariners"};
    mlbTeams["St. Louis Cardinals"] = {"St. Louis Cardinals", "stl", "St. Louis Cardinals"};
    mlbTeams["Tampa Bay Rays"] = {"Tampa Bay Rays", "tb", "Tampa Bay Rays"};
    mlbTeams["Texas Rangers"] = {"Texas Rangers", "tex", "Texas Rangers"};
    mlbTeams["Toronto Blue Jays"] = {"Toronto Blue Jays", "tor", "Toronto Blue Jays"};
    mlbTeams["Washington Nationals"] = {"Washington Nationals", "was", "Washington Nationals"};

    std::cout << "[MarketMatcher] Initialized team mappings: " 
              << nbaTeams.size() << " NBA, " 
              << nhlTeams.size() << " NHL, " 
              << mlbTeams.size() << " MLB teams" << std::endl;
}

// Format date for slug (YYYY-MM-DD)
std::string MarketMatcher::formatDateForSlug(const std::string &iso)
{
    return dateOnly(iso);
}

// Generate slug for a game based on sport, teams, and date
std::string MarketMatcher::generateSlugForGame(const polymarket_bot::common::RawOddsGame& game)
{
    std::string sport = game.sport_key;
    std::string awayTeam = game.away_team;
    std::string homeTeam = game.home_team;
    std::string gameDate = formatDateForSlug(game.commence_time);
    
    // Determine sport prefix
    std::string sportPrefix;
    if (sport == "basketball_nba" || sport == "basketball_nba_summer_league") {
        sportPrefix = "nba";
    } else if (sport == "icehockey_nhl") {
        sportPrefix = "nhl";
    } else if (sport == "baseball_mlb") {
        sportPrefix = "mlb";
    } else {
        std::cout << "[MarketMatcher] Unsupported sport: " << sport << std::endl;
        return ""; // Unsupported sport
    }
    
    // Get team codes
    std::string awayCode, homeCode;
    std::unordered_map<std::string, TeamMapping>* teamMap = nullptr;
    
    if (sportPrefix == "nba") {
        teamMap = &nbaTeams;
    } else if (sportPrefix == "nhl") {
        teamMap = &nhlTeams;
    } else if (sportPrefix == "mlb") {
        teamMap = &mlbTeams;
    }
    
    if (!teamMap) {
        return "";
    }
    
    // Find team codes
    auto awayIt = teamMap->find(awayTeam);
    auto homeIt = teamMap->find(homeTeam);
    
    if (awayIt == teamMap->end() || homeIt == teamMap->end()) {
        std::cout << "[MarketMatcher] Warning: Could not find team mapping for "
                  << awayTeam << " or " << homeTeam << " in " << sportPrefix << std::endl;
        return "";
    }
    
    awayCode = awayIt->second.polymarketCode;
    homeCode = homeIt->second.polymarketCode;
    
    // Generate slug: sport-away-home-date
    std::string slug = sportPrefix + "-" + awayCode + "-" + homeCode + "-" + gameDate;
    
    std::cout << "[MarketMatcher] Generated slug: " << slug 
              << " for " << awayTeam << " vs " << homeTeam 
              << " on " << gameDate << std::endl;
    
    return slug;
}

// Find Polymarket market by slug
std::string MarketMatcher::findPolymarketMarketBySlug(const std::string& slug)
{
    for (const auto& market : gammaMarkets) {
        if (market.slug && *market.slug == slug) {
            if (market.id) {
                return *market.id;
            }
        }
    }
    return "";
}

// New slug-based matching method
std::vector<std::pair<std::string, std::string>> MarketMatcher::matchMarketsBySlug()
{
    std::cout << "[MarketMatcher] Starting slug-based matching..." << std::endl;
    
    std::vector<std::pair<std::string, std::string>> results;
    int matchedCount = 0;
    int totalGames = 0;
    
    for (const auto& game : oddsGames) {
        totalGames++;
        
        // Generate slug for this game
        std::string slug = generateSlugForGame(game);
        if (slug.empty()) {
            std::cout << "[MarketMatcher] Skipping game: " << game.away_team 
                      << " vs " << game.home_team << " (unsupported sport or missing team mapping)" << std::endl;
            continue;
        }
        
        // Fetch market directly by slug from Polymarket API
        auto market = fetchMarketBySlug(slug);
        if (market.has_value()) {
            if (market->id) {
                results.emplace_back(*market->id, game.id);
                matchedCount++;
                std::cout << "[MarketMatcher] ✓ Matched: " << slug << " -> " << *market->id << std::endl;
            }
        } else {
            std::cout << "[MarketMatcher] ✗ No market found for slug: " << slug << std::endl;
        }
    }
    
    std::cout << "[MarketMatcher] Slug-based matching complete. "
              << "Matched " << matchedCount << " out of " << totalGames << " games." << std::endl;
    
    return results;
}

// Fetch market by slug directly from Polymarket API
std::optional<polymarket_bot::common::GammaMarket> MarketMatcher::fetchMarketBySlug(const std::string& slug)
{
    // Helper function to try a specific slug
    auto trySlug = [this](const std::string& testSlug) -> std::optional<polymarket_bot::common::GammaMarket> {
        try {
            std::string url = "https://gamma-api.polymarket.com/markets?slug=" + testSlug;
            
            CURL *curl = curl_easy_init();
            if (!curl) {
                return std::nullopt;
            }
            
            std::string readBuffer;
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            
            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            
            if (res != CURLE_OK) {
                return std::nullopt;
            }
            
            // Parse the response
            try {
                auto jsonResponse = nlohmann::json::parse(readBuffer);
                
                // The API returns an array directly, not nested under "markets"
                if (jsonResponse.is_array() && jsonResponse.size() > 0) {
                    // Parse the first market
                    polymarket_bot::common::GammaMarket market;
                    from_json(jsonResponse[0], market);
                    std::cout << "[MarketMatcher] ✓ Found market for slug: " << testSlug << " (ID: " << (market.id ? *market.id : "unknown") << ")" << std::endl;
                    return market;
                }
                
                return std::nullopt;
                
            } catch (const std::exception& e) {
                return std::nullopt;
            }
            
        } catch (const std::exception& e) {
            return std::nullopt;
        }
    };
    
    // Try the original slug first
    auto result = trySlug(slug);
    if (result.has_value()) {
        return result;
    }
    
    // If not found, try variations with different dates and team orders
    std::cout << "[MarketMatcher] Trying variations for slug: " << slug << std::endl;
    
    // Parse the original slug to extract components
    // Format: sport-away-home-date
    size_t firstDash = slug.find('-');
    if (firstDash == std::string::npos) return std::nullopt;
    
    size_t secondDash = slug.find('-', firstDash + 1);
    if (secondDash == std::string::npos) return std::nullopt;
    
    size_t thirdDash = slug.find('-', secondDash + 1);
    if (thirdDash == std::string::npos) return std::nullopt;
    
    std::string sport = slug.substr(0, firstDash);
    std::string awayTeam = slug.substr(firstDash + 1, secondDash - firstDash - 1);
    std::string homeTeam = slug.substr(secondDash + 1, thirdDash - secondDash - 1);
    std::string date = slug.substr(thirdDash + 1);
    
    // Parse the date
    int year, month, day;
    if (sscanf(date.c_str(), "%d-%d-%d", &year, &month, &day) != 3) {
        return std::nullopt;
    }
    
    // Try variations:
    // 1. Day before
    std::string dayBefore = sport + "-" + awayTeam + "-" + homeTeam + "-" + 
                           std::to_string(year) + "-" + 
                           std::string(month < 10 ? "0" : "") + std::to_string(month) + "-" + 
                           std::string((day-1) < 10 ? "0" : "") + std::to_string(day-1);
    result = trySlug(dayBefore);
    if (result.has_value()) {
        std::cout << "[MarketMatcher] Found market with day before variation: " << dayBefore << std::endl;
        return result;
    }
    
    // 2. Day after
    std::string dayAfter = sport + "-" + awayTeam + "-" + homeTeam + "-" + 
                          std::to_string(year) + "-" + 
                          std::string(month < 10 ? "0" : "") + std::to_string(month) + "-" + 
                          std::string((day+1) < 10 ? "0" : "") + std::to_string(day+1);
    result = trySlug(dayAfter);
    if (result.has_value()) {
        std::cout << "[MarketMatcher] Found market with day after variation: " << dayAfter << std::endl;
        return result;
    }
    
    // 3. Swapped team order (home vs away)
    std::string swappedOrder = sport + "-" + homeTeam + "-" + awayTeam + "-" + date;
    result = trySlug(swappedOrder);
    if (result.has_value()) {
        std::cout << "[MarketMatcher] Found market with swapped team order: " << swappedOrder << std::endl;
        return result;
    }
    
    // 4. Swapped team order with day before
    std::string swappedDayBefore = sport + "-" + homeTeam + "-" + awayTeam + "-" + 
                                  std::to_string(year) + "-" + 
                                  std::string(month < 10 ? "0" : "") + std::to_string(month) + "-" + 
                                  std::string((day-1) < 10 ? "0" : "") + std::to_string(day-1);
    result = trySlug(swappedDayBefore);
    if (result.has_value()) {
        std::cout << "[MarketMatcher] Found market with swapped order and day before: " << swappedDayBefore << std::endl;
        return result;
    }
    
    // 5. Swapped team order with day after
    std::string swappedDayAfter = sport + "-" + homeTeam + "-" + awayTeam + "-" + 
                                 std::to_string(year) + "-" + 
                                 std::string(month < 10 ? "0" : "") + std::to_string(month) + "-" + 
                                 std::string((day+1) < 10 ? "0" : "") + std::to_string(day+1);
    result = trySlug(swappedDayAfter);
    if (result.has_value()) {
        std::cout << "[MarketMatcher] Found market with swapped order and day after: " << swappedDayAfter << std::endl;
        return result;
    }
    
    std::cout << "[MarketMatcher] No market found for slug: " << slug << " (tried all variations)" << std::endl;
    return std::nullopt;
}

// Slug-based matching (replaces interactive matching)
std::vector<std::pair<std::string, std::string>> MarketMatcher::matchMarkets()
{
    return matchMarketsBySlug();
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
    // Get bankroll from environment variable
    const char* bankrollEnv = std::getenv("BANKROLL");
    double bankroll = 10000.0; // Default bankroll if not set
    
    if (bankrollEnv) {
        try {
            bankroll = std::stod(bankrollEnv);
        } catch (const std::exception& e) {
            std::cout << "[MarketMatcher] Warning: Invalid BANKROLL environment variable, using default: " << bankroll << std::endl;
        }
    } else {
        std::cout << "[MarketMatcher] Warning: BANKROLL environment variable not set, using default: " << bankroll << std::endl;
    }
    
    // Kelly Criterion: f = (bp - q) / b
    // where:
    // f = fraction of bankroll to bet
    // b = net odds received on the bet (decimal odds - 1)
    // p = probability of winning
    // q = probability of losing (1 - p)
    
    // For arbitrage, we need to calculate Kelly for both sides
    // We'll use the edge to determine the optimal fraction
    
    if (edge <= 0.0) {
        return 0.0; // No edge, no bet
    }
    
    // Conservative Kelly: use a fraction of the full Kelly to reduce risk
    // Common practice is to use 1/4 to 1/2 of full Kelly
    const double kellyFraction = 0.25; // 25% of full Kelly
    
    // Calculate Kelly fraction based on edge
    // For arbitrage, the edge represents the opportunity
    double kellyFractionOfBankroll = edge * kellyFraction;
    
    // Cap the maximum bet at 5% of bankroll for risk management
    const double maxBetFraction = 0.05;
    kellyFractionOfBankroll = std::min(kellyFractionOfBankroll, maxBetFraction);
    
    double recommendedStake = bankroll * kellyFractionOfBankroll;
    
    // Ensure minimum stake of $10
    recommendedStake = std::max(recommendedStake, 10.0);
    
    std::cout << "[MarketMatcher] Kelly calculation:" << std::endl;
    std::cout << "  Bankroll: $" << bankroll << std::endl;
    std::cout << "  Edge: " << (edge * 100) << "%" << std::endl;
    std::cout << "  Kelly fraction: " << (kellyFractionOfBankroll * 100) << "%" << std::endl;
    std::cout << "  Recommended stake: $" << recommendedStake << std::endl;
    
    return recommendedStake;
}

std::vector<ArbitrageOpportunity> MarketMatcher::findArbitrageOpportunities(double minEdge) {
    std::cout << "[ArbitrageFinder] Starting arbitrage analysis..." << std::endl;
    
    std::vector<ArbitrageOpportunity> opportunities;
    
    // Get matched markets using slug-based matching
    auto matchedMarkets = matchMarketsBySlug();
    std::cout << "[ArbitrageFinder] Found " << matchedMarkets.size() << " matched markets" << std::endl;
    
    if (matchedMarkets.empty()) {
        std::cout << "[ArbitrageFinder] No matched markets found. Cannot analyze arbitrage." << std::endl;
        return opportunities;
    }
    
    for (const auto& match : matchedMarkets) {
        const std::string& polymarketId = match.first;
        const std::string& oddsId = match.second;
        
        // Find the corresponding odds game
        const polymarket_bot::common::RawOddsGame* oddsGame = nullptr;
        for (const auto& game : oddsGames) {
            if (game.id == oddsId) {
                oddsGame = &game;
                break;
            }
        }
        
        if (!oddsGame) {
            std::cout << "[ArbitrageFinder] Warning: Could not find odds game for ID: " << oddsId << std::endl;
            continue;
        }
        
        // Fetch the Polymarket market data
        std::string slug = generateSlugForGame(*oddsGame);
        auto polymarketMarket = fetchMarketBySlug(slug);
        
        if (!polymarketMarket.has_value()) {
            std::cout << "[ArbitrageFinder] Warning: Could not fetch Polymarket market for slug: " << slug << std::endl;
            continue;
        }
        
        std::cout << "\n[ArbitrageFinder] Analyzing: " << oddsGame->away_team << " vs " << oddsGame->home_team << std::endl;
        std::cout << "Polymarket Market ID: " << (polymarketMarket->id ? *polymarketMarket->id : "unknown") << std::endl;
        
        // Parse Polymarket outcomes and prices
        std::vector<std::pair<std::string, double>> polyOutcomes;
        if (polymarketMarket->outcomes && polymarketMarket->outcomePrices) {
            try {
                auto outcomesJson = nlohmann::json::parse(*polymarketMarket->outcomes);
                auto pricesJson = nlohmann::json::parse(*polymarketMarket->outcomePrices);
                
                if (outcomesJson.is_array() && pricesJson.is_array() && 
                    outcomesJson.size() == pricesJson.size()) {
                    
                    for (size_t i = 0; i < outcomesJson.size(); ++i) {
                        std::string outcome = outcomesJson[i].get<std::string>();
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
            }
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
            continue;
        }
        
        std::cout << "[ArbitrageFinder] Found " << polyOutcomes.size() << " Polymarket outcomes and " 
                  << oddsOutcomes.size() << " Odds outcomes" << std::endl;
        
        // Simple outcome matching based on team names
        for (const auto& polyOutcome : polyOutcomes) {
            for (const auto& oddsOutcome : oddsOutcomes) {
                // Simple matching - check if team names are similar
                std::string polyTeam = polyOutcome.first;
                std::string oddsTeam = oddsOutcome.first;
                
                // Normalize team names for comparison
                std::transform(polyTeam.begin(), polyTeam.end(), polyTeam.begin(), ::tolower);
                std::transform(oddsTeam.begin(), oddsTeam.end(), oddsTeam.begin(), ::tolower);
                
                // Remove common words
                std::vector<std::string> commonWords = {"team", "the", "and", "&"};
                for (const auto& word : commonWords) {
                    size_t pos = polyTeam.find(word);
                    if (pos != std::string::npos) {
                        polyTeam.erase(pos, word.length());
                    }
                    pos = oddsTeam.find(word);
                    if (pos != std::string::npos) {
                        oddsTeam.erase(pos, word.length());
                    }
                }
                
                // Check if teams match (simple substring matching)
                bool teamsMatch = false;
                if (polyTeam.find(oddsTeam) != std::string::npos || 
                    oddsTeam.find(polyTeam) != std::string::npos) {
                    teamsMatch = true;
                }
                
                if (teamsMatch) {
                    double polyProb = calculatePolymarketProbability(polyOutcome.second);
                    double oddsProb = calculateImpliedProbability(oddsOutcome.second);
                    double edge = calculateEdge(polyProb, oddsProb);
                    
                    std::cout << "[ArbitrageFinder] MATCH FOUND:" << std::endl;
                    std::cout << "  Polymarket: " << polyOutcome.first << " @ " << polyOutcome.second 
                              << " (implied prob: " << (polyProb * 100) << "%)" << std::endl;
                    std::cout << "  Odds: " << oddsOutcome.first << " @ " << oddsOutcome.second 
                              << " (implied prob: " << (oddsProb * 100) << "%)" << std::endl;
                    std::cout << "  Edge: " << (edge * 100) << "%" << std::endl;
                    
                    // Create arbitrage opportunity (regardless of edge)
                    ArbitrageOpportunity opp;
                    opp.polymarketId = polymarketMarket->id ? *polymarketMarket->id : "";
                    opp.polymarketSlug = slug;
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
                    
                    if (edge >= minEdge) {
                        std::cout << "  *** ARBITRAGE OPPORTUNITY DETECTED ***" << std::endl;
                        std::cout << "  Market: " << opp.polymarketSlug << std::endl;
                        std::cout << "  Game: " << opp.oddsGame << std::endl;
                        std::cout << "  Recommended Action: " << opp.recommendedAction << std::endl;
                        std::cout << "  Recommended Stake: $" << opp.recommendedStake << std::endl;
                    } else {
                        std::cout << "  Edge too small (min required: " << (minEdge * 100) << "%)" << std::endl;
                    }
                    std::cout << std::endl;
                }
            }
        }
    }
    
    std::cout << "[ArbitrageFinder] Analysis complete. Found " << opportunities.size() 
              << " arbitrage opportunities (all edges listed)" << std::endl;
    
    return opportunities;
}
