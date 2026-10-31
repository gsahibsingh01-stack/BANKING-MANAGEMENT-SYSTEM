// ====================================================================
// Apex National Bank - Main Application Controller
// ====================================================================

const App = {
    currentView: 'dashboard',
    userAccountsCache: [],

    init() {
        this.setupAuthListeners();
        this.checkAuthState();
    },

    setupAuthListeners() {
        window.addEventListener('auth:expired', () => {
            this.showToast('Your session has expired. Please log in again.', 'error');
            this.checkAuthState();
        });
    },

    checkAuthState() {
        const authSection = document.getElementById('auth-section');
        const appSection = document.getElementById('app-section');

        if (API.isLoggedIn()) {
            authSection.style.display = 'none';
            appSection.style.display = 'flex';
            this.setupUserUI();
            this.navigate(this.currentView);
        } else {
            authSection.style.display = 'flex';
            appSection.style.display = 'none';
        }
    },

    setupUserUI() {
        const user = API.getUser();
        if (!user) return;

        // Topbar and sidebar user details
        const nameEl = document.getElementById('sidebar-user-name');
        const roleEl = document.getElementById('sidebar-user-role');
        const avatarEl = document.getElementById('sidebar-user-avatar');

        if (nameEl) nameEl.textContent = user.full_name || user.username;
        if (roleEl) roleEl.textContent = user.role.toUpperCase();
        if (avatarEl) avatarEl.textContent = (user.full_name || user.username).charAt(0).toUpperCase();

        // Show/hide Manager & Admin navigation items
        const managerNav = document.getElementById('nav-item-manager');
        const adminNav = document.getElementById('nav-item-admin');

        if (managerNav) {
            managerNav.style.display = (user.role === 'manager' || user.role === 'admin') ? 'flex' : 'none';
        }
        if (adminNav) {
            adminNav.style.display = (user.role === 'admin') ? 'flex' : 'none';
        }
    },

    // -------------------------------------------------------------
    // Navigation Router
    // -------------------------------------------------------------
    navigate(view) {
        this.currentView = view;

        // Update active class on nav links
        document.querySelectorAll('.nav-link').forEach(link => {
            if (link.dataset.view === view) {
                link.classList.add('active');
            } else {
                link.classList.remove('active');
            }
        });

        // Update topbar title
        const topbarTitle = document.getElementById('topbar-page-title');
        const viewTitles = {
            dashboard: 'Portfolio Overview',
            accounts: 'Deposit & Savings Accounts',
            transfer: 'Wire & Fund Transfers',
            depositWithdraw: 'Teller & ATM Terminal',
            loans: 'Credit & Loan Portfolio',
            cards: 'Virtual Cards & Security',
            transactions: 'Comprehensive Statement',
            calculator: 'EMI & Interest Calculator',
            manager: 'Branch Manager Operations',
            admin: 'System Administration'
        };

        if (topbarTitle) {
            topbarTitle.textContent = viewTitles[view] || 'Banking Portal';
        }

        const container = document.getElementById('main-content-area');
        if (!container) return;

        // Close mobile sidebar if open
        const sidebar = document.getElementById('sidebar');
        if (sidebar && sidebar.classList.contains('open')) {
            sidebar.classList.remove('open');
        }

        // Render selected view
        switch (view) {
            case 'dashboard':
                Views.renderDashboard(container);
                break;
            case 'accounts':
                Views.renderAccounts(container);
                break;
            case 'transfer':
                Views.renderTransfer(container);
                break;
            case 'depositWithdraw':
                Views.renderDepositWithdraw(container);
                break;
            case 'loans':
                Views.renderLoans(container);
                break;
            case 'cards':
                Views.renderCards(container);
                break;
            case 'transactions':
                Views.renderTransactions(container);
                break;
            case 'calculator':
                Views.renderCalculator(container);
                break;
            case 'manager':
                Views.renderManagerPortal(container);
                break;
            case 'admin':
                Views.renderAdminPortal(container);
                break;
            default:
                Views.renderDashboard(container);
        }
    },

    toggleMobileMenu() {
        const sidebar = document.getElementById('sidebar');
        if (sidebar) sidebar.classList.toggle('open');
    },

    // -------------------------------------------------------------
    // Authentication Handlers
    // -------------------------------------------------------------
    switchAuthTab(tab) {
        const loginTabBtn = document.getElementById('auth-tab-login');
        const regTabBtn = document.getElementById('auth-tab-register');
        const loginForm = document.getElementById('login-form');
        const regForm = document.getElementById('register-form');

        if (tab === 'login') {
            loginTabBtn.classList.add('active');
            regTabBtn.classList.remove('active');
            loginForm.style.display = 'flex';
            regForm.style.display = 'none';
        } else {
            loginTabBtn.classList.remove('active');
            regTabBtn.classList.add('active');
            loginForm.style.display = 'none';
            regForm.style.display = 'flex';
        }
    },

    fillDemoLogin(username, password) {
        this.switchAuthTab('login');
        document.getElementById('login-username').value = username;
        document.getElementById('login-password').value = password;
        this.handleLoginSubmit();
    },

    async handleLoginSubmit(event) {
        if (event) event.preventDefault();
        const u = document.getElementById('login-username').value.trim();
        const p = document.getElementById('login-password').value;

        if (!u || !p) {
            this.showToast('Please enter both username and password.', 'error');
            return;
        }

        const btn = document.getElementById('btn-login-submit');
        const origText = btn.innerHTML;
        btn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Authenticating...';
        btn.disabled = true;

        try {
            const res = await API.login(u, p);
            this.showToast(`Welcome back, ${res.user.full_name || res.user.username}!`, 'success');
            this.checkAuthState();
        } catch (err) {
            this.showToast(err.message, 'error');
        } finally {
            btn.innerHTML = origText;
            btn.disabled = false;
        }
    },

    async handleRegisterSubmit(event) {
        if (event) event.preventDefault();

        const username = document.getElementById('reg-username').value.trim();
        const password = document.getElementById('reg-password').value;
        const full_name = document.getElementById('reg-fullname').value.trim();
        const email = document.getElementById('reg-email').value.trim();
        const phone = document.getElementById('reg-phone').value.trim();
        const account_type = document.getElementById('reg-account-type').value;
        const initial_deposit = parseFloat(document.getElementById('reg-initial-deposit').value) || 500.0;

        const btn = document.getElementById('btn-register-submit');
        const origText = btn.innerHTML;
        btn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Creating Account...';
        btn.disabled = true;

        try {
            const res = await API.register({
                username, password, full_name, email, phone, account_type, initial_deposit
            });
            this.showToast(res.message, 'success');
            this.checkAuthState();
        } catch (err) {
            this.showToast(err.message, 'error');
        } finally {
            btn.innerHTML = origText;
            btn.disabled = false;
        }
    },

    async logout() {
        if (confirm('Are you sure you want to log out of your banking session?')) {
            await API.logout();
            this.showToast('You have been logged out securely.', 'info');
            this.checkAuthState();
        }
    },

    // -------------------------------------------------------------
    // Transfer Handlers
    // -------------------------------------------------------------
    async lookupRecipient(accountNumber) {
        const statusEl = document.getElementById('recipient-verify-status');
        if (!accountNumber || accountNumber.trim().length < 5) {
            if (statusEl) statusEl.innerHTML = '';
            return;
        }

        try {
            const res = await API.lookupAccount(accountNumber.trim());
            if (res.success) {
                statusEl.innerHTML = `<span style="color:var(--accent-emerald);"><i class="fas fa-check-circle"></i> Verified Account: <strong>${res.owner_name}</strong> (${res.account_type})</span>`;
            }
        } catch (err) {
            statusEl.innerHTML = `<span style="color:var(--accent-rose);"><i class="fas fa-exclamation-circle"></i> ${err.message}</span>`;
        }
    },

    handleBeneficiarySelect(select) {
        if (select.value) {
            const input = document.getElementById('transfer-to-account');
            input.value = select.value;
            this.lookupRecipient(select.value);
        }
    },

    quickTransferTo(accountNumber) {
        this.navigate('transfer');
        setTimeout(() => {
            const input = document.getElementById('transfer-to-account');
            if (input) {
                input.value = accountNumber;
                this.lookupRecipient(accountNumber);
            }
        }, 100);
    },

    async handleTransferSubmit(event) {
        event.preventDefault();

        const fromId = document.getElementById('transfer-from-account').value;
        const toAcc = document.getElementById('transfer-to-account').value.trim();
        const amount = document.getElementById('transfer-amount').value;
        const desc = document.getElementById('transfer-desc').value.trim();

        const btn = document.getElementById('btn-submit-transfer');
        const origText = btn.innerHTML;
        btn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Processing Wire...';
        btn.disabled = true;

        try {
            const res = await API.transfer(fromId, toAcc, amount, desc);
            this.showToast(`Transfer of $${amount} to ${res.recipient_name} successful! Ref: ${res.reference_number}`, 'success');
            document.getElementById('transfer-form').reset();
            const statusEl = document.getElementById('recipient-verify-status');
            if (statusEl) statusEl.innerHTML = '';
            this.navigate('transfer');
        } catch (err) {
            this.showToast(err.message, 'error');
        } finally {
            btn.innerHTML = origText;
            btn.disabled = false;
        }
    },

    // -------------------------------------------------------------
    // Deposit & Withdraw Handlers
    // -------------------------------------------------------------
    async handleDepositSubmit(event) {
        event.preventDefault();
        const accId = document.getElementById('deposit-account-select').value;
        const amount = document.getElementById('deposit-amount-input').value;
        const desc = document.getElementById('deposit-desc-input').value.trim();

        try {
            const res = await API.deposit(accId, amount, desc);
            this.showToast(`Deposit of $${amount} completed! New Balance: $${res.new_balance.toFixed(2)}`, 'success');
            this.navigate('depositWithdraw');
        } catch (err) {
            this.showToast(err.message, 'error');
        }
    },

    async handleWithdrawSubmit(event) {
        event.preventDefault();
        const accId = document.getElementById('withdraw-account-select').value;
        const amount = document.getElementById('withdraw-amount-input').value;
        const desc = document.getElementById('withdraw-desc-input').value.trim();

        try {
            const res = await API.withdraw(accId, amount, desc);
            this.showToast(`ATM Withdrawal of $${amount} dispensed! New Balance: $${res.new_balance.toFixed(2)}`, 'success');
            this.navigate('depositWithdraw');
        } catch (err) {
            this.showToast(err.message, 'error');
        }
    },

    // -------------------------------------------------------------
    // Card Actions
    // -------------------------------------------------------------
    toggleCardReveal(cardId, fullNumber, fullCvv) {
        const el = document.getElementById(`card-num-${cardId}`);
        if (!el) return;

        if (el.dataset.revealed === 'true') {
            el.dataset.revealed = 'false';
            el.textContent = fullNumber.substr(0, 4) + ' •••• •••• ' + fullNumber.substr(fullNumber.length - 4);
        } else {
            el.dataset.revealed = 'true';
            el.textContent = fullNumber.match(/.{1,4}/g).join(' ') + `  (CVV: ${fullCvv})`;
        }
    },

    async toggleCardLock(cardId) {
        try {
            const res = await API.toggleCardLock(cardId);
            this.showToast(res.message, 'success');
            this.navigate('cards');
        } catch (err) {
            this.showToast(err.message, 'error');
        }
    },

    // -------------------------------------------------------------
    // Modals Management
    // -------------------------------------------------------------
    openModal(modalId) {
        const modal = document.getElementById(modalId);
        if (modal) modal.classList.add('active');
    },

    closeModal(modalId) {
        const modal = document.getElementById(modalId);
        if (modal) modal.classList.remove('active');
    },

    async openCreateAccountModal() {
        this.openModal('modal-create-account');
    },

    async handleCreateAccountSubmit(event) {
        event.preventDefault();
        const type = document.getElementById('new-acc-type').value;
        const deposit = document.getElementById('new-acc-deposit').value;

        try {
            const res = await API.createAccount(type, deposit);
            this.showToast(res.message, 'success');
            this.closeModal('modal-create-account');
            this.navigate('accounts');
        } catch (err) {
            this.showToast(err.message, 'error');
        }
    },

    openDepositModal(accountId) {
        this.navigate('depositWithdraw');
        if (accountId) {
            setTimeout(() => {
                const select = document.getElementById('deposit-account-select');
                if (select) select.value = accountId;
            }, 100);
        }
    },

    openWithdrawModal(accountId) {
        this.navigate('depositWithdraw');
        if (accountId) {
            setTimeout(() => {
                const select = document.getElementById('withdraw-account-select');
                if (select) select.value = accountId;
            }, 100);
        }
    },

    openTransferModal(accountId) {
        this.navigate('transfer');
        if (accountId) {
            setTimeout(() => {
                const select = document.getElementById('transfer-from-account');
                if (select) select.value = accountId;
            }, 100);
        }
    },

    async openApplyLoanModal() {
        const res = await API.getAccounts();
        const accounts = res.accounts || [];
        const select = document.getElementById('loan-account-select');
        select.innerHTML = '';
        accounts.forEach(a => {
            select.innerHTML += `<option value="${a.account_id}">${a.account_type} (${a.account_number})</option>`;
        });
        this.openModal('modal-apply-loan');
    },

    async handleApplyLoanSubmit(event) {
        event.preventDefault();
        const accId = document.getElementById('loan-account-select').value;
        const type = document.getElementById('loan-type-select').value;
        const amount = document.getElementById('loan-amount-input').value;
        const months = document.getElementById('loan-months-input').value;
        const purpose = document.getElementById('loan-purpose-input').value;

        try {
            const res = await API.applyLoan(accId, type, amount, months, purpose);
            this.showToast(res.message, 'success');
            this.closeModal('modal-apply-loan');
            this.navigate('loans');
        } catch (err) {
            this.showToast(err.message, 'error');
        }
    },

    async openRepayLoanModal(loanId, remaining, emi) {
        document.getElementById('repay-loan-id').value = loanId;
        document.getElementById('repay-loan-remaining').textContent = `$${remaining.toFixed(2)}`;
        document.getElementById('repay-amount-input').value = emi.toFixed(2);

        const res = await API.getAccounts();
        const accounts = res.accounts || [];
        const select = document.getElementById('repay-account-select');
        select.innerHTML = '';
        accounts.forEach(a => {
            select.innerHTML += `<option value="${a.account_id}">${a.account_type} (${a.account_number}) - Available: $${a.balance.toFixed(2)}</option>`;
        });

        this.openModal('modal-repay-loan');
    },

    async handleRepayLoanSubmit(event) {
        event.preventDefault();
        const loanId = document.getElementById('repay-loan-id').value;
        const accId = document.getElementById('repay-account-select').value;
        const amount = document.getElementById('repay-amount-input').value;

        try {
            const res = await API.repayLoan(loanId, accId, amount);
            this.showToast(res.message, 'success');
            this.closeModal('modal-repay-loan');
            this.navigate('loans');
        } catch (err) {
            this.showToast(err.message, 'error');
        }
    },

    async openIssueCardModal() {
        const res = await API.getAccounts();
        const accounts = res.accounts || [];
        const select = document.getElementById('issue-card-account-select');
        select.innerHTML = '';
        accounts.forEach(a => {
            select.innerHTML += `<option value="${a.account_id}">${a.account_type} (${a.account_number})</option>`;
        });
        this.openModal('modal-issue-card');
    },

    async handleIssueCardSubmit(event) {
        event.preventDefault();
        const accId = document.getElementById('issue-card-account-select').value;
        const type = document.getElementById('issue-card-type-select').value;
        const pin = document.getElementById('issue-card-pin').value;
        const limit = document.getElementById('issue-card-limit').value;

        try {
            const res = await API.issueCard(accId, type, pin, limit);
            this.showToast(res.message, 'success');
            this.closeModal('modal-issue-card');
            this.navigate('cards');
        } catch (err) {
            this.showToast(err.message, 'error');
        }
    },

    openAddBeneficiaryModal() {
        this.openModal('modal-add-beneficiary');
    },

    async handleAddBeneficiarySubmit(event) {
        event.preventDefault();
        const name = document.getElementById('ben-name').value.trim();
        const account_number = document.getElementById('ben-acc-num').value.trim();
        const bank_name = document.getElementById('ben-bank-name').value.trim();
        const routing_code = document.getElementById('ben-routing').value.trim();
        const nickname = document.getElementById('ben-nickname').value.trim();
        const email = document.getElementById('ben-email').value.trim();

        try {
            const res = await API.addBeneficiary({
                name, account_number, bank_name, routing_code, nickname, email
            });
            this.showToast(res.message, 'success');
            this.closeModal('modal-add-beneficiary');
            this.navigate('transfer');
        } catch (err) {
            this.showToast(err.message, 'error');
        }
    },

    async removeBeneficiary(id) {
        if (confirm('Are you sure you want to remove this saved beneficiary?')) {
            try {
                await API.deleteBeneficiary(id);
                this.showToast('Beneficiary deleted.', 'info');
                this.navigate('transfer');
            } catch (err) {
                this.showToast(err.message, 'error');
            }
        }
    },

    // -------------------------------------------------------------
    // Manager & Admin Actions
    // -------------------------------------------------------------
    async reviewLoan(loanId, decision) {
        let reason = '';
        if (decision === 'REJECTED') {
            reason = prompt('Please enter reason for loan rejection:') || 'Credit evaluation criteria not met.';
        }

        try {
            const res = await API.reviewLoan(loanId, decision, reason);
            this.showToast(res.message, 'success');
            this.navigate('manager');
        } catch (err) {
            this.showToast(err.message, 'error');
        }
    },

    async toggleAccountStatus(accountId, nextStatus) {
        try {
            const res = await API.updateAccountStatus(accountId, nextStatus);
            this.showToast(res.message, 'success');
            this.navigate('manager');
        } catch (err) {
            this.showToast(err.message, 'error');
        }
    },

    openCreateUserModal() {
        this.openModal('modal-admin-create-user');
    },

    async handleAdminCreateUserSubmit(event) {
        event.preventDefault();
        const username = document.getElementById('admin-u-username').value.trim();
        const password = document.getElementById('admin-u-password').value;
        const full_name = document.getElementById('admin-u-fullname').value.trim();
        const email = document.getElementById('admin-u-email').value.trim();
        const phone = document.getElementById('admin-u-phone').value.trim();
        const role = document.getElementById('admin-u-role').value;

        try {
            const res = await API.createAdminUser({
                username, password, full_name, email, phone, role
            });
            this.showToast(res.message, 'success');
            this.closeModal('modal-admin-create-user');
            this.navigate('admin');
        } catch (err) {
            this.showToast(err.message, 'error');
        }
    },

    async toggleUserStatus(userId) {
        try {
            const res = await API.toggleUserStatus(userId);
            this.showToast(res.message, 'success');
            this.navigate('admin');
        } catch (err) {
            this.showToast(err.message, 'error');
        }
    },

    // -------------------------------------------------------------
    // Calculator Math
    // -------------------------------------------------------------
    updateCalculator() {
        const principalEl = document.getElementById('calc-slider-principal');
        const rateEl = document.getElementById('calc-slider-rate');
        const monthsEl = document.getElementById('calc-slider-months');

        if (!principalEl || !rateEl || !monthsEl) return;

        const p = parseFloat(principalEl.value);
        const r = parseFloat(rateEl.value);
        const n = parseInt(monthsEl.value);

        document.getElementById('calc-val-principal').textContent = `$${p.toLocaleString()}`;
        document.getElementById('calc-val-rate').textContent = `${r.toFixed(2)}%`;
        document.getElementById('calc-val-months').textContent = `${n} Months (${(n/12).toFixed(1)} Yrs)`;

        // Monthly Rate
        const monthlyRate = (r / 100) / 12;
        let emi = 0;
        if (monthlyRate === 0) {
            emi = p / n;
        } else {
            emi = (p * monthlyRate * Math.pow(1 + monthlyRate, n)) / (Math.pow(1 + monthlyRate, n) - 1);
        }

        const totalPayable = emi * n;
        const totalInterest = totalPayable - p;

        document.getElementById('calc-res-emi').textContent = `$${emi.toFixed(2)}`;
        document.getElementById('calc-res-principal').textContent = `$${p.toLocaleString(undefined, { minimumFractionDigits: 2 })}`;
        document.getElementById('calc-res-interest').textContent = `$${totalInterest.toLocaleString(undefined, { minimumFractionDigits: 2 })}`;
        document.getElementById('calc-res-total').textContent = `$${totalPayable.toLocaleString(undefined, { minimumFractionDigits: 2 })}`;
    },

    // -------------------------------------------------------------
    // Transactions Filtering & Export
    // -------------------------------------------------------------
    filterTransactionsTable() {
        const search = (document.getElementById('tx-search-input')?.value || '').toLowerCase();
        const type = document.getElementById('tx-type-filter')?.value || '';
        const rows = document.querySelectorAll('#transactions-full-table tbody tr');

        rows.forEach(r => {
            const text = r.textContent.toLowerCase();
            const matchesSearch = !search || text.includes(search);
            const matchesType = !type || text.includes(type.toLowerCase());
            r.style.display = (matchesSearch && matchesType) ? '' : 'none';
        });
    },

    exportTransactionsCSV() {
        const rows = document.querySelectorAll('#transactions-full-table tr');
        let csv = [];
        rows.forEach(r => {
            let row = [];
            r.querySelectorAll('th, td').forEach(cell => {
                row.push('"' + cell.innerText.replace(/"/g, '""').replace(/\n/g, ' ') + '"');
            });
            csv.push(row.join(','));
        });

        const csvContent = 'data:text/csv;charset=utf-8,' + csv.join('\n');
        const encodedUri = encodeURI(csvContent);
        const link = document.createElement('a');
        link.setAttribute('href', encodedUri);
        link.setAttribute('download', `apex_bank_statement_${new Date().toISOString().slice(0,10)}.csv`);
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
    },

    // -------------------------------------------------------------
    // Toast Notification System
    // -------------------------------------------------------------
    showToast(message, type = 'info') {
        const container = document.getElementById('toast-container');
        if (!container) return;

        const toast = document.createElement('div');
        toast.className = `toast toast-${type}`;
        
        let icon = 'fa-info-circle';
        if (type === 'success') icon = 'fa-check-circle';
        else if (type === 'error') icon = 'fa-exclamation-circle';

        toast.innerHTML = `<i class="fas ${icon}"></i> <span>${message}</span>`;
        container.appendChild(toast);

        setTimeout(() => {
            toast.style.opacity = '0';
            toast.style.transform = 'translateX(100%)';
            toast.style.transition = 'all 0.3s ease';
            setTimeout(() => toast.remove(), 300);
        }, 4000);
    }
};

document.addEventListener('DOMContentLoaded', () => {
    App.init();
});

window.App = App;
