// 管理页面逻辑

const user = checkAuth();
if (user) {
    initSidebar();
    if (user.role !== 'admin') {
        document.querySelector('.main-content').innerHTML =
            '<div class="card" style="text-align:center;padding:60px"><h2>权限不足</h2><p style="color:#999;margin-top:12px">此页面仅管理员可访问</p><a href="/dashboard.html" class="btn btn-primary mt-3">返回主面板</a></div>';
    }
}

// Tab switching
function switchTab(tab) {
    document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
    document.querySelectorAll('[id^="tab-"]').forEach(t => t.classList.add('hidden'));

    event.target.classList.add('active');
    document.getElementById('tab-' + tab).classList.remove('hidden');

    if (tab === 'users') loadUsers();
    else if (tab === 'billing') loadBillingRules();
    else if (tab === 'passes') loadPasses();
}

// === Users ===
async function loadUsers() {
    const res = await get('/api/user/list');
    const tbody = document.getElementById('users-table');
    if (!res || !res.ok) {
        tbody.innerHTML = '<tr><td colspan="7" style="text-align:center;color:#999">加载失败</td></tr>';
        return;
    }
    const users = res.data.users || [];
    if (users.length === 0) {
        tbody.innerHTML = '<tr><td colspan="7" style="text-align:center;color:#999">暂无用户</td></tr>';
        return;
    }
    tbody.innerHTML = users.map(u => `
        <tr>
            <td>${u.id}</td>
            <td><strong>${u.username}</strong></td>
            <td>${u.telephone}</td>
            <td>${u.truename}</td>
            <td><span class="badge ${u.role === 'admin' ? 'badge-danger' : 'badge-primary'}">${u.role === 'admin' ? '管理员' : '用户'}</span></td>
            <td>${formatDateTime(u.created_at)}</td>
            <td>
                <button class="btn btn-default btn-sm" onclick="editUser(${u.id},'${u.username}','${u.telephone}','${u.truename}','${u.role}')">编辑</button>
                <button class="btn btn-danger btn-sm" onclick="deleteUser(${u.id})">删除</button>
            </td>
        </tr>
    `).join('');
}

function editUser(id, username, telephone, truename, role) {
    document.getElementById('user-modal-title').textContent = '编辑用户';
    document.getElementById('edit-user-id').value = id;
    document.getElementById('modal-username').value = username;
    document.getElementById('modal-password').value = '';
    document.getElementById('modal-password').placeholder = '留空则不修改密码';
    document.getElementById('modal-telephone').value = telephone;
    document.getElementById('modal-truename').value = truename;
    document.getElementById('modal-role').value = role;
    showModal('user-modal');
}

async function saveUser() {
    const editId = document.getElementById('edit-user-id').value;
    const username = document.getElementById('modal-username').value.trim();
    const password = document.getElementById('modal-password').value;
    const telephone = document.getElementById('modal-telephone').value.trim();
    const truename = document.getElementById('modal-truename').value.trim();
    const role = document.getElementById('modal-role').value;

    let res;
    if (editId) {
        res = await put('/api/user/update', { id: parseInt(editId), telephone, truename, role, password: password || undefined });
    } else {
        if (!password) { alert('请输入密码'); return; }
        res = await post('/api/user/add', { username, password, telephone, truename, role });
    }

    if (res && res.ok) {
        hideModal('user-modal');
        showSuccess('alert-box', editId ? '用户已更新' : '用户已添加');
        loadUsers();
        resetUserModal();
    } else {
        alert(res?.data?.error || '操作失败');
    }
}

function resetUserModal() {
    document.getElementById('edit-user-id').value = '';
    document.getElementById('user-modal-title').textContent = '添加用户';
    document.getElementById('modal-username').value = '';
    document.getElementById('modal-password').value = '';
    document.getElementById('modal-password').placeholder = '请输入密码';
    document.getElementById('modal-telephone').value = '';
    document.getElementById('modal-truename').value = '';
    document.getElementById('modal-role').value = 'user';
}

async function deleteUser(id) {
    if (!confirm('确定要删除此用户吗？')) return;
    const res = await del('/api/user/' + id);
    if (res && res.ok) {
        showSuccess('alert-box', '用户已删除');
        loadUsers();
    } else {
        showError('alert-box', res?.data?.error || '删除失败');
    }
}

// === Billing Rules ===
async function loadBillingRules() {
    const res = await get('/api/parking/billing-rules');
    const tbody = document.getElementById('billing-table');
    if (!res || !res.ok) {
        tbody.innerHTML = '<tr><td colspan="8" style="text-align:center;color:#999">加载失败</td></tr>';
        return;
    }
    const rules = res.data.rules || [];
    tbody.innerHTML = rules.map(r => `
        <tr>
            <td>${r.rule_name}</td>
            <td><span class="badge badge-primary">${r.rule_type}</span></td>
            <td>${r.free_minutes}</td>
            <td>${r.hourly_rate}</td>
            <td>${r.max_daily_fee || '-'}</td>
            <td>${r.description || '-'}</td>
            <td>${r.is_active ? '<span class="badge badge-success">启用</span>' : '<span class="badge badge-danger">禁用</span>'}</td>
            <td><button class="btn btn-default btn-sm" onclick="editBillingRule(${r.id})">编辑</button></td>
        </tr>
    `).join('');
}

function editBillingRule(id) {
    // Find rule data from table
    const rules = document.querySelectorAll('#billing-table tr');
    for (const row of rules) {
        const editBtn = row.querySelector(`button[onclick="editBillingRule(${id})"]`);
        if (editBtn) {
            const cells = row.querySelectorAll('td');
            document.getElementById('edit-billing-id').value = id;
            document.getElementById('billing-name').value = cells[0].textContent;
            document.getElementById('billing-free').value = cells[2].textContent;
            document.getElementById('billing-rate').value = cells[3].textContent;
            document.getElementById('billing-max').value = cells[4].textContent === '-' ? '' : cells[4].textContent;
            document.getElementById('billing-desc').value = cells[5].textContent === '-' ? '' : cells[5].textContent;
            document.getElementById('billing-active').checked = cells[6].textContent.includes('启用');
            showModal('billing-modal');
            break;
        }
    }
}

async function saveBillingRule() {
    const id = document.getElementById('edit-billing-id').value;
    const body = {
        rule_name: document.getElementById('billing-name').value.trim(),
        rule_type: 'standard', // Preserve original type
        free_minutes: parseInt(document.getElementById('billing-free').value),
        hourly_rate: parseFloat(document.getElementById('billing-rate').value),
        max_daily_fee: parseFloat(document.getElementById('billing-max').value) || 0,
        description: document.getElementById('billing-desc').value.trim(),
        is_active: document.getElementById('billing-active').checked
    };
    // Get original type from table
    const rules = document.querySelectorAll('#billing-table tr');
    for (const row of rules) {
        const editBtn = row.querySelector(`button[onclick="editBillingRule(${id})"]`);
        if (editBtn) {
            const badge = row.querySelector('.badge-primary');
            if (badge) body.rule_type = badge.textContent;
            break;
        }
    }

    const res = await put('/api/parking/billing-rules/' + id, body);
    if (res && res.ok) {
        hideModal('billing-modal');
        showSuccess('alert-box', '规则已更新');
        loadBillingRules();
    } else {
        alert(res?.data?.error || '更新失败');
    }
}

// === Monthly Passes ===
async function loadPasses() {
    const res = await get('/api/parking/monthly-passes');
    const tbody = document.getElementById('passes-table');
    if (!res || !res.ok) {
        tbody.innerHTML = '<tr><td colspan="8" style="text-align:center;color:#999">加载失败</td></tr>';
        return;
    }
    const passes = res.data.passes || [];
    if (passes.length === 0) {
        tbody.innerHTML = '<tr><td colspan="8" style="text-align:center;color:#999">暂无月卡</td></tr>';
        return;
    }
    tbody.innerHTML = passes.map(p => `
        <tr>
            <td>${p.id}</td>
            <td><strong>${p.license_plate}</strong></td>
            <td>${p.pass_type}</td>
            <td>${formatDate(p.start_date)}</td>
            <td>${formatDate(p.end_date)}</td>
            <td>${formatFee(p.fee)}</td>
            <td>${p.is_active ? '<span class="badge badge-success">有效</span>' : '<span class="badge badge-danger">过期</span>'}</td>
            <td><button class="btn btn-danger btn-sm" onclick="deletePass(${p.id})">删除</button></td>
        </tr>
    `).join('');
}

async function savePass() {
    const body = {
        license_plate: document.getElementById('pass-plate').value.trim(),
        pass_type: document.getElementById('pass-type').value,
        start_date: document.getElementById('pass-start').value,
        end_date: document.getElementById('pass-end').value,
        fee: parseFloat(document.getElementById('pass-fee').value)
    };
    if (!body.license_plate || !body.start_date || !body.end_date) {
        alert('请填写完整信息');
        return;
    }
    const res = await post('/api/parking/monthly-passes', body);
    if (res && res.ok) {
        hideModal('pass-modal');
        showSuccess('alert-box', '月卡已添加');
        loadPasses();
    } else {
        alert(res?.data?.error || '添加失败');
    }
}

async function deletePass(id) {
    if (!confirm('确定要删除此月卡吗？')) return;
    const res = await del('/api/parking/monthly-passes/' + id);
    if (res && res.ok) {
        showSuccess('alert-box', '月卡已删除');
        loadPasses();
    } else {
        showError('alert-box', '删除失败');
    }
}

// Init
loadUsers();
