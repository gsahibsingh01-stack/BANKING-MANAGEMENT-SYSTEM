// ====================================================================
// Apex National Bank - Views & UI Component Renderers
// ====================================================================

const Views = {
    // Helper to format currency
    formatCurrency(val, currency = '$') {
        const num = parseFloat(val) || 0.0;
        return `${currency}${num.toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 })}`;
    },

    // Helper for formatting date strings
    formatDate(str) {
        if (!str) return 'N/A';
        const d = new Date(str);
        if (isNaN(d.getTime())) return str;
        return d.toLocaleDateString('en-US', { month: 'short', day: 'numeric', year: 'numeric', hour: '2-digit', minute: '2-digit' });
    },

    // -------------------------------------------------------------
    // 1. DASHBOARD VIEW
    // -------------------------------------------------------------
    async renderDashboard(container) {
        container.innerHTML = `<div style="text-align:center;padding:40px;"><div class="status-dot" style="margin:0 auto 16px;width:16px;height:16px;"></div><p style="color:var(--text-secondary);">Loading your financial portfolio...</p></div>`;

        try {
            const data = await API.getDashboardSummary();
            const user = data.user;
            const accounts = data.accounts || [];
            const recentTx = data.recent_transactions || [];
            const loans = data.loans || [];
            const cards = data.cards || [];

            let accountsCardsHtml = '';
            accounts.forEach(acc => {
                accountsCardsHtml += `
                    <div class="stat-card" style="flex-direction:column;gap:12px;">
                        <div style="display:flex;justify-content:space-between;width:100%;align-items:center;">
                            <span class="badge ${acc.status === 'active' ? 'badge-success' : 'badge-danger'}">${acc.status.toUpperCase()}</span>
                            <span style="font-size:12px;color:var(--text-muted);font-family:monospace;">${acc.account_number}</span>
                        </div>
                        <div class="stat-info">
                            <p>${acc.account_type} Account (${acc.interest_rate}% APY)</p>
                            <h3>${Views.formatCurrency(acc.balance)}</h3>
                        </div>
                        <div style="display:flex;gap:8px;width:100%;margin-top:6px;">
                            <button class="btn-secondary" style="flex:1;padding:6px;font-size:12px;" onclick="App.openDepositModal(${acc.account_id})">Deposit</button>
                            <button class="btn-secondary" style="flex:1;padding:6px;font-size:12px;" onclick="App.openTransferModal(${acc.account_id})">Transfer</button>
                        </div>
                    </div>
                `;
            });

            let txRowsHtml = '';
            if (recentTx.length === 0) {
                txRowsHtml = `<tr><td colspan="5" style="text-align:center;color:var(--text-muted);padding:30px;">No recent transactions found.</td></tr>`;
            } else {
                recentTx.forEach(tx => {
                    const isCredit = (tx.type === 'DEPOSIT' || tx.type === 'INTEREST_CREDIT' || tx.type === 'LOAN_DISBURSEMENT');
                    const sign = isCredit ? '+' : '-';
                    const amountClass = isCredit ? 'amount-inflow' : 'amount-outflow';
                    const badgeClass = isCredit ? 'badge-success' : (tx.type === 'TRANSFER' ? 'badge-info' : 'badge-warning');

                    txRowsHtml += `
                        <tr>
                            <td>
                                <span style="font-weight:600;display:block;">${tx.description || tx.type}</span>
                                <span style="font-size:11px;color:var(--text-muted);font-family:monospace;">${tx.reference_number}</span>
                            </td>
                            <td><span class="badge ${badgeClass}">${tx.type}</span></td>
                            <td>${Views.formatDate(tx.created_at)}</td>
                            <td class="${amountClass}">${sign}${Views.formatCurrency(tx.amount)}</td>
                            <td>${Views.formatCurrency(tx.balance_after)}</td>
                        </tr>
                    `;
                });
            }

            container.innerHTML = `
                <!-- Top Metric Cards -->
                <div class="grid-cols-4">
                    <div class="stat-card">
                        <div class="stat-info">
                            <p>Total Net Worth</p>
                            <h3>${Views.formatCurrency(data.total_balance)}</h3>
                        </div>
                        <div class="stat-icon emerald"><i class="fas fa-wallet"></i></div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-info">
                            <p>Monthly Inflow</p>
                            <h3 style="color:var(--accent-emerald);">+${Views.formatCurrency(data.total_inflow)}</h3>
                        </div>
                        <div class="stat-icon cyan"><i class="fas fa-arrow-down-left"></i></div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-info">
                            <p>Monthly Outflow</p>
                            <h3 style="color:var(--accent-rose);">${Views.formatCurrency(data.total_outflow)}</h3>
                        </div>
                        <div class="stat-icon rose"><i class="fas fa-arrow-up-right"></i></div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-info">
                            <p>Active Loan Debt</p>
                            <h3 style="color:var(--accent-amber);">${Views.formatCurrency(data.total_loan_debt)}</h3>
                        </div>
                        <div class="stat-icon amber"><i class="fas fa-hand-holding-usd"></i></div>
                    </div>
                </div>

                <!-- Quick Actions Bar -->
                <div class="quick-actions-bar">
                    <div class="action-btn-card" onclick="App.openTransferModal()">
                        <div class="action-icon"><i class="fas fa-paper-plane"></i></div>
                        <div class="action-label">
                            <h4>Send Money</h4>
                            <p>Instant wire transfer</p>
                        </div>
                    </div>
                    <div class="action-btn-card" onclick="App.openDepositModal()">
                        <div class="action-icon" style="background:var(--accent-emerald-bg);color:var(--accent-emerald);"><i class="fas fa-plus-circle"></i></div>
                        <div class="action-label">
                            <h4>Deposit Funds</h4>
                            <p>Add cash to account</p>
                        </div>
                    </div>
                    <div class="action-btn-card" onclick="App.openWithdrawModal()">
                        <div class="action-icon" style="background:var(--accent-rose-bg);color:var(--accent-rose);"><i class="fas fa-money-bill-wave"></i></div>
                        <div class="action-label">
                            <h4>ATM Withdrawal</h4>
                            <p>Debit cash instantly</p>
                        </div>
                    </div>
                    <div class="action-btn-card" onclick="App.openApplyLoanModal()">
                        <div class="action-icon" style="background:var(--accent-amber-bg);color:var(--accent-amber);"><i class="fas fa-file-invoice-dollar"></i></div>
                        <div class="action-label">
                            <h4>Apply for Loan</h4>
                            <p>Instant rate quote</p>
                        </div>
                    </div>
                </div>

                <!-- Accounts Grid -->
                <div class="card-panel">
                    <div class="panel-header">
                        <div class="panel-title"><i class="fas fa-landmark"></i> Your Bank Accounts</div>
                        <button class="btn-secondary" onclick="App.openCreateAccountModal()"><i class="fas fa-plus"></i> Open New Account</button>
                    </div>
                    <div class="grid-cols-3" style="margin-bottom:0;">
                        ${accountsCardsHtml}
                    </div>
                </div>

                <!-- Recent Transactions Panel -->
                <div class="card-panel">
                    <div class="panel-header">
                        <div class="panel-title"><i class="fas fa-history"></i> Recent Activity</div>
                        <button class="btn-secondary" onclick="App.navigate('transactions')">View All Activity</button>
                    </div>
                    <div class="table-responsive">
                        <table class="custom-table">
                            <thead>
                                <tr>
                                    <th>Description / Ref</th>
                                    <th>Type</th>
                                    <th>Date</th>
                                    <th>Amount</th>
                                    <th>Balance After</th>
                                </tr>
                            </thead>
                            <tbody>
                                ${txRowsHtml}
                            </tbody>
                        </table>
                    </div>
                </div>
            `;
        } catch (err) {
            container.innerHTML = `<div class="card-panel" style="color:var(--accent-rose);text-align:center;padding:40px;">Error loading dashboard: ${err.message}</div>`;
        }
    },

    // -------------------------------------------------------------
    // 2. ACCOUNTS VIEW
    // -------------------------------------------------------------
    async renderAccounts(container) {
        container.innerHTML = `<div style="text-align:center;padding:40px;"><p style="color:var(--text-secondary);">Loading accounts...</p></div>`;

        try {
            const data = await API.getAccounts();
            const accounts = data.accounts || [];

            let html = `
                <div class="card-panel">
                    <div class="panel-header">
                        <div>
                            <h2 style="font-size:20px;font-weight:700;color:#fff;">Deposit & Savings Accounts</h2>
                            <p style="font-size:13px;color:var(--text-secondary);">Manage and monitor your active bank accounts with Apex National Bank</p>
                        </div>
                        <button class="btn-primary" style="width:auto;padding:10px 20px;" onclick="App.openCreateAccountModal()">
                            <i class="fas fa-plus"></i> Open New Account
                        </button>
                    </div>

                    <div class="grid-cols-2">
            `;

            accounts.forEach(acc => {
                html += `
                    <div class="stat-card" style="flex-direction:column;padding:26px;gap:16px;">
                        <div style="display:flex;justify-content:space-between;width:100%;align-items:center;">
                            <div>
                                <span class="badge ${acc.status === 'active' ? 'badge-success' : 'badge-danger'}" style="margin-right:8px;">${acc.status.toUpperCase()}</span>
                                <span class="badge badge-info">${acc.currency}</span>
                            </div>
                            <span style="font-family:monospace;font-size:14px;color:var(--text-secondary);">${acc.account_number}</span>
                        </div>

                        <div>
                            <h3 style="font-size:16px;color:var(--text-secondary);font-weight:500;">${acc.account_type} Account</h3>
                            <h2 style="font-size:32px;font-weight:800;color:#fff;margin:6px 0;">${Views.formatCurrency(acc.balance)}</h2>
                            <p style="font-size:12px;color:var(--accent-cyan);"><i class="fas fa-percent"></i> Annual Percentage Yield (APY): ${acc.interest_rate}%</p>
                        </div>

                        <div style="border-top:1px solid var(--border-color);padding-top:16px;width:100%;display:flex;justify-content:space-between;align-items:center;">
                            <span style="font-size:12px;color:var(--text-muted);">Opened: ${Views.formatDate(acc.created_at)}</span>
                            <div style="display:flex;gap:8px;">
                                <button class="btn-secondary" style="padding:6px 12px;font-size:13px;" onclick="App.openDepositModal(${acc.account_id})">Deposit</button>
                                <button class="btn-secondary" style="padding:6px 12px;font-size:13px;" onclick="App.openWithdrawModal(${acc.account_id})">Withdraw</button>
                                <button class="btn-primary" style="padding:6px 12px;font-size:13px;width:auto;" onclick="App.openTransferModal(${acc.account_id})">Transfer</button>
                            </div>
                        </div>
                    </div>
                `;
            });

            html += `
                    </div>
                </div>
            `;

            container.innerHTML = html;
        } catch (err) {
            container.innerHTML = `<div class="card-panel" style="color:var(--accent-rose);">Failed to load accounts: ${err.message}</div>`;
        }
    },

    // -------------------------------------------------------------
    // 3. TRANSFER & PAYMENTS VIEW
    // -------------------------------------------------------------
    async renderTransfer(container) {
        container.innerHTML = `<div style="text-align:center;padding:40px;"><p style="color:var(--text-secondary);">Loading transfer portal...</p></div>`;

        try {
            const [accData, benData] = await Promise.all([
                API.getAccounts(),
                API.getBeneficiaries()
            ]);

            const accounts = accData.accounts || [];
            const beneficiaries = benData.beneficiaries || [];

            let accountOptions = '';
            accounts.forEach(a => {
                if (a.status === 'active') {
                    accountOptions += `<option value="${a.account_id}">${a.account_type} (${a.account_number}) - Available: ${Views.formatCurrency(a.balance)}</option>`;
                }
            });

            let benOptions = '<option value="">-- Or choose from Saved Beneficiaries --</option>';
            beneficiaries.forEach(b => {
                benOptions += `<option value="${b.account_number}">${b.name} (${b.nickname ? b.nickname + ' - ' : ''}${b.account_number})</option>`;
            });

            let benCardsHtml = '';
            if (beneficiaries.length === 0) {
                benCardsHtml = `<p style="color:var(--text-muted);font-size:13px;">No saved payees yet. Save frequent contacts below for 1-click transfers.</p>`;
            } else {
                beneficiaries.forEach(b => {
                    benCardsHtml += `
                        <div style="background:rgba(255,255,255,0.03);border:1px solid var(--border-color);border-radius:var(--radius-sm);padding:12px;display:flex;justify-content:space-between;align-items:center;margin-bottom:8px;">
                            <div>
                                <h4 style="font-size:14px;color:#fff;">${b.name}</h4>
                                <span style="font-size:12px;color:var(--text-muted);font-family:monospace;">${b.bank_name} • ${b.account_number}</span>
                            </div>
                            <div style="display:flex;gap:6px;">
                                <button class="btn-secondary" style="padding:4px 10px;font-size:12px;" onclick="App.quickTransferTo('${b.account_number}')">Send</button>
                                <button class="sidebar-logout-btn" onclick="App.removeBeneficiary(${b.beneficiary_id})"><i class="fas fa-trash"></i></button>
                            </div>
                        </div>
                    `;
                });
            }

            container.innerHTML = `
                <div class="grid-cols-2">
                    <!-- Transfer Form -->
                    <div class="card-panel">
                        <div class="panel-header">
                            <div class="panel-title"><i class="fas fa-paper-plane"></i> Send Funds Online</div>
                        </div>
                        <form id="transfer-form" onsubmit="App.handleTransferSubmit(event)" style="display:flex;flex-direction:column;gap:16px;">
                            <div class="form-group">
                                <label>From Source Account</label>
                                <select class="form-select" id="transfer-from-account" required>
                                    ${accountOptions}
                                </select>
                            </div>

                            <div class="form-group">
                                <label>Quick Select Saved Payee</label>
                                <select class="form-select" id="transfer-beneficiary-select" onchange="App.handleBeneficiarySelect(this)">
                                    ${benOptions}
                                </select>
                            </div>

                            <div class="form-group">
                                <label>Recipient Account Number</label>
                                <div style="display:flex;gap:8px;">
                                    <input type="text" class="form-input" id="transfer-to-account" placeholder="e.g. 100120260003" required onblur="App.lookupRecipient(this.value)">
                                    <button type="button" class="btn-secondary" onclick="App.lookupRecipient(document.getElementById('transfer-to-account').value)" style="white-space:nowrap;">Verify</button>
                                </div>
                                <span id="recipient-verify-status" style="font-size:12px;margin-top:4px;display:block;"></span>
                            </div>

                            <div class="form-group">
                                <label>Transfer Amount (USD)</label>
                                <input type="number" step="0.01" min="1" class="form-input" id="transfer-amount" placeholder="0.00" required>
                            </div>

                            <div class="form-group">
                                <label>Reference Note / Purpose</label>
                                <input type="text" class="form-input" id="transfer-desc" placeholder="e.g. Freelance invoice, rent, dinner" value="Online Funds Transfer">
                            </div>

                            <button type="submit" class="btn-primary" id="btn-submit-transfer">
                                <i class="fas fa-check-circle"></i> Complete Wire Transfer
                            </button>
                        </form>
                    </div>

                    <!-- Beneficiaries List -->
                    <div class="card-panel">
                        <div class="panel-header">
                            <div class="panel-title"><i class="fas fa-users"></i> Saved Beneficiaries</div>
                            <button class="btn-secondary" onclick="App.openAddBeneficiaryModal()"><i class="fas fa-user-plus"></i> Add New</button>
                        </div>
                        <div style="max-height:440px;overflow-y:auto;">
                            ${benCardsHtml}
                        </div>
                    </div>
                </div>
            `;
        } catch (err) {
            container.innerHTML = `<div class="card-panel" style="color:var(--accent-rose);">Error: ${err.message}</div>`;
        }
    },

    // -------------------------------------------------------------
    // 4. DEPOSIT & WITHDRAW VIEW
    // -------------------------------------------------------------
    async renderDepositWithdraw(container) {
        container.innerHTML = `<div style="text-align:center;padding:40px;"><p style="color:var(--text-secondary);">Loading teller portal...</p></div>`;

        try {
            const accData = await API.getAccounts();
            const accounts = accData.accounts || [];

            let accountOptions = '';
            accounts.forEach(a => {
                accountOptions += `<option value="${a.account_id}">${a.account_type} (${a.account_number}) - Balance: ${Views.formatCurrency(a.balance)}</option>`;
            });

            container.innerHTML = `
                <div class="grid-cols-2">
                    <!-- Deposit Panel -->
                    <div class="card-panel">
                        <div class="panel-header">
                            <div class="panel-title"><i class="fas fa-arrow-circle-down" style="color:var(--accent-emerald);"></i> Cash Deposit Simulation</div>
                        </div>
                        <form onsubmit="App.handleDepositSubmit(event)" style="display:flex;flex-direction:column;gap:16px;">
                            <div class="form-group">
                                <label>Target Bank Account</label>
                                <select class="form-select" id="deposit-account-select" required>
                                    ${accountOptions}
                                </select>
                            </div>
                            <div class="form-group">
                                <label>Deposit Amount (USD)</label>
                                <input type="number" step="0.01" min="1" class="form-input" id="deposit-amount-input" placeholder="0.00" required>
                            </div>
                            <div style="display:flex;gap:8px;">
                                <button type="button" class="btn-secondary" style="flex:1;" onclick="document.getElementById('deposit-amount-input').value = 100;">+$100</button>
                                <button type="button" class="btn-secondary" style="flex:1;" onclick="document.getElementById('deposit-amount-input').value = 500;">+$500</button>
                                <button type="button" class="btn-secondary" style="flex:1;" onclick="document.getElementById('deposit-amount-input').value = 1000;">+$1,000</button>
                                <button type="button" class="btn-secondary" style="flex:1;" onclick="document.getElementById('deposit-amount-input').value = 5000;">+$5,000</button>
                            </div>
                            <div class="form-group">
                                <label>Description / Deposit Source</label>
                                <input type="text" class="form-input" id="deposit-desc-input" value="Direct Cash Deposit">
                            </div>
                            <button type="submit" class="btn-primary" style="background:linear-gradient(135deg, #10b981 0%, #059669 100%);">
                                <i class="fas fa-check"></i> Deposit Funds Now
                            </button>
                        </form>
                    </div>

                    <!-- Withdrawal Panel -->
                    <div class="card-panel">
                        <div class="panel-header">
                            <div class="panel-title"><i class="fas fa-arrow-circle-up" style="color:var(--accent-rose);"></i> ATM Cash Withdrawal</div>
                        </div>
                        <form onsubmit="App.handleWithdrawSubmit(event)" style="display:flex;flex-direction:column;gap:16px;">
                            <div class="form-group">
                                <label>Withdraw From Account</label>
                                <select class="form-select" id="withdraw-account-select" required>
                                    ${accountOptions}
                                </select>
                            </div>
                            <div class="form-group">
                                <label>Withdrawal Amount (USD)</label>
                                <input type="number" step="0.01" min="1" class="form-input" id="withdraw-amount-input" placeholder="0.00" required>
                            </div>
                            <div style="display:flex;gap:8px;">
                                <button type="button" class="btn-secondary" style="flex:1;" onclick="document.getElementById('withdraw-amount-input').value = 50;">$50</button>
                                <button type="button" class="btn-secondary" style="flex:1;" onclick="document.getElementById('withdraw-amount-input').value = 100;">$100</button>
                                <button type="button" class="btn-secondary" style="flex:1;" onclick="document.getElementById('withdraw-amount-input').value = 200;">$200</button>
                                <button type="button" class="btn-secondary" style="flex:1;" onclick="document.getElementById('withdraw-amount-input').value = 500;">$500</button>
                            </div>
                            <div class="form-group">
                                <label>ATM Terminal / Reason</label>
                                <input type="text" class="form-input" id="withdraw-desc-input" value="ATM Cash Withdrawal - Main Campus">
                            </div>
                            <button type="submit" class="btn-primary" style="background:linear-gradient(135deg, #f43f5e 0%, #e11d48 100%);">
                                <i class="fas fa-money-bill-wave"></i> Confirm Cash Dispense
                            </button>
                        </form>
                    </div>
                </div>
            `;
        } catch (err) {
            container.innerHTML = `<div class="card-panel" style="color:var(--accent-rose);">Error: ${err.message}</div>`;
        }
    },

    // -------------------------------------------------------------
    // 5. LOANS & MORTGAGES VIEW
    // -------------------------------------------------------------
    async renderLoans(container) {
        container.innerHTML = `<div style="text-align:center;padding:40px;"><p style="color:var(--text-secondary);">Loading loan facilities...</p></div>`;

        try {
            const data = await API.getLoans();
            const loans = data.loans || [];

            let loansHtml = '';
            if (loans.length === 0) {
                loansHtml = `<tr><td colspan="8" style="text-align:center;color:var(--text-muted);padding:30px;">No loan applications found. Apply for personal, vehicle, education, or home loans today!</td></tr>`;
            } else {
                loans.forEach(ln => {
                    let badgeClass = 'badge-warning';
                    if (ln.status === 'APPROVED') badgeClass = 'badge-success';
                    else if (ln.status === 'REJECTED') badgeClass = 'badge-danger';
                    else if (ln.status === 'CLOSED') badgeClass = 'badge-secondary';

                    loansHtml += `
                        <tr>
                            <td><strong>#${ln.loan_id}</strong></td>
                            <td>${ln.loan_type}</td>
                            <td>${Views.formatCurrency(ln.principal_amount)}</td>
                            <td>${ln.interest_rate}%</td>
                            <td>${ln.duration_months} mo.</td>
                            <td style="color:var(--accent-cyan);font-weight:600;">${Views.formatCurrency(ln.monthly_emi)} /mo</td>
                            <td style="color:var(--accent-rose);font-weight:600;">${Views.formatCurrency(ln.remaining_balance)}</td>
                            <td><span class="badge ${badgeClass}">${ln.status}</span></td>
                            <td>
                                ${ln.status === 'APPROVED' && ln.remaining_balance > 0 ? 
                                    `<button class="btn-secondary" style="padding:4px 10px;font-size:12px;" onclick="App.openRepayLoanModal(${ln.loan_id}, ${ln.remaining_balance}, ${ln.monthly_emi})">Pay EMI</button>` : 
                                    '-'}
                            </td>
                        </tr>
                    `;
                });
            }

            container.innerHTML = `
                <div class="card-panel">
                    <div class="panel-header">
                        <div>
                            <h2 style="font-size:20px;font-weight:700;color:#fff;">Credit & Loan Portfolio</h2>
                            <p style="font-size:13px;color:var(--text-secondary);">Track active mortgages, education, personal, and auto loans</p>
                        </div>
                        <button class="btn-primary" style="width:auto;padding:10px 20px;" onclick="App.openApplyLoanModal()">
                            <i class="fas fa-plus"></i> Apply for a New Loan
                        </button>
                    </div>

                    <div class="table-responsive">
                        <table class="custom-table">
                            <thead>
                                <tr>
                                    <th>Loan ID</th>
                                    <th>Type</th>
                                    <th>Principal</th>
                                    <th>Rate</th>
                                    <th>Tenure</th>
                                    <th>Monthly EMI</th>
                                    <th>Remaining Debt</th>
                                    <th>Status</th>
                                    <th>Action</th>
                                </tr>
                            </thead>
                            <tbody>
                                ${loansHtml}
                            </tbody>
                        </table>
                    </div>
                </div>
            `;
        } catch (err) {
            container.innerHTML = `<div class="card-panel" style="color:var(--accent-rose);">Error: ${err.message}</div>`;
        }
    },

    // -------------------------------------------------------------
    // 6. CARDS & SECURITY VIEW
    // -------------------------------------------------------------
    async renderCards(container) {
        container.innerHTML = `<div style="text-align:center;padding:40px;"><p style="color:var(--text-secondary);">Loading cards...</p></div>`;

        try {
            const data = await API.getCards();
            const cards = data.cards || [];

            let cardsHtml = '';
            cards.forEach(cd => {
                const isLocked = cd.status === 'LOCKED';
                cardsHtml += `
                    <div style="display:flex;flex-direction:column;gap:16px;">
                        <div class="virtual-card" style="background:${cd.card_type === 'Credit' ? 'var(--card-gradient-2)' : 'var(--card-gradient-1)'};filter:${isLocked ? 'grayscale(80%)' : 'none'};">
                            <div class="card-top">
                                <span class="card-bank-name">APEX NATIONAL BANK</span>
                                <div class="card-chip"></div>
                            </div>
                            <div class="card-number" id="card-num-${cd.card_id}">
                                ${cd.card_number}
                            </div>
                            <div class="card-bottom">
                                <div class="card-holder-info">
                                    <label>Cardholder Name</label>
                                    <p>${cd.card_holder_name}</p>
                                </div>
                                <div class="card-holder-info">
                                    <label>Expires</label>
                                    <p>${cd.expiry_str}</p>
                                </div>
                                <div class="card-type-logo">
                                    ${cd.card_type.toUpperCase()}
                                </div>
                            </div>
                        </div>

                        <div class="stat-card" style="flex-direction:column;gap:12px;">
                            <div style="display:flex;justify-content:space-between;width:100%;align-items:center;">
                                <span class="badge ${isLocked ? 'badge-danger' : 'badge-success'}">${cd.status}</span>
                                <span style="font-size:12px;color:var(--text-muted);">Linked Account: ${cd.account_number}</span>
                            </div>

                            <div style="display:flex;justify-content:space-between;align-items:center;width:100%;">
                                <div>
                                    <span style="font-size:12px;color:var(--text-secondary);">Daily Limit</span>
                                    <h4 style="font-size:16px;color:#fff;">${Views.formatCurrency(cd.daily_limit)}</h4>
                                </div>
                                <div style="display:flex;gap:8px;">
                                    <button class="btn-secondary" style="padding:6px 12px;font-size:13px;" onclick="App.toggleCardReveal(${cd.card_id}, '${cd.card_number_full}', '${cd.cvv_full}')">
                                        <i class="fas fa-eye"></i> Show Details
                                    </button>
                                    <button class="${isLocked ? 'btn-success' : 'btn-danger'}" style="padding:6px 12px;font-size:13px;" onclick="App.toggleCardLock(${cd.card_id})">
                                        <i class="fas ${isLocked ? 'fa-unlock' : 'fa-lock'}"></i> ${isLocked ? 'Unlock Card' : 'Freeze Card'}
                                    </button>
                                </div>
                            </div>
                        </div>
                    </div>
                `;
            });

            container.innerHTML = `
                <div class="card-panel">
                    <div class="panel-header">
                        <div>
                            <h2 style="font-size:20px;font-weight:700;color:#fff;">Debit & Credit Cards</h2>
                            <p style="font-size:13px;color:var(--text-secondary);">Instant virtual card management, freeze controls, and spending limits</p>
                        </div>
                        <button class="btn-primary" style="width:auto;padding:10px 20px;" onclick="App.openIssueCardModal()">
                            <i class="fas fa-plus"></i> Issue New Card
                        </button>
                    </div>

                    <div class="grid-cols-2">
                        ${cardsHtml}
                    </div>
                </div>
            `;
        } catch (err) {
            container.innerHTML = `<div class="card-panel" style="color:var(--accent-rose);">Error: ${err.message}</div>`;
        }
    },

    // -------------------------------------------------------------
    // 7. TRANSACTIONS STATEMENT VIEW
    // -------------------------------------------------------------
    async renderTransactions(container) {
        container.innerHTML = `<div style="text-align:center;padding:40px;"><p style="color:var(--text-secondary);">Loading transaction statement...</p></div>`;

        try {
            const data = await API.getTransactions({ limit: 100 });
            const transactions = data.transactions || [];

            let rows = '';
            if (transactions.length === 0) {
                rows = `<tr><td colspan="7" style="text-align:center;color:var(--text-muted);padding:30px;">No transactions recorded.</td></tr>`;
            } else {
                transactions.forEach(tx => {
                    const isCredit = (tx.type === 'DEPOSIT' || tx.type === 'INTEREST_CREDIT' || tx.type === 'LOAN_DISBURSEMENT');
                    const sign = isCredit ? '+' : '-';
                    const amountClass = isCredit ? 'amount-inflow' : 'amount-outflow';
                    const badgeClass = isCredit ? 'badge-success' : (tx.type === 'TRANSFER' ? 'badge-info' : 'badge-warning');

                    rows += `
                        <tr>
                            <td><span style="font-family:monospace;font-size:12px;color:var(--accent-cyan);">${tx.reference_number}</span></td>
                            <td>${Views.formatDate(tx.created_at)}</td>
                            <td><span class="badge ${badgeClass}">${tx.type}</span></td>
                            <td>${tx.description || '-'}</td>
                            <td>${tx.from_account_number || tx.to_account_number || '-'}</td>
                            <td class="${amountClass}" style="font-size:15px;">${sign}${Views.formatCurrency(tx.amount)}</td>
                            <td style="font-weight:600;">${Views.formatCurrency(tx.balance_after)}</td>
                        </tr>
                    `;
                });
            }

            container.innerHTML = `
                <div class="card-panel">
                    <div class="panel-header">
                        <div>
                            <h2 style="font-size:20px;font-weight:700;color:#fff;">Comprehensive Account Statement</h2>
                            <p style="font-size:13px;color:var(--text-secondary);">Full ledger history of deposits, withdrawals, transfers, and accrued interest</p>
                        </div>
                        <div style="display:flex;gap:10px;">
                            <button class="btn-secondary" onclick="App.exportTransactionsCSV()"><i class="fas fa-file-csv"></i> Export CSV</button>
                            <button class="btn-secondary" onclick="window.print()"><i class="fas fa-print"></i> Print Statement</button>
                        </div>
                    </div>

                    <!-- Filter Bar -->
                    <div style="display:flex;gap:12px;margin-bottom:20px;flex-wrap:wrap;">
                        <input type="text" class="form-input" id="tx-search-input" placeholder="Search by reference, description..." style="max-width:300px;" oninput="App.filterTransactionsTable()">
                        <select class="form-select" id="tx-type-filter" style="max-width:200px;" onchange="App.filterTransactionsTable()">
                            <option value="">All Transaction Types</option>
                            <option value="DEPOSIT">Deposits</option>
                            <option value="WITHDRAW">Withdrawals</option>
                            <option value="TRANSFER">Transfers</option>
                            <option value="LOAN_DISBURSEMENT">Loan Disbursements</option>
                            <option value="LOAN_REPAYMENT">Loan Repayments</option>
                            <option value="INTEREST_CREDIT">Interest Credits</option>
                        </select>
                    </div>

                    <div class="table-responsive">
                        <table class="custom-table" id="transactions-full-table">
                            <thead>
                                <tr>
                                    <th>Ref Number</th>
                                    <th>Timestamp</th>
                                    <th>Type</th>
                                    <th>Description</th>
                                    <th>Account</th>
                                    <th>Amount</th>
                                    <th>Balance</th>
                                </tr>
                            </thead>
                            <tbody>
                                ${rows}
                            </tbody>
                        </table>
                    </div>
                </div>
            `;
        } catch (err) {
            container.innerHTML = `<div class="card-panel" style="color:var(--accent-rose);">Error: ${err.message}</div>`;
        }
    },

    // -------------------------------------------------------------
    // 8. FINANCIAL CALCULATOR VIEW
    // -------------------------------------------------------------
    renderCalculator(container) {
        container.innerHTML = `
            <div class="card-panel">
                <div class="panel-header">
                    <div>
                        <h2 style="font-size:20px;font-weight:700;color:#fff;">Financial Planning & EMI Calculator</h2>
                        <p style="font-size:13px;color:var(--text-secondary);">Accurate mathematical simulation of amortized loan installments and interest earnings</p>
                    </div>
                </div>

                <div class="calc-container">
                    <!-- Calculator Inputs -->
                    <div style="display:flex;flex-direction:column;gap:20px;">
                        <div class="form-group">
                            <div style="display:flex;justify-content:space-between;">
                                <label>Loan Principal Amount ($)</label>
                                <span id="calc-val-principal" style="font-weight:700;color:var(--accent-cyan);">$25,000</span>
                            </div>
                            <input type="range" min="1000" max="200000" step="1000" value="25000" class="form-input" id="calc-slider-principal" oninput="App.updateCalculator()" style="padding:0;cursor:pointer;">
                        </div>

                        <div class="form-group">
                            <div style="display:flex;justify-content:space-between;">
                                <label>Annual Interest Rate (%)</label>
                                <span id="calc-val-rate" style="font-weight:700;color:var(--accent-cyan);">6.5%</span>
                            </div>
                            <input type="range" min="1.0" max="25.0" step="0.25" value="6.5" class="form-input" id="calc-slider-rate" oninput="App.updateCalculator()" style="padding:0;cursor:pointer;">
                        </div>

                        <div class="form-group">
                            <div style="display:flex;justify-content:space-between;">
                                <label>Loan Tenure (Months)</label>
                                <span id="calc-val-months" style="font-weight:700;color:var(--accent-cyan);">36 Months (3 Years)</span>
                            </div>
                            <input type="range" min="6" max="120" step="6" value="36" class="form-input" id="calc-slider-months" oninput="App.updateCalculator()" style="padding:0;cursor:pointer;">
                        </div>

                        <div style="background:rgba(255,255,255,0.03);padding:16px;border-radius:var(--radius-sm);border:1px solid var(--border-color);font-size:13px;color:var(--text-secondary);">
                            <p><strong>Formula Used:</strong> Standard Equated Monthly Installment (EMI)</p>
                            <p style="font-family:monospace;margin-top:4px;color:var(--accent-emerald);">EMI = [P × r × (1 + r)ⁿ] / [(1 + r)ⁿ - 1]</p>
                        </div>
                    </div>

                    <!-- Calculator Results -->
                    <div class="calc-result-box">
                        <span style="font-size:13px;text-transform:uppercase;letter-spacing:1px;color:var(--text-muted);font-weight:600;">Monthly Equated Installment</span>
                        <div class="calc-emi-amount" id="calc-res-emi">$766.23</div>

                        <div style="width:100%;">
                            <div class="calc-breakdown-row">
                                <span style="color:var(--text-secondary);">Principal Borrowed</span>
                                <span id="calc-res-principal" style="font-weight:600;color:#fff;">$25,000.00</span>
                            </div>
                            <div class="calc-breakdown-row">
                                <span style="color:var(--text-secondary);">Total Interest Amount</span>
                                <span id="calc-res-interest" style="font-weight:600;color:var(--accent-amber);">$2,584.28</span>
                            </div>
                            <div class="calc-breakdown-row" style="border-bottom:none;padding-top:14px;">
                                <span style="color:var(--text-primary);font-weight:600;">Total Amount Payable</span>
                                <span id="calc-res-total" style="font-weight:700;color:var(--accent-cyan);font-size:16px;">$27,584.28</span>
                            </div>
                        </div>

                        <button class="btn-primary" style="margin-top:20px;width:100%;" onclick="App.openApplyLoanModal()">
                            <i class="fas fa-check"></i> Apply for this Loan Rate
                        </button>
                    </div>
                </div>
            </div>
        `;

        App.updateCalculator();
    },

    // -------------------------------------------------------------
    // 9. BRANCH MANAGER PORTAL VIEW
    // -------------------------------------------------------------
    async renderManagerPortal(container) {
        container.innerHTML = `<div style="text-align:center;padding:40px;"><p style="color:var(--text-secondary);">Loading manager controls...</p></div>`;

        try {
            const [loansData, accountsData] = await Promise.all([
                API.getLoans(),
                API.getManagerAccounts()
            ]);

            const loans = loansData.loans || [];
            const accounts = accountsData.accounts || [];

            let loanRows = '';
            const pendingLoans = loans.filter(l => l.status === 'PENDING');
            if (pendingLoans.length === 0) {
                loanRows = `<tr><td colspan="7" style="text-align:center;color:var(--text-muted);padding:24px;">No pending loan applications awaiting review.</td></tr>`;
            } else {
                pendingLoans.forEach(l => {
                    loanRows += `
                        <tr>
                            <td><strong>#${l.loan_id}</strong></td>
                            <td>${l.user_name}</td>
                            <td>${l.loan_type}</td>
                            <td>${Views.formatCurrency(l.principal_amount)}</td>
                            <td>${l.duration_months} mo.</td>
                            <td>${l.purpose || 'Personal finances'}</td>
                            <td>
                                <button class="btn-success" style="padding:4px 8px;font-size:12px;margin-right:4px;" onclick="App.reviewLoan(${l.loan_id}, 'APPROVED')"><i class="fas fa-check"></i> Approve</button>
                                <button class="btn-danger" style="padding:4px 8px;font-size:12px;" onclick="App.reviewLoan(${l.loan_id}, 'REJECTED')"><i class="fas fa-times"></i> Reject</button>
                            </td>
                        </tr>
                    `;
                });
            }

            let accRows = '';
            accounts.forEach(a => {
                const isFrozen = a.status === 'frozen';
                accRows += `
                    <tr>
                        <td><span style="font-family:monospace;font-weight:600;">${a.account_number}</span></td>
                        <td>${a.owner_name}</td>
                        <td>${a.account_type}</td>
                        <td>${Views.formatCurrency(a.balance)}</td>
                        <td><span class="badge ${a.status === 'active' ? 'badge-success' : 'badge-danger'}">${a.status.toUpperCase()}</span></td>
                        <td>
                            <button class="${isFrozen ? 'btn-success' : 'btn-danger'}" style="padding:4px 10px;font-size:12px;" onclick="App.toggleAccountStatus(${a.account_id}, '${isFrozen ? 'active' : 'frozen'}')">
                                ${isFrozen ? 'Unfreeze' : 'Freeze'}
                            </button>
                        </td>
                    </tr>
                `;
            });

            container.innerHTML = `
                <!-- Pending Loans Review -->
                <div class="card-panel">
                    <div class="panel-header">
                        <div class="panel-title"><i class="fas fa-clipboard-check" style="color:var(--accent-amber);"></i> Pending Loan Applications (${pendingLoans.length})</div>
                    </div>
                    <div class="table-responsive">
                        <table class="custom-table">
                            <thead>
                                <tr>
                                    <th>Loan ID</th>
                                    <th>Applicant</th>
                                    <th>Loan Type</th>
                                    <th>Requested Amount</th>
                                    <th>Tenure</th>
                                    <th>Purpose</th>
                                    <th>Action</th>
                                </tr>
                            </thead>
                            <tbody>
                                ${loanRows}
                            </tbody>
                        </table>
                    </div>
                </div>

                <!-- Customer Accounts Management -->
                <div class="card-panel">
                    <div class="panel-header">
                        <div class="panel-title"><i class="fas fa-users-cog"></i> Customer Accounts Control</div>
                    </div>
                    <div class="table-responsive">
                        <table class="custom-table">
                            <thead>
                                <tr>
                                    <th>Account Number</th>
                                    <th>Owner</th>
                                    <th>Type</th>
                                    <th>Balance</th>
                                    <th>Status</th>
                                    <th>Action</th>
                                </tr>
                            </thead>
                            <tbody>
                                ${accRows}
                            </tbody>
                        </table>
                    </div>
                </div>
            `;
        } catch (err) {
            container.innerHTML = `<div class="card-panel" style="color:var(--accent-rose);">Error: ${err.message}</div>`;
        }
    },

    // -------------------------------------------------------------
    // 10. SYSTEM ADMIN PORTAL VIEW
    // -------------------------------------------------------------
    async renderAdminPortal(container) {
        container.innerHTML = `<div style="text-align:center;padding:40px;"><p style="color:var(--text-secondary);">Loading system administration...</p></div>`;

        try {
            const [statsData, usersData, logsData] = await Promise.all([
                API.getAdminStats(),
                API.getAdminUsers(),
                API.getAuditLogs()
            ]);

            const users = usersData.users || [];
            const logs = logsData.logs || [];

            let userRows = '';
            users.forEach(u => {
                const isSuspended = u.status === 'suspended';
                userRows += `
                    <tr>
                        <td>#${u.user_id}</td>
                        <td><strong>${u.username}</strong></td>
                        <td>${u.full_name}</td>
                        <td>${u.email}</td>
                        <td><span class="badge ${u.role === 'admin' ? 'badge-danger' : (u.role === 'manager' ? 'badge-warning' : 'badge-info')}">${u.role.toUpperCase()}</span></td>
                        <td><span class="badge ${u.status === 'active' ? 'badge-success' : 'badge-danger'}">${u.status}</span></td>
                        <td>
                            <button class="${isSuspended ? 'btn-success' : 'btn-danger'}" style="padding:4px 8px;font-size:12px;" onclick="App.toggleUserStatus(${u.user_id})">
                                ${isSuspended ? 'Activate' : 'Suspend'}
                            </button>
                        </td>
                    </tr>
                `;
            });

            let logRows = '';
            logs.slice(0, 15).forEach(l => {
                logRows += `
                    <tr>
                        <td>${Views.formatDate(l.created_at)}</td>
                        <td><span class="badge badge-secondary">${l.action}</span></td>
                        <td><strong>${l.username || 'System'}</strong></td>
                        <td>${l.details || '-'}</td>
                        <td><span style="font-family:monospace;font-size:11px;color:var(--text-muted);">${l.ip_address || '127.0.0.1'}</span></td>
                    </tr>
                `;
            });

            container.innerHTML = `
                <!-- System KPI Metrics -->
                <div class="grid-cols-4">
                    <div class="stat-card">
                        <div class="stat-info">
                            <p>Total Registered Users</p>
                            <h3>${statsData.total_users || 0}</h3>
                        </div>
                        <div class="stat-icon primary"><i class="fas fa-users"></i></div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-info">
                            <p>Active Accounts</p>
                            <h3>${statsData.total_accounts || 0}</h3>
                        </div>
                        <div class="stat-icon emerald"><i class="fas fa-landmark"></i></div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-info">
                            <p>Total Vault Deposits</p>
                            <h3>${Views.formatCurrency(statsData.total_deposits || 0)}</h3>
                        </div>
                        <div class="stat-icon cyan"><i class="fas fa-vault"></i></div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-info">
                            <p>Total Transactions</p>
                            <h3>${statsData.total_transactions_count || 0}</h3>
                        </div>
                        <div class="stat-icon amber"><i class="fas fa-exchange-alt"></i></div>
                    </div>
                </div>

                <!-- User Management Table -->
                <div class="card-panel">
                    <div class="panel-header">
                        <div class="panel-title"><i class="fas fa-user-shield"></i> User Directory & Access Control</div>
                        <button class="btn-primary" style="width:auto;padding:8px 16px;" onclick="App.openCreateUserModal()"><i class="fas fa-user-plus"></i> Create New User</button>
                    </div>
                    <div class="table-responsive">
                        <table class="custom-table">
                            <thead>
                                <tr>
                                    <th>User ID</th>
                                    <th>Username</th>
                                    <th>Full Name</th>
                                    <th>Email</th>
                                    <th>Role</th>
                                    <th>Status</th>
                                    <th>Action</th>
                                </tr>
                            </thead>
                            <tbody>
                                ${userRows}
                            </tbody>
                        </table>
                    </div>
                </div>

                <!-- Audit Logs -->
                <div class="card-panel">
                    <div class="panel-header">
                        <div class="panel-title"><i class="fas fa-shield-alt"></i> Security Audit Trail</div>
                    </div>
                    <div class="table-responsive">
                        <table class="custom-table">
                            <thead>
                                <tr>
                                    <th>Timestamp</th>
                                    <th>Action</th>
                                    <th>User</th>
                                    <th>Event Details</th>
                                    <th>IP Address</th>
                                </tr>
                            </thead>
                            <tbody>
                                ${logRows}
                            </tbody>
                        </table>
                    </div>
                </div>
            `;
        } catch (err) {
            container.innerHTML = `<div class="card-panel" style="color:var(--accent-rose);">Error: ${err.message}</div>`;
        }
    }
};

window.Views = Views;
