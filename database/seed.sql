-- ====================================================================
-- Banking Management System - Seed Data
-- ====================================================================

-- 1. Insert Initial Users
-- Password for admin: admin123
-- Password for manager: manager123
-- Password for customer: customer123
-- Password for alice: alice123
-- Password for bob: bob123
-- Salt: apexbank_salt_2026

INSERT OR IGNORE INTO users (user_id, username, password_hash, salt, full_name, email, phone, role, status) VALUES
(1, 'admin', '7eb1ea2131e665bd08342030bc69bca6d2517776f95a71b1366c81a955f6ff72', 'apexbank_salt_2026', 'System Administrator', 'admin@apexbank.com', '+1-800-555-0100', 'admin', 'active'),
(2, 'manager', '6d77dec9ecc727bbc1c044d93212981bdad55bd34737e2224c2da879a9cc42ed', 'apexbank_salt_2026', 'Sarah Connor (Branch Manager)', 'sconnor@apexbank.com', '+1-800-555-0155', 'manager', 'active'),
(3, 'customer', '9928dc6c57bf480da59240d2a503b0eb38527f683e6d68791c24f2db9f2e7067', 'apexbank_salt_2026', 'John Doe', 'john.doe@example.com', '+1-555-019-2834', 'customer', 'active'),
(4, 'alice', '8ce406e5eb82be1a0371ea6c8f393bad84d5c33794077081f315e3bb5ef8dfb0', 'apexbank_salt_2026', 'Alice Smith', 'alice.smith@example.com', '+1-555-018-9122', 'customer', 'active'),
(5, 'bob', '9753412632027d7ece8ec807867a154a0518525e26eafeb51399fd4b7d3f0895', 'apexbank_salt_2026', 'Bob Johnson', 'bob.johnson@example.com', '+1-555-014-6631', 'customer', 'active');

-- 2. Insert Accounts
INSERT OR IGNORE INTO accounts (account_id, user_id, account_number, account_type, balance, currency, interest_rate, status) VALUES
(1, 3, '100120260001', 'Checking', 8450.50, 'USD', 0.5, 'active'),
(2, 3, '100120260002', 'Savings', 25000.00, 'USD', 4.25, 'active'),
(3, 4, '100120260003', 'Savings', 14200.00, 'USD', 4.0, 'active'),
(4, 5, '100120260004', 'Business', 42800.75, 'USD', 2.0, 'active'),
(5, 3, '100120260005', 'Fixed Deposit', 10000.00, 'USD', 6.5, 'active');

-- 3. Insert Initial Transactions
INSERT OR IGNORE INTO transactions (transaction_id, reference_number, from_account_id, to_account_id, type, amount, balance_after, description, status, created_at) VALUES
(1, 'TXN-INIT-001', NULL, 1, 'DEPOSIT', 5000.00, 5000.00, 'Initial Account Opening Deposit', 'COMPLETED', '2026-10-01 09:30:00'),
(2, 'TXN-INIT-002', NULL, 2, 'DEPOSIT', 25000.00, 25000.00, 'Initial High-Yield Savings Deposit', 'COMPLETED', '2026-10-02 11:15:00'),
(3, 'TXN-SAL-1029', NULL, 1, 'DEPOSIT', 4250.50, 9250.50, 'Monthly Corporate Payroll Direct Deposit', 'COMPLETED', '2026-10-15 08:00:00'),
(4, 'TXN-TRF-2041', 1, 3, 'TRANSFER', 500.00, 8750.50, 'Payment for Freelance Design Work to Alice', 'COMPLETED', '2026-10-20 14:22:10'),
(5, 'TXN-WTH-3092', 1, NULL, 'WITHDRAW', 300.00, 8450.50, 'ATM Cash Withdrawal - Main St Branch', 'COMPLETED', '2026-10-25 18:45:00'),
(6, 'TXN-INT-4011', NULL, 2, 'INTEREST_CREDIT', 88.54, 25088.54, 'Monthly Accrued Interest Credit', 'COMPLETED', '2026-10-31 00:00:00');

-- 4. Insert Cards
INSERT OR IGNORE INTO cards (card_id, account_id, card_number, card_holder_name, card_type, expiry_month, expiry_year, cvv, pin, daily_limit, status) VALUES
(1, 1, '4532890123456789', 'JOHN DOE', 'Debit', 12, 2029, '382', '1234', 2500.00, 'ACTIVE'),
(2, 2, '5412751234567890', 'JOHN DOE', 'Credit', 8, 2028, '719', '5678', 5000.00, 'ACTIVE');

-- 5. Insert Loans
INSERT OR IGNORE INTO loans (loan_id, user_id, account_id, loan_type, principal_amount, interest_rate, duration_months, monthly_emi, total_payable, remaining_balance, purpose, status, applied_at, reviewed_by, reviewed_at) VALUES
(1, 3, 1, 'Auto Loan', 18000.00, 5.5, 36, 543.80, 19576.80, 14200.00, 'Purchase of 2024 Electric Hybrid Vehicle', 'APPROVED', '2026-08-10 10:00:00', 2, '2026-08-11 14:30:00'),
(2, 4, 3, 'Education Loan', 12000.00, 6.0, 24, 531.85, 12764.40, 12000.00, 'Master Degree Tuition Fee', 'PENDING', '2026-10-28 16:20:00', NULL, NULL);

-- 6. Insert Beneficiaries for John Doe (user_id = 3)
INSERT OR IGNORE INTO beneficiaries (beneficiary_id, user_id, name, account_number, bank_name, routing_code, nickname, email) VALUES
(1, 3, 'Alice Smith', '100120260003', 'Apex National Bank', 'APEX0001', 'Alice Personal', 'alice.smith@example.com'),
(2, 3, 'Bob Johnson', '100120260004', 'Apex National Bank', 'APEX0001', 'Bob Business', 'bob.johnson@example.com'),
(3, 3, 'TechCorp Utilities', '998877665544', 'Metro Global Bank', 'MTRB0099', 'Electricity & Internet', 'billing@techcorp.com');

-- 7. Insert Audit Logs
INSERT OR IGNORE INTO audit_logs (log_id, user_id, action, details, ip_address) VALUES
(1, 1, 'SYSTEM_INIT', 'System initialized with default banking configuration and schema', '127.0.0.1'),
(2, 3, 'LOGIN_SUCCESS', 'Customer John Doe logged into online banking portal', '127.0.0.1'),
(3, 3, 'TRANSFER', 'Transfer of $500.00 from account 100120260001 to 100120260003', '127.0.0.1'),
(4, 2, 'LOAN_APPROVAL', 'Manager Sarah Connor approved Auto Loan for user John Doe', '127.0.0.1');
