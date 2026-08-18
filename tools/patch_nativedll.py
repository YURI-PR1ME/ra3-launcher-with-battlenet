#!/usr/bin/env python3
"""
patch_nativedll.py — 自动修补 RA3BattleNet 的 NativeDll.dll

把二进制中的本地代理地址 127.0.3.8 替换为 127.0.0.1（等长替换，
不破坏 PE 结构）。用于 macOS CrossOver：Wine 无法绑定 127.0.3.8，
会静默回退到 127.0.0.1，导致游戏访问的 URL 与实际监听地址不一致。

用法:
    python3 patch_nativedll.py [路径]

    不传路径时默认处理当前目录下的 NativeDll.dll。

行为:
    - 备份原文件为 <文件名>.bak（已存在则跳过备份）
    - 原地替换所有 ASCII "127.0.3.8" -> "127.0.0.1"
    - 已修补过的文件再次运行会提示无需处理（幂等）

支持 macOS / Linux（需要 python3，macOS 自带或随 Xcode 工具安装）。
"""

import hashlib
import os
import sys

OLD = b"127.0.3.8"
NEW = b"127.0.0.1"


def md5(data: bytes) -> str:
    return hashlib.md5(data).hexdigest()


def main() -> int:
    path = sys.argv[1] if len(sys.argv) > 1 else "NativeDll.dll"
    path = os.path.abspath(path)

    if not os.path.isfile(path):
        print(f"[错误] 找不到文件: {path}", file=sys.stderr)
        return 1

    with open(path, "rb") as f:
        data = f.read()

    count = data.count(OLD)

    if count == 0:
        if data.count(NEW) > 0:
            print(f"[提示] {path}")
            print("       已包含 127.0.0.1，可能已经修补过，无需处理。")
        else:
            print(f"[提示] {path}")
            print("       未找到 127.0.3.8，也未见 127.0.0.1。")
            print("       这个版本的 NativeDll 可能已经改用其他地址，")
            print("       请检查游戏日志确认实际地址后再手动处理。")
        return 0

    # 备份
    backup = path + ".bak"
    if os.path.exists(backup):
        print(f"[提示] 备份已存在，跳过: {backup}")
    else:
        with open(backup, "wb") as f:
            f.write(data)
        print(f"[备份] {path} -> {backup}")

    # 修补
    patched = data.replace(OLD, NEW)
    with open(path, "wb") as f:
        f.write(patched)

    # 验证
    assert OLD not in patched, "修补后仍残留 127.0.3.8！"
    print(f"[修补] {path}")
    print(f"       替换 {count} 处 127.0.3.8 -> 127.0.0.1")
    print(f"       原文件 md5: {md5(data)}")
    print(f"       新文件 md5: {md5(patched)}")
    print(f"       文件大小:   {len(patched)} 字节 (不变)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
