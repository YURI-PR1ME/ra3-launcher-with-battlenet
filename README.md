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
├── winmm_dll/                    # 注入器 + 启动器源码（一键构建）
│   ├── Makefile                  # 构建 winmm.dll 和 launcher/RA3.exe
│   ├── winmm_dll.cpp             # DLL 主源码
│   ├── forwarders.c              # 179 个 winmm 转发桩（自动生成）
│   ├── winmm_mgw.def             # winmm 导出列表
│   ├── gen_forwarders.sh         # 重新生成 forwarders.c 的脚本
│   └── launcher/                 # RA3.exe 启动器源码（RA3Bar fork，MIT © 2018 Lanyi）
│       ├── Makefile              # 启动器构建（上游 CI 同款命令）
│       ├── main.cpp              # 启动逻辑：解压 DLL + 写 ra3bn.ini + 启动游戏
│       ├── UserInterface.cpp     # 主界面 + 战网代理复选框（Wine 注册表写入）
│       ├── resource.rc           # 将 winmm.dll 作为 PROXY_DLL 资源内嵌
│       ├── launcher.patch        # 相对上游 RA3Bar-RA3Launcher 的修改 diff
│       └── *.bmp / *.csf / *.ico # 界面资源
└── prebuilt/
    ├── winmm.dll                 # 预编译 DLL
    └── RA3.exe                   # 预编译启动器（已内嵌 winmm.dll）
```

## 编译（一键）

需要 32 位 MinGW 交叉编译器：

```bash
sudo apt-get install g++-mingw-w64-i686

cd winmm_dll
make
# 产出：
#   winmm.dll           (仅依赖 KERNEL32/USER32/msvcrt)
#   launcher/RA3.exe    (已把 winmm.dll 内嵌为资源，随 DLL 自动重新链接)
```

单独构建启动器：`cd winmm_dll/launcher && make`（需要目录下已有 `winmm_proxy.dll`，通常由上层 make 自动复制）。

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
   - 解压内嵌的 `winmm.dll` 到 `Data/`
   - 写入 `ra3bn.ini`
5. 游戏启动后自动加载战网

取消勾选再点 Play 会删除 `Data/winmm.dll`，恢复纯净状态。

## ra3bn.ini 格式

```ini
[ra3bn]
path=Z:\path\to\RA3BattleNet\contents\NativeDll.dll
```

放在游戏根目录（`RA3.exe` 同级）或 `Data/` 目录。

## FPU 精度模式（失步排查）

DLL 会在每个游戏线程创建时把 x87 浮点精度设为 Windows 默认的 **53-bit（PC=10）**，防止 Wine 环境下与 Windows 对手产生「失去同步」。可通过环境变量切换：

| 环境变量 | 取值 | 说明 |
|---|---|---|
| `RA3BN_FPU` | `53`（默认） | 每线程强制 53-bit（Windows 默认 0x027F；Wine 下本就是 53-bit，相当于兜底 no-op） |
| | `off` | 完全不碰 FPU/SSE，并关闭线程回调（与验证不失步的 dsound 代理行为一致） |
| | `80` | 旧行为：强制 80-bit 扩展精度（PC=11）。**不是** Windows 默认，仅用于 A/B 对照实验 |
| `RA3BN_FPU_LOG` | `1` | 诊断：把每个线程的 x87 控制字/MXCSR 读写到 `%TEMP%\ra3bn_fpu.log`（最多 32 行）。预期 Wine 下 `preCW=0x027F`（53-bit） |

用法示例（环境变量会随 CreateProcess 继承到游戏进程）：

```bash
RA3BN_FPU=off wine RA3.exe                 # 走启动器
RA3BN_FPU=off wine ra3_1.12.game           # 直接启动游戏（绕过启动器）
RA3BN_FPU_LOG=1 wine RA3.exe               # 记录 FPU 诊断日志
```

编译期可改默认值：`make CXXFLAGS+="-DRA3BN_FPU_DEFAULT=0"`（0=off、1=53、2=80）。

> 历史：旧代码曾把精度强制为 80-bit 并误认为那是「Windows 默认」——实际 Windows 默认是 53-bit（0x027F），80 与 53-bit 的浮点差异在锁步模拟中累积后正是「运行一段时间后失去同步」的典型成因。

## 技术细节

- **winmm 转发**: 把系统 winmm.dll 复制到 `%TEMP%\rwmm_proxy.dll` 改名加载，避免模块名冲突（我们的 DLL 也叫 winmm.dll）
- **NativeDll 加载**: 在工作线程中等待游戏窗口出现后加载，避免 DllMain 中加载导致的 loader lock 死锁
- **启动器集成**: `winmm.dll` 作为 RCDATA 资源嵌入 `RA3.exe`，运行时解压；启动器源码基于 [RA3Bar-RA3Launcher](https://github.com/CnCNet/RA3Bar-RA3Launcher)（MIT），修改点见 `winmm_dll/launcher/launcher.patch`

## License

MIT
