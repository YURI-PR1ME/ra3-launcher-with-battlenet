# RA3 BattleNet Proxy

在 Linux/Wine/CrossOver 下让红色警戒3 连接 [RA3 Battle.net](https://ra3battle.net/) 战网。

## 原理

RA3 启动时自动加载 `Data/winmm.dll`（DLL 搜索顺序劫持），该 DLL：

1. 转发 179 个 winmm 函数到系统 `C:\Windows\System32\winmm.dll`（游戏正常音效/计时）
2. 等游戏窗口出现后，加载 RA3BN 的 `NativeDll.dll` 并调用其入口点
3. NativeDll 接管所有网络层（DNS 重定向、FESL、大厅等）

不再需要 .NET 启动器（Wine 不支持），也不需要 EasyHook 注入。

## 文件结构

```
ra3bnet-proxy/
├── README.md
├── winmm_dll/                    # DLL 源码
│   ├── Makefile                  # MinGW 交叉编译
│   ├── winmm_dll.cpp             # 主源码 (~180 行)
│   ├── forwarders.c              # 179 个 winmm 转发桩 (自动生成)
│   ├── winmm_mgw.def             # winmm 导出列表
│   └── gen_forwarders.sh         # 重新生成 forwarders.c 的脚本
├── launcher/                     # RA3Bar 启动器修改
│   ├── resource.h                # 新增资源/控件 ID
│   ├── resource.rc               # 嵌入 winmm.dll 为二进制资源
│   ├── UserInterface.hpp         # LaunchOptions 新增 proxy 字段
│   ├── UserInterface.cpp         # UI: 战网代理复选框 + 路径选择
│   ├── main.cpp                  # 启动时解压 DLL + 写配置
│   └── launcher.patch            # 基于原始 RA3Bar 的 diff
└── prebuilt/
    ├── winmm.dll                 # 预编译 DLL (可选)
    └── RA3.exe                   # 预编译启动器 (可选)
```

## 编译 winmm.dll

```bash
# 需要 32 位 MinGW 交叉编译器
sudo apt-get install g++-mingw-w64-i686

cd winmm_dll
make
# 产出 winmm.dll (~360KB，仅依赖 KERNEL32/USER32/msvcrt/VERSION)
```

## 编译启动器

将 launcher/ 中的文件覆盖到 RA3Bar-RA3Launcher 源码目录，然后把编译好的 `winmm.dll` 复制为 `winmm_proxy.dll`，按原 RA3Bar 构建流程编译。

```bash
cp winmm_dll/winmm.dll launcher/winmm_proxy.dll
cd /path/to/RA3Bar-RA3Launcher
# 按原有方式编译: windres + g++
```

## DLL 加载机制

**Wine/CrossOver 默认使用自带的 `winmm.dll` (builtin)，不会自动加载我们放在 `Data/` 里的版本。**

启动器通过**直接写 Wine 注册表**解决：
- 勾选战网代理：写 `HKCU\Software\Wine\DllOverrides\winmm = native,builtin`
- 取消勾选：删除该键值，恢复默认

与手动运行 `winecfg` 效果完全相同，无需任何额外配置。

> 注：`SetEnvironmentVariable("WINEDLLOVERRIDES")` 在 Wine 中无效，因为 Wine server 在进程启动时读取环境变量，运行时设置的不会被 wine server 识别。

## 使用

1. 启动 `RA3.exe`（RA3Bar 启动器）
2. 勾选 "战网代理" 复选框
3. 点击 "浏览..." 选择你的 `NativeDll.dll` 路径
4. 点击 Play — 启动器自动：
   - 设置 `WINEDLLOVERRIDES=winmm=n,b` 环境变量
   - 解压 `winmm.dll` 到 `Data/`
   - 写入 `ra3bn.ini`
5. 游戏启动后自动加载战网

取消勾选再点 Play 会删除 `Data/winmm.dll`，恢复纯净状态。

## ra3bn.ini 格式

```ini
[ra3bn]
path=Z:\path\to\RA3BattleNet\contents\NativeDll.dll
```

放在游戏根目录（`RA3.exe` 同级）或 `Data/` 目录。

## 技术细节

- **winmm 转发**: 把系统 winmm.dll 复制到 `%TEMP%\rwmm_proxy.dll` 改名加载，避免模块名冲突（我们的 DLL 也叫 winmm.dll）
- **NativeDll 加载**: 在工作线程中等待游戏窗口出现后加载，避免 DllMain 中加载导致的 loader lock 死锁
- **启动器集成**: `winmm.dll` 作为 RCDATA 资源嵌入 `RA3.exe`，运行时解压

## License

MIT
