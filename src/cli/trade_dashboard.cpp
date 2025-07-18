#include "trade_dashboard.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <cmath>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

using namespace polymarket_bot::cli;
using namespace polymarket_bot::trading;
using namespace polymarket_bot::api;

// Color codes
const std::string TradeDashboard::COLOR_GREEN = "\033[32m";
const std::string TradeDashboard::COLOR_RED = "\033[31m";
const std::string TradeDashboard::COLOR_YELLOW = "\033[33m";
const std::string TradeDashboard::COLOR_BLUE = "\033[34m";
const std::string TradeDashboard::COLOR_RESET = "\033[0m";

TradeDashboard::TradeDashboard(std::shared_ptr<TradeManager> tradeManager,
                               std::shared_ptr<PolymarketApiClient> polyClient)
    : tradeManager(tradeManager), polyClient(polyClient) {
}

void TradeDashboard::clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void TradeDashboard::printHeader(const std::string& title) {
    std::cout << std::endl;
    printSeparator('=', 80);
    std::cout << "  " << colorText(title, COLOR_BLUE) << std::endl;
    printSeparator('=', 80);
    std::cout << std::endl;
}

void TradeDashboard::printSeparator(char ch, int length) {
    std::cout << std::string(length, ch) << std::endl;
}

void TradeDashboard::printTableHeader(const std::vector<std::string>& headers, const std::vector<int>& widths) {
    for (size_t i = 0; i < headers.size(); ++i) {
        std::cout << std::left << std::setw(widths[i]) << headers[i];
        if (i < headers.size() - 1) std::cout << " | ";
    }
    std::cout << std::endl;
    
    int totalWidth = 0;
    for (size_t i = 0; i < widths.size(); ++i) {
        totalWidth += widths[i];
        if (i < widths.size() - 1) totalWidth += 3; // " | "
    }
    printSeparator('-', totalWidth);
}

void TradeDashboard::printTableRow(const std::vector<std::string>& values, const std::vector<int>& widths) {
    for (size_t i = 0; i < values.size() && i < widths.size(); ++i) {
        std::string truncated = truncateString(values[i], widths[i]);
        std::cout << std::left << std::setw(widths[i]) << truncated;
        if (i < values.size() - 1) std::cout << " | ";
    }
    std::cout << std::endl;
}

std::string TradeDashboard::formatCurrency(double amount) {
    std::stringstream ss;
    ss << "$" << std::fixed << std::setprecision(2) << amount;
    return ss.str();
}

std::string TradeDashboard::formatPercentage(double percentage) {
    // Handle invalid values
    if (std::isnan(percentage) || std::isinf(percentage)) {
        return "N/A";
    }
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << (percentage * 100) << "%";
    return ss.str();
}

std::string TradeDashboard::formatDate(const std::string& isoDate) {
    // Simple date formatting - convert ISO to readable format
    if (isoDate.length() >= 10) {
        return isoDate.substr(0, 10); // YYYY-MM-DD
    }
    return isoDate;
}

std::string TradeDashboard::truncateString(const std::string& str, int maxLength) {
    if (str.length() <= static_cast<size_t>(maxLength)) {
        return str;
    }
    return str.substr(0, maxLength - 3) + "...";
}

std::string TradeDashboard::colorText(const std::string& text, const std::string& color) {
    return color + text + COLOR_RESET;
}

void TradeDashboard::showPortfolioSummary() {
    printHeader("PORTFOLIO SUMMARY");
    
    try {
        // Get balance from Polymarket
        std::string userAddress = ""; // Would need to get from config
        double balance = 0.0; // polyClient->getBalance(userAddress);
        
        // Get performance metrics
        double totalProfit = tradeManager->getTotalProfit();
        double winRate = tradeManager->getWinRate(30);
        double roi = tradeManager->getROI(30);
        double dailyStake = tradeManager->getDailyStakeUsed();
        
        std::cout << "Account Balance:     " << colorText(formatCurrency(balance), COLOR_BLUE) << std::endl;
        std::cout << "Total P&L (30d):    " << colorText(formatCurrency(totalProfit), 
                                                        totalProfit >= 0 ? COLOR_GREEN : COLOR_RED) << std::endl;
        std::cout << "Win Rate (30d):     " << colorText(formatPercentage(winRate), COLOR_YELLOW) << std::endl;
        std::cout << "ROI (30d):          " << colorText(formatPercentage(roi), 
                                                        roi >= 0 ? COLOR_GREEN : COLOR_RED) << std::endl;
        std::cout << "Today's Stake Used: " << colorText(formatCurrency(dailyStake), COLOR_BLUE) << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << colorText("Error loading portfolio data: ", COLOR_RED) << e.what() << std::endl;
    }
    
    std::cout << std::endl;
}

void TradeDashboard::showRecentTrades(int limit) {
    printHeader("RECENT TRADES");
    
    try {
        auto trades = tradeManager->getTradeHistory(limit);
        
        if (trades.empty()) {
            std::cout << "No trades found." << std::endl;
            return;
        }
        
        std::vector<std::string> headers = {"Date", "Market", "Outcome", "Stake", "Edge", "P&L", "Status"};
        std::vector<int> widths = {12, 25, 12, 10, 8, 10, 10};
        
        printTableHeader(headers, widths);
        
        for (const auto& trade : trades) {
            std::string profitColor = trade.actualProfit >= 0 ? COLOR_GREEN : COLOR_RED;
            std::string statusColor = (trade.status == "SETTLED") ? COLOR_GREEN : COLOR_YELLOW;
            
            std::vector<std::string> row = {
                formatDate(trade.createdAt),
                truncateString(trade.polymarketSlug, 25),
                trade.outcome,
                formatCurrency(trade.stakeAmount),
                formatPercentage(trade.edgePercentage),
                colorText(formatCurrency(trade.actualProfit), profitColor),
                colorText(trade.status, statusColor)
            };
            
            printTableRow(row, widths);
        }
        
    } catch (const std::exception& e) {
        std::cout << colorText("Error loading trade history: ", COLOR_RED) << e.what() << std::endl;
    }
    
    std::cout << std::endl;
}

void TradeDashboard::showDailyPerformance(int days) {
    printHeader("DAILY PERFORMANCE");
    
    try {
        auto performance = tradeManager->getDailyPerformance(days);
        
        if (performance.empty()) {
            std::cout << "No performance data available." << std::endl;
            return;
        }
        
        std::vector<std::string> headers = {"Date", "Trades", "Stake", "Profit", "Win Rate", "Avg Edge"};
        std::vector<int> widths = {12, 8, 12, 12, 10, 10};
        
        printTableHeader(headers, widths);
        
        for (const auto& perf : performance) {
            std::string profitColor = perf.totalProfit >= 0 ? COLOR_GREEN : COLOR_RED;
            
            std::vector<std::string> row = {
                perf.date,
                std::to_string(perf.tradesCount),
                formatCurrency(perf.totalStake),
                colorText(formatCurrency(perf.totalProfit), profitColor),
                formatPercentage(perf.winRate / 100.0),
                formatPercentage(perf.avgEdge / 100.0)
            };
            
            printTableRow(row, widths);
        }
        
    } catch (const std::exception& e) {
        std::cout << colorText("Error loading performance data: ", COLOR_RED) << e.what() << std::endl;
    }
    
    std::cout << std::endl;
}

void TradeDashboard::showActivePositions() {
    printHeader("ACTIVE POSITIONS");
    
    try {
        // Get active trades
        auto activeTrades = tradeManager->getActivetrades();
        
        if (activeTrades.empty()) {
            std::cout << "No active positions." << std::endl;
            return;
        }
        
        std::vector<std::string> headers = {"Market", "Outcome", "Stake", "Expected P&L", "Status", "Age"};
        std::vector<int> widths = {25, 12, 10, 12, 12, 8};
        
        printTableHeader(headers, widths);
        
        for (const auto& trade : activeTrades) {
            std::string statusColor = (trade.status == "EXECUTED") ? COLOR_GREEN : COLOR_YELLOW;
            
            std::vector<std::string> row = {
                truncateString(trade.polymarketSlug, 25),
                trade.outcome,
                formatCurrency(trade.stakeAmount),
                formatCurrency(trade.expectedProfit),
                colorText(trade.status, statusColor),
                formatDate(trade.createdAt)
            };
            
            printTableRow(row, widths);
        }
        
    } catch (const std::exception& e) {
        std::cout << colorText("Error loading active positions: ", COLOR_RED) << e.what() << std::endl;
    }
    
    std::cout << std::endl;
}

void TradeDashboard::showArbitrageOpportunities(const std::vector<ArbitrageOpportunity>& opportunities) {
    printHeader("CURRENT POLYMARKET TRADING OPPORTUNITIES");
    
    if (opportunities.empty()) {
        std::cout << "No trading opportunities found." << std::endl;
        return;
    }
    
    std::vector<std::string> headers = {"Market", "Outcome", "Edge %", "Poly Price", "Odds Price", "Action", "Stake"};
    std::vector<int> widths = {25, 15, 12, 12, 12, 18, 12};
    
    printTableHeader(headers, widths);
    
    for (const auto& opp : opportunities) {
        std::string edgeColor = (opp.edge >= 0.05) ? COLOR_GREEN : COLOR_YELLOW;
        
        std::vector<std::string> row = {
            truncateString(opp.polymarketSlug, 25),
            truncateString(opp.outcome, 15),
            colorText(formatPercentage(opp.edge), edgeColor),
            formatCurrency(opp.polymarketPrice),
            formatCurrency(opp.oddsPrice),
            opp.recommendedAction,
            formatCurrency(opp.recommendedStake)
        };
        
        printTableRow(row, widths);
    }
    
    std::cout << std::endl;
}

void TradeDashboard::displayFullDashboard() {
    printHeader("POLYMARKET TRADING BOT - DASHBOARD");
    
    showPortfolioSummary();
    showRecentTrades(5);
    showDailyPerformance(7);
    showActivePositions();
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::cout << "Last updated: " << std::ctime(&time_t) << std::endl;
}

void TradeDashboard::displaySummaryDashboard() {
    printHeader("POLYMARKET TRADING BOT - SUMMARY");
    
    showPortfolioSummary();
    showRecentTrades(3);
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::cout << "Last updated: " << std::ctime(&time_t) << std::endl;
}

void TradeDashboard::displayOpportunities(const std::vector<ArbitrageOpportunity>& opportunities) {
    showArbitrageOpportunities(opportunities);
}

void TradeDashboard::runInteractiveMode() {
    bool running = true;
    
    while (running) {
        displayFullDashboard();
        
        std::cout << std::endl;
        std::cout << colorText("Interactive Commands:", COLOR_BLUE) << std::endl;
        std::cout << "  [r] Refresh dashboard" << std::endl;
        std::cout << "  [h] Show trade history" << std::endl;
        std::cout << "  [p] Show performance metrics" << std::endl;
        std::cout << "  [a] Show active positions" << std::endl;
        std::cout << "  [q] Quit" << std::endl;
        std::cout << std::endl;
        std::cout << "Enter command: ";
        
        char command;
        std::cin >> command;
        
        switch (command) {
            case 'r':
            case 'R':
                // Refresh (just redisplay)
                break;
            case 'h':
            case 'H':
                displayTradeHistory(20);
                waitForKeyPress();
                break;
            case 'p':
            case 'P':
                displayPerformanceMetrics(30);
                waitForKeyPress();
                break;
            case 'a':
            case 'A':
                displayPositions();
                waitForKeyPress();
                break;
            case 'q':
            case 'Q':
                running = false;
                break;
            default:
                std::cout << "Invalid command. Please try again." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                break;
        }
    }
}

void TradeDashboard::displayTradeHistory(int limit) {
    printHeader("TRADE HISTORY");
    showRecentTrades(limit);
}

void TradeDashboard::displayPerformanceMetrics(int days) {
    printHeader("PERFORMANCE METRICS");
    showDailyPerformance(days);
    showPortfolioSummary();
}

void TradeDashboard::displayPositions() {
    printHeader("POSITIONS");
    showActivePositions();
}

void TradeDashboard::waitForKeyPress() {
    std::cout << std::endl << "Press Enter to continue...";
    std::cin.ignore();
    std::cin.get();
}