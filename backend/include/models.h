#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <vector>
#include <optional>
#include "../../dependencies/json.hpp"

using json = nlohmann::json;

namespace Models {

struct User {
    int user_id = 0;
    std::string username;
    std::string password_hash;
    std::string salt;
    std::string full_name;
    std::string email;
    std::string phone;
    std::string role; // customer, manager, admin
    std::string status; // active, suspended
    std::string created_at;

    json toJson(bool includePrivate = false) const {
        json j = {
            {"user_id", user_id},
            {"username", username},
            {"full_name", full_name},
            {"email", email},
            {"phone", phone},
            {"role", role},
            {"status", status},
            {"created_at", created_at}
        };
        if (includePrivate) {
            j["password_hash"] = password_hash;
            j["salt"] = salt;
        }
        return j;
    }
};

struct Account {
    int account_id = 0;
    int user_id = 0;
    std::string account_number;
    std::string account_type; // Savings, Checking, Fixed Deposit, Business
    double balance = 0.0;
    std::string currency = "USD";
    double interest_rate = 3.5;
    std::string status = "active"; // active, frozen, closed
    std::string created_at;
    std::string owner_name;

    json toJson() const {
        return {
            {"account_id", account_id},
            {"user_id", user_id},
            {"account_number", account_number},
            {"account_type", account_type},
            {"balance", balance},
            {"currency", currency},
            {"interest_rate", interest_rate},
            {"status", status},
            {"created_at", created_at},
            {"owner_name", owner_name}
        };
    }
};

struct Transaction {
    int transaction_id = 0;
    std::string reference_number;
    int from_account_id = 0;
    int to_account_id = 0;
    std::string from_account_number;
    std::string to_account_number;
    std::string type; // DEPOSIT, WITHDRAW, TRANSFER, LOAN_DISBURSEMENT, LOAN_REPAYMENT, INTEREST_CREDIT
    double amount = 0.0;
    double balance_after = 0.0;
    std::string description;
    std::string status = "COMPLETED";
    std::string created_at;

    json toJson() const {
        return {
            {"transaction_id", transaction_id},
            {"reference_number", reference_number},
            {"from_account_id", from_account_id > 0 ? json(from_account_id) : json(nullptr)},
            {"to_account_id", to_account_id > 0 ? json(to_account_id) : json(nullptr)},
            {"from_account_number", from_account_number},
            {"to_account_number", to_account_number},
            {"type", type},
            {"amount", amount},
            {"balance_after", balance_after},
            {"description", description},
            {"status", status},
            {"created_at", created_at}
        };
    }
};

struct Loan {
    int loan_id = 0;
    int user_id = 0;
    int account_id = 0;
    std::string user_name;
    std::string account_number;
    std::string loan_type;
    double principal_amount = 0.0;
    double interest_rate = 0.0;
    int duration_months = 0;
    double monthly_emi = 0.0;
    double total_payable = 0.0;
    double remaining_balance = 0.0;
    std::string purpose;
    std::string status = "PENDING"; // PENDING, APPROVED, REJECTED, CLOSED
    std::string applied_at;
    int reviewed_by = 0;
    std::string reviewer_name;
    std::string reviewed_at;
    std::string rejection_reason;

    json toJson() const {
        return {
            {"loan_id", loan_id},
            {"user_id", user_id},
            {"account_id", account_id},
            {"user_name", user_name},
            {"account_number", account_number},
            {"loan_type", loan_type},
            {"principal_amount", principal_amount},
            {"interest_rate", interest_rate},
            {"duration_months", duration_months},
            {"monthly_emi", monthly_emi},
            {"total_payable", total_payable},
            {"remaining_balance", remaining_balance},
            {"purpose", purpose},
            {"status", status},
            {"applied_at", applied_at},
            {"reviewed_by", reviewed_by > 0 ? json(reviewed_by) : json(nullptr)},
            {"reviewer_name", reviewer_name},
            {"reviewed_at", reviewed_at},
            {"rejection_reason", rejection_reason}
        };
    }
};

struct Card {
    int card_id = 0;
    int account_id = 0;
    std::string account_number;
    std::string card_number;
    std::string card_holder_name;
    std::string card_type = "Debit";
    int expiry_month = 1;
    int expiry_year = 2028;
    std::string cvv;
    std::string pin;
    double daily_limit = 2000.0;
    std::string status = "ACTIVE"; // ACTIVE, LOCKED, EXPIRED
    std::string created_at;

    json toJson(bool maskDetails = true) const {
        std::string maskedNumber = card_number;
        if (maskDetails && maskedNumber.length() >= 12) {
            maskedNumber = maskedNumber.substr(0, 4) + " •••• •••• " + maskedNumber.substr(maskedNumber.length() - 4);
        }
        return {
            {"card_id", card_id},
            {"account_id", account_id},
            {"account_number", account_number},
            {"card_number", maskDetails ? maskedNumber : card_number},
            {"card_number_full", card_number},
            {"card_holder_name", card_holder_name},
            {"card_type", card_type},
            {"expiry_month", expiry_month},
            {"expiry_year", expiry_year},
            {"expiry_str", (expiry_month < 10 ? "0" : "") + std::to_string(expiry_month) + "/" + std::to_string(expiry_year % 100)},
            {"cvv", maskDetails ? "•••" : cvv},
            {"cvv_full", cvv},
            {"daily_limit", daily_limit},
            {"status", status},
            {"created_at", created_at}
        };
    }
};

struct Beneficiary {
    int beneficiary_id = 0;
    int user_id = 0;
    std::string name;
    std::string account_number;
    std::string bank_name;
    std::string routing_code;
    std::string nickname;
    std::string email;
    std::string created_at;

    json toJson() const {
        return {
            {"beneficiary_id", beneficiary_id},
            {"user_id", user_id},
            {"name", name},
            {"account_number", account_number},
            {"bank_name", bank_name},
            {"routing_code", routing_code},
            {"nickname", nickname},
            {"email", email},
            {"created_at", created_at}
        };
    }
};

struct AuditLog {
    int log_id = 0;
    int user_id = 0;
    std::string username;
    std::string action;
    std::string details;
    std::string ip_address;
    std::string created_at;

    json toJson() const {
        return {
            {"log_id", log_id},
            {"user_id", user_id > 0 ? json(user_id) : json(nullptr)},
            {"username", username},
            {"action", action},
            {"details", details},
            {"ip_address", ip_address},
            {"created_at", created_at}
        };
    }
};

} // namespace Models

#endif // MODELS_H
