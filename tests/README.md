# OddsApiClient Tests

This directory contains tests for the OddsApiClient class.

## Test Files

- `test_odds_api_client_simple.cpp` - Simple test suite using basic C++ assertions
- `test_odds_api_client.cpp` - Comprehensive test suite using Google Test framework

## Running Tests

### Simple Tests (Recommended)

The simple tests don't require any external testing framework and can be run immediately:

```bash
# Build the project
mkdir -p build && cd build
cmake ..
make

# Run the simple tests
./bin/polymarket_bot_tests
```

### Google Test Suite (Optional)

To run the comprehensive Google Test suite:

```bash
# Build with Google Test
mkdir -p build && cd build
cmake ..
make

# Run the tests
ctest --verbose
```

## Test Coverage

The tests cover the following functionality:

1. **Constructor Test** - Verifies the OddsApiClient can be instantiated
2. **Health Check Test** - Tests the `isHealthy()` method
3. **Rate Limit Test** - Tests rate limit configuration
4. **JSON Parsing Test** - Tests parsing of valid JSON responses
5. **Invalid JSON Test** - Tests error handling for malformed JSON
6. **Data Structure Test** - Tests creation and manipulation of data structures
7. **URL Construction Test** - Tests API URL generation

## Test Data

The tests use sample JSON data that matches the format returned by the Odds API:

```json
[
  {
    "id": "test_game_1",
    "sport_key": "americanfootball_nfl",
    "commence_time": "2024-01-01T20:00:00Z",
    "home_team": "New England Patriots",
    "away_team": "Buffalo Bills",
    "bookmakers": [
      {
        "key": "pinnacle",
        "title": "Pinnacle",
        "last_update": "2024-01-01T19:30:00Z",
        "markets": [
          {
            "key": "h2h",
            "outcomes": [
              {
                "name": "New England Patriots",
                "price": 150
              },
              {
                "name": "Buffalo Bills",
                "price": -170
              }
            ]
          }
        ]
      }
    ]
  }
]
```

## Adding New Tests

To add new tests:

1. For simple tests, add them to `test_odds_api_client_simple.cpp`
2. For Google Test framework, add them to `test_odds_api_client.cpp`
3. Follow the existing test patterns and naming conventions
4. Make sure to test both success and failure cases

## Dependencies

- libcurl (for HTTP requests)
- nlohmann/json (for JSON parsing)
- Google Test (optional, for comprehensive tests) 