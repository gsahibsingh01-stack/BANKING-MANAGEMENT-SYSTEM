#include "../include/auth.h"
#include "../include/sha256.h"
#include "../include/utils.h"
#include <iostream>

std::pair<bool, std::string> AuthManager::authenticateUser(Database& db, const std::string& username, const std::string& password, Models::User& outUser) {
    if (username.empty() || password.empty()) {
        return {false, "Username and password cannot be empty."};
    }

    auto userOpt = db.getUserByUsername(username);
    if (!userOpt.has_value()) {
        return {false, "Invalid username or password."};
    }

    Models::User user = userOpt.value();
    if (user.status != "active") {
        return {false, "This user account is suspended or inactive. Please contact bank administration."};
    }

    std::string computedHash = Crypto::SHA256::hashWithSalt(password, user.salt);
    if (computedHash != user.password_hash) {
        return {false, "Invalid username or password."};
    }

    outUser = user;
    return {true, "Authentication successful."};
}

std::string AuthManager::createSession(Database& db, int userId) {
    std::string token = Utils::generateToken(40);
    if (db.createSession(token, userId, 24)) {
        return token;
    }
    return "";
}

std::optional<Models::User> AuthManager::validateSession(Database& db, const std::string& token) {
    if (token.empty()) return std::nullopt;
    return db.getUserBySessionToken(token);
}

bool AuthManager::logout(Database& db, const std::string& token) {
    if (token.empty()) return false;
    return db.deleteSession(token);
}

std::pair<bool, std::string> AuthManager::registerUser(Database& db, const std::string& username, const std::string& password,
                                                        const std::string& fullName, const std::string& email, const std::string& phone,
                                                        const std::string& accountType, double initialDeposit) {
    if (username.length() < 3) {
        return {false, "Username must be at least 3 characters."};
    }
    if (password.length() < 6) {
        return {false, "Password must be at least 6 characters."};
    }
    if (fullName.empty()) {
        return {false, "Full name is required."};
    }
    if (email.find('@') == std::string::npos) {
        return {false, "Valid email address is required."};
    }

    // Check existing
    if (db.getUserByUsername(username).has_value()) {
        return {false, "Username already exists. Please choose another username."};
    }

    std::string salt = Utils::generateToken(16);
    std::string hash = Crypto::SHA256::hashWithSalt(password, salt);

    Models::User newUser;
    newUser.username = username;
    newUser.password_hash = hash;
    newUser.salt = salt;
    newUser.full_name = fullName;
    newUser.email = email;
    newUser.phone = phone;
    newUser.role = "customer";
    newUser.status = "active";

    int newUserId = db.createUser(newUser);
    if (newUserId <= 0) {
        return {false, "Database error: unable to create user account."};
    }

    // Automatically create primary bank account for new customer
    std::string chosenType = accountType.empty() ? "Checking" : accountType;
    double deposit = initialDeposit < 0.0 ? 0.0 : initialDeposit;
    double rate = (chosenType == "Savings") ? 4.0 : 0.5;

    int newAccId = db.createAccount(newUserId, chosenType, deposit, "USD", rate);
    if (newAccId > 0) {
        // Issue active debit card for customer
        db.issueCard(newAccId, fullName, "Debit", "1234", 2000.0);
    }

    db.logAudit(newUserId, "REGISTER_USER", "New customer registered: " + username + " with account #" + std::to_string(newAccId), "127.0.0.1");

    return {true, "Account created successfully! You can now log in."};
}

std::string AuthManager::extractTokenFromRequest(const httplib::Request& req) {
    // 1. Check Authorization: Bearer <token>
    if (req.has_header("Authorization")) {
        std::string auth = req.get_header_value("Authorization");
        if (auth.rfind("Bearer ", 0) == 0) {
            return auth.substr(7);
        }
    }

    // 2. Check X-Auth-Token header
    if (req.has_header("X-Auth-Token")) {
        return req.get_header_value("X-Auth-Token");
    }

    // 3. Check Cookie: session_token=<token>
    if (req.has_header("Cookie")) {
        std::string cookie = req.get_header_value("Cookie");
        std::string key = "session_token=";
        size_t pos = cookie.find(key);
        if (pos != std::string::npos) {
            size_t start = pos + key.length();
            size_t end = cookie.find(';', start);
            if (end == std::string::npos) {
                return cookie.substr(start);
            } else {
                return cookie.substr(start, end - start);
            }
        }
    }

    // 4. Check Query Parameter: ?token=<token>
    if (req.has_param("token")) {
        return req.get_param_value("token");
    }

    return "";
}
