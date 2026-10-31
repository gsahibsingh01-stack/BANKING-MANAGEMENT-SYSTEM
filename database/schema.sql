-- ====================================================================
-- Banking Management System Database Schema (SQLite)
-- ====================================================================

PRAGMA foreign_keys = ON;

-- 1. Users Table
CREATE TABLE IF NOT EXISTS users (
    user_id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,
    salt TEXT NOT NULL,
    full_name TEXT NOT NULL,
    email TEXT UNIQUE NOT NULL,
    phone TEXT,
    role TEXT CHECK(role IN ('customer', 'manager', 'admin')) NOT NULL DEFAULT 'customer',
    status TEXT CHECK(status IN ('active', 'suspended', 'inactive')) NOT NULL DEFAULT 'active',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 2. Sessions Table (for token authentication)
CREATE TABLE IF NOT EXISTS sessions (
    token TEXT PRIMARY KEY,
    user_id INTEGER NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    expires_at DATETIME NOT NULL,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
);

-- 3. Accounts Table
CREATE TABLE IF NOT EXISTS accounts (
    account_id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    account_number TEXT UNIQUE NOT NULL,
    account_type TEXT CHECK(account_type IN ('Savings', 'Checking', 'Fixed Deposit', 'Business')) NOT NULL,
    balance REAL NOT NULL DEFAULT 0.0 CHECK(balance >= 0.0),
    currency TEXT NOT NULL DEFAULT 'USD',
    interest_rate REAL NOT NULL DEFAULT 3.5,
    status TEXT CHECK(status IN ('active', 'frozen', 'closed')) NOT NULL DEFAULT 'active',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE RESTRICT
);

-- 4. Transactions Table
CREATE TABLE IF NOT EXISTS transactions (
    transaction_id INTEGER PRIMARY KEY AUTOINCREMENT,
    reference_number TEXT UNIQUE NOT NULL,
    from_account_id INTEGER,
    to_account_id INTEGER,
    type TEXT CHECK(type IN ('DEPOSIT', 'WITHDRAW', 'TRANSFER', 'LOAN_DISBURSEMENT', 'LOAN_REPAYMENT', 'INTEREST_CREDIT')) NOT NULL,
    amount REAL NOT NULL CHECK(amount > 0),
    balance_after REAL NOT NULL,
    description TEXT,
    status TEXT CHECK(status IN ('COMPLETED', 'FAILED', 'PENDING')) NOT NULL DEFAULT 'COMPLETED',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (from_account_id) REFERENCES accounts(account_id) ON DELETE SET NULL,
    FOREIGN KEY (to_account_id) REFERENCES accounts(account_id) ON DELETE SET NULL
);

-- 5. Loans Table
CREATE TABLE IF NOT EXISTS loans (
    loan_id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    account_id INTEGER NOT NULL,
    loan_type TEXT CHECK(loan_type IN ('Personal Loan', 'Home Loan', 'Education Loan', 'Auto Loan', 'Business Loan')) NOT NULL,
    principal_amount REAL NOT NULL CHECK(principal_amount > 0),
    interest_rate REAL NOT NULL,
    duration_months INTEGER NOT NULL CHECK(duration_months > 0),
    monthly_emi REAL NOT NULL,
    total_payable REAL NOT NULL,
    remaining_balance REAL NOT NULL,
    purpose TEXT,
    status TEXT CHECK(status IN ('PENDING', 'APPROVED', 'REJECTED', 'CLOSED')) NOT NULL DEFAULT 'PENDING',
    applied_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    reviewed_by INTEGER,
    reviewed_at DATETIME,
    rejection_reason TEXT,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE RESTRICT,
    FOREIGN KEY (account_id) REFERENCES accounts(account_id) ON DELETE RESTRICT,
    FOREIGN KEY (reviewed_by) REFERENCES users(user_id) ON DELETE SET NULL
);

-- 6. Cards Table
CREATE TABLE IF NOT EXISTS cards (
    card_id INTEGER PRIMARY KEY AUTOINCREMENT,
    account_id INTEGER NOT NULL,
    card_number TEXT UNIQUE NOT NULL,
    card_holder_name TEXT NOT NULL,
    card_type TEXT CHECK(card_type IN ('Debit', 'Credit')) NOT NULL DEFAULT 'Debit',
    expiry_month INTEGER NOT NULL CHECK(expiry_month BETWEEN 1 AND 12),
    expiry_year INTEGER NOT NULL,
    cvv TEXT NOT NULL,
    pin TEXT NOT NULL,
    daily_limit REAL NOT NULL DEFAULT 2000.0,
    status TEXT CHECK(status IN ('ACTIVE', 'LOCKED', 'EXPIRED')) NOT NULL DEFAULT 'ACTIVE',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (account_id) REFERENCES accounts(account_id) ON DELETE CASCADE
);

-- 7. Beneficiaries Table
CREATE TABLE IF NOT EXISTS beneficiaries (
    beneficiary_id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    name TEXT NOT NULL,
    account_number TEXT NOT NULL,
    bank_name TEXT NOT NULL DEFAULT 'Apex National Bank',
    routing_code TEXT NOT NULL DEFAULT 'APEX0001',
    nickname TEXT,
    email TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
);

-- 8. Audit Logs Table
CREATE TABLE IF NOT EXISTS audit_logs (
    log_id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER,
    action TEXT NOT NULL,
    details TEXT,
    ip_address TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE SET NULL
);

-- Indexes for fast lookups
CREATE INDEX IF NOT EXISTS idx_accounts_user ON accounts(user_id);
CREATE INDEX IF NOT EXISTS idx_transactions_from ON transactions(from_account_id);
CREATE INDEX IF NOT EXISTS idx_transactions_to ON transactions(to_account_id);
CREATE INDEX IF NOT EXISTS idx_transactions_created ON transactions(created_at);
CREATE INDEX IF NOT EXISTS idx_loans_user ON loans(user_id);
CREATE INDEX IF NOT EXISTS idx_cards_account ON cards(account_id);
CREATE INDEX IF NOT EXISTS idx_sessions_token ON sessions(token);
CREATE INDEX IF NOT EXISTS idx_beneficiaries_user ON beneficiaries(user_id);
