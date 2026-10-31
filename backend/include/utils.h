#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cmath>

namespace Utils {

inline std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    struct tm timeinfo;
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&timeinfo, &in_time_t);
#else
    localtime_r(&in_time_t, &timeinfo);
#endif
    ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

inline std::string generateToken(size_t length = 32) {
    const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> dist(0, (int)chars.size() - 1);
    std::string token;
    for (size_t i = 0; i < length; ++i) {
        token += chars[dist(generator)];
    }
    return token;
}

inline std::string generateAccountNumber() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<long long> dist(10000000, 99999999);
    return "1001" + std::to_string(dist(gen));
}

inline std::string generateTransactionReference() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    struct tm timeinfo;
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&timeinfo, &in_time_t);
#else
    localtime_r(&in_time_t, &timeinfo);
#endif
    ss << "TXN-" << std::put_time(&timeinfo, "%Y%m%d") << "-";
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(100000, 999999);
    ss << dist(gen);
    return ss.str();
}

inline std::string generateCardNumber(const std::string& prefix = "4532") {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 9);
    std::string card = prefix;
    while (card.length() < 16) {
        card += std::to_string(dist(gen));
    }
    return card;
}

inline std::string generateCVV() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(100, 999);
    return std::to_string(dist(gen));
}

inline double calculateMonthlyEMI(double principal, double annualRatePercentage, int months) {
    if (months <= 0 || principal <= 0.0) return 0.0;
    if (annualRatePercentage <= 0.0) return principal / months;
    
    double monthlyRate = (annualRatePercentage / 100.0) / 12.0;
    double emi = (principal * monthlyRate * std::pow(1.0 + monthlyRate, months)) / 
                 (std::pow(1.0 + monthlyRate, months) - 1.0);
    return std::round(emi * 100.0) / 100.0;
}

inline double roundTo2Decimals(double value) {
    return std::round(value * 100.0) / 100.0;
}

} // namespace Utils

#endif // UTILS_H
