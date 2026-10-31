#ifndef AUTH_H
#define AUTH_H

#include <string>
#include <optional>
#include "database.h"
#include "models.h"
#include "../../dependencies/httplib.h"

class AuthManager {
public:
    static std::pair<bool, std::string> authenticateUser(Database& db, const std::string& username, const std::string& password, Models::User& outUser);
    static std::string createSession(Database& db, int userId);
    static std::optional<Models::User> validateSession(Database& db, const std::string& token);
    static bool logout(Database& db, const std::string& token);
    static std::pair<bool, std::string> registerUser(Database& db, const std::string& username, const std::string& password, 
                                                    const std::string& fullName, const std::string& email, const std::string& phone,
                                                    const std::string& accountType = "Checking", double initialDeposit = 500.0);
    static std::string extractTokenFromRequest(const httplib::Request& req);
};

#endif // AUTH_H
