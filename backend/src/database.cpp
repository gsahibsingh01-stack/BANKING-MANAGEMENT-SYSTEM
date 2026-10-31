#include "../include/database.h"
#include "../include/utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

Database::Database() : db_(nullptr) {}

Database::~Database() {
    close();
}

std::string Database::readFileContents(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

bool Database::open(const std::string& dbPath) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }

    int rc = sqlite3_open(dbPath.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "[Database] Failed to open database " << dbPath << ": " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    char* errMsg = nullptr;
    sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &errMsg);
    sqlite3_exec(db_, "PRAGMA journal_mode = WAL;", nullptr, nullptr, &errMsg);
    if (errMsg) sqlite3_free(errMsg);

    std::cout << "[Database] Connected to SQLite database: " << dbPath << std::endl;
    return true;
}

void Database::close() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool Database::execute(const std::string& sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[Database] SQL Exec Error: " << (errMsg ? errMsg : "Unknown") << " | SQL: " << sql << std::endl;
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool Database::initSchemaAndSeed(const std::string& schemaPath, const std::string& seedPath) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return false;

    // Check if users table exists
    sqlite3_stmt* stmt = nullptr;
    std::string checkSql = "SELECT name FROM sqlite_master WHERE type='table' AND name='users';";
    int rc = sqlite3_prepare_v2(db_, checkSql.c_str(), -1, &stmt, nullptr);
    bool hasUsersTable = false;
    if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
        hasUsersTable = true;
    }
    sqlite3_finalize(stmt);

    if (!hasUsersTable) {
        std::cout << "[Database] Initializing schema..." << std::endl;
        std::string schemaSql = readFileContents(schemaPath);
        if (schemaSql.empty()) {
            std::cerr << "[Database] Could not read " << schemaPath << ", attempting fallback..." << std::endl;
        } else {
            char* errMsg = nullptr;
            if (sqlite3_exec(db_, schemaSql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
                std::cerr << "[Database] Schema init error: " << (errMsg ? errMsg : "") << std::endl;
                if (errMsg) sqlite3_free(errMsg);
            }
        }

        std::cout << "[Database] Seeding initial data..." << std::endl;
        std::string seedSql = readFileContents(seedPath);
        if (!seedSql.empty()) {
            char* errMsg = nullptr;
            if (sqlite3_exec(db_, seedSql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
                std::cerr << "[Database] Seed data error: " << (errMsg ? errMsg : "") << std::endl;
                if (errMsg) sqlite3_free(errMsg);
            }
        }
    } else {
        std::cout << "[Database] Database schema already present." << std::endl;
    }

    return true;
}

// User Operations
std::optional<Models::User> Database::getUserByUsername(const std::string& username) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return std::nullopt;

    const char* sql = "SELECT user_id, username, password_hash, salt, full_name, email, phone, role, status, created_at "
                      "FROM users WHERE username = ? COLLATE NOCASE;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    std::optional<Models::User> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Models::User u;
        u.user_id = sqlite3_column_int(stmt, 0);
        u.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        u.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        u.salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        u.full_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        u.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const char* ph = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        u.phone = ph ? ph : "";
        u.role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        u.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        u.created_at = ca ? ca : "";
        result = u;
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<Models::User> Database::getUserById(int userId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return std::nullopt;

    const char* sql = "SELECT user_id, username, password_hash, salt, full_name, email, phone, role, status, created_at "
                      "FROM users WHERE user_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_int(stmt, 1, userId);

    std::optional<Models::User> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Models::User u;
        u.user_id = sqlite3_column_int(stmt, 0);
        u.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        u.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        u.salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        u.full_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        u.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const char* ph = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        u.phone = ph ? ph : "";
        u.role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        u.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        u.created_at = ca ? ca : "";
        result = u;
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<Models::User> Database::getAllUsers() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<Models::User> list;
    if (!db_) return list;

    const char* sql = "SELECT user_id, username, password_hash, salt, full_name, email, phone, role, status, created_at "
                      "FROM users ORDER BY user_id ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Models::User u;
        u.user_id = sqlite3_column_int(stmt, 0);
        u.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        u.password_hash = ""; // hidden
        u.salt = "";
        u.full_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        u.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const char* ph = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        u.phone = ph ? ph : "";
        u.role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        u.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        u.created_at = ca ? ca : "";
        list.push_back(u);
    }
    sqlite3_finalize(stmt);
    return list;
}

int Database::createUser(const Models::User& user) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return -1;

    const char* sql = "INSERT INTO users (username, password_hash, salt, full_name, email, phone, role, status) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, user.username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user.password_hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, user.salt.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, user.full_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, user.email.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, user.phone.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, user.role.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, user.status.c_str(), -1, SQLITE_STATIC);

    int newId = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        newId = static_cast<int>(sqlite3_last_insert_rowid(db_));
    }
    sqlite3_finalize(stmt);
    return newId;
}

bool Database::updateUserStatus(int userId, const std::string& status) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return false;

    const char* sql = "UPDATE users SET status = ? WHERE user_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, userId);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool Database::updateUserRole(int userId, const std::string& role) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return false;

    const char* sql = "UPDATE users SET role = ? WHERE user_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, role.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, userId);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool Database::updatePassword(int userId, const std::string& newHash, const std::string& newSalt) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return false;

    const char* sql = "UPDATE users SET password_hash = ?, salt = ? WHERE user_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, newHash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, newSalt.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, userId);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool Database::updateProfile(int userId, const std::string& fullName, const std::string& email, const std::string& phone) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return false;

    const char* sql = "UPDATE users SET full_name = ?, email = ?, phone = ?, updated_at = CURRENT_TIMESTAMP WHERE user_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, fullName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, email.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, phone.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, userId);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

// Sessions
bool Database::createSession(const std::string& token, int userId, int expiryHours) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return false;

    const char* sql = "INSERT OR REPLACE INTO sessions (token, user_id, expires_at) "
                      "VALUES (?, ?, datetime('now', '+' || ? || ' hours'));";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, userId);
    sqlite3_bind_int(stmt, 3, expiryHours);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

std::optional<Models::User> Database::getUserBySessionToken(const std::string& token) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return std::nullopt;

    const char* sql = "SELECT u.user_id, u.username, u.password_hash, u.salt, u.full_name, u.email, u.phone, u.role, u.status, u.created_at "
                      "FROM users u INNER JOIN sessions s ON u.user_id = s.user_id "
                      "WHERE s.token = ? AND datetime('now') < s.expires_at;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_STATIC);

    std::optional<Models::User> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Models::User u;
        u.user_id = sqlite3_column_int(stmt, 0);
        u.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        u.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        u.salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        u.full_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        u.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const char* ph = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        u.phone = ph ? ph : "";
        u.role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        u.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        u.created_at = ca ? ca : "";
        result = u;
    }
    sqlite3_finalize(stmt);
    return result;
}

bool Database::deleteSession(const std::string& token) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return false;

    const char* sql = "DELETE FROM sessions WHERE token = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_STATIC);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

// Accounts
std::vector<Models::Account> Database::getAccountsByUserId(int userId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<Models::Account> list;
    if (!db_) return list;

    const char* sql = "SELECT a.account_id, a.user_id, a.account_number, a.account_type, a.balance, a.currency, "
                      "a.interest_rate, a.status, a.created_at, u.full_name "
                      "FROM accounts a INNER JOIN users u ON a.user_id = u.user_id "
                      "WHERE a.user_id = ? ORDER BY a.account_id ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    sqlite3_bind_int(stmt, 1, userId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Models::Account acc;
        acc.account_id = sqlite3_column_int(stmt, 0);
        acc.user_id = sqlite3_column_int(stmt, 1);
        acc.account_number = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        acc.account_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        acc.balance = sqlite3_column_double(stmt, 4);
        acc.currency = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        acc.interest_rate = sqlite3_column_double(stmt, 6);
        acc.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        acc.created_at = ca ? ca : "";
        const char* fn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        acc.owner_name = fn ? fn : "";
        list.push_back(acc);
    }
    sqlite3_finalize(stmt);
    return list;
}

std::optional<Models::Account> Database::getAccountById(int accountId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return std::nullopt;

    const char* sql = "SELECT a.account_id, a.user_id, a.account_number, a.account_type, a.balance, a.currency, "
                      "a.interest_rate, a.status, a.created_at, u.full_name "
                      "FROM accounts a INNER JOIN users u ON a.user_id = u.user_id "
                      "WHERE a.account_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_int(stmt, 1, accountId);

    std::optional<Models::Account> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Models::Account acc;
        acc.account_id = sqlite3_column_int(stmt, 0);
        acc.user_id = sqlite3_column_int(stmt, 1);
        acc.account_number = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        acc.account_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        acc.balance = sqlite3_column_double(stmt, 4);
        acc.currency = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        acc.interest_rate = sqlite3_column_double(stmt, 6);
        acc.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        acc.created_at = ca ? ca : "";
        const char* fn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        acc.owner_name = fn ? fn : "";
        result = acc;
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<Models::Account> Database::getAccountByNumber(const std::string& accountNumber) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return std::nullopt;

    const char* sql = "SELECT a.account_id, a.user_id, a.account_number, a.account_type, a.balance, a.currency, "
                      "a.interest_rate, a.status, a.created_at, u.full_name "
                      "FROM accounts a INNER JOIN users u ON a.user_id = u.user_id "
                      "WHERE a.account_number = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_text(stmt, 1, accountNumber.c_str(), -1, SQLITE_STATIC);

    std::optional<Models::Account> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Models::Account acc;
        acc.account_id = sqlite3_column_int(stmt, 0);
        acc.user_id = sqlite3_column_int(stmt, 1);
        acc.account_number = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        acc.account_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        acc.balance = sqlite3_column_double(stmt, 4);
        acc.currency = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        acc.interest_rate = sqlite3_column_double(stmt, 6);
        acc.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        acc.created_at = ca ? ca : "";
        const char* fn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        acc.owner_name = fn ? fn : "";
        result = acc;
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<Models::Account> Database::getAllAccounts() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<Models::Account> list;
    if (!db_) return list;

    const char* sql = "SELECT a.account_id, a.user_id, a.account_number, a.account_type, a.balance, a.currency, "
                      "a.interest_rate, a.status, a.created_at, u.full_name "
                      "FROM accounts a INNER JOIN users u ON a.user_id = u.user_id "
                      "ORDER BY a.account_id DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Models::Account acc;
        acc.account_id = sqlite3_column_int(stmt, 0);
        acc.user_id = sqlite3_column_int(stmt, 1);
        acc.account_number = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        acc.account_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        acc.balance = sqlite3_column_double(stmt, 4);
        acc.currency = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        acc.interest_rate = sqlite3_column_double(stmt, 6);
        acc.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        acc.created_at = ca ? ca : "";
        const char* fn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        acc.owner_name = fn ? fn : "";
        list.push_back(acc);
    }
    sqlite3_finalize(stmt);
    return list;
}

int Database::createAccount(int userId, const std::string& accountType, double initialDeposit, const std::string& currency, double interestRate) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return -1;

    std::string accNum = Utils::generateAccountNumber();

    execute("BEGIN TRANSACTION;");

    const char* sql = "INSERT INTO accounts (user_id, account_number, account_type, balance, currency, interest_rate, status) "
                      "VALUES (?, ?, ?, ?, ?, ?, 'active');";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        execute("ROLLBACK;");
        return -1;
    }

    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_text(stmt, 2, accNum.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, accountType.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 4, initialDeposit);
    sqlite3_bind_text(stmt, 5, currency.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 6, interestRate);

    int newAccId = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        newAccId = static_cast<int>(sqlite3_last_insert_rowid(db_));
    }
    sqlite3_finalize(stmt);

    if (newAccId <= 0) {
        execute("ROLLBACK;");
        return -1;
    }

    if (initialDeposit > 0.0) {
        std::string ref = Utils::generateTransactionReference();
        const char* txSql = "INSERT INTO transactions (reference_number, to_account_id, type, amount, balance_after, description, status) "
                            "VALUES (?, ?, 'DEPOSIT', ?, ?, 'Initial Account Opening Deposit', 'COMPLETED');";
        sqlite3_stmt* txStmt = nullptr;
        if (sqlite3_prepare_v2(db_, txSql, -1, &txStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(txStmt, 1, ref.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(txStmt, 2, newAccId);
            sqlite3_bind_double(txStmt, 3, initialDeposit);
            sqlite3_bind_double(txStmt, 4, initialDeposit);
            sqlite3_step(txStmt);
            sqlite3_finalize(txStmt);
        }
    }

    execute("COMMIT;");
    return newAccId;
}

bool Database::updateAccountStatus(int accountId, const std::string& status) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return false;

    const char* sql = "UPDATE accounts SET status = ? WHERE account_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, accountId);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

// Transactions
bool Database::executeDeposit(int accountId, double amount, const std::string& description, std::string& outRefNumber) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_ || amount <= 0.0) return false;

    execute("BEGIN TRANSACTION;");

    // Fetch current balance and status
    const char* chkSql = "SELECT balance, status FROM accounts WHERE account_id = ?;";
    sqlite3_stmt* chkStmt = nullptr;
    if (sqlite3_prepare_v2(db_, chkSql, -1, &chkStmt, nullptr) != SQLITE_OK) {
        execute("ROLLBACK;");
        return false;
    }
    sqlite3_bind_int(chkStmt, 1, accountId);

    if (sqlite3_step(chkStmt) != SQLITE_ROW) {
        sqlite3_finalize(chkStmt);
        execute("ROLLBACK;");
        return false;
    }

    double currentBalance = sqlite3_column_double(chkStmt, 0);
    std::string status = reinterpret_cast<const char*>(sqlite3_column_text(chkStmt, 1));
    sqlite3_finalize(chkStmt);

    if (status != "active") {
        execute("ROLLBACK;");
        return false;
    }

    double newBalance = Utils::roundTo2Decimals(currentBalance + amount);

    // Update account
    const char* upSql = "UPDATE accounts SET balance = ? WHERE account_id = ?;";
    sqlite3_stmt* upStmt = nullptr;
    if (sqlite3_prepare_v2(db_, upSql, -1, &upStmt, nullptr) != SQLITE_OK) {
        execute("ROLLBACK;");
        return false;
    }
    sqlite3_bind_double(upStmt, 1, newBalance);
    sqlite3_bind_int(upStmt, 2, accountId);
    if (sqlite3_step(upStmt) != SQLITE_DONE) {
        sqlite3_finalize(upStmt);
        execute("ROLLBACK;");
        return false;
    }
    sqlite3_finalize(upStmt);

    outRefNumber = Utils::generateTransactionReference();
    const char* txSql = "INSERT INTO transactions (reference_number, to_account_id, type, amount, balance_after, description, status) "
                        "VALUES (?, ?, 'DEPOSIT', ?, ?, ?, 'COMPLETED');";
    sqlite3_stmt* txStmt = nullptr;
    if (sqlite3_prepare_v2(db_, txSql, -1, &txStmt, nullptr) != SQLITE_OK) {
        execute("ROLLBACK;");
        return false;
    }
    sqlite3_bind_text(txStmt, 1, outRefNumber.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(txStmt, 2, accountId);
    sqlite3_bind_double(txStmt, 3, amount);
    sqlite3_bind_double(txStmt, 4, newBalance);
    sqlite3_bind_text(txStmt, 5, description.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(txStmt) != SQLITE_DONE) {
        sqlite3_finalize(txStmt);
        execute("ROLLBACK;");
        return false;
    }
    sqlite3_finalize(txStmt);

    execute("COMMIT;");
    return true;
}

bool Database::executeWithdrawal(int accountId, double amount, const std::string& description, std::string& outRefNumber, std::string& outError) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_ || amount <= 0.0) {
        outError = "Invalid withdrawal amount.";
        return false;
    }

    execute("BEGIN TRANSACTION;");

    const char* chkSql = "SELECT balance, status FROM accounts WHERE account_id = ?;";
    sqlite3_stmt* chkStmt = nullptr;
    if (sqlite3_prepare_v2(db_, chkSql, -1, &chkStmt, nullptr) != SQLITE_OK) {
        execute("ROLLBACK;");
        outError = "Internal database error.";
        return false;
    }
    sqlite3_bind_int(chkStmt, 1, accountId);

    if (sqlite3_step(chkStmt) != SQLITE_ROW) {
        sqlite3_finalize(chkStmt);
        execute("ROLLBACK;");
        outError = "Account not found.";
        return false;
    }

    double currentBalance = sqlite3_column_double(chkStmt, 0);
    std::string status = reinterpret_cast<const char*>(sqlite3_column_text(chkStmt, 1));
    sqlite3_finalize(chkStmt);

    if (status != "active") {
        execute("ROLLBACK;");
        outError = "Account is frozen or closed.";
        return false;
    }

    if (currentBalance < amount) {
        execute("ROLLBACK;");
        outError = "Insufficient funds in account.";
        return false;
    }

    double newBalance = Utils::roundTo2Decimals(currentBalance - amount);

    const char* upSql = "UPDATE accounts SET balance = ? WHERE account_id = ?;";
    sqlite3_stmt* upStmt = nullptr;
    if (sqlite3_prepare_v2(db_, upSql, -1, &upStmt, nullptr) != SQLITE_OK) {
        execute("ROLLBACK;");
        outError = "Failed to update balance.";
        return false;
    }
    sqlite3_bind_double(upStmt, 1, newBalance);
    sqlite3_bind_int(upStmt, 2, accountId);
    if (sqlite3_step(upStmt) != SQLITE_DONE) {
        sqlite3_finalize(upStmt);
        execute("ROLLBACK;");
        outError = "Balance deduction failed.";
        return false;
    }
    sqlite3_finalize(upStmt);

    outRefNumber = Utils::generateTransactionReference();
    const char* txSql = "INSERT INTO transactions (reference_number, from_account_id, type, amount, balance_after, description, status) "
                        "VALUES (?, ?, 'WITHDRAW', ?, ?, ?, 'COMPLETED');";
    sqlite3_stmt* txStmt = nullptr;
    if (sqlite3_prepare_v2(db_, txSql, -1, &txStmt, nullptr) != SQLITE_OK) {
        execute("ROLLBACK;");
        outError = "Failed to record transaction.";
        return false;
    }
    sqlite3_bind_text(txStmt, 1, outRefNumber.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(txStmt, 2, accountId);
    sqlite3_bind_double(txStmt, 3, amount);
    sqlite3_bind_double(txStmt, 4, newBalance);
    sqlite3_bind_text(txStmt, 5, description.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(txStmt) != SQLITE_DONE) {
        sqlite3_finalize(txStmt);
        execute("ROLLBACK;");
        outError = "Failed to finalize transaction.";
        return false;
    }
    sqlite3_finalize(txStmt);

    execute("COMMIT;");
    return true;
}

bool Database::executeTransfer(int fromAccountId, int toAccountId, double amount, const std::string& description, std::string& outRefNumber, std::string& outError) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_ || amount <= 0.0) {
        outError = "Invalid transfer amount.";
        return false;
    }

    if (fromAccountId == toAccountId) {
        outError = "Cannot transfer funds to the same account.";
        return false;
    }

    execute("BEGIN TRANSACTION;");

    // Fetch Sender
    const char* sndSql = "SELECT balance, status FROM accounts WHERE account_id = ?;";
    sqlite3_stmt* sndStmt = nullptr;
    if (sqlite3_prepare_v2(db_, sndSql, -1, &sndStmt, nullptr) != SQLITE_OK) {
        execute("ROLLBACK;");
        outError = "Database error verifying sender.";
        return false;
    }
    sqlite3_bind_int(sndStmt, 1, fromAccountId);
    if (sqlite3_step(sndStmt) != SQLITE_ROW) {
        sqlite3_finalize(sndStmt);
        execute("ROLLBACK;");
        outError = "Source account not found.";
        return false;
    }
    double sndBalance = sqlite3_column_double(sndStmt, 0);
    std::string sndStatus = reinterpret_cast<const char*>(sqlite3_column_text(sndStmt, 1));
    sqlite3_finalize(sndStmt);

    if (sndStatus != "active") {
        execute("ROLLBACK;");
        outError = "Source account is not active.";
        return false;
    }
    if (sndBalance < amount) {
        execute("ROLLBACK;");
        outError = "Insufficient balance for transfer.";
        return false;
    }

    // Fetch Recipient
    const char* rcvSql = "SELECT balance, status FROM accounts WHERE account_id = ?;";
    sqlite3_stmt* rcvStmt = nullptr;
    if (sqlite3_prepare_v2(db_, rcvSql, -1, &rcvStmt, nullptr) != SQLITE_OK) {
        execute("ROLLBACK;");
        outError = "Database error verifying recipient.";
        return false;
    }
    sqlite3_bind_int(rcvStmt, 1, toAccountId);
    if (sqlite3_step(rcvStmt) != SQLITE_ROW) {
        sqlite3_finalize(rcvStmt);
        execute("ROLLBACK;");
        outError = "Destination account not found.";
        return false;
    }
    double rcvBalance = sqlite3_column_double(rcvStmt, 0);
    std::string rcvStatus = reinterpret_cast<const char*>(sqlite3_column_text(rcvStmt, 1));
    sqlite3_finalize(rcvStmt);

    if (rcvStatus != "active") {
        execute("ROLLBACK;");
        outError = "Destination account is frozen or closed.";
        return false;
    }

    double newSndBalance = Utils::roundTo2Decimals(sndBalance - amount);
    double newRcvBalance = Utils::roundTo2Decimals(rcvBalance + amount);

    // Deduct sender
    const char* upSndSql = "UPDATE accounts SET balance = ? WHERE account_id = ?;";
    sqlite3_stmt* upSndStmt = nullptr;
    if (sqlite3_prepare_v2(db_, upSndSql, -1, &upSndStmt, nullptr) != SQLITE_OK) {
        execute("ROLLBACK;");
        outError = "Failed updating source account balance.";
        return false;
    }
    sqlite3_bind_double(upSndStmt, 1, newSndBalance);
    sqlite3_bind_int(upSndStmt, 2, fromAccountId);
    if (sqlite3_step(upSndStmt) != SQLITE_DONE) {
        sqlite3_finalize(upSndStmt);
        execute("ROLLBACK;");
        outError = "Failed debiting source balance.";
        return false;
    }
    sqlite3_finalize(upSndStmt);

    // Credit recipient
    const char* upRcvSql = "UPDATE accounts SET balance = ? WHERE account_id = ?;";
    sqlite3_stmt* upRcvStmt = nullptr;
    if (sqlite3_prepare_v2(db_, upRcvSql, -1, &upRcvStmt, nullptr) != SQLITE_OK) {
        execute("ROLLBACK;");
        outError = "Failed updating destination balance.";
        return false;
    }
    sqlite3_bind_double(upRcvStmt, 1, newRcvBalance);
    sqlite3_bind_int(upRcvStmt, 2, toAccountId);
    if (sqlite3_step(upRcvStmt) != SQLITE_DONE) {
        sqlite3_finalize(upRcvStmt);
        execute("ROLLBACK;");
        outError = "Failed crediting destination balance.";
        return false;
    }
    sqlite3_finalize(upRcvStmt);

    // Record Transaction
    outRefNumber = Utils::generateTransactionReference();
    const char* txSql = "INSERT INTO transactions (reference_number, from_account_id, to_account_id, type, amount, balance_after, description, status) "
                        "VALUES (?, ?, ?, 'TRANSFER', ?, ?, ?, 'COMPLETED');";
    sqlite3_stmt* txStmt = nullptr;
    if (sqlite3_prepare_v2(db_, txSql, -1, &txStmt, nullptr) != SQLITE_OK) {
        execute("ROLLBACK;");
        outError = "Failed recording transfer log.";
        return false;
    }
    sqlite3_bind_text(txStmt, 1, outRefNumber.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(txStmt, 2, fromAccountId);
    sqlite3_bind_int(txStmt, 3, toAccountId);
    sqlite3_bind_double(txStmt, 4, amount);
    sqlite3_bind_double(txStmt, 5, newSndBalance);
    sqlite3_bind_text(txStmt, 6, description.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(txStmt) != SQLITE_DONE) {
        sqlite3_finalize(txStmt);
        execute("ROLLBACK;");
        outError = "Failed finalizing transfer record.";
        return false;
    }
    sqlite3_finalize(txStmt);

    execute("COMMIT;");
    return true;
}

std::vector<Models::Transaction> Database::getTransactionsByAccountId(int accountId, int limit) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<Models::Transaction> list;
    if (!db_) return list;

    const char* sql = "SELECT t.transaction_id, t.reference_number, t.from_account_id, t.to_account_id, "
                      "fa.account_number AS from_acc_num, ta.account_number AS to_acc_num, "
                      "t.type, t.amount, t.balance_after, t.description, t.status, t.created_at "
                      "FROM transactions t "
                      "LEFT JOIN accounts fa ON t.from_account_id = fa.account_id "
                      "LEFT JOIN accounts ta ON t.to_account_id = ta.account_id "
                      "WHERE t.from_account_id = ? OR t.to_account_id = ? "
                      "ORDER BY t.created_at DESC, t.transaction_id DESC LIMIT ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    sqlite3_bind_int(stmt, 1, accountId);
    sqlite3_bind_int(stmt, 2, accountId);
    sqlite3_bind_int(stmt, 3, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Models::Transaction tx;
        tx.transaction_id = sqlite3_column_int(stmt, 0);
        tx.reference_number = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        tx.from_account_id = sqlite3_column_type(stmt, 2) != SQLITE_NULL ? sqlite3_column_int(stmt, 2) : 0;
        tx.to_account_id = sqlite3_column_type(stmt, 3) != SQLITE_NULL ? sqlite3_column_int(stmt, 3) : 0;
        const char* fn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        tx.from_account_number = fn ? fn : "";
        const char* tn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        tx.to_account_number = tn ? tn : "";
        tx.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        tx.amount = sqlite3_column_double(stmt, 7);
        tx.balance_after = sqlite3_column_double(stmt, 8);
        const char* desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        tx.description = desc ? desc : "";
        tx.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        tx.created_at = ca ? ca : "";
        list.push_back(tx);
    }
    sqlite3_finalize(stmt);
    return list;
}

std::vector<Models::Transaction> Database::getTransactionsByUserId(int userId, int limit) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<Models::Transaction> list;
    if (!db_) return list;

    const char* sql = "SELECT DISTINCT t.transaction_id, t.reference_number, t.from_account_id, t.to_account_id, "
                      "fa.account_number AS from_acc_num, ta.account_number AS to_acc_num, "
                      "t.type, t.amount, t.balance_after, t.description, t.status, t.created_at "
                      "FROM transactions t "
                      "LEFT JOIN accounts fa ON t.from_account_id = fa.account_id "
                      "LEFT JOIN accounts ta ON t.to_account_id = ta.account_id "
                      "WHERE fa.user_id = ? OR ta.user_id = ? "
                      "ORDER BY t.created_at DESC, t.transaction_id DESC LIMIT ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, userId);
    sqlite3_bind_int(stmt, 3, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Models::Transaction tx;
        tx.transaction_id = sqlite3_column_int(stmt, 0);
        tx.reference_number = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        tx.from_account_id = sqlite3_column_type(stmt, 2) != SQLITE_NULL ? sqlite3_column_int(stmt, 2) : 0;
        tx.to_account_id = sqlite3_column_type(stmt, 3) != SQLITE_NULL ? sqlite3_column_int(stmt, 3) : 0;
        const char* fn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        tx.from_account_number = fn ? fn : "";
        const char* tn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        tx.to_account_number = tn ? tn : "";
        tx.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        tx.amount = sqlite3_column_double(stmt, 7);
        tx.balance_after = sqlite3_column_double(stmt, 8);
        const char* desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        tx.description = desc ? desc : "";
        tx.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        tx.created_at = ca ? ca : "";
        list.push_back(tx);
    }
    sqlite3_finalize(stmt);
    return list;
}

std::vector<Models::Transaction> Database::getAllTransactions(int limit) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<Models::Transaction> list;
    if (!db_) return list;

    const char* sql = "SELECT t.transaction_id, t.reference_number, t.from_account_id, t.to_account_id, "
                      "fa.account_number AS from_acc_num, ta.account_number AS to_acc_num, "
                      "t.type, t.amount, t.balance_after, t.description, t.status, t.created_at "
                      "FROM transactions t "
                      "LEFT JOIN accounts fa ON t.from_account_id = fa.account_id "
                      "LEFT JOIN accounts ta ON t.to_account_id = ta.account_id "
                      "ORDER BY t.created_at DESC, t.transaction_id DESC LIMIT ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    sqlite3_bind_int(stmt, 1, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Models::Transaction tx;
        tx.transaction_id = sqlite3_column_int(stmt, 0);
        tx.reference_number = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        tx.from_account_id = sqlite3_column_type(stmt, 2) != SQLITE_NULL ? sqlite3_column_int(stmt, 2) : 0;
        tx.to_account_id = sqlite3_column_type(stmt, 3) != SQLITE_NULL ? sqlite3_column_int(stmt, 3) : 0;
        const char* fn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        tx.from_account_number = fn ? fn : "";
        const char* tn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        tx.to_account_number = tn ? tn : "";
        tx.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        tx.amount = sqlite3_column_double(stmt, 7);
        tx.balance_after = sqlite3_column_double(stmt, 8);
        const char* desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        tx.description = desc ? desc : "";
        tx.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        tx.created_at = ca ? ca : "";
        list.push_back(tx);
    }
    sqlite3_finalize(stmt);
    return list;
}

// Loans
int Database::applyForLoan(int userId, int accountId, const std::string& loanType, double principal, double rate, int months, const std::string& purpose) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_ || principal <= 0.0 || months <= 0) return -1;

    double monthlyEmi = Utils::calculateMonthlyEMI(principal, rate, months);
    double totalPayable = Utils::roundTo2Decimals(monthlyEmi * months);
    double remaining = totalPayable;

    const char* sql = "INSERT INTO loans (user_id, account_id, loan_type, principal_amount, interest_rate, "
                      "duration_months, monthly_emi, total_payable, remaining_balance, purpose, status) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 'PENDING');";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;

    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, accountId);
    sqlite3_bind_text(stmt, 3, loanType.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 4, principal);
    sqlite3_bind_double(stmt, 5, rate);
    sqlite3_bind_int(stmt, 6, months);
    sqlite3_bind_double(stmt, 7, monthlyEmi);
    sqlite3_bind_double(stmt, 8, totalPayable);
    sqlite3_bind_double(stmt, 9, remaining);
    sqlite3_bind_text(stmt, 10, purpose.c_str(), -1, SQLITE_STATIC);

    int loanId = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        loanId = static_cast<int>(sqlite3_last_insert_rowid(db_));
    }
    sqlite3_finalize(stmt);
    return loanId;
}

std::vector<Models::Loan> Database::getLoansByUserId(int userId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<Models::Loan> list;
    if (!db_) return list;

    const char* sql = "SELECT l.loan_id, l.user_id, l.account_id, u.full_name, a.account_number, "
                      "l.loan_type, l.principal_amount, l.interest_rate, l.duration_months, "
                      "l.monthly_emi, l.total_payable, l.remaining_balance, l.purpose, l.status, "
                      "l.applied_at, l.reviewed_by, ru.full_name, l.reviewed_at, l.rejection_reason "
                      "FROM loans l "
                      "INNER JOIN users u ON l.user_id = u.user_id "
                      "INNER JOIN accounts a ON l.account_id = a.account_id "
                      "LEFT JOIN users ru ON l.reviewed_by = ru.user_id "
                      "WHERE l.user_id = ? ORDER BY l.applied_at DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    sqlite3_bind_int(stmt, 1, userId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Models::Loan ln;
        ln.loan_id = sqlite3_column_int(stmt, 0);
        ln.user_id = sqlite3_column_int(stmt, 1);
        ln.account_id = sqlite3_column_int(stmt, 2);
        const char* un = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        ln.user_name = un ? un : "";
        const char* an = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        ln.account_number = an ? an : "";
        ln.loan_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        ln.principal_amount = sqlite3_column_double(stmt, 6);
        ln.interest_rate = sqlite3_column_double(stmt, 7);
        ln.duration_months = sqlite3_column_int(stmt, 8);
        ln.monthly_emi = sqlite3_column_double(stmt, 9);
        ln.total_payable = sqlite3_column_double(stmt, 10);
        ln.remaining_balance = sqlite3_column_double(stmt, 11);
        const char* pur = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
        ln.purpose = pur ? pur : "";
        ln.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13));
        const char* ap = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 14));
        ln.applied_at = ap ? ap : "";
        ln.reviewed_by = sqlite3_column_type(stmt, 15) != SQLITE_NULL ? sqlite3_column_int(stmt, 15) : 0;
        const char* rvn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 16));
        ln.reviewer_name = rvn ? rvn : "";
        const char* rva = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 17));
        ln.reviewed_at = rva ? rva : "";
        const char* rr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 18));
        ln.rejection_reason = rr ? rr : "";
        list.push_back(ln);
    }
    sqlite3_finalize(stmt);
    return list;
}

std::vector<Models::Loan> Database::getAllLoans() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<Models::Loan> list;
    if (!db_) return list;

    const char* sql = "SELECT l.loan_id, l.user_id, l.account_id, u.full_name, a.account_number, "
                      "l.loan_type, l.principal_amount, l.interest_rate, l.duration_months, "
                      "l.monthly_emi, l.total_payable, l.remaining_balance, l.purpose, l.status, "
                      "l.applied_at, l.reviewed_by, ru.full_name, l.reviewed_at, l.rejection_reason "
                      "FROM loans l "
                      "INNER JOIN users u ON l.user_id = u.user_id "
                      "INNER JOIN accounts a ON l.account_id = a.account_id "
                      "LEFT JOIN users ru ON l.reviewed_by = ru.user_id "
                      "ORDER BY l.applied_at DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Models::Loan ln;
        ln.loan_id = sqlite3_column_int(stmt, 0);
        ln.user_id = sqlite3_column_int(stmt, 1);
        ln.account_id = sqlite3_column_int(stmt, 2);
        const char* un = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        ln.user_name = un ? un : "";
        const char* an = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        ln.account_number = an ? an : "";
        ln.loan_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        ln.principal_amount = sqlite3_column_double(stmt, 6);
        ln.interest_rate = sqlite3_column_double(stmt, 7);
        ln.duration_months = sqlite3_column_int(stmt, 8);
        ln.monthly_emi = sqlite3_column_double(stmt, 9);
        ln.total_payable = sqlite3_column_double(stmt, 10);
        ln.remaining_balance = sqlite3_column_double(stmt, 11);
        const char* pur = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
        ln.purpose = pur ? pur : "";
        ln.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13));
        const char* ap = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 14));
        ln.applied_at = ap ? ap : "";
        ln.reviewed_by = sqlite3_column_type(stmt, 15) != SQLITE_NULL ? sqlite3_column_int(stmt, 15) : 0;
        const char* rvn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 16));
        ln.reviewer_name = rvn ? rvn : "";
        const char* rva = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 17));
        ln.reviewed_at = rva ? rva : "";
        const char* rr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 18));
        ln.rejection_reason = rr ? rr : "";
        list.push_back(ln);
    }
    sqlite3_finalize(stmt);
    return list;
}

std::optional<Models::Loan> Database::getLoanById(int loanId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return std::nullopt;

    const char* sql = "SELECT l.loan_id, l.user_id, l.account_id, u.full_name, a.account_number, "
                      "l.loan_type, l.principal_amount, l.interest_rate, l.duration_months, "
                      "l.monthly_emi, l.total_payable, l.remaining_balance, l.purpose, l.status, "
                      "l.applied_at, l.reviewed_by, ru.full_name, l.reviewed_at, l.rejection_reason "
                      "FROM loans l "
                      "INNER JOIN users u ON l.user_id = u.user_id "
                      "INNER JOIN accounts a ON l.account_id = a.account_id "
                      "LEFT JOIN users ru ON l.reviewed_by = ru.user_id "
                      "WHERE l.loan_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_int(stmt, 1, loanId);

    std::optional<Models::Loan> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Models::Loan ln;
        ln.loan_id = sqlite3_column_int(stmt, 0);
        ln.user_id = sqlite3_column_int(stmt, 1);
        ln.account_id = sqlite3_column_int(stmt, 2);
        const char* un = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        ln.user_name = un ? un : "";
        const char* an = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        ln.account_number = an ? an : "";
        ln.loan_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        ln.principal_amount = sqlite3_column_double(stmt, 6);
        ln.interest_rate = sqlite3_column_double(stmt, 7);
        ln.duration_months = sqlite3_column_int(stmt, 8);
        ln.monthly_emi = sqlite3_column_double(stmt, 9);
        ln.total_payable = sqlite3_column_double(stmt, 10);
        ln.remaining_balance = sqlite3_column_double(stmt, 11);
        const char* pur = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
        ln.purpose = pur ? pur : "";
        ln.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13));
        const char* ap = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 14));
        ln.applied_at = ap ? ap : "";
        ln.reviewed_by = sqlite3_column_type(stmt, 15) != SQLITE_NULL ? sqlite3_column_int(stmt, 15) : 0;
        const char* rvn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 16));
        ln.reviewer_name = rvn ? rvn : "";
        const char* rva = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 17));
        ln.reviewed_at = rva ? rva : "";
        const char* rr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 18));
        ln.rejection_reason = rr ? rr : "";
        result = ln;
    }
    sqlite3_finalize(stmt);
    return result;
}

bool Database::reviewLoan(int loanId, int reviewerId, const std::string& decision, const std::string& reason, std::string& outError) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) {
        outError = "Database not open.";
        return false;
    }

    execute("BEGIN TRANSACTION;");

    // Fetch loan details
    const char* lnSql = "SELECT account_id, principal_amount, status, loan_type FROM loans WHERE loan_id = ?;";
    sqlite3_stmt* lnStmt = nullptr;
    if (sqlite3_prepare_v2(db_, lnSql, -1, &lnStmt, nullptr) != SQLITE_OK) {
        execute("ROLLBACK;");
        outError = "Failed to query loan.";
        return false;
    }
    sqlite3_bind_int(lnStmt, 1, loanId);

    if (sqlite3_step(lnStmt) != SQLITE_ROW) {
        sqlite3_finalize(lnStmt);
        execute("ROLLBACK;");
        outError = "Loan application not found.";
        return false;
    }

    int accountId = sqlite3_column_int(lnStmt, 0);
    double principal = sqlite3_column_double(lnStmt, 1);
    std::string currentStatus = reinterpret_cast<const char*>(sqlite3_column_text(lnStmt, 2));
    std::string loanType = reinterpret_cast<const char*>(sqlite3_column_text(lnStmt, 3));
    sqlite3_finalize(lnStmt);

    if (currentStatus != "PENDING") {
        execute("ROLLBACK;");
        outError = "Loan application is already " + currentStatus;
        return false;
    }

    if (decision == "APPROVED") {
        // Disburse funds into applicant account
        const char* accSql = "SELECT balance FROM accounts WHERE account_id = ?;";
        sqlite3_stmt* accStmt = nullptr;
        if (sqlite3_prepare_v2(db_, accSql, -1, &accStmt, nullptr) != SQLITE_OK) {
            execute("ROLLBACK;");
            outError = "Failed reading account balance.";
            return false;
        }
        sqlite3_bind_int(accStmt, 1, accountId);
        if (sqlite3_step(accStmt) != SQLITE_ROW) {
            sqlite3_finalize(accStmt);
            execute("ROLLBACK;");
            outError = "Target account not found.";
            return false;
        }
        double currentBal = sqlite3_column_double(accStmt, 0);
        sqlite3_finalize(accStmt);

        double newBal = Utils::roundTo2Decimals(currentBal + principal);
        const char* upAccSql = "UPDATE accounts SET balance = ? WHERE account_id = ?;";
        sqlite3_stmt* upAccStmt = nullptr;
        if (sqlite3_prepare_v2(db_, upAccSql, -1, &upAccStmt, nullptr) != SQLITE_OK) {
            execute("ROLLBACK;");
            outError = "Failed to update account.";
            return false;
        }
        sqlite3_bind_double(upAccStmt, 1, newBal);
        sqlite3_bind_int(upAccStmt, 2, accountId);
        sqlite3_step(upAccStmt);
        sqlite3_finalize(upAccStmt);

        // Record loan disbursement transaction
        std::string ref = Utils::generateTransactionReference();
        const char* txSql = "INSERT INTO transactions (reference_number, to_account_id, type, amount, balance_after, description, status) "
                            "VALUES (?, ?, 'LOAN_DISBURSEMENT', ?, ?, ?, 'COMPLETED');";
        sqlite3_stmt* txStmt = nullptr;
        if (sqlite3_prepare_v2(db_, txSql, -1, &txStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(txStmt, 1, ref.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(txStmt, 2, accountId);
            sqlite3_bind_double(txStmt, 3, principal);
            sqlite3_bind_double(txStmt, 4, newBal);
            std::string desc = "Approved " + loanType + " Disbursement";
            sqlite3_bind_text(txStmt, 5, desc.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(txStmt);
            sqlite3_finalize(txStmt);
        }

        // Update loan status
        const char* upLnSql = "UPDATE loans SET status = 'APPROVED', reviewed_by = ?, reviewed_at = CURRENT_TIMESTAMP WHERE loan_id = ?;";
        sqlite3_stmt* upLnStmt = nullptr;
        if (sqlite3_prepare_v2(db_, upLnSql, -1, &upLnStmt, nullptr) != SQLITE_OK) {
            execute("ROLLBACK;");
            outError = "Failed to update loan record.";
            return false;
        }
        sqlite3_bind_int(upLnStmt, 1, reviewerId);
        sqlite3_bind_int(upLnStmt, 2, loanId);
        sqlite3_step(upLnStmt);
        sqlite3_finalize(upLnStmt);
    } else {
        // REJECTED
        const char* upLnSql = "UPDATE loans SET status = 'REJECTED', reviewed_by = ?, reviewed_at = CURRENT_TIMESTAMP, rejection_reason = ? WHERE loan_id = ?;";
        sqlite3_stmt* upLnStmt = nullptr;
        if (sqlite3_prepare_v2(db_, upLnSql, -1, &upLnStmt, nullptr) != SQLITE_OK) {
            execute("ROLLBACK;");
            outError = "Failed to reject loan record.";
            return false;
        }
        sqlite3_bind_int(upLnStmt, 1, reviewerId);
        sqlite3_bind_text(upLnStmt, 2, reason.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(upLnStmt, 3, loanId);
        sqlite3_step(upLnStmt);
        sqlite3_finalize(upLnStmt);
    }

    execute("COMMIT;");
    return true;
}

bool Database::repayLoan(int loanId, int accountId, double amount, std::string& outError) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_ || amount <= 0.0) {
        outError = "Invalid repayment amount.";
        return false;
    }

    execute("BEGIN TRANSACTION;");

    // Fetch loan
    const char* lnSql = "SELECT remaining_balance, status, loan_type FROM loans WHERE loan_id = ?;";
    sqlite3_stmt* lnStmt = nullptr;
    if (sqlite3_prepare_v2(db_, lnSql, -1, &lnStmt, nullptr) != SQLITE_OK) {
        execute("ROLLBACK;");
        outError = "Failed to fetch loan.";
        return false;
    }
    sqlite3_bind_int(lnStmt, 1, loanId);
    if (sqlite3_step(lnStmt) != SQLITE_ROW) {
        sqlite3_finalize(lnStmt);
        execute("ROLLBACK;");
        outError = "Loan not found.";
        return false;
    }
    double remaining = sqlite3_column_double(lnStmt, 0);
    std::string loanStatus = reinterpret_cast<const char*>(sqlite3_column_text(lnStmt, 1));
    std::string loanType = reinterpret_cast<const char*>(sqlite3_column_text(lnStmt, 2));
    sqlite3_finalize(lnStmt);

    if (loanStatus != "APPROVED") {
        execute("ROLLBACK;");
        outError = "Loan is not active for repayments.";
        return false;
    }

    // Check account balance
    const char* accSql = "SELECT balance, status FROM accounts WHERE account_id = ?;";
    sqlite3_stmt* accStmt = nullptr;
    if (sqlite3_prepare_v2(db_, accSql, -1, &accStmt, nullptr) != SQLITE_OK) {
        execute("ROLLBACK;");
        outError = "Failed to check account.";
        return false;
    }
    sqlite3_bind_int(accStmt, 1, accountId);
    if (sqlite3_step(accStmt) != SQLITE_ROW) {
        sqlite3_finalize(accStmt);
        execute("ROLLBACK;");
        outError = "Payment account not found.";
        return false;
    }
    double accBal = sqlite3_column_double(accStmt, 0);
    std::string accStat = reinterpret_cast<const char*>(sqlite3_column_text(accStmt, 1));
    sqlite3_finalize(accStmt);

    if (accStat != "active") {
        execute("ROLLBACK;");
        outError = "Payment account is not active.";
        return false;
    }
    if (accBal < amount) {
        execute("ROLLBACK;");
        outError = "Insufficient balance in account to pay loan installment.";
        return false;
    }

    double payAmount = std::min(amount, remaining);
    double newAccBal = Utils::roundTo2Decimals(accBal - payAmount);
    double newRemaining = Utils::roundTo2Decimals(remaining - payAmount);
    std::string newLoanStatus = (newRemaining <= 0.05) ? "CLOSED" : "APPROVED";

    // Deduct account balance
    const char* upAccSql = "UPDATE accounts SET balance = ? WHERE account_id = ?;";
    sqlite3_stmt* upAccStmt = nullptr;
    if (sqlite3_prepare_v2(db_, upAccSql, -1, &upAccStmt, nullptr) != SQLITE_OK) {
        execute("ROLLBACK;");
        outError = "Failed deducting account balance.";
        return false;
    }
    sqlite3_bind_double(upAccStmt, 1, newAccBal);
    sqlite3_bind_int(upAccStmt, 2, accountId);
    sqlite3_step(upAccStmt);
    sqlite3_finalize(upAccStmt);

    // Update loan
    const char* upLnSql = "UPDATE loans SET remaining_balance = ?, status = ? WHERE loan_id = ?;";
    sqlite3_stmt* upLnStmt = nullptr;
    if (sqlite3_prepare_v2(db_, upLnSql, -1, &upLnStmt, nullptr) != SQLITE_OK) {
        execute("ROLLBACK;");
        outError = "Failed updating loan balance.";
        return false;
    }
    sqlite3_bind_double(upLnStmt, 1, newRemaining <= 0.05 ? 0.0 : newRemaining);
    sqlite3_bind_text(upLnStmt, 2, newLoanStatus.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(upLnStmt, 3, loanId);
    sqlite3_step(upLnStmt);
    sqlite3_finalize(upLnStmt);

    // Record transaction
    std::string ref = Utils::generateTransactionReference();
    const char* txSql = "INSERT INTO transactions (reference_number, from_account_id, type, amount, balance_after, description, status) "
                        "VALUES (?, ?, 'LOAN_REPAYMENT', ?, ?, ?, 'COMPLETED');";
    sqlite3_stmt* txStmt = nullptr;
    if (sqlite3_prepare_v2(db_, txSql, -1, &txStmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(txStmt, 1, ref.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(txStmt, 2, accountId);
        sqlite3_bind_double(txStmt, 3, payAmount);
        sqlite3_bind_double(txStmt, 4, newAccBal);
        std::string desc = "EMI / Repayment for " + loanType + " (Loan #" + std::to_string(loanId) + ")";
        sqlite3_bind_text(txStmt, 5, desc.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(txStmt);
        sqlite3_finalize(txStmt);
    }

    execute("COMMIT;");
    return true;
}

// Cards
std::vector<Models::Card> Database::getCardsByUserId(int userId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<Models::Card> list;
    if (!db_) return list;

    const char* sql = "SELECT c.card_id, c.account_id, a.account_number, c.card_number, c.card_holder_name, "
                      "c.card_type, c.expiry_month, c.expiry_year, c.cvv, c.pin, c.daily_limit, c.status, c.created_at "
                      "FROM cards c INNER JOIN accounts a ON c.account_id = a.account_id "
                      "WHERE a.user_id = ? ORDER BY c.card_id ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    sqlite3_bind_int(stmt, 1, userId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Models::Card cd;
        cd.card_id = sqlite3_column_int(stmt, 0);
        cd.account_id = sqlite3_column_int(stmt, 1);
        cd.account_number = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        cd.card_number = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        cd.card_holder_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        cd.card_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        cd.expiry_month = sqlite3_column_int(stmt, 6);
        cd.expiry_year = sqlite3_column_int(stmt, 7);
        cd.cvv = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        cd.pin = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        cd.daily_limit = sqlite3_column_double(stmt, 10);
        cd.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
        cd.created_at = ca ? ca : "";
        list.push_back(cd);
    }
    sqlite3_finalize(stmt);
    return list;
}

std::vector<Models::Card> Database::getCardsByAccountId(int accountId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<Models::Card> list;
    if (!db_) return list;

    const char* sql = "SELECT c.card_id, c.account_id, a.account_number, c.card_number, c.card_holder_name, "
                      "c.card_type, c.expiry_month, c.expiry_year, c.cvv, c.pin, c.daily_limit, c.status, c.created_at "
                      "FROM cards c INNER JOIN accounts a ON c.account_id = a.account_id "
                      "WHERE c.account_id = ? ORDER BY c.card_id ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    sqlite3_bind_int(stmt, 1, accountId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Models::Card cd;
        cd.card_id = sqlite3_column_int(stmt, 0);
        cd.account_id = sqlite3_column_int(stmt, 1);
        cd.account_number = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        cd.card_number = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        cd.card_holder_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        cd.card_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        cd.expiry_month = sqlite3_column_int(stmt, 6);
        cd.expiry_year = sqlite3_column_int(stmt, 7);
        cd.cvv = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        cd.pin = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        cd.daily_limit = sqlite3_column_double(stmt, 10);
        cd.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
        cd.created_at = ca ? ca : "";
        list.push_back(cd);
    }
    sqlite3_finalize(stmt);
    return list;
}

std::optional<Models::Card> Database::getCardById(int cardId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return std::nullopt;

    const char* sql = "SELECT c.card_id, c.account_id, a.account_number, c.card_number, c.card_holder_name, "
                      "c.card_type, c.expiry_month, c.expiry_year, c.cvv, c.pin, c.daily_limit, c.status, c.created_at "
                      "FROM cards c INNER JOIN accounts a ON c.account_id = a.account_id "
                      "WHERE c.card_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_int(stmt, 1, cardId);

    std::optional<Models::Card> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Models::Card cd;
        cd.card_id = sqlite3_column_int(stmt, 0);
        cd.account_id = sqlite3_column_int(stmt, 1);
        cd.account_number = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        cd.card_number = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        cd.card_holder_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        cd.card_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        cd.expiry_month = sqlite3_column_int(stmt, 6);
        cd.expiry_year = sqlite3_column_int(stmt, 7);
        cd.cvv = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        cd.pin = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        cd.daily_limit = sqlite3_column_double(stmt, 10);
        cd.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
        cd.created_at = ca ? ca : "";
        result = cd;
    }
    sqlite3_finalize(stmt);
    return result;
}

int Database::issueCard(int accountId, const std::string& holderName, const std::string& cardType, const std::string& pin, double dailyLimit) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return -1;

    std::string cardNum = Utils::generateCardNumber(cardType == "Credit" ? "5412" : "4532");
    std::string cvv = Utils::generateCVV();
    int expMonth = 12;
    int expYear = 2029;

    const char* sql = "INSERT INTO cards (account_id, card_number, card_holder_name, card_type, expiry_month, expiry_year, cvv, pin, daily_limit, status) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, 'ACTIVE');";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;

    sqlite3_bind_int(stmt, 1, accountId);
    sqlite3_bind_text(stmt, 2, cardNum.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, holderName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, cardType.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, expMonth);
    sqlite3_bind_int(stmt, 6, expYear);
    sqlite3_bind_text(stmt, 7, cvv.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, pin.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 9, dailyLimit > 0 ? dailyLimit : 2000.0);

    int cardId = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        cardId = static_cast<int>(sqlite3_last_insert_rowid(db_));
    }
    sqlite3_finalize(stmt);
    return cardId;
}

bool Database::toggleCardLock(int cardId, int userId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return false;

    // Check ownership
    const char* chkSql = "SELECT c.status FROM cards c INNER JOIN accounts a ON c.account_id = a.account_id WHERE c.card_id = ? AND a.user_id = ?;";
    sqlite3_stmt* chkStmt = nullptr;
    if (sqlite3_prepare_v2(db_, chkSql, -1, &chkStmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(chkStmt, 1, cardId);
    sqlite3_bind_int(chkStmt, 2, userId);

    if (sqlite3_step(chkStmt) != SQLITE_ROW) {
        sqlite3_finalize(chkStmt);
        return false;
    }

    std::string currentStatus = reinterpret_cast<const char*>(sqlite3_column_text(chkStmt, 0));
    sqlite3_finalize(chkStmt);

    std::string nextStatus = (currentStatus == "ACTIVE") ? "LOCKED" : "ACTIVE";

    const char* upSql = "UPDATE cards SET status = ? WHERE card_id = ?;";
    sqlite3_stmt* upStmt = nullptr;
    if (sqlite3_prepare_v2(db_, upSql, -1, &upStmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(upStmt, 1, nextStatus.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(upStmt, 2, cardId);

    bool ok = (sqlite3_step(upStmt) == SQLITE_DONE);
    sqlite3_finalize(upStmt);
    return ok;
}

bool Database::updateCardLimit(int cardId, int userId, double newLimit) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_ || newLimit <= 0.0) return false;

    const char* sql = "UPDATE cards SET daily_limit = ? WHERE card_id = ? AND account_id IN (SELECT account_id FROM accounts WHERE user_id = ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_double(stmt, 1, newLimit);
    sqlite3_bind_int(stmt, 2, cardId);
    sqlite3_bind_int(stmt, 3, userId);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

// Beneficiaries
std::vector<Models::Beneficiary> Database::getBeneficiariesByUserId(int userId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<Models::Beneficiary> list;
    if (!db_) return list;

    const char* sql = "SELECT beneficiary_id, user_id, name, account_number, bank_name, routing_code, nickname, email, created_at "
                      "FROM beneficiaries WHERE user_id = ? ORDER BY beneficiary_id DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    sqlite3_bind_int(stmt, 1, userId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Models::Beneficiary b;
        b.beneficiary_id = sqlite3_column_int(stmt, 0);
        b.user_id = sqlite3_column_int(stmt, 1);
        b.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        b.account_number = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        b.bank_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        b.routing_code = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const char* nn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        b.nickname = nn ? nn : "";
        const char* em = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        b.email = em ? em : "";
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        b.created_at = ca ? ca : "";
        list.push_back(b);
    }
    sqlite3_finalize(stmt);
    return list;
}

int Database::addBeneficiary(int userId, const std::string& name, const std::string& accNumber, const std::string& bankName, const std::string& routing, const std::string& nickname, const std::string& email) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return -1;

    const char* sql = "INSERT INTO beneficiaries (user_id, name, account_number, bank_name, routing_code, nickname, email) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;

    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, accNumber.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, bankName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, routing.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, nickname.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, email.c_str(), -1, SQLITE_STATIC);

    int id = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        id = static_cast<int>(sqlite3_last_insert_rowid(db_));
    }
    sqlite3_finalize(stmt);
    return id;
}

bool Database::deleteBeneficiary(int beneficiaryId, int userId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return false;

    const char* sql = "DELETE FROM beneficiaries WHERE beneficiary_id = ? AND user_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, beneficiaryId);
    sqlite3_bind_int(stmt, 2, userId);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

// Audit Logs & Stats
void Database::logAudit(int userId, const std::string& action, const std::string& details, const std::string& ip) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return;

    const char* sql = "INSERT INTO audit_logs (user_id, action, details, ip_address) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return;

    if (userId > 0) sqlite3_bind_int(stmt, 1, userId);
    else sqlite3_bind_null(stmt, 1);
    sqlite3_bind_text(stmt, 2, action.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, details.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, ip.c_str(), -1, SQLITE_STATIC);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<Models::AuditLog> Database::getAuditLogs(int limit) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<Models::AuditLog> list;
    if (!db_) return list;

    const char* sql = "SELECT l.log_id, l.user_id, u.username, l.action, l.details, l.ip_address, l.created_at "
                      "FROM audit_logs l LEFT JOIN users u ON l.user_id = u.user_id "
                      "ORDER BY l.log_id DESC LIMIT ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    sqlite3_bind_int(stmt, 1, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Models::AuditLog lg;
        lg.log_id = sqlite3_column_int(stmt, 0);
        lg.user_id = sqlite3_column_type(stmt, 1) != SQLITE_NULL ? sqlite3_column_int(stmt, 1) : 0;
        const char* un = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        lg.username = un ? un : "system";
        lg.action = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char* det = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        lg.details = det ? det : "";
        const char* ip = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        lg.ip_address = ip ? ip : "";
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        lg.created_at = ca ? ca : "";
        list.push_back(lg);
    }
    sqlite3_finalize(stmt);
    return list;
}

json Database::getSystemStats() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    json stats = {
        {"total_users", 0},
        {"total_customers", 0},
        {"total_accounts", 0},
        {"total_deposits", 0.0},
        {"active_loans_count", 0},
        {"total_loans_disbursed", 0.0},
        {"total_transactions_count", 0},
        {"server_status", "ONLINE"},
        {"database_engine", "SQLite 3.45"}
    };
    if (!db_) return stats;

    sqlite3_stmt* stmt = nullptr;
    // Total users
    if (sqlite3_prepare_v2(db_, "SELECT COUNT(*), SUM(CASE WHEN role='customer' THEN 1 ELSE 0 END) FROM users;", -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats["total_users"] = sqlite3_column_int(stmt, 0);
            stats["total_customers"] = sqlite3_column_int(stmt, 1);
        }
        sqlite3_finalize(stmt);
    }

    // Total Accounts & Total Balance
    if (sqlite3_prepare_v2(db_, "SELECT COUNT(*), COALESCE(SUM(balance), 0.0) FROM accounts WHERE status='active';", -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats["total_accounts"] = sqlite3_column_int(stmt, 0);
            stats["total_deposits"] = Utils::roundTo2Decimals(sqlite3_column_double(stmt, 1));
        }
        sqlite3_finalize(stmt);
    }

    // Loans
    if (sqlite3_prepare_v2(db_, "SELECT COUNT(*), COALESCE(SUM(principal_amount), 0.0) FROM loans WHERE status='APPROVED';", -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats["active_loans_count"] = sqlite3_column_int(stmt, 0);
            stats["total_loans_disbursed"] = Utils::roundTo2Decimals(sqlite3_column_double(stmt, 1));
        }
        sqlite3_finalize(stmt);
    }

    // Transactions count
    if (sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM transactions;", -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats["total_transactions_count"] = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    return stats;
}
