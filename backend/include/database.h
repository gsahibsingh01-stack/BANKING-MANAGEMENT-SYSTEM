#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <optional>
#include <mutex>
#include "../../dependencies/sqlite/sqlite3.h"
#include "../../dependencies/json.hpp"
#include "models.h"

using json = nlohmann::json;

class Database {
public:
    Database();
    ~Database();

    bool open(const std::string& dbPath);
    void close();
    bool execute(const std::string& sql);

    // Schema & Data Init
    bool initSchemaAndSeed(const std::string& schemaPath, const std::string& seedPath);

    // User Operations
    std::optional<Models::User> getUserByUsername(const std::string& username);
    std::optional<Models::User> getUserById(int userId);
    std::vector<Models::User> getAllUsers();
    int createUser(const Models::User& user);
    bool updateUserStatus(int userId, const std::string& status);
    bool updateUserRole(int userId, const std::string& role);
    bool updatePassword(int userId, const std::string& newHash, const std::string& newSalt);
    bool updateProfile(int userId, const std::string& fullName, const std::string& email, const std::string& phone);

    // Sessions
    bool createSession(const std::string& token, int userId, int expiryHours = 24);
    std::optional<Models::User> getUserBySessionToken(const std::string& token);
    bool deleteSession(const std::string& token);

    // Accounts
    std::vector<Models::Account> getAccountsByUserId(int userId);
    std::optional<Models::Account> getAccountById(int accountId);
    std::optional<Models::Account> getAccountByNumber(const std::string& accountNumber);
    std::vector<Models::Account> getAllAccounts();
    int createAccount(int userId, const std::string& accountType, double initialDeposit, const std::string& currency, double interestRate);
    bool updateAccountStatus(int accountId, const std::string& status);

    // Transactions (with ACID safety)
    bool executeDeposit(int accountId, double amount, const std::string& description, std::string& outRefNumber);
    bool executeWithdrawal(int accountId, double amount, const std::string& description, std::string& outRefNumber, std::string& outError);
    bool executeTransfer(int fromAccountId, int toAccountId, double amount, const std::string& description, std::string& outRefNumber, std::string& outError);
    std::vector<Models::Transaction> getTransactionsByAccountId(int accountId, int limit = 50);
    std::vector<Models::Transaction> getTransactionsByUserId(int userId, int limit = 100);
    std::vector<Models::Transaction> getAllTransactions(int limit = 200);

    // Loans
    int applyForLoan(int userId, int accountId, const std::string& loanType, double principal, double rate, int months, const std::string& purpose);
    std::vector<Models::Loan> getLoansByUserId(int userId);
    std::vector<Models::Loan> getAllLoans();
    std::optional<Models::Loan> getLoanById(int loanId);
    bool reviewLoan(int loanId, int reviewerId, const std::string& decision, const std::string& reason, std::string& outError);
    bool repayLoan(int loanId, int accountId, double amount, std::string& outError);

    // Cards
    std::vector<Models::Card> getCardsByUserId(int userId);
    std::vector<Models::Card> getCardsByAccountId(int accountId);
    std::optional<Models::Card> getCardById(int cardId);
    int issueCard(int accountId, const std::string& holderName, const std::string& cardType, const std::string& pin, double dailyLimit);
    bool toggleCardLock(int cardId, int userId);
    bool updateCardLimit(int cardId, int userId, double newLimit);

    // Beneficiaries
    std::vector<Models::Beneficiary> getBeneficiariesByUserId(int userId);
    int addBeneficiary(int userId, const std::string& name, const std::string& accNumber, const std::string& bankName, const std::string& routing, const std::string& nickname, const std::string& email);
    bool deleteBeneficiary(int beneficiaryId, int userId);

    // Auditing & Stats
    void logAudit(int userId, const std::string& action, const std::string& details, const std::string& ip);
    std::vector<Models::AuditLog> getAuditLogs(int limit = 100);
    json getSystemStats();

private:
    sqlite3* db_ = nullptr;
    std::mutex dbMutex_;

    std::string readFileContents(const std::string& path);
};

#endif // DATABASE_H
