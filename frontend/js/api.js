// ====================================================================
// Apex National Bank - API Service Layer
// ====================================================================

const API = {
    baseUrl: '/api',

    getToken() {
        return localStorage.getItem('apex_token') || '';
    },

    setToken(token) {
        if (token) localStorage.setItem('apex_token', token);
        else localStorage.removeItem('apex_token');
    },

    getUser() {
        const u = localStorage.getItem('apex_user');
        try {
            return u ? JSON.parse(u) : null;
        } catch {
            return null;
        }
    },

    setUser(user) {
        if (user) localStorage.setItem('apex_user', JSON.stringify(user));
        else localStorage.removeItem('apex_user');
    },

    isLoggedIn() {
        return !!this.getToken() && !!this.getUser();
    },

    async request(endpoint, options = {}) {
        const url = `${this.baseUrl}${endpoint}`;
        const headers = {
            'Content-Type': 'application/json',
            ...(options.headers || {})
        };

        const token = this.getToken();
        if (token) {
            headers['Authorization'] = `Bearer ${token}`;
        }

        try {
            const config = {
                ...options,
                headers
            };

            const response = await fetch(url, config);
            const data = await response.json().catch(() => ({ error: 'Invalid JSON response from server' }));

            if (!response.ok) {
                // If 401 Unauthorized, automatically handle session expiry
                if (response.status === 401 && !endpoint.includes('/auth/login')) {
                    this.clearSession();
                    window.dispatchEvent(new CustomEvent('auth:expired'));
                }
                throw new Error(data.error || `HTTP ${response.status}: Request failed`);
            }

            return data;
        } catch (err) {
            console.error(`API Error [${endpoint}]:`, err);
            throw err;
        }
    },

    get(endpoint, params = {}) {
        const query = new URLSearchParams(params).toString();
        const url = query ? `${endpoint}?${query}` : endpoint;
        return this.request(url, { method: 'GET' });
    },

    post(endpoint, body = {}) {
        return this.request(endpoint, {
            method: 'POST',
            body: JSON.stringify(body)
        });
    },

    delete(endpoint) {
        return this.request(endpoint, { method: 'DELETE' });
    },

    // -------------------------------------------------------------
    // Authentication Endpoints
    // -------------------------------------------------------------
    async login(username, password) {
        const res = await this.post('/auth/login', { username, password });
        if (res.success && res.token) {
            this.setToken(res.token);
            this.setUser(res.user);
        }
        return res;
    },

    async register(userData) {
        const res = await this.post('/auth/register', userData);
        if (res.success && res.token) {
            this.setToken(res.token);
            this.setUser(res.user);
        }
        return res;
    },

    async logout() {
        try {
            await this.post('/auth/logout');
        } catch (e) {
            // Ignore failure on logout
        } finally {
            this.clearSession();
        }
    },

    clearSession() {
        this.setToken('');
        this.setUser(null);
    },

    getMe() {
        return this.get('/auth/me');
    },

    changePassword(oldPassword, newPassword) {
        return this.post('/auth/change-password', {
            old_password: oldPassword,
            new_password: newPassword
        });
    },

    updateProfile(profileData) {
        return this.post('/auth/profile', profileData);
    },

    // -------------------------------------------------------------
    // Dashboard & Overview
    // -------------------------------------------------------------
    getDashboardSummary() {
        return this.get('/dashboard/summary');
    },

    // -------------------------------------------------------------
    // Accounts
    // -------------------------------------------------------------
    getAccounts() {
        return this.get('/accounts');
    },

    createAccount(accountType, initialDeposit, currency = 'USD') {
        return this.post('/accounts/create', {
            account_type: accountType,
            initial_deposit: parseFloat(initialDeposit),
            currency
        });
    },

    lookupAccount(accountNumber) {
        return this.get('/accounts/lookup', { account_number: accountNumber });
    },

    // -------------------------------------------------------------
    // Transactions
    // -------------------------------------------------------------
    deposit(accountId, amount, description) {
        return this.post('/transactions/deposit', {
            account_id: parseInt(accountId),
            amount: parseFloat(amount),
            description
        });
    },

    withdraw(accountId, amount, description) {
        return this.post('/transactions/withdraw', {
            account_id: parseInt(accountId),
            amount: parseFloat(amount),
            description
        });
    },

    transfer(fromAccountId, toAccountNumber, amount, description) {
        return this.post('/transactions/transfer', {
            from_account_id: parseInt(fromAccountId),
            to_account_number: toAccountNumber,
            amount: parseFloat(amount),
            description
        });
    },

    getTransactions(filters = {}) {
        return this.get('/transactions', filters);
    },

    // -------------------------------------------------------------
    // Loans
    // -------------------------------------------------------------
    getLoans() {
        return this.get('/loans');
    },

    applyLoan(accountId, loanType, amount, durationMonths, purpose) {
        return this.post('/loans/apply', {
            account_id: parseInt(accountId),
            loan_type: loanType,
            amount: parseFloat(amount),
            duration_months: parseInt(durationMonths),
            purpose
        });
    },

    repayLoan(loanId, accountId, amount) {
        return this.post('/loans/repay', {
            loan_id: parseInt(loanId),
            account_id: parseInt(accountId),
            amount: parseFloat(amount)
        });
    },

    reviewLoan(loanId, decision, reason = '') {
        return this.post('/loans/review', {
            loan_id: parseInt(loanId),
            decision,
            reason
        });
    },

    // -------------------------------------------------------------
    // Cards
    // -------------------------------------------------------------
    getCards() {
        return this.get('/cards');
    },

    issueCard(accountId, cardType, pin, dailyLimit) {
        return this.post('/cards/issue', {
            account_id: parseInt(accountId),
            card_type: cardType,
            pin,
            daily_limit: parseFloat(dailyLimit)
        });
    },

    toggleCardLock(cardId) {
        return this.post('/cards/toggle-lock', { card_id: parseInt(cardId) });
    },

    updateCardLimit(cardId, dailyLimit) {
        return this.post('/cards/update-limit', {
            card_id: parseInt(cardId),
            daily_limit: parseFloat(dailyLimit)
        });
    },

    // -------------------------------------------------------------
    // Beneficiaries
    // -------------------------------------------------------------
    getBeneficiaries() {
        return this.get('/beneficiaries');
    },

    addBeneficiary(beneficiaryData) {
        return this.post('/beneficiaries/add', beneficiaryData);
    },

    deleteBeneficiary(id) {
        return this.delete(`/beneficiaries/${id}`);
    },

    // -------------------------------------------------------------
    // Admin & Manager
    // -------------------------------------------------------------
    getAdminStats() {
        return this.get('/admin/stats');
    },

    getAdminUsers() {
        return this.get('/admin/users');
    },

    createAdminUser(userData) {
        return this.post('/admin/users/create', userData);
    },

    toggleUserStatus(userId) {
        return this.post('/admin/users/toggle-status', { user_id: parseInt(userId) });
    },

    getAuditLogs() {
        return this.get('/admin/audit-logs');
    },

    getManagerAccounts() {
        return this.get('/manager/accounts');
    },

    updateAccountStatus(accountId, status) {
        return this.post('/manager/account-status', {
            account_id: parseInt(accountId),
            status
        });
    },

    // -------------------------------------------------------------
    // Financial Calculator
    // -------------------------------------------------------------
    calculateEMI(principal, rate, months) {
        return this.post('/calculator/emi', {
            principal: parseFloat(principal),
            rate: parseFloat(rate),
            months: parseInt(months)
        });
    }
};

window.API = API;
