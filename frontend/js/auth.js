// 登录页面逻辑
async function handleLogin() {
    const btn = document.getElementById('login-btn');
    const username = document.getElementById('username').value.trim();
    const password = document.getElementById('password').value;

    if (!username || !password) {
        showError('alert-box', '用户名和密码不能为空');
        return;
    }

    btn.disabled = true;
    btn.textContent = '登录中...';

    const res = await post('/api/auth/login', { username, password });

    btn.disabled = false;
    btn.textContent = '登 录';

    if (res && res.ok) {
        localStorage.setItem('token', res.data.token);
        localStorage.setItem('user', JSON.stringify(res.data.user));
        showSuccess('alert-box', '登录成功，正在跳转...');
        setTimeout(() => window.location.href = '/dashboard.html', 800);
    } else {
        showError('alert-box', res?.data?.error || '登录失败');
    }
}

// Enter key support
document.addEventListener('keydown', e => {
    if (e.key === 'Enter') handleLogin();
});
