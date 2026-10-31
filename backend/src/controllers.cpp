#include "../include/controllers.h"
#include "../include/auth.h"
#include "../include/utils.h"
#include "../include/sha256.h"
#include <iostream>
#include <algorithm>

void Controllers::sendJson(httplib::Response& res, int statusCode, const json& data) {
    res.status = statusCode;
    res.set_content(data.dump(), "application/json");
}

void Controllers::sendError(httplib::Response& res, int statusCode, const std::string& message) {
    json err = {
        {"success", false},
        {"error", message}
    };
    sendJson(res, statusCode, err);
}

std::optional<Models::User> Controllers::authenticateRequest(const httplib::Request& req, httplib::Response& res, Database& db) {
    std::string token = AuthManager::extractTokenFromRequest(req);
    if (token.empty()) {
        sendError(res, 401, "Authentication token missing. Please sign in.");
        return std::nullopt;
    }

    auto userOpt = AuthManager::validateSession(db, token);
    if (!userOpt.has_value()) {
        sendError(res, 401, "Session expired or invalid. Please sign in again.");
        return std::nullopt;
    }

    if (userOpt->status != "active") {
        sendError(res, 403, "User account is suspended or inactive.");
        return std::nullopt;
    }

    return userOpt;
}

bool Controllers::requireRole(const Models::User& user, const std::vector<std::string>& allowedRoles, httplib::Response& res) {
    for (const auto& r : allowedRoles) {
        if (user.role == r) return true;
    }
    sendError(res, 403, "Access denied. Insufficient permissions for role: " + user.role);
    return false;
}

void Controllers::registerRoutes(httplib::Server& svr, Database& db) {
    // -------------------------------------------------------------
    // CORS Preflight Handler for all routes
    // -------------------------------------------------------------
    svr.Options(".*", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Auth-Token");
        res.status = 204;
    });

    // -------------------------------------------------------------
    // AUTHENTICATION ROUTES
    // -------------------------------------------------------------
    // POST /api/auth/login
    svr.Post("/api/auth/login", [&db](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = json::parse(req.body);
            std::string username = body.value("username", "");
            std::string password = body.value("password", "");

            Models::User user;
            auto authResult = AuthManager::authenticateUser(db, username, password, user);
            if (!authResult.first) {
                sendError(res, 401, authResult.second);
                return;
            }

            std::string token = AuthManager::createSession(db, user.user_id);
            if (token.empty()) {
                sendError(res, 500, "Failed to create user session.");
                return;
            }

            db.logAudit(user.user_id, "LOGIN_SUCCESS", "User logged in: " + user.username + " (" + user.role + ")", req.remote_addr);

            json resp = {
                {"success", true},
                {"message", "Login successful."},
                {"token", token},
                {"user", user.toJson()}
            };
            sendJson(res, 200, resp);
        } catch (const std::exception& e) {
            sendError(res, 400, std::string("Malformed JSON payload: ") + e.what());
        }
    });

    // POST /api/auth/register
    svr.Post("/api/auth/register", [&db](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = json::parse(req.body);
            std::string username = body.value("username", "");
            std::string password = body.value("password", "");
            std::string fullName = body.value("full_name", "");
            std::string email = body.value("email", "");
            std::string phone = body.value("phone", "");
            std::string accountType = body.value("account_type", "Checking");
            double initialDeposit = body.value("initial_deposit", 500.0);

            auto regResult = AuthManager::registerUser(db, username, password, fullName, email, phone, accountType, initialDeposit);
            if (!regResult.first) {
                sendError(res, 400, regResult.second);
                return;
            }

            // Auto-login newly registered user
            Models::User user;
            AuthManager::authenticateUser(db, username, password, user);
            std::string token = AuthManager::createSession(db, user.user_id);

            json resp = {
                {"success", true},
                {"message", regResult.second},
                {"token", token},
                {"user", user.toJson()}
            };
            sendJson(res, 201, resp);
        } catch (const std::exception& e) {
            sendError(res, 400, std::string("Malformed JSON payload: ") + e.what());
        }
    });

    // POST /api/auth/logout
    svr.Post("/api/auth/logout", [&db](const httplib::Request& req, httplib::Response& res) {
        std::string token = AuthManager::extractTokenFromRequest(req);
        if (!token.empty()) {
            auto userOpt = db.getUserBySessionToken(token);
            if (userOpt.has_value()) {
                db.logAudit(userOpt->user_id, "LOGOUT", "User logged out: " + userOpt->username, req.remote_addr);
            }
            AuthManager::logout(db, token);
        }
        sendJson(res, 200, {{"success", true}, {"message", "Logged out successfully."}});
    });

    // GET /api/auth/me
    svr.Get("/api/auth/me", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        sendJson(res, 200, {
            {"success", true},
            {"user", userOpt->toJson()}
        });
    });

    // POST /api/auth/change-password
    svr.Post("/api/auth/change-password", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        try {
            json body = json::parse(req.body);
            std::string oldPass = body.value("old_password", "");
            std::string newPass = body.value("new_password", "");

            if (newPass.length() < 6) {
                sendError(res, 400, "New password must be at least 6 characters.");
                return;
            }

            std::string oldHash = Crypto::SHA256::hashWithSalt(oldPass, userOpt->salt);
            if (oldHash != userOpt->password_hash) {
                sendError(res, 400, "Current password does not match.");
                return;
            }

            std::string newSalt = Utils::generateToken(16);
            std::string newHash = Crypto::SHA256::hashWithSalt(newPass, newSalt);

            if (db.updatePassword(userOpt->user_id, newHash, newSalt)) {
                db.logAudit(userOpt->user_id, "CHANGE_PASSWORD", "Password changed successfully", req.remote_addr);
                sendJson(res, 200, {{"success", true}, {"message", "Password changed successfully."}});
            } else {
                sendError(res, 500, "Failed to update password in database.");
            }
        } catch (const std::exception& e) {
            sendError(res, 400, e.what());
        }
    });

    // POST /api/auth/profile
    svr.Post("/api/auth/profile", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        try {
            json body = json::parse(req.body);
            std::string fullName = body.value("full_name", userOpt->full_name);
            std::string email = body.value("email", userOpt->email);
            std::string phone = body.value("phone", userOpt->phone);

            if (fullName.empty() || email.find('@') == std::string::npos) {
                sendError(res, 400, "Full name and a valid email are required.");
                return;
            }

            if (db.updateProfile(userOpt->user_id, fullName, email, phone)) {
                auto updated = db.getUserById(userOpt->user_id);
                sendJson(res, 200, {{"success", true}, {"message", "Profile updated."}, {"user", updated->toJson()}});
            } else {
                sendError(res, 500, "Failed to update profile.");
            }
        } catch (const std::exception& e) {
            sendError(res, 400, e.what());
        }
    });

    // -------------------------------------------------------------
    // DASHBOARD & SUMMARY
    // -------------------------------------------------------------
    svr.Get("/api/dashboard/summary", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        int userId = userOpt->user_id;
        auto accounts = db.getAccountsByUserId(userId);
        auto transactions = db.getTransactionsByUserId(userId, 10);
        auto loans = db.getLoansByUserId(userId);
        auto cards = db.getCardsByUserId(userId);

        double totalBalance = 0.0;
        json accountsJson = json::array();
        for (const auto& acc : accounts) {
            totalBalance += acc.balance;
            accountsJson.push_back(acc.toJson());
        }

        json txJson = json::array();
        double totalInflow = 0.0;
        double totalOutflow = 0.0;
        for (const auto& tx : transactions) {
            txJson.push_back(tx.toJson());
            if (tx.type == "DEPOSIT" || tx.type == "INTEREST_CREDIT" || tx.type == "LOAN_DISBURSEMENT") {
                totalInflow += tx.amount;
            } else if (tx.type == "WITHDRAW" || tx.type == "LOAN_REPAYMENT") {
                totalOutflow += tx.amount;
            } else if (tx.type == "TRANSFER") {
                // If destination matches user account, it's inflow; if source, it's outflow
                bool isUserTo = false;
                for (const auto& a : accounts) {
                    if (a.account_id == tx.to_account_id) { isUserTo = true; break; }
                }
                if (isUserTo) totalInflow += tx.amount;
                else totalOutflow += tx.amount;
            }
        }

        json loansJson = json::array();
        int pendingLoans = 0;
        double totalLoanDebt = 0.0;
        for (const auto& ln : loans) {
            if (ln.status == "PENDING") pendingLoans++;
            if (ln.status == "APPROVED") totalLoanDebt += ln.remaining_balance;
            loansJson.push_back(ln.toJson());
        }

        json cardsJson = json::array();
        for (const auto& cd : cards) {
            cardsJson.push_back(cd.toJson(true));
        }

        json summary = {
            {"success", true},
            {"user", userOpt->toJson()},
            {"total_balance", Utils::roundTo2Decimals(totalBalance)},
            {"accounts", accountsJson},
            {"recent_transactions", txJson},
            {"loans", loansJson},
            {"cards", cardsJson},
            {"total_inflow", Utils::roundTo2Decimals(totalInflow)},
            {"total_outflow", Utils::roundTo2Decimals(totalOutflow)},
            {"pending_loans_count", pendingLoans},
            {"total_loan_debt", Utils::roundTo2Decimals(totalLoanDebt)},
            {"active_cards_count", (int)cards.size()}
        };

        sendJson(res, 200, summary);
    });

    // -------------------------------------------------------------
    // ACCOUNT ROUTES
    // -------------------------------------------------------------
    // GET /api/accounts
    svr.Get("/api/accounts", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        auto list = db.getAccountsByUserId(userOpt->user_id);
        json arr = json::array();
        for (const auto& a : list) {
            arr.push_back(a.toJson());
        }
        sendJson(res, 200, {{"success", true}, {"accounts", arr}});
    });

    // POST /api/accounts/create
    svr.Post("/api/accounts/create", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        try {
            json body = json::parse(req.body);
            std::string type = body.value("account_type", "Savings");
            double initialDeposit = body.value("initial_deposit", 100.0);
            std::string currency = body.value("currency", "USD");

            if (type != "Savings" && type != "Checking" && type != "Fixed Deposit" && type != "Business") {
                sendError(res, 400, "Invalid account type. Choose Savings, Checking, Fixed Deposit, or Business.");
                return;
            }

            double rate = 3.5;
            if (type == "Fixed Deposit") rate = 6.5;
            else if (type == "Savings") rate = 4.25;
            else if (type == "Business") rate = 2.0;
            else rate = 0.5;

            int newId = db.createAccount(userOpt->user_id, type, initialDeposit, currency, rate);
            if (newId <= 0) {
                sendError(res, 500, "Failed to create account.");
                return;
            }

            // Issue debit card for new checking/savings account
            if (type == "Checking" || type == "Savings") {
                db.issueCard(newId, userOpt->full_name, "Debit", "1234", 2500.0);
            }

            db.logAudit(userOpt->user_id, "CREATE_ACCOUNT", "Created new " + type + " account #" + std::to_string(newId), req.remote_addr);

            auto newAcc = db.getAccountById(newId);
            sendJson(res, 201, {
                {"success", true},
                {"message", "New bank account successfully opened!"},
                {"account", newAcc->toJson()}
            });
        } catch (const std::exception& e) {
            sendError(res, 400, e.what());
        }
    });

    // GET /api/accounts/lookup?account_number=...
    svr.Get("/api/accounts/lookup", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        if (!req.has_param("account_number")) {
            sendError(res, 400, "Missing account_number query parameter.");
            return;
        }

        std::string accNum = req.get_param_value("account_number");
        auto acc = db.getAccountByNumber(accNum);
        if (!acc.has_value()) {
            sendError(res, 404, "Beneficiary account number not found.");
            return;
        }

        sendJson(res, 200, {
            {"success", true},
            {"account_number", acc->account_number},
            {"account_type", acc->account_type},
            {"owner_name", acc->owner_name},
            {"status", acc->status}
        });
    });

    // -------------------------------------------------------------
    // TRANSACTION ROUTES
    // -------------------------------------------------------------
    // POST /api/transactions/deposit
    svr.Post("/api/transactions/deposit", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        try {
            json body = json::parse(req.body);
            int accountId = body.value("account_id", 0);
            double amount = body.value("amount", 0.0);
            std::string desc = body.value("description", "Direct Online Deposit");

            if (accountId <= 0 || amount <= 0.0) {
                sendError(res, 400, "Valid account_id and positive amount are required.");
                return;
            }

            // Verify account belongs to user (or user is manager/admin)
            auto acc = db.getAccountById(accountId);
            if (!acc.has_value()) {
                sendError(res, 404, "Account not found.");
                return;
            }
            if (acc->user_id != userOpt->user_id && userOpt->role == "customer") {
                sendError(res, 403, "Access denied: Account does not belong to you.");
                return;
            }

            std::string refNum;
            if (!db.executeDeposit(accountId, amount, desc, refNum)) {
                sendError(res, 400, "Deposit failed. Account may be frozen or inactive.");
                return;
            }

            db.logAudit(userOpt->user_id, "DEPOSIT", "Deposited $" + std::to_string(amount) + " to account " + acc->account_number + " [Ref: " + refNum + "]", req.remote_addr);

            auto updatedAcc = db.getAccountById(accountId);
            sendJson(res, 200, {
                {"success", true},
                {"message", "Deposit completed successfully!"},
                {"reference_number", refNum},
                {"amount", amount},
                {"new_balance", updatedAcc->balance}
            });
        } catch (const std::exception& e) {
            sendError(res, 400, e.what());
        }
    });

    // POST /api/transactions/withdraw
    svr.Post("/api/transactions/withdraw", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        try {
            json body = json::parse(req.body);
            int accountId = body.value("account_id", 0);
            double amount = body.value("amount", 0.0);
            std::string desc = body.value("description", "Online Withdrawal");

            if (accountId <= 0 || amount <= 0.0) {
                sendError(res, 400, "Valid account_id and positive amount are required.");
                return;
            }

            auto acc = db.getAccountById(accountId);
            if (!acc.has_value()) {
                sendError(res, 404, "Account not found.");
                return;
            }
            if (acc->user_id != userOpt->user_id && userOpt->role == "customer") {
                sendError(res, 403, "Access denied: Account does not belong to you.");
                return;
            }

            std::string refNum;
            std::string error;
            if (!db.executeWithdrawal(accountId, amount, desc, refNum, error)) {
                sendError(res, 400, error);
                return;
            }

            db.logAudit(userOpt->user_id, "WITHDRAW", "Withdrew $" + std::to_string(amount) + " from account " + acc->account_number + " [Ref: " + refNum + "]", req.remote_addr);

            auto updatedAcc = db.getAccountById(accountId);
            sendJson(res, 200, {
                {"success", true},
                {"message", "Withdrawal completed successfully!"},
                {"reference_number", refNum},
                {"amount", amount},
                {"new_balance", updatedAcc->balance}
            });
        } catch (const std::exception& e) {
            sendError(res, 400, e.what());
        }
    });

    // POST /api/transactions/transfer
    svr.Post("/api/transactions/transfer", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        try {
            json body = json::parse(req.body);
            int fromAccountId = body.value("from_account_id", 0);
            std::string toAccountNumber = body.value("to_account_number", "");
            double amount = body.value("amount", 0.0);
            std::string desc = body.value("description", "Online Funds Transfer");

            if (fromAccountId <= 0 || toAccountNumber.empty() || amount <= 0.0) {
                sendError(res, 400, "Source account, recipient account number, and positive amount are required.");
                return;
            }

            // Verify source account
            auto fromAcc = db.getAccountById(fromAccountId);
            if (!fromAcc.has_value()) {
                sendError(res, 404, "Source account not found.");
                return;
            }
            if (fromAcc->user_id != userOpt->user_id && userOpt->role == "customer") {
                sendError(res, 403, "Access denied: Source account does not belong to you.");
                return;
            }

            // Lookup destination account
            auto toAcc = db.getAccountByNumber(toAccountNumber);
            if (!toAcc.has_value()) {
                sendError(res, 404, "Recipient account number '" + toAccountNumber + "' was not found.");
                return;
            }

            if (fromAcc->account_id == toAcc->account_id) {
                sendError(res, 400, "Cannot transfer funds to the same account.");
                return;
            }

            std::string refNum;
            std::string error;
            if (!db.executeTransfer(fromAcc->account_id, toAcc->account_id, amount, desc, refNum, error)) {
                sendError(res, 400, error);
                return;
            }

            db.logAudit(userOpt->user_id, "TRANSFER", "Transferred $" + std::to_string(amount) + " from " + fromAcc->account_number + " to " + toAcc->account_number + " [Ref: " + refNum + "]", req.remote_addr);

            auto updatedFrom = db.getAccountById(fromAccountId);
            sendJson(res, 200, {
                {"success", true},
                {"message", "Funds transferred successfully!"},
                {"reference_number", refNum},
                {"amount", amount},
                {"new_balance", updatedFrom->balance},
                {"recipient_name", toAcc->owner_name},
                {"recipient_account", toAcc->account_number}
            });
        } catch (const std::exception& e) {
            sendError(res, 400, e.what());
        }
    });

    // GET /api/transactions
    svr.Get("/api/transactions", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        int limit = 100;
        if (req.has_param("limit")) {
            try { limit = std::stoi(req.get_param_value("limit")); } catch (...) {}
        }

        std::vector<Models::Transaction> list;
        if (req.has_param("account_id")) {
            int accId = std::stoi(req.get_param_value("account_id"));
            // verify ownership
            auto acc = db.getAccountById(accId);
            if (acc.has_value() && (acc->user_id == userOpt->user_id || userOpt->role != "customer")) {
                list = db.getTransactionsByAccountId(accId, limit);
            } else {
                sendError(res, 403, "Access denied to specified account transactions.");
                return;
            }
        } else {
            list = db.getTransactionsByUserId(userOpt->user_id, limit);
        }

        // Optional filter by type
        if (req.has_param("type") && !req.get_param_value("type").empty()) {
            std::string filterType = req.get_param_value("type");
            std::vector<Models::Transaction> filtered;
            for (const auto& tx : list) {
                if (tx.type == filterType) filtered.push_back(tx);
            }
            list = filtered;
        }

        json arr = json::array();
        for (const auto& tx : list) {
            arr.push_back(tx.toJson());
        }

        sendJson(res, 200, {{"success", true}, {"transactions", arr}});
    });

    // -------------------------------------------------------------
    // LOAN ROUTES
    // -------------------------------------------------------------
    // GET /api/loans
    svr.Get("/api/loans", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        std::vector<Models::Loan> list;
        if (userOpt->role == "manager" || userOpt->role == "admin") {
            list = db.getAllLoans();
        } else {
            list = db.getLoansByUserId(userOpt->user_id);
        }

        json arr = json::array();
        for (const auto& ln : list) {
            arr.push_back(ln.toJson());
        }
        sendJson(res, 200, {{"success", true}, {"loans", arr}});
    });

    // POST /api/loans/apply
    svr.Post("/api/loans/apply", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        try {
            json body = json::parse(req.body);
            int accountId = body.value("account_id", 0);
            std::string loanType = body.value("loan_type", "Personal Loan");
            double amount = body.value("amount", 0.0);
            int months = body.value("duration_months", 12);
            std::string purpose = body.value("purpose", "");

            if (accountId <= 0 || amount <= 0.0 || months <= 0) {
                sendError(res, 400, "Valid account, loan amount, and duration in months are required.");
                return;
            }

            auto acc = db.getAccountById(accountId);
            if (!acc.has_value() || acc->user_id != userOpt->user_id) {
                sendError(res, 403, "Invalid disbursement account selected.");
                return;
            }

            // Determine interest rate based on loan type
            double rate = 7.5;
            if (loanType == "Home Loan") rate = 4.5;
            else if (loanType == "Education Loan") rate = 5.0;
            else if (loanType == "Auto Loan") rate = 5.5;
            else if (loanType == "Business Loan") rate = 6.8;

            int loanId = db.applyForLoan(userOpt->user_id, accountId, loanType, amount, rate, months, purpose);
            if (loanId <= 0) {
                sendError(res, 500, "Failed to submit loan application.");
                return;
            }

            db.logAudit(userOpt->user_id, "LOAN_APPLY", "Applied for " + loanType + " of $" + std::to_string(amount) + " (Loan #" + std::to_string(loanId) + ")", req.remote_addr);

            auto loan = db.getLoanById(loanId);
            sendJson(res, 201, {
                {"success", true},
                {"message", "Loan application submitted successfully! It is pending branch manager review."},
                {"loan", loan->toJson()}
            });
        } catch (const std::exception& e) {
            sendError(res, 400, e.what());
        }
    });

    // POST /api/loans/repay
    svr.Post("/api/loans/repay", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        try {
            json body = json::parse(req.body);
            int loanId = body.value("loan_id", 0);
            int accountId = body.value("account_id", 0);
            double amount = body.value("amount", 0.0);

            if (loanId <= 0 || accountId <= 0 || amount <= 0.0) {
                sendError(res, 400, "Loan ID, paying account ID, and positive amount are required.");
                return;
            }

            auto loan = db.getLoanById(loanId);
            if (!loan.has_value()) {
                sendError(res, 404, "Loan not found.");
                return;
            }
            if (loan->user_id != userOpt->user_id && userOpt->role == "customer") {
                sendError(res, 403, "Access denied to loan.");
                return;
            }

            std::string error;
            if (!db.repayLoan(loanId, accountId, amount, error)) {
                sendError(res, 400, error);
                return;
            }

            db.logAudit(userOpt->user_id, "LOAN_REPAY", "Paid $" + std::to_string(amount) + " towards Loan #" + std::to_string(loanId), req.remote_addr);

            auto updatedLoan = db.getLoanById(loanId);
            sendJson(res, 200, {
                {"success", true},
                {"message", "Loan installment payment completed successfully!"},
                {"remaining_balance", updatedLoan->remaining_balance},
                {"loan_status", updatedLoan->status}
            });
        } catch (const std::exception& e) {
            sendError(res, 400, e.what());
        }
    });

    // POST /api/loans/review (Manager / Admin)
    svr.Post("/api/loans/review", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        if (!requireRole(*userOpt, {"manager", "admin"}, res)) return;

        try {
            json body = json::parse(req.body);
            int loanId = body.value("loan_id", 0);
            std::string decision = body.value("decision", "");
            std::string reason = body.value("reason", "");

            if (loanId <= 0 || (decision != "APPROVED" && decision != "REJECTED")) {
                sendError(res, 400, "Valid loan_id and decision ('APPROVED' or 'REJECTED') are required.");
                return;
            }

            std::string error;
            if (!db.reviewLoan(loanId, userOpt->user_id, decision, reason, error)) {
                sendError(res, 400, error);
                return;
            }

            db.logAudit(userOpt->user_id, "LOAN_REVIEW", "Manager " + userOpt->username + " " + decision + " Loan #" + std::to_string(loanId), req.remote_addr);

            auto loan = db.getLoanById(loanId);
            sendJson(res, 200, {
                {"success", true},
                {"message", "Loan has been " + decision + " successfully!"},
                {"loan", loan->toJson()}
            });
        } catch (const std::exception& e) {
            sendError(res, 400, e.what());
        }
    });

    // -------------------------------------------------------------
    // CARD ROUTES
    // -------------------------------------------------------------
    // GET /api/cards
    svr.Get("/api/cards", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        auto cards = db.getCardsByUserId(userOpt->user_id);
        json arr = json::array();
        for (const auto& c : cards) {
            arr.push_back(c.toJson(true));
        }
        sendJson(res, 200, {{"success", true}, {"cards", arr}});
    });

    // POST /api/cards/issue
    svr.Post("/api/cards/issue", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        try {
            json body = json::parse(req.body);
            int accountId = body.value("account_id", 0);
            std::string cardType = body.value("card_type", "Debit");
            std::string pin = body.value("pin", "1234");
            double dailyLimit = body.value("daily_limit", 2500.0);

            auto acc = db.getAccountById(accountId);
            if (!acc.has_value() || acc->user_id != userOpt->user_id) {
                sendError(res, 403, "Invalid account selected for card issuance.");
                return;
            }

            int cardId = db.issueCard(accountId, userOpt->full_name, cardType, pin, dailyLimit);
            if (cardId <= 0) {
                sendError(res, 500, "Failed to issue new card.");
                return;
            }

            db.logAudit(userOpt->user_id, "ISSUE_CARD", "Issued new " + cardType + " card for account " + acc->account_number, req.remote_addr);

            auto card = db.getCardById(cardId);
            sendJson(res, 201, {
                {"success", true},
                {"message", "Virtual card issued successfully!"},
                {"card", card->toJson(true)}
            });
        } catch (const std::exception& e) {
            sendError(res, 400, e.what());
        }
    });

    // POST /api/cards/toggle-lock
    svr.Post("/api/cards/toggle-lock", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        try {
            json body = json::parse(req.body);
            int cardId = body.value("card_id", 0);

            if (cardId <= 0) {
                sendError(res, 400, "Valid card_id is required.");
                return;
            }

            if (!db.toggleCardLock(cardId, userOpt->user_id)) {
                sendError(res, 400, "Failed to update card status or card not found.");
                return;
            }

            auto card = db.getCardById(cardId);
            db.logAudit(userOpt->user_id, "CARD_STATUS", "Card #" + std::to_string(cardId) + " status toggled to " + card->status, req.remote_addr);

            sendJson(res, 200, {
                {"success", true},
                {"message", "Card status successfully updated to " + card->status},
                {"card", card->toJson(true)}
            });
        } catch (const std::exception& e) {
            sendError(res, 400, e.what());
        }
    });

    // POST /api/cards/update-limit
    svr.Post("/api/cards/update-limit", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        try {
            json body = json::parse(req.body);
            int cardId = body.value("card_id", 0);
            double newLimit = body.value("daily_limit", 0.0);

            if (cardId <= 0 || newLimit <= 0.0) {
                sendError(res, 400, "Valid card_id and positive limit are required.");
                return;
            }

            if (!db.updateCardLimit(cardId, userOpt->user_id, newLimit)) {
                sendError(res, 400, "Failed to update card limit.");
                return;
            }

            sendJson(res, 200, {
                {"success", true},
                {"message", "Card daily limit updated to $" + std::to_string(newLimit)}
            });
        } catch (const std::exception& e) {
            sendError(res, 400, e.what());
        }
    });

    // -------------------------------------------------------------
    // BENEFICIARIES ROUTES
    // -------------------------------------------------------------
    // GET /api/beneficiaries
    svr.Get("/api/beneficiaries", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        auto list = db.getBeneficiariesByUserId(userOpt->user_id);
        json arr = json::array();
        for (const auto& b : list) {
            arr.push_back(b.toJson());
        }
        sendJson(res, 200, {{"success", true}, {"beneficiaries", arr}});
    });

    // POST /api/beneficiaries/add
    svr.Post("/api/beneficiaries/add", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        try {
            json body = json::parse(req.body);
            std::string name = body.value("name", "");
            std::string accNumber = body.value("account_number", "");
            std::string bankName = body.value("bank_name", "Apex National Bank");
            std::string routing = body.value("routing_code", "APEX0001");
            std::string nickname = body.value("nickname", "");
            std::string email = body.value("email", "");

            if (name.empty() || accNumber.empty()) {
                sendError(res, 400, "Beneficiary name and account number are required.");
                return;
            }

            int id = db.addBeneficiary(userOpt->user_id, name, accNumber, bankName, routing, nickname, email);
            if (id <= 0) {
                sendError(res, 500, "Failed to add beneficiary.");
                return;
            }

            db.logAudit(userOpt->user_id, "ADD_BENEFICIARY", "Added beneficiary: " + name + " (" + accNumber + ")", req.remote_addr);

            sendJson(res, 201, {
                {"success", true},
                {"message", "Beneficiary added successfully!"},
                {"beneficiary_id", id}
            });
        } catch (const std::exception& e) {
            sendError(res, 400, e.what());
        }
    });

    // DELETE /api/beneficiaries/:id
    svr.Delete(R"(/api/beneficiaries/(\d+))", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        int bId = std::stoi(req.matches[1]);
        if (db.deleteBeneficiary(bId, userOpt->user_id)) {
            sendJson(res, 200, {{"success", true}, {"message", "Beneficiary removed."}});
        } else {
            sendError(res, 400, "Failed to remove beneficiary.");
        }
    });

    // -------------------------------------------------------------
    // ADMIN & MANAGER MANAGEMENT ROUTES
    // -------------------------------------------------------------
    // GET /api/admin/stats
    svr.Get("/api/admin/stats", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        if (!requireRole(*userOpt, {"admin", "manager"}, res)) return;

        json stats = db.getSystemStats();
        stats["success"] = true;
        sendJson(res, 200, stats);
    });

    // GET /api/admin/users
    svr.Get("/api/admin/users", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        if (!requireRole(*userOpt, {"admin"}, res)) return;

        auto users = db.getAllUsers();
        json arr = json::array();
        for (const auto& u : users) {
            arr.push_back(u.toJson(false));
        }
        sendJson(res, 200, {{"success", true}, {"users", arr}});
    });

    // POST /api/admin/users/create
    svr.Post("/api/admin/users/create", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        if (!requireRole(*userOpt, {"admin"}, res)) return;

        try {
            json body = json::parse(req.body);
            std::string username = body.value("username", "");
            std::string password = body.value("password", "");
            std::string fullName = body.value("full_name", "");
            std::string email = body.value("email", "");
            std::string phone = body.value("phone", "");
            std::string role = body.value("role", "customer");

            if (username.length() < 3 || password.length() < 6 || fullName.empty()) {
                sendError(res, 400, "Username, password (min 6 chars), and full name are required.");
                return;
            }

            if (role != "customer" && role != "manager" && role != "admin") {
                sendError(res, 400, "Invalid role. Must be customer, manager, or admin.");
                return;
            }

            std::string salt = Utils::generateToken(16);
            std::string hash = Crypto::SHA256::hashWithSalt(password, salt);

            Models::User nu;
            nu.username = username;
            nu.password_hash = hash;
            nu.salt = salt;
            nu.full_name = fullName;
            nu.email = email;
            nu.phone = phone;
            nu.role = role;
            nu.status = "active";

            int newId = db.createUser(nu);
            if (newId <= 0) {
                sendError(res, 400, "Failed to create user. Username or email may already be in use.");
                return;
            }

            if (role == "customer") {
                db.createAccount(newId, "Checking", 1000.0, "USD", 0.5);
            }

            db.logAudit(userOpt->user_id, "ADMIN_CREATE_USER", "Created " + role + " user: " + username, req.remote_addr);

            sendJson(res, 201, {{"success", true}, {"message", "User created successfully!"}, {"user_id", newId}});
        } catch (const std::exception& e) {
            sendError(res, 400, e.what());
        }
    });

    // POST /api/admin/users/toggle-status
    svr.Post("/api/admin/users/toggle-status", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        if (!requireRole(*userOpt, {"admin"}, res)) return;

        try {
            json body = json::parse(req.body);
            int targetUserId = body.value("user_id", 0);

            if (targetUserId <= 0) {
                sendError(res, 400, "Valid user_id required.");
                return;
            }
            if (targetUserId == userOpt->user_id) {
                sendError(res, 400, "Cannot change status of your own administrator account.");
                return;
            }

            auto targetUser = db.getUserById(targetUserId);
            if (!targetUser.has_value()) {
                sendError(res, 404, "User not found.");
                return;
            }

            std::string nextStatus = (targetUser->status == "active") ? "suspended" : "active";
            db.updateUserStatus(targetUserId, nextStatus);

            db.logAudit(userOpt->user_id, "ADMIN_USER_STATUS", "Set status of " + targetUser->username + " to " + nextStatus, req.remote_addr);

            sendJson(res, 200, {
                {"success", true},
                {"message", "User status changed to " + nextStatus},
                {"status", nextStatus}
            });
        } catch (const std::exception& e) {
            sendError(res, 400, e.what());
        }
    });

    // GET /api/admin/audit-logs
    svr.Get("/api/admin/audit-logs", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        if (!requireRole(*userOpt, {"admin", "manager"}, res)) return;

        auto logs = db.getAuditLogs(150);
        json arr = json::array();
        for (const auto& l : logs) {
            arr.push_back(l.toJson());
        }
        sendJson(res, 200, {{"success", true}, {"logs", arr}});
    });

    // GET /api/manager/accounts
    svr.Get("/api/manager/accounts", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        if (!requireRole(*userOpt, {"manager", "admin"}, res)) return;

        auto accounts = db.getAllAccounts();
        json arr = json::array();
        for (const auto& a : accounts) {
            arr.push_back(a.toJson());
        }
        sendJson(res, 200, {{"success", true}, {"accounts", arr}});
    });

    // POST /api/manager/account-status
    svr.Post("/api/manager/account-status", [&db](const httplib::Request& req, httplib::Response& res) {
        auto userOpt = authenticateRequest(req, res, db);
        if (!userOpt.has_value()) return;

        if (!requireRole(*userOpt, {"manager", "admin"}, res)) return;

        try {
            json body = json::parse(req.body);
            int accountId = body.value("account_id", 0);
            std::string status = body.value("status", "frozen");

            if (accountId <= 0 || (status != "active" && status != "frozen" && status != "closed")) {
                sendError(res, 400, "Valid account_id and status ('active', 'frozen', or 'closed') required.");
                return;
            }

            if (!db.updateAccountStatus(accountId, status)) {
                sendError(res, 400, "Failed to update account status.");
                return;
            }

            db.logAudit(userOpt->user_id, "ACCOUNT_STATUS_CHANGE", "Changed status of account #" + std::to_string(accountId) + " to " + status, req.remote_addr);

            sendJson(res, 200, {
                {"success", true},
                {"message", "Account status updated to " + status}
            });
        } catch (const std::exception& e) {
            sendError(res, 400, e.what());
        }
    });

    // -------------------------------------------------------------
    // FINANCIAL CALCULATOR API (For quick server-verified computation)
    // -------------------------------------------------------------
    svr.Post("/api/calculator/emi", [](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = json::parse(req.body);
            double principal = body.value("principal", 10000.0);
            double rate = body.value("rate", 8.5);
            int months = body.value("months", 12);

            double emi = Utils::calculateMonthlyEMI(principal, rate, months);
            double totalPayable = Utils::roundTo2Decimals(emi * months);
            double totalInterest = Utils::roundTo2Decimals(totalPayable - principal);

            sendJson(res, 200, {
                {"success", true},
                {"principal", principal},
                {"annual_rate", rate},
                {"months", months},
                {"monthly_emi", emi},
                {"total_interest", totalInterest},
                {"total_payable", totalPayable}
            });
        } catch (const std::exception& e) {
            sendError(res, 400, e.what());
        }
    });
}
