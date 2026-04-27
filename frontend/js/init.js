// 系统初始化页面逻辑

// 检查是否已初始化
(async function() {
    const res = await get('/api/init/status');
    if (res && res.ok && res.data.initialized) {
        document.querySelector('.init-container').innerHTML =
            '<h2>系统已初始化</h2>' +
            '<p class="subtitle">停车场: ' + res.data.parking_name + '</p>' +
            '<a href="/index.html" class="btn btn-primary btn-block btn-lg" style="margin-top:24px">前往登录</a>';
    }
})();

async function handleInit() {
    const btn = document.getElementById('init-btn');
    const config = {
        host: document.getElementById('host').value.trim(),
        port: parseInt(document.getElementById('port').value),
        database: document.getElementById('database').value.trim(),
        user: document.getElementById('user').value.trim(),
        password: document.getElementById('password').value,
        parking_name: document.getElementById('parking_name').value.trim(),
        fee: parseFloat(document.getElementById('fee').value),
        capacity: parseInt(document.getElementById('capacity').value),
        server_port: parseInt(document.getElementById('server_port').value)
    };

    if (!config.password) {
        showError('alert-box', '请输入数据库密码');
        return;
    }

    btn.disabled = true;
    btn.textContent = '初始化中...';

    const res = await post('/api/init/database', config);

    btn.disabled = false;
    btn.textContent = '初始化系统';

    if (res && res.ok) {
        showSuccess('alert-box', '初始化成功！即将跳转到登录页面...');
        setTimeout(() => window.location.href = '/index.html', 2000);
    } else {
        showError('alert-box', res?.data?.error || '初始化失败，请检查配置');
    }
}
