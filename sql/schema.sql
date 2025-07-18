-- Polymarket Trading Bot Database Schema
-- Trade management and duplicate prevention

-- Table to track executed arbitrage trades
CREATE TABLE IF NOT EXISTS executed_trades (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    trade_id TEXT UNIQUE NOT NULL,  -- Unique identifier for each trade
    polymarket_market_id TEXT NOT NULL,
    polymarket_slug TEXT NOT NULL,
    odds_game_id TEXT NOT NULL,
    outcome TEXT NOT NULL,  -- The specific outcome traded (e.g., "home_team", "away_team")
    
    -- Price data at time of trade
    polymarket_price REAL NOT NULL,
    odds_price REAL NOT NULL,
    edge_percentage REAL NOT NULL,
    
    -- Trade execution details
    recommended_action TEXT NOT NULL,  -- "BUY_POLYMARKET" or "BUY_ODDS"
    stake_amount REAL NOT NULL,
    expected_profit REAL NOT NULL,
    
    -- Polymarket order details
    polymarket_order_id TEXT,
    polymarket_order_status TEXT DEFAULT 'PENDING',  -- PENDING, FILLED, PARTIALLY_FILLED, CANCELLED, FAILED
    polymarket_fill_amount REAL DEFAULT 0,
    
    -- External sportsbook tracking (for manual trades)
    external_bet_placed BOOLEAN DEFAULT FALSE,
    external_bet_amount REAL DEFAULT 0,
    external_bet_odds REAL DEFAULT 0,
    
    -- Trade lifecycle timestamps
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    executed_at DATETIME,
    settled_at DATETIME,
    
    -- Final results (populated after settlement)
    final_polymarket_result TEXT,  -- WIN, LOSS, PUSH
    final_external_result TEXT,    -- WIN, LOSS, PUSH
    actual_profit REAL DEFAULT 0,
    
    -- Status tracking
    status TEXT DEFAULT 'PENDING' CHECK (status IN ('PENDING', 'EXECUTED', 'PARTIALLY_EXECUTED', 'SETTLED', 'FAILED'))
);

-- Table to prevent duplicate trades on the same opportunity
CREATE TABLE IF NOT EXISTS trade_opportunities (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    opportunity_hash TEXT UNIQUE NOT NULL,  -- Hash of key trade parameters
    polymarket_market_id TEXT NOT NULL,
    odds_game_id TEXT NOT NULL,
    outcome TEXT NOT NULL,
    first_seen DATETIME DEFAULT CURRENT_TIMESTAMP,
    last_seen DATETIME DEFAULT CURRENT_TIMESTAMP,
    times_seen INTEGER DEFAULT 1,
    status TEXT DEFAULT 'ACTIVE' CHECK (status IN ('ACTIVE', 'TRADED', 'EXPIRED'))
);

-- Table to track market activity and prevent over-trading
CREATE TABLE IF NOT EXISTS market_activity (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    polymarket_market_id TEXT NOT NULL,
    odds_game_id TEXT NOT NULL,
    date DATE NOT NULL,
    trade_count INTEGER DEFAULT 0,
    total_stake REAL DEFAULT 0,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    
    UNIQUE(polymarket_market_id, odds_game_id, date)
);

-- Table to store trade performance metrics
CREATE TABLE IF NOT EXISTS trade_performance (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    date DATE NOT NULL,
    total_trades INTEGER DEFAULT 0,
    successful_trades INTEGER DEFAULT 0,
    total_stake REAL DEFAULT 0,
    total_profit REAL DEFAULT 0,
    win_rate REAL DEFAULT 0,
    roi REAL DEFAULT 0,
    
    UNIQUE(date)
);

-- Indexes for performance
CREATE INDEX IF NOT EXISTS idx_executed_trades_market_outcome ON executed_trades(polymarket_market_id, outcome);
CREATE INDEX IF NOT EXISTS idx_executed_trades_created_at ON executed_trades(created_at);
CREATE INDEX IF NOT EXISTS idx_executed_trades_status ON executed_trades(status);
CREATE INDEX IF NOT EXISTS idx_opportunity_hash ON trade_opportunities(opportunity_hash);
CREATE INDEX IF NOT EXISTS idx_opportunity_market_outcome ON trade_opportunities(polymarket_market_id, outcome);
CREATE INDEX IF NOT EXISTS idx_market_activity_date ON market_activity(date);
CREATE INDEX IF NOT EXISTS idx_trade_performance_date ON trade_performance(date);

-- Function to generate opportunity hash (will be implemented in C++)
-- Hash based on: polymarket_market_id + odds_game_id + outcome + date
-- This prevents trading the same opportunity multiple times per day

-- Views for easy querying
CREATE VIEW IF NOT EXISTS active_trades AS
SELECT * FROM executed_trades 
WHERE status IN ('PENDING', 'EXECUTED', 'PARTIALLY_EXECUTED');

CREATE VIEW IF NOT EXISTS daily_performance AS
SELECT 
    DATE(created_at) as trade_date,
    COUNT(*) as trades_count,
    SUM(stake_amount) as total_stake,
    SUM(CASE WHEN actual_profit > 0 THEN 1 ELSE 0 END) as winning_trades,
    SUM(actual_profit) as total_profit,
    AVG(edge_percentage) as avg_edge,
    (SUM(CASE WHEN actual_profit > 0 THEN 1 ELSE 0 END) * 100.0 / COUNT(*)) as win_rate
FROM executed_trades 
WHERE status = 'SETTLED'
GROUP BY DATE(created_at)
ORDER BY trade_date DESC;

CREATE VIEW IF NOT EXISTS market_summary AS
SELECT 
    polymarket_slug,
    COUNT(*) as total_trades,
    SUM(stake_amount) as total_stake,
    SUM(actual_profit) as total_profit,
    AVG(edge_percentage) as avg_edge,
    MAX(created_at) as last_trade_date
FROM executed_trades 
GROUP BY polymarket_slug
ORDER BY total_profit DESC;