// Dashboard 逻辑

const user = checkAuth();
if (user) initSidebar();

let pieChart = null;

// 初始化饼图
function initPieChart() {
    const dom = document.getElementById('pie-chart');
    if (!dom) return;
    pieChart = echarts.init(dom);
    pieChart.setOption({
        tooltip: { trigger: 'item', formatter: '{b}: {c} ({d}%)' },
        legend: { bottom: 0, itemWidth: 12, itemHeight: 12, textStyle: { fontSize: 12 } },
        series: [{
            type: 'pie',
            radius: ['40%', '65%'],
            center: ['50%', '45%'],
            avoidLabelOverlap: false,
            itemStyle: { borderRadius: 6, borderColor: '#fff', borderWidth: 2 },
            label: { show: true, formatter: '{b}\n{c}', fontSize: 12 },
            data: [
                { value: 0, name: '已占用', itemStyle: { color: '#ff4d4f' } },
                { value: 0, name: '已预约', itemStyle: { color: '#faad14' } },
                { value: 0, name: '剩余车位', itemStyle: { color: '#52c41a' } }
            ]
        }]
    });
}

// 更新停车状态
async function loadStatus() {
    const res = await get('/api/parking/status');
    if (!res || !res.ok) return;
    const d = res.data;

    document.getElementById('stat-total').textContent = d.P_total_count;
    document.getElementById('stat-occupied').textContent = d.P_current_count;
    document.getElementById('stat-reserved').textContent = d.P_reserve_count;
    document.getElementById('stat-available').textContent = d.P_available_count;
    document.getElementById('parking-name').value = d.P_name;
    document.getElementById('parking-fee').value = d.P_fee;
    document.getElementById('parking-capacity').value = d.P_total_count;

    if (pieChart) {
        pieChart.setOption({
            series: [{
                data: [
                    { value: d.P_current_count, name: '已占用', itemStyle: { color: '#ff4d4f' } },
                    { value: d.P_reserve_count, name: '已预约', itemStyle: { color: '#faad14' } },
                    { value: d.P_available_count, name: '剩余车位', itemStyle: { color: '#52c41a' } }
                ]
            }]
        });
    }
}

// 加载最近记录
async function loadRecentRecords() {
    const res = await get('/api/vehicle/query');
    const tbody = document.getElementById('recent-records');
    if (!res || !res.ok || !res.data.records || res.data.records.length === 0) {
        tbody.innerHTML = '<tr><td colspan="5" style="text-align:center;color:#999">暂无记录</td></tr>';
        return;
    }
    const records = res.data.records.slice(0, 10);
    tbody.innerHTML = records.map(r => `
        <tr>
            <td><strong>${r.license_plate}</strong></td>
            <td>${formatDateTime(r.check_in_time)}</td>
            <td>${formatDateTime(r.check_out_time)}</td>
            <td>${formatFee(r.fee)}</td>
            <td>${r.check_out_time ? '<span class="badge badge-success">已出库</span>' : '<span class="badge badge-primary">停放中</span>'}</td>
        </tr>
    `).join('');
}

// 车辆入库
async function handleCheckIn() {
    const plate = document.getElementById('plate-input').value.trim();
    const billingType = document.getElementById('billing-type').value;

    if (!plate) { showError('vehicle-alert', '请输入车牌号'); return; }

    const res = await post('/api/vehicle/checkin', { license_plate: plate, billing_type: billingType });
    if (res && res.ok) {
        showSuccess('vehicle-alert', `车辆 ${plate} 入库成功！`);
        document.getElementById('plate-input').value = '';
        loadStatus();
        loadRecentRecords();
    } else {
        showError('vehicle-alert', res?.data?.error || '入库失败');
    }
}

// 车辆出库
async function handleCheckOut() {
    const plate = document.getElementById('plate-input').value.trim();
    if (!plate) { showError('vehicle-alert', '请输入车牌号'); return; }

    const res = await post('/api/vehicle/checkout', { license_plate: plate });
    if (res && res.ok) {
        showSuccess('vehicle-alert', `车辆 ${plate} 出库成功！费用: ${formatFee(res.data.fee)}`);
        document.getElementById('plate-input').value = '';
        loadStatus();
        loadRecentRecords();
    } else {
        showError('vehicle-alert', res?.data?.error || '出库失败');
    }
}

// 车牌识别(预留)
async function handlePlateRecognize() {
    const res = await post('/api/plate/recognize', {});
    const container = document.getElementById('plate-result');
    if (res && res.ok) {
        container.innerHTML = `
            <div class="alert alert-warning">${res.data.message}</div>
        `;
    }
}

// 更新停车场设置
async function updateSettings() {
    const fee = parseFloat(document.getElementById('parking-fee').value);
    const capacity = parseInt(document.getElementById('parking-capacity').value);

    const res = await put('/api/parking/settings', { fee, capacity });
    if (res && res.ok) {
        showSuccess('vehicle-alert', '停车场设置已更新');
        loadStatus();
    } else {
        showError('vehicle-alert', res?.data?.error || '更新失败');
    }
}

// Enter key for plate input
document.getElementById('plate-input')?.addEventListener('keydown', e => {
    if (e.key === 'Enter') handleCheckIn();
});

// Init
initPieChart();
loadStatus();
loadRecentRecords();

// Auto refresh every 5 seconds
setInterval(() => { loadStatus(); }, 5000);
