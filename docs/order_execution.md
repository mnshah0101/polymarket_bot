# Polymarket Order Execution

This document describes how to use the Polymarket API client to execute orders on the Polymarket CLOB (Central Limit Order Book).

## Overview

The Polymarket API client provides two methods for executing orders:

1. `executeOrder(const PolymarketOpenOrder& order)` - Uses a pre-configured order structure
2. `createOrder(...)` - Takes individual parameters for more flexibility

## Order Types

The Polymarket CLOB API supports the following order types:

- **FOK (Fill-Or-Kill)**: Market order that must be executed immediately in its entirety, otherwise cancelled
- **FAK (Fill-And-Kill)**: Market order that executes immediately for available shares, remaining portion cancelled
- **GTC (Good-Til-Cancelled)**: Limit order that remains active until fulfilled or cancelled
- **GTD (Good-Til-Date)**: Limit order active until specified date, unless fulfilled or cancelled

## API Endpoint

All order operations use the `POST /<clob-endpoint>/order` endpoint with L2 authentication headers.

## Usage Examples

### Example 1: Using PolymarketOpenOrder Structure

```cpp
#include "src/api/polymarket_api_client.h"

// Create API client
PolymarketApiClient client(baseUrl, gammaBaseUrl, dataBaseUrl, address, 
                          signature, timestamp, apiKey, passphrase, chainId);

// Create order
PolymarketOpenOrder order;
order.id = "12345";
order.maker_address = "0x..."; // Your wallet address
order.owner = "your-api-key";
order.asset_id = "0x1234567890abcdef"; // Token ID
order.original_size = "1000000"; // Amount in wei
order.size_matched = "0";
order.expiration = std::to_string(std::time(nullptr) + 3600); // 1 hour
order.side = "buy"; // or "sell"
order.type = "GTC"; // Order type

// Execute order
PolymarketOrderResponse response = client.executeOrder(order);

if (response.success) {
    std::cout << "Order ID: " << response.orderId << std::endl;
    for (const auto& hash : response.orderHashes) {
        std::cout << "Transaction hash: " << hash << std::endl;
    }
} else {
    std::cout << "Error: " << response.errorMsg << std::endl;
}
```

### Example 2: Using Individual Parameters

```cpp
PolymarketOrderResponse response = client.createOrder(
    "0x...",                    // maker address
    "0x...",                    // signer address
    "your-api-key",             // taker
    "0x1234567890abcdef",       // tokenId
    "500000",                   // makerAmount
    "1000000",                  // takerAmount
    std::to_string(std::time(nullptr) + 1800), // expiration
    "67890",                    // nonce
    "0",                        // feeRateBps
    "sell",                     // side
    0,                          // signatureType
    "",                         // signature
    "your-api-key",             // owner
    "FOK"                       // orderType
);
```

## Order Object Structure

The order object sent to the API contains the following required fields:

| Field | Type | Description |
|-------|------|-------------|
| salt | integer | Random salt for unique order |
| maker | string | Maker address (funder) |
| signer | string | Signing address |
| taker | string | Taker address (operator) |
| tokenId | string | ERC1155 token ID |
| makerAmount | string | Maximum amount maker is willing to spend |
| takerAmount | string | Minimum amount taker will pay |
| expiration | string | Unix expiration timestamp |
| nonce | string | Maker's exchange nonce |
| feeRateBps | string | Fee rate basis points |
| side | string | "buy" or "sell" |
| signatureType | integer | Signature type enum |
| signature | string | Hex encoded signature |

## Response Structure

The API returns a `PolymarketOrderResponse` with the following fields:

| Field | Type | Description |
|-------|------|-------------|
| success | boolean | Server-side success indicator |
| errorMsg | string | Error message if unsuccessful |
| orderId | string | ID of the placed order |
| orderHashes | string[] | Transaction hashes if order was marketable |

## Error Handling

The API may return various error messages:

- `INVALID_ORDER_MIN_TICK_SIZE`: Price breaks minimum tick size rules
- `INVALID_ORDER_MIN_SIZE`: Order size below minimum threshold
- `INVALID_ORDER_DUPLICATED`: Order already placed
- `INVALID_ORDER_NOT_ENOUGH_BALANCE`: Insufficient balance/allowance
- `INVALID_ORDER_EXPIRATION`: Expiration time before now
- `INVALID_ORDER_ERROR`: System error inserting order
- `EXECUTION_ERROR`: System error during trade execution
- `ORDER_DELAYED`: Order placement delayed due to market conditions
- `FOK_ORDER_NOT_FILLED_ERROR`: FOK order not fully filled
- `MARKET_NOT_READY`: Market not ready to process orders

## Order Status

Orders may have the following statuses:

- `matched`: Order placed and matched with existing order
- `live`: Order placed and resting on the book
- `delayed`: Order marketable but subject to matching delay
- `unmatched`: Order marketable but failure delaying, placement successful

## Best Practices

1. **Check Balance**: Always verify sufficient balance before placing orders
2. **Use Appropriate Order Types**: 
   - Use FOK for immediate execution
   - Use GTC for limit orders that should remain active
   - Use GTD for time-limited orders
3. **Handle Errors**: Always check the `success` field and handle `errorMsg`
4. **Monitor Order Status**: Track order status and transaction hashes
5. **Use Proper Signatures**: Ensure valid signatures for authenticated requests

## Configuration

Make sure your `config.json` contains the necessary API credentials:

```json
{
  "api": {
    "baseUrl": "https://clob.polymarket.com",
    "address": "your-wallet-address",
    "apiKey": "your-api-key",
    "passphrase": "your-passphrase",
    "signature": "your-signature",
    "timestamp": "timestamp",
    "chainId": 137
  }
}
```

## Running the Example

To run the order execution example:

```bash
cd examples
g++ -std=c++17 -I../src order_execution_example.cpp -o order_example
./order_example
```

Make sure to update the configuration file with your actual API credentials before running. 