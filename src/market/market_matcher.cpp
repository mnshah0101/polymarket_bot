#include <iostream>
#include <cstdlib>
#include "api/odds_api_client.h"
#include "api/polymarket_api_client.h"
#include "config/config_manager.h"

class MarketMatcher {
    private:
        polymarket_bot::api::PolymarketApiClient polyClient;
        polymarket_bot::api::OddsApiClient oddsClient;
        polymarket_bot::config::ConfigManager configManager;
        std::vector<polymarket_bot::common::RawOddsGame> oddsGames;
        std::vector<polymarket_bot::common::GammaMarket> gammaMarkets;

    public:
        MarketMatcher(polymarket_bot::api::PolymarketApiClient polyClient, polymarket_bot::api::OddsApiClient oddsClient, polymarket_bot::config::ConfigManager configManager);
        ~MarketMatcher();

        void getAllPolymarketMarkets(){
            bool hasMore = true;
            int page = 1;
            while (hasMore) {
                auto markets = polyClient.getGammaMarkets(page, 100);
                if (markets.markets.empty()) {
                    hasMore = false;
                } else {
                    gammaMarkets.insert(gammaMarkets.end(), markets.markets.cbegin(), markets.markets.cend());
                    page++;
                }
            }
            std::cout << "Found " << gammaMarkets.size() << " gamma markets" << std::endl;
        };
        void getAllBettingMarkets(){
            oddsGames = oddsClient.fetchOdds(configManager.getSports());
            std::cout << "Found " << oddsGames.size() << " odds games" << std::endl;
            std::cout << "Odds games: " << oddsGames.size() << std::endl;
            return;

        };


        void matchMarkets();
        //needs to ingest gamma markets and polymarket markets and return a list of pairs of markets that are the same
        //lets decide matching logic
};