# 🤖 polymarket bot

a c++ bot that automatically spots and takes advantage of price differences between traditional sportsbooks and polymarket. think of it as your personal arbitrage sidekick.

---

## what’s going on

1. **watch** odds on the odds api and polymarket  
2. **spot** when there’s a way to bet both sides and lock in profit  
3. **bet** automatically when the numbers check out  
4. **record** every move so you can geek out on your winning stats

## tech stuff

- **c++17** – fast and efficient  
- **libcurl** – for talking to web apis  
- **nlohmann/json** – because json is life  
- **sqlite** – tiny database to keep your history  
- **cmake** – makes building painless

## getting started

1. clone and build:
   ```bash
   git clone https://github.com/yourusername/polymarket_bot.git
   cd polymarket_bot
   mkdir build && cd build
   cmake ..
   make
````

2. set your keys:

   ```bash
   export ODDS_API_KEY="your_odds_api_key"
   export POLYMARKET_PRIVATE_KEY="your_polymarket_key"
   ```
3. fire it up:

   ```bash
   ./bin/polymarket_bot
   ```

## configuration

edit `config/config.json` to tweak what you monitor and how you size bets:

```json
{
  "apis": {
    "oddsApi": {
      "baseUrl": "https://api.the-odds-api.com/v4",
      "apiKey": "${ODDS_API_KEY}"
    },
    "polymarket": {
      "baseUrl": "https://clob.polymarket.com",
      "privateKey": "${POLYMARKET_PRIVATE_KEY}",
      "chainId": 137
    }
  },
  "sports": ["americanfootball_nfl", "basketball_nba", "baseball_mlb"],
  "kelly": {
    "fractionOfKelly": 0.25,
    "minEdge": 0.02,
    "maxPositionSize": 0.05
  }
}
```

* **sports**: list any you’re into
* **fractionofkelly**: how aggressively you bet
* **minedge**: minimum profit margin to pull the trigger
* **maxpositionsize**: caps your bet size

## testing

run all tests:

```bash
ctest --verbose
```

or try a specific suite:

```bash
./polymarket_bot_simple_tests
./polymarket_bot_gtest
```

## a few tips

* environment variables only—no secrets in code
* keep an eye on rate limits so you don’t get blocked
* check your logs regularly to see how it’s doing



