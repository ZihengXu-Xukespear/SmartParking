# SmartParking

智慧停车管理系统 — 基于 C++17 和 Crow 框架的高性能 Web 停车管理应用。

## 功能特性

- **车辆出入管理** — 车辆入库、出库登记，自动计算停车费用
- **车位实时监控** — 仪表盘展示总车位、已占用、已预约、剩余车位的实时状态（ECharts 饼图）
- **预约系统** — 在线预约车位，超时 30 分钟自动取消（MySQL 定时事件）
- **多级计费规则** — 支持标准计费、阶梯计费、会员计费、特殊车辆（免费），可自定义费率与封顶
- **月卡管理** — 月卡车辆自动识别，免费通行
- **用户管理** — 用户注册/登录（SHA-256 密码哈希 + Token 认证），管理员可增删改用户
- **车牌识别预留** — 预留 OpenCV/EasyPR 接口，集成后可实现自动车牌识别
- **初始化向导** — 首次运行通过 Web 页面完成数据库配置与表结构创建
- **响应式前端** — 纯 HTML/CSS/JS + ECharts，无需 Node.js 构建工具

## 技术栈

| 层面 | 技术 |
|------|------|
| 语言 | C++17 |
| Web 框架 | [Crow](https://github.com/CrowCpp/Crow) (header-only) |
| 数据库 | MySQL 8.0 (C API + 自实现连接池) |
| 异步 IO | Asio 1.28 |
| 前端 | Vanilla HTML5 / CSS3 / JavaScript + ECharts 5 |
| 构建 | CMake 3.16+ + Conan 2 |
| 加密 | 自实现 SHA-256 |

## 项目结构

```
SmartParking/
├── CMakeLists.txt              # CMake 构建配置
├── CMakePresets.json           # CMake 预设（Visual Studio 2022）
├── conanfile.txt               # Conan 依赖声明
├── config/
│   └── db_config.example.json  # 数据库配置示例
├── sql/
│   └── init.sql                # 数据库初始化 SQL（手动执行用）
├── src/
│   ├── main.cpp                # 入口：启动服务器、注册路由、静态文件服务
│   ├── config.h                # 运行时配置（单例，JSON 持久化）
│   ├── sha256.h                # SHA-256 自实现
│   ├── permissions.h           # 权限节点与角色-权限映射
│   ├── controller/             # 控制器层（路由注册）
│   ├── service/                # 服务层（业务逻辑）
│   ├── model/                  # 数据模型
│   └── database/               # 数据库层（连接池、自动建表）
├── frontend/                   # 前端页面
│   ├── index.html              #   登录页
│   ├── register.html           #   注册页
│   ├── init.html               #   初始化向导
│   ├── dashboard.html          #   主面板（车位状态 + 出入库 + 图表）
│   ├── vehicles.html           #   车辆记录查询
│   ├── reservation.html        #   预约管理
│   ├── admin.html              #   管理页面（用户/计费/月卡/公告）
│   ├── profile.html            #   个人信息
│   ├── css/style.css           #   统一样式
│   └── js/                     #   前端逻辑
└── third_party/                # 第三方库（header-only）
    └── crow.h, crow/           #   Crow Web 框架
```

## 环境要求

- **操作系统**: Windows 10/11 或 Linux
- **编译器**: MSVC 2022 (Windows) 或 GCC 9+ / Clang 10+ (Linux)
- **CMake**: >= 3.16
- **Conan**: >= 2.0
- **MySQL**: >= 8.0（需安装 C 开发库 / Connector/C）
- **Git**: 用于克隆仓库

## 快速开始（Windows + MSVC 2022）

### 1. 安装 MySQL 8.0

**下载安装 MySQL Server 8.0**（安装时勾选 **"Development Libraries"** 或装完后单独安装 **Connector/C 6.1**）：

- 方式一：安装 [MySQL Installer](https://dev.mysql.com/downloads/windows/installer/8.0/)，选择 **"Developer Default"** 或自定义并勾选 **"MySQL Connector/C"**
- 方式二：仅安装 [MySQL Connector/C](https://dev.mysql.com/downloads/connector/c/)（如果已有 MySQL Server）

安装后确认以下目录存在（版本号可能略有不同）：

- 头文件: `C:\Program Files\MySQL\MySQL Server 8.0\include`
- 库文件: `C:\Program Files\MySQL\MySQL Server 8.0\lib`
- DLL 文件: `C:\Program Files\MySQL\MySQL Server 8.0\lib\libmysql.dll`

> 如果路径不同，构建前需修改 `CMakeLists.txt` 中的 `MYSQL_INCLUDE_DIR` 和 `MYSQL_LIB_DIR`。

### 2. 安装 Conan 2

```bash
pip install conan
conan --version  # 确认 >= 2.0
```

如果 `pip` 不可用，先安装 Python 3.8+，参见 [python.org](https://www.python.org/downloads/)。

### 3. 克隆项目

```bash
git clone https://github.com/your-org/SmartParking.git
cd SmartParking
```

### 4. 安装 Conan 依赖

```bash
conan install . --output-folder=build --build=missing
```

这将下载 Asio 1.28 并生成 CMake 预设文件到 `build/` 目录。

### 5. 配置 CMake

```bash
cmake --preset conan-default
```

如果 MySQL 路径与 CMakeLists.txt 默认值不同，可以先设置环境变量：

```bash
set MYSQL_INCLUDE_DIR=C:/Program Files/MySQL/MySQL Server 8.0/include
set MYSQL_LIB_DIR=C:/Program Files/MySQL/MySQL Server 8.0/lib
cmake --preset conan-default
```

### 6. 构建

```bash
cmake --build --preset conan-release
```

构建完成后，可执行文件位于 `build/Release/smart_parking.exe`。构建系统会自动将 `frontend/` 和 `config/` 目录复制到可执行文件所在目录。

### 7. 运行

```bash
# 确保在项目根目录下运行（这样程序能找到 frontend/ 和 config/ 目录）
cd build/Release
smart_parking.exe
```

启动后终端会显示：

```
==========================================
    Smart Parking 停车管理系统 v1.0
==========================================

[WARN] 数据库连接失败，请检查配置

[OK] 服务启动于 http://localhost:8080
[OK] 前端页面: http://localhost:8080/index.html
[OK] 初始化向导: http://localhost:8080/init.html
```

首次运行由于没有配置文件，会先进入初始化向导。

> **注意**: 程序启动时会从 `config/db_config.json` 加载配置。如果文件不存在或数据库连接失败，会显示警告但仍会启动服务器。未完成初始化前请不要关闭终端窗口。

## 初始化（第一次使用）

1. 浏览器打开 `http://localhost:8080/init.html`
2. 填写 MySQL 连接信息：
   - **主机**: `localhost`
   - **端口**: `3306`
   - **数据库名**: `smart_parking`
   - **用户名**: `root`
   - **密码**: 你的 MySQL root 密码
   - **停车场名称**: 如 `智慧停车场`
   - **每小时费率**: 如 `5.00`
   - **总车位数**: 如 `100`
   - **服务端口**: `8080`
3. 点击 **"初始化数据库"**
4. 等待页面显示"初始化成功"，然后点击"进入系统"
5. 使用默认管理员账号登录：**用户名** `root`，**密码** `root123456`（如果初始化时创建了该账号）

## 快速开始（Linux）

### 1. 安装依赖

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y mysql-server mysql-client libmysqlclient-dev cmake g++ python3-pip
pip install conan

# 启动 MySQL
sudo systemctl start mysql
sudo systemctl enable mysql
```

### 2. 克隆、构建、运行

```bash
git clone https://github.com/your-org/SmartParking.git
cd SmartParking

conan install . --output-folder=build --build=missing
cmake --preset conan-default
cmake --build --preset conan-release

cd build/Release
./smart_parking
```

## 登录与角色

初始化完成后使用以下账号登录：

| 用户名 | 密码 | 角色 | 说明 |
|--------|------|------|------|
| `root` | `root123456` | **超级管理员** | 所有权限，包括系统初始化 |
| ... | ... | **管理员** | 用户管理、计费规则、月卡、黑名单、公告编辑 |
| ... | ... | **操作员** | 车辆出入库、查询、车牌识别 |
| ... | ... | **普通用户** | 查看车位、预约、个人余额、购买套餐 |

管理员可以在 **管理页面 → 用户管理** 中添加不同角色的用户。

## 构建选项

### Debug 构建

```bash
cmake --build --preset conan-debug
```

### 自定义 MySQL 路径

如果 MySQL 安装在非默认位置，编辑 `CMakeLists.txt` 或通过 `-D` 参数指定：

```bash
cmake --preset conan-default \
  -DMYSQL_INCLUDE_DIR="D:/MySQL/include" \
  -DMYSQL_LIB_DIR="D:/MySQL/lib"
```

## 常见问题

### build/CMakePresets.json 不存在

Conan 安装步骤被跳过或失败。运行：

```bash
conan install . --output-folder=build --build=missing
```

这会生成 `build/CMakePresets.json`。

### 找不到 mysqlclient.lib / libmysql.dll

确认 MySQL Connector/C 已安装，且路径匹配 `CMakeLists.txt` 中的默认值。可以手动指定路径：

```bash
cmake --preset conan-default \
  -DMYSQL_INCLUDE_DIR="C:/Program Files/MySQL/MySQL Connector C 6.1/include" \
  -DMYSQL_LIB_DIR="C:/Program Files/MySQL/MySQL Connector C 6.1/lib/vs14"
```

### 运行时提示 "数据库连接失败"

- 检查 MySQL 服务是否在运行
- 检查用户名密码是否正确
- 检查 `config/db_config.json` 是否存在且配置正确
- 如果尚未初始化，打开 `http://localhost:8080/init.html` 完成初始化

### 前端页面无法加载（404）

确保工作目录正确，程序需要能访问到 `frontend/` 和 `config/` 目录。从 `build/Release/` 目录运行可执行文件（构建系统已自动将这两个目录复制过去）。

### 端口被占用

修改 `config/db_config.json` 中的 `server_port` 字段，或重新通过初始化向导设置。修改后重启程序。

## 配置

### 手动配置

配置文件 `config/db_config.json` 会在初始化时自动生成，也可手动创建：

```json
{
    "host": "localhost",
    "port": 3306,
    "database": "smart_parking",
    "user": "root",
    "password": "your_password",
    "parking_name": "智慧停车场",
    "fee": 5.00,
    "capacity": 100,
    "server_port": 8080
}
```

### 公告

在 **管理页面 → 公告编辑** 中修改公告内容。公告内容存储在 `config/db_config.json` 的 `notice` 字段中，修改后会自动保存到配置文件。

## API 概览

### 认证 `/api/auth`
| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/auth/login` | 用户登录，返回 Token |
| POST | `/api/auth/register` | 用户注册 |
| POST | `/api/auth/logout` | 登出（Token 失效） |
| GET  | `/api/auth/check` | 校验 Token 有效性 |

### 车辆 `/api/vehicle`
| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/vehicle/checkin` | 车辆入库 |
| POST | `/api/vehicle/checkout` | 车辆出库（含费用计算） |
| GET  | `/api/vehicle/query` | 查询停车记录 |
| DELETE | `/api/vehicle/<id>` | 删除记录 |

### 停车场 `/api/parking`
| 方法 | 路径 | 说明 |
|------|------|------|
| GET  | `/api/parking/status` | 停车场实时状态 |
| GET  | `/api/parking/stats` | 饼图统计数据 |
| PUT  | `/api/parking/settings` | 更新费率/容量 |
| GET/PUT | `/api/parking/billing-rules` | 计费规则管理 |
| GET/POST | `/api/parking/monthly-passes` | 月卡管理 |

### 车牌识别 `/api/plate`
| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/plate/recognize` | 车牌识别（预留） |
| POST | `/api/plate/validate` | 车牌格式校验 |

## 数据库表

| 表名 | 说明 |
|------|------|
| `USER` | 用户（管理员/普通用户） |
| `PARKING_LOT` | 停车场信息与实时计数 |
| `CAR_RECORD` | 停车记录 |
| `RESERVATION` | 预约记录（含触发器自动更新计数） |
| `BILLING_RULE` | 计费规则（4 种预设规则） |
| `MONTHLY_PASS` | 月卡通行证 |

## 待扩展功能

- [ ] 集成 OpenCV + EasyPR 实现真实车牌识别
- [ ] 添加中间件层实现统一的 Token 鉴权拦截
- [ ] 支付接口对接（微信/支付宝）
- [ ] Docker 容器化部署
- [ ] 单元测试覆盖
