#ifndef CONTROLLERS_H
#define CONTROLLERS_H

#include "../../dependencies/httplib.h"
#include "../../dependencies/json.hpp"
#include "database.h"
#include "models.h"

using json = nlohmann::json;

class Controllers {
public:
    static void registerRoutes(httplib::Server& svr, Database& db);

private:
    static void sendJson(httplib::Response& res, int statusCode, const json& data);
    static void sendError(httplib::Response& res, int statusCode, const std::string& message);
    static std::optional<Models::User> authenticateRequest(const httplib::Request& req, httplib::Response& res, Database& db);
    static bool requireRole(const Models::User& user, const std::vector<std::string>& allowedRoles, httplib::Response& res);
};

#endif // CONTROLLERS_H
