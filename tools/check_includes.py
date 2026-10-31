# -*- coding: utf-8 -*-
"""头文件引用体检：查重、查缺守卫、查守卫重名、查冗余直连。

用法（在工程根目录，Graduation_F407_layered/ 下）：
    python tools/check_includes.py

检查项：
    ① 同名头文件      —— 递归 include path 下，同名头会互相遮蔽（真 bug）
    ② 缺 include 守卫 —— 会被重复解析，可能重定义
    ③ 守卫宏重名      —— 第二个头会被静默跳过（最隐蔽的一类 bug）
    ④ 同文件重复 include —— 无害但多余
    ⑤ 冗余直连        —— 直连的头已由另一个直连带入（提示，不一定改）

退出码：0 = 无 ①②③ 类致命问题；1 = 有。
"""
import os
import re
import sys
from collections import defaultdict

ROOTS = ["App", "Service", "Device", "Bsp", "Core"]
BUILD_DIRS = {"Cmake-build-debug-st-mingw-1331", "build"}

INC_RE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)
GUARD_RE = re.compile(r'^\s*#\s*ifndef\s+(\w+)', re.M)


def collect():
    files = []
    for root in ROOTS:
        for dp, dns, fns in os.walk(root):
            dns[:] = [d for d in dns if d not in BUILD_DIRS]
            for f in fns:
                if f.endswith((".c", ".h")):
                    files.append(os.path.join(dp, f).replace("\\", "/"))
    return files


def includes(path):
    return INC_RE.findall(open(path, encoding="utf-8", errors="ignore").read())


def main():
    files = collect()
    by_name = defaultdict(list)
    for f in files:
        by_name[os.path.basename(f)].append(f)

    fatal = 0

    # ① 同名头文件
    print("=" * 68)
    print("① 同名头文件（递归 include path 下会互相遮蔽）")
    dup = {k: v for k, v in by_name.items() if len(v) > 1}
    if dup:
        fatal = 1
        for k, v in dup.items():
            print(f"   [重名] {k}")
            for p in v:
                print(f"          {p}")
    else:
        print("   OK  无")

    # ② 缺 include 守卫
    print()
    print("=" * 68)
    print("② 缺 include 守卫的头文件")
    no_guard = []
    for f in files:
        if f.endswith(".h") and not GUARD_RE.search(
                open(f, encoding="utf-8", errors="ignore").read()):
            no_guard.append(f)
    if no_guard:
        fatal = 1
        for f in no_guard:
            print(f"   [缺守卫] {f}")
    else:
        print("   OK  全部有 #ifndef 守卫")

    # ③ 守卫宏重名
    print()
    print("=" * 68)
    print("③ 守卫宏重名（后果：第二个头被静默跳过）")
    guards = defaultdict(list)
    for f in files:
        if not f.endswith(".h"):
            continue
        m = GUARD_RE.search(open(f, encoding="utf-8", errors="ignore").read())
        if m:
            guards[m.group(1)].append(f)
    clash = {k: v for k, v in guards.items() if len(v) > 1}
    if clash:
        fatal = 1
        for k, v in clash.items():
            print(f"   [重名] {k}")
            for p in v:
                print(f"          {p}")
    else:
        print(f"   OK  {len(guards)} 个守卫宏互不重复")

    # ④ 同文件重复 include
    print()
    print("=" * 68)
    print("④ 同一文件内重复 include 同一个头")
    found = False
    for f in files:
        seen = defaultdict(int)
        for i in includes(f):
            seen[i] += 1
        d = {k: v for k, v in seen.items() if v > 1}
        if d:
            found = True
            print(f"   {f}: {d}")
    if not found:
        print("   OK  无")

    # ⑤ 冗余直连（提示，不判死）
    print()
    print("=" * 68)
    print("⑤ 冗余直连（提示项：直连的头已由另一个直连带入；CubeMX 生成文件可忽略）")

    def resolve(name):
        p = by_name.get(os.path.basename(name))
        return p[0] if p else None

    def closure(path, seen=None):
        if seen is None:
            seen = set()
        for inc in includes(path):
            p = resolve(inc)
            if p and p not in seen:
                seen.add(p)
                closure(p, seen)
        return seen

    hit = False
    for f in files:
        if not f.endswith(".c"):
            continue
        direct = list(dict.fromkeys(d for d in (resolve(i) for i in includes(f)) if d))
        red = []
        for d in direct:
            for other in direct:
                if other != d and d in closure(other):
                    red.append((os.path.basename(d), os.path.basename(other)))
                    break
        if red:
            hit = True
            print(f"   {f}")
            for a, via in red:
                print(f"      直连 {a:<18} 已由 {via} 带入")
    if not hit:
        print("   OK  无")

    print()
    print("=" * 68)
    print("结论：" + ("发现致命问题（①/②/③），见上" if fatal else "无致命问题"))
    return fatal


if __name__ == "__main__":
    sys.exit(main())
