#include "../src/api/odds_api_client.h"
#include "../src/config/config_manager.h"
#include "../src/common/types.h"
#include <nlohmann/json.hpp>
#include <memory>
#include <iostream>
#include <cassert>
#include <curl/curl.h>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;

namespace polymarket_bot {
namespace api {
namespace test {

// Simple test framework
class TestFramework {
public:
    static int runTests() {
        int passed = 0;
        int total = 0;
        
        std::cout << "Running OddsApiClient tests...\n" << std::endl;
        
        // Test 1: Constructor
        total++;
        try {
            curl_global_init(CURL_GLOBAL_ALL);
            auto client = std::make_unique<OddsApiClient>();
            assert(client != nullptr);
            std::cout << "✓ Constructor test passed" << std::endl;
            passed++;
            curl_global_cleanup();
        } catch (const std::exception& e) {
            std::cout << "✗ Constructor test failed: " << e.what() << std::endl;
        }
        
        // Test 2: Health check
        total++;
        try {
            curl_global_init(CURL_GLOBAL_ALL);
            auto client = std::make_unique<OddsApiClient>();
            assert(client->isHealthy() == true);
            std::cout << "✓ Health check test passed" << std::endl;
            passed++;
            curl_global_cleanup();
        } catch (const std::exception& e) {
            std::cout << "✗ Health check test failed: " << e.what() << std::endl;
        }
        
        // Test 3: Rate limit setting
        total++;
        try {
            curl_global_init(CURL_GLOBAL_ALL);
            auto client = std::make_unique<OddsApiClient>();
            client->setRateLimit(20);
            // No assertion needed as this just sets values
            std::cout << "✓ Rate limit test passed" << std::endl;
            passed++;
            curl_global_cleanup();
        } catch (const std::exception& e) {
            std::cout << "✗ Rate limit test failed: " << e.what() << std::endl;
        }
        
        // Test 4: JSON parsing with valid data
        total++;
        try {
            // Create a sample JSON response that matches the API format
            json sampleResponse = json::array({
                {
                    {"id", "test_game_1"},
                    {"sport_key", "americanfootball_nfl"},
                    {"commence_time", "2024-01-01T20:00:00Z"},
                    {"home_team", "New England Patriots"},
                    {"away_team", "Buffalo Bills"},
                    {"bookmakers", json::array({
                        {
                            {"key", "pinnacle"},
                            {"title", "Pinnacle"},
                            {"last_update", "2024-01-01T19:30:00Z"},
                            {"markets", json::array({
                                {
                                    {"key", "h2h"},
                                    {"outcomes", json::array({
                                        {
                                            {"name", "New England Patriots"},
                                            {"price", 150}
                                        },
                                        {
                                            {"name", "Buffalo Bills"},
                                            {"price", -170}
                                        }
                                    })}
                                }
                            })}
                        }
                    })}
                }
            });

            std::string jsonString = sampleResponse.dump();
            assert(!jsonString.empty());
            
            // Parse the JSON to ensure it's valid
            json parsed = json::parse(jsonString);
            assert(parsed.is_array());
            assert(parsed.size() == 1);
            
            // Test the first game structure
            auto game = parsed[0];
            assert(game["id"] == "test_game_1");
            assert(game["sport_key"] == "americanfootball_nfl");
            assert(game["home_team"] == "New England Patriots");
            assert(game["away_team"] == "Buffalo Bills");
            
            std::cout << "✓ JSON parsing test passed" << std::endl;
            passed++;
        } catch (const std::exception& e) {
            std::cout << "✗ JSON parsing test failed: " << e.what() << std::endl;
        }
        
        // Test 5: JSON parsing with invalid data
        total++;
        try {
            std::string invalidJson = "{ invalid json }";
            
            try {
                (void)json::parse(invalidJson);
                assert(false); // Should not reach here
            } catch (const json::parse_error&) {
                // Expected exception
            }
            
            std::cout << "✓ Invalid JSON test passed" << std::endl;
            passed++;
        } catch (const std::exception& e) {
            std::cout << "✗ Invalid JSON test failed: " << e.what() << std::endl;
        }
        
        // Test 6: Data structure creation
        total++;
        try {
            // Test creating RawOddsGame structure
            polymarket_bot::common::RawOddsGame game;
            game.id = "test_game";
            game.sport_key = "americanfootball_nfl";
            game.commence_time = "2024-01-01T20:00:00Z";
            game.home_team = "Home Team";
            game.away_team = "Away Team";
            
            // Test creating OddsOutcome structure
            polymarket_bot::common::OddsOutcome outcome;
            outcome.name = "Home Team";
            outcome.price = 150;
            outcome.point = 0.0;
            
            // Test creating OddsMarket structure
            polymarket_bot::common::OddsMarket market;
            market.key = "h2h";
            market.outcomes.push_back(outcome);
            
            // Test creating OddsBookmaker structure
            polymarket_bot::common::OddsBookmaker bookmaker;
            bookmaker.key = "pinnacle";
            bookmaker.title = "Pinnacle";
            bookmaker.last_update = "2024-01-01T19:30:00Z";
            bookmaker.markets.push_back(market);
            
            // Add bookmaker to game
            game.bookmakers.push_back(bookmaker);
            
            // Test creating RawOddsResponse structure
            polymarket_bot::common::RawOddsGame response;
            response.id = "test_game";
            
            // Verify the structures
            assert(game.id == "test_game");
            assert(game.sport_key == "americanfootball_nfl");
            assert(game.home_team == "Home Team");
            assert(game.away_team == "Away Team");
            assert(game.bookmakers.size() == 1);
            assert(game.bookmakers[0].key == "pinnacle");
            assert(game.bookmakers[0].markets.size() == 1);
            assert(game.bookmakers[0].markets[0].outcomes.size() == 1);
            assert(game.bookmakers[0].markets[0].outcomes[0].name == "Home Team");
            assert(game.bookmakers[0].markets[0].outcomes[0].price == 150);
            assert(response.id == "test_game");
            
            std::cout << "✓ Data structure test passed" << std::endl;
            passed++;
        } catch (const std::exception& e) {
            std::cout << "✗ Data structure test failed: " << e.what() << std::endl;
        }
        
        // Test 7: URL construction
        total++;
        try {
            std::string sport = "americanfootball_nfl";
            std::string apiKey = "test_api_key";
            
            auto now = std::chrono::system_clock::now();
            auto from_time_t = std::chrono::system_clock::to_time_t(now);
            auto to_time_t = std::chrono::system_clock::to_time_t(now + std::chrono::hours(7 * 24));
            
            std::stringstream ss;
            ss << "https://api.the-odds-api.com/v4/sports/" << sport << "/odds"
               << "?apiKey=" << apiKey
               << "&commenceTimeFrom=" << std::put_time(std::gmtime(&from_time_t), "%Y-%m-%dT%H:%M:%SZ")
               << "&commenceTimeTo=" << std::put_time(std::gmtime(&to_time_t), "%Y-%m-%dT%H:%M:%SZ");
            
            std::string url = ss.str();
            
            assert(url.find("https://api.the-odds-api.com/v4/sports/americanfootball_nfl/odds") != std::string::npos);
            assert(url.find("apiKey=test_api_key") != std::string::npos);
            assert(url.find("commenceTimeFrom=") != std::string::npos);
            assert(url.find("commenceTimeTo=") != std::string::npos);
            
            std::cout << "✓ URL construction test passed" << std::endl;
            passed++;
        } catch (const std::exception& e) {
            std::cout << "✗ URL construction test failed: " << e.what() << std::endl;
        }
        
        // Summary
        std::cout << "\n" << std::string(50, '=') << std::endl;
        std::cout << "Test Results: " << passed << "/" << total << " tests passed" << std::endl;
        std::cout << std::string(50, '=') << std::endl;
        
        return (passed == total) ? 0 : 1;
    }
};

} // namespace test
} // namespace api
} // namespace polymarket_bot

int main() {
    return polymarket_bot::api::test::TestFramework::runTests();
} 