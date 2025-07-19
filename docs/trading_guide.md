# Polymarket Trading Bot Guide

## Overview

The Polymarket Trading Bot now includes automated trading capabilities with:

- **Arbitrage Detection**: Finds profitable opportunities between Polymarket and sportsbooks
- **Risk Management**: SQL-based duplicate prevention and position limits
- **Trade Execution**: Automated order placement on Polymarket
- **Performance Tracking**: Visual CLI dashboard for monitoring results

## Quick Start

### 1. Set Environment Variables

```bash
export ODDS_API_KEY="your_odds_api_key"
export POLY_ADDRESS="your_polymarket_address"
export POLY_API_KEY="your_polymarket_api_key"
export POLY_PASSPHRASE="your_polymarket_passphrase"
export POLY_TIMESTAMP="current_unix_timestamp"
export BANKROLL="10000"  # Your trading bankroll in USD
```

### 2. Build with Trading Support

```bash
mkdir build && cd build
cmake ..
make
```

### 3. Run Trading Bot

```bash
# Interactive dashboard mode
./bin/polymarket_trading_bot --interactive

# Automated trading mode
./bin/polymarket_trading_bot

# Dry run mode (scan only, no trades)
./bin/polymarket_trading_bot --dry-run

# Custom scan interval
./bin/polymarket_trading_bot --interval 120  # 2 minutes
```

## Trading Components

### TradeExecutor (`src/trading/trade_executor.h/cpp`)
- Executes individual trades on Polymarket
- Validates trade parameters and limits
- Handles order creation and submission

### TradeManager (`src/trading/trade_manager.h/cpp`)
- SQL-based trade tracking and duplicate prevention
- Risk management and position limits
- Performance metrics calculation

### TradeDashboard (`src/cli/trade_dashboard.h/cpp`)
- Visual CLI interface for monitoring trades
- Real-time performance metrics
- Trade history and active positions display

## Database Schema

The bot uses SQLite to track trades and prevent duplicates:

### Key Tables:
- `executed_trades`: Complete trade history and results
- `trade_opportunities`: Duplicate prevention tracking
- `market_activity`: Daily trading limits per market
- `trade_performance`: Aggregated performance metrics

### Views:
- `daily_performance`: Daily P&L and win rates
- `market_summary`: Performance by market
- `active_trades`: Currently open positions

## Risk Management

### Position Limits:
- **Max per trade**: 5% of bankroll (default)
- **Max daily**: 20% of bankroll (default)
- **Min edge**: 3% (default)

### Duplicate Prevention:
- Hash-based opportunity tracking
- Prevents trading same market/outcome multiple times per day
- 24-hour cooldown on market/outcome combinations

### Market Limits:
- Maximum trades per market per day
- Stake limits per market
- Automatic cooldown periods

## Dashboard Features

### Interactive Mode Commands:
- `r`: Refresh dashboard
- `h`: Show trade history
- `p`: Show performance metrics
- `a`: Show active positions
- `q`: Quit

### Display Sections:
- **Portfolio Summary**: Balance, P&L, ROI, win rate
- **Recent Trades**: Latest trade executions and results
- **Daily Performance**: Day-by-day breakdown
- **Active Positions**: Currently open trades
- **Arbitrage Opportunities**: Real-time opportunity scanning

## Configuration

### Trading Parameters:
```bash
# Set custom limits (percentages of bankroll)
export MAX_STAKE_PER_TRADE=0.03  # 3% per trade
export MAX_DAILY_STAKE=0.15      # 15% per day
export MIN_EDGE=0.02             # 2% minimum edge
```

### Scan Settings:
- Default scan interval: 5 minutes
- Configurable via `--interval` parameter
- Rate limits respect Polymarket API constraints

## API Integration

### Polymarket APIs Used:
- **CLOB API**: Order placement and execution
- **Gamma API**: Market data and pricing
- **Data API**: Account balance and positions

### Rate Limits:
- Order placement: 500/10s (burst), 3000/10min
- Market data: Configurable, default 10/minute
- Automatic rate limiting and backoff

## Monitoring and Alerts

### Logging:
- Trade execution logs
- Error handling and recovery
- Performance metrics logging

### Status Monitoring:
- API health checks
- Database connectivity
- Trade execution success rates

## Safety Features

### Dry Run Mode:
- Test arbitrage detection without executing trades
- Validate configuration and connectivity
- Monitor opportunities without risk

### Circuit Breakers:
- Daily loss limits
- Maximum trade frequency
- Automatic shutdown on repeated failures

### Data Validation:
- Price verification before execution
- Market status checks
- Balance and position validation

## Troubleshooting

### Common Issues:

1. **Database errors**: Check `data/trades.db` permissions
2. **API failures**: Verify environment variables and connectivity
3. **No opportunities**: Adjust `MIN_EDGE` threshold
4. **Trade rejections**: Check position limits and balance

### Debug Mode:
```bash
# Enable verbose logging
CMAKE_BUILD_TYPE=Debug make
./bin/polymarket_trading_bot --dry-run
```

## Performance Optimization

### Recommended Settings:
- Scan interval: 60-300 seconds
- Max trades per day: 10-20
- Edge threshold: 2-5%

### Hardware Requirements:
- Minimal CPU/Memory requirements
- Stable internet connection essential
- SSD recommended for database performance

## Legal and Compliance

- Review Polymarket Terms of Service
- Ensure compliance with local regulations
- Monitor for any API usage violations
- Keep detailed records for tax/audit purposes

## Support

For issues with the trading bot:
1. Check logs in the console output
2. Verify environment variables
3. Test with `--dry-run` mode
4. Review database schema and data integrity