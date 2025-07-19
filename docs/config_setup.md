# Configuration Setup Guide

## Overview

The Polymarket Bot uses a JSON configuration file with environment variable substitution. Environment variables are expected to be set in the Docker container, eliminating the need for .env files.

## Configuration File Structure

The main configuration file is located at `config/config.json` and contains the following structure:

```json
{
  "apis": {
    "oddsApi": {
      "baseUrl": "https://api.the-odds-api.com/v4",
      "rateLimitPerMinute": 60
    },
    "polymarket": {
      "baseUrl": "https://clob.polymarket.com",
      "chainId": 137
    }
  },
  "database": {
    "path": "/data/polymarket_bot.db",
    "backupEnabled": true,
    "backupInterval": 3600
  },
  "sharpBooks": [
    "pinnacle",
    "bet365",
    "draftkings"
  ],
  "sports": [
    "basketball_nba",
    "basketball_ncaab",
    "football_nfl",
    "football_ncaaf"
  ],
  "kelly": {
    "fractionOfKelly": 0.25,
    "minEdge": 0.02,
    "maxPositionSize": 0.1
  },
  "risk": {
    "maxDrawdown": 0.1,
    "maxDailyTrades": 50,
    "maxDailyVolume": 1000.0,
    "circuitBreakerEnabled": true
  },
  "matching": {
    "minConfidenceScore": 0.8,
    "maxTimeDifference": 300
  },
  "sync": {
    "positionSyncInterval": 60,
    "accountSyncInterval": 300,
    "priceUpdateInterval": 30
  }
}
```

## Environment Variables

API keys and private keys are read directly from environment variables and should not be stored in the configuration file. This ensures sensitive credentials are not committed to version control.

### Required Environment Variables

```bash
# API Keys
ODDS_API_KEY=your_odds_api_key_here

# Polymarket Headers
POLY_ADDRESS=your_polygon_address_here
POLY_TIMESTAMP=your_unix_timestamp_here
POLY_API_KEY=your_polymarket_api_key_here
POLY_PASSPHRASE=your_polymarket_passphrase_here

# Optional: Override default values
DATABASE_PATH=/custom/path/to/database.db
LOG_LEVEL=INFO
```

### Docker Environment Setup

When running the bot in Docker, set environment variables using the `-e` flag or an environment file:

```bash
# Using command line
docker run -e ODDS_API_KEY=your_key -e POLY_ADDRESS=your_address -e POLY_TIMESTAMP=your_timestamp -e POLY_API_KEY=your_api_key -e POLY_PASSPHRASE=your_passphrase polymarket_bot

# Using environment file
docker run --env-file .env.docker polymarket_bot

Example `.env.docker` file:
```env
ODDS_API_KEY=your_odds_api_key_here
POLY_ADDRESS=your_polygon_address_here
POLY_TIMESTAMP=your_unix_timestamp_here
POLY_API_KEY=your_polymarket_api_key_here
POLY_PASSPHRASE=your_polymarket_passphrase_here
DATABASE_PATH=/data/polymarket_bot.db
LOG_LEVEL=INFO
```

## Configuration Sections

### APIs
- **oddsApi**: Configuration for the odds API service
  - `baseUrl`: API base URL
  - `rateLimitPerMinute`: Rate limiting configuration
  - API key is read from `ODDS_API_KEY` environment variable

- **polymarket**: Configuration for Polymarket integration
  - `baseUrl`: Polymarket API base URL
  - `chainId`: Blockchain network ID
  - Headers are read from environment variables:
    - `POLY_ADDRESS`: Polygon address
    - `POLY_TIMESTAMP`: Current UNIX timestamp
    - `POLY_API_KEY`: Polymarket API key
    - `POLY_PASSPHRASE`: Polymarket API key passphrase

### Database
- `path`: Database file path
- `backupEnabled`: Whether to enable automatic backups
- `backupInterval`: Backup interval in seconds

### Trading Configuration
- **sharpBooks**: List of trusted sportsbooks for odds comparison
- **sports**: List of sports to monitor
- **kelly**: Kelly criterion parameters
  - `fractionOfKelly`: Fraction of Kelly to use (0.0-1.0)
  - `minEdge`: Minimum edge required for trades
  - `maxPositionSize`: Maximum position size as fraction of bankroll

### Risk Management
- `maxDrawdown`: Maximum allowed drawdown (0.0-1.0)
- `maxDailyTrades`: Maximum number of trades per day
- `maxDailyVolume`: Maximum trading volume per day
- `circuitBreakerEnabled`: Whether to enable circuit breaker

### Market Matching
- `minConfidenceScore`: Minimum confidence score for market matching (0.0-1.0)
- `maxTimeDifference`: Maximum time difference for price matching (seconds)

### Sync Intervals
- `positionSyncInterval`: How often to sync positions (seconds)
- `accountSyncInterval`: How often to sync account balance (seconds)
- `priceUpdateInterval`: How often to update prices (seconds)

## Usage in Code

```cpp
#include "src/config/config_manager.h"

// Get configuration manager instance
auto& configManager = polymarket_bot::config::ConfigManager::getInstance();

// Load configuration
if (!configManager.loadConfig("config/config.json")) {
    std::cerr << "Failed to load config: " << configManager.getLastError() << std::endl;
    return 1;
}

// Access configuration values
std::string apiKey = configManager.getOddsApiKey();
double kellyFraction = configManager.getKellyFraction();
double maxDrawdown = configManager.getMaxDrawdown();
```

## Validation

The configuration system includes built-in validation:

- Required fields are checked
- Numeric values are validated for reasonable ranges
- API credentials are verified to be present
- Environment variables are validated to exist

## Error Handling

If configuration loading fails, check:

1. **File not found**: Ensure `config/config.json` exists
2. **Invalid JSON**: Validate JSON syntax
3. **Missing environment variables**: Check that required environment variables are set
4. **Validation errors**: Review error messages for specific validation failures

## Security Considerations

- Never commit API keys or private keys to version control
- Use Docker secrets or environment variables for sensitive data
- Ensure proper file permissions on configuration files
- Consider using encrypted configuration for production deployments

## Example Docker Compose

```yaml
version: '3.8'
services:
  polymarket-bot:
    build: .
    environment:
      - ODDS_API_KEY=${ODDS_API_KEY}
      - POLYMARKET_PRIVATE_KEY=${POLYMARKET_PRIVATE_KEY}
      - DATABASE_PATH=/data/polymarket_bot.db
      - LOG_LEVEL=INFO
    volumes:
      - ./data:/data
      - ./config:/app/config
    restart: unless-stopped
```

## Testing Configuration

Use the provided example to test your configuration:

```bash
# Compile and run the example
g++ -std=c++17 examples/config_usage_updated.cpp src/config/config_manager.cpp -o config_test
./config_test
```

This will load your configuration and display all the values, helping you verify that everything is set up correctly. 