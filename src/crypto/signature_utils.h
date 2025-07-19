#pragma once

#include <string>
#include <map>

namespace polymarket_bot {
namespace crypto {

struct PolyHeaders {
    std::string address;
    std::string signature;
    std::string timestamp;
    std::string nonce;
    std::string apiKey;
    std::string passphrase;
};

class SignatureUtils {
public:
    static std::string buildHmacSignature(
        const std::string& secret,
        const std::string& timestamp,
        const std::string& method,
        const std::string& requestPath,
        const std::string& body = ""
    );
    
    static std::string signClobAuthMessage(
        const std::string& privateKey,
        const std::string& timestamp,
        const std::string& nonce = "0"
    );
    
    static PolyHeaders createLevel1Headers(
        const std::string& privateKey,
        const std::string& address,
        const std::string& nonce = "0"
    );
    
    static PolyHeaders createLevel2Headers(
        const std::string& privateKey,
        const std::string& address,
        const std::string& apiKey,
        const std::string& secret,
        const std::string& passphrase,
        const std::string& method,
        const std::string& requestPath,
        const std::string& body = ""
    );

private:
    static std::string getCurrentTimestamp();
    static std::string base64UrlDecode(const std::string& input);
    static std::string base64UrlEncode(const std::string& input);
};

} // namespace crypto
} // namespace polymarket_bot