#include "signature_utils.h"
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/bn.h>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <regex>

namespace polymarket_bot {
namespace crypto {

std::string SignatureUtils::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    return std::to_string(timestamp);
}

std::string SignatureUtils::base64UrlDecode(const std::string& input) {
    std::string decoded = input;
    
    // Replace URL-safe characters
    std::replace(decoded.begin(), decoded.end(), '-', '+');
    std::replace(decoded.begin(), decoded.end(), '_', '/');
    
    // Add padding if needed
    while (decoded.length() % 4) {
        decoded += '=';
    }
    
    // Use OpenSSL's base64 decode
    BIO* bio = BIO_new_mem_buf(decoded.c_str(), decoded.length());
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);
    
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    
    char buffer[1024];
    int decodedLength = BIO_read(bio, buffer, decoded.length());
    BIO_free_all(bio);
    
    return std::string(buffer, decodedLength);
}

std::string SignatureUtils::base64UrlEncode(const std::string& input) {
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);
    
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, input.c_str(), input.length());
    BIO_flush(bio);
    
    char* encoded_data;
    long encoded_length = BIO_get_mem_data(bio, &encoded_data);
    
    std::string result(encoded_data, encoded_length);
    BIO_free_all(bio);
    
    // Convert to URL-safe base64
    std::replace(result.begin(), result.end(), '+', '-');
    std::replace(result.begin(), result.end(), '/', '_');
    
    // Remove padding
    result.erase(std::find(result.rbegin(), result.rend(), '=').base(), result.end());
    
    return result;
}

std::string SignatureUtils::buildHmacSignature(
    const std::string& secret,
    const std::string& timestamp,
    const std::string& method,
    const std::string& requestPath,
    const std::string& body
) {
    // Decode the base64 secret
    std::string decodedSecret = base64UrlDecode(secret);
    
    // Build message
    std::string message = timestamp + method + requestPath;
    if (!body.empty()) {
        // Replace single quotes with double quotes for consistency
        std::string processedBody = body;
        std::replace(processedBody.begin(), processedBody.end(), '\'', '"');
        message += processedBody;
    }
    
    // Create HMAC
    unsigned char* digest = HMAC(EVP_sha256(), 
                                decodedSecret.c_str(), decodedSecret.length(),
                                reinterpret_cast<const unsigned char*>(message.c_str()), message.length(),
                                nullptr, nullptr);
    
    if (!digest) {
        return "";
    }
    
    // Convert to string and base64 encode
    std::string hashString(reinterpret_cast<char*>(digest), SHA256_DIGEST_LENGTH);
    return base64UrlEncode(hashString);
}

std::string SignatureUtils::signClobAuthMessage(
    const std::string& privateKey,
    const std::string& timestamp,
    const std::string& nonce
) {
    // This is a simplified implementation
    // In a real implementation, you would use EIP-712 signing
    // For now, we'll create a placeholder that follows the pattern
    
    // Remove 0x prefix if present
    std::string cleanPrivateKey = privateKey;
    if (cleanPrivateKey.substr(0, 2) == "0x") {
        cleanPrivateKey = cleanPrivateKey.substr(2);
    }
    
    // Create the message to sign (simplified EIP-712 structure)
    std::string message = "ClobAuth:" + timestamp + ":" + nonce;
    
    // For now, return a placeholder signature
    // In production, this should use proper ECDSA signing with EIP-712
    return "0x" + std::string(130, '0'); // Placeholder signature
}

PolyHeaders SignatureUtils::createLevel1Headers(
    const std::string& privateKey,
    const std::string& address,
    const std::string& nonce
) {
    std::string timestamp = getCurrentTimestamp();
    std::string signature = signClobAuthMessage(privateKey, timestamp, nonce);
    
    PolyHeaders headers;
    headers.address = address;
    headers.signature = signature;
    headers.timestamp = timestamp;
    headers.nonce = nonce;
    
    return headers;
}

PolyHeaders SignatureUtils::createLevel2Headers(
    const std::string& privateKey,
    const std::string& address,
    const std::string& apiKey,
    const std::string& secret,
    const std::string& passphrase,
    const std::string& method,
    const std::string& requestPath,
    const std::string& body
) {
    std::string timestamp = getCurrentTimestamp();
    std::string hmacSignature = buildHmacSignature(secret, timestamp, method, requestPath, body);
    
    PolyHeaders headers;
    headers.address = address;
    headers.signature = hmacSignature;
    headers.timestamp = timestamp;
    headers.apiKey = apiKey;
    headers.passphrase = passphrase;
    
    return headers;
}

} // namespace crypto
} // namespace polymarket_bot