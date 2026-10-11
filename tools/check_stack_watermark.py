#!/usr/bin/env python3
"""任务栈水位分析 —— 读取 tools/dump_task_stacks.cfg 生成的栈镜像并算出峰值占用。

用法：
    1) cd Graduation_F407
       openocd -f st_link.cfg -f tools/dump_task_stacks.cfg
    2) python tools/check_stack_watermark.py

判定：
    configCHECK_FOR_STACK_OVERFLOW = 2 时，FreeRTOS 创建任务会把整块栈填 0xA5，
    运行中被写过的部分不再是 0xA5。从栈底往上找第一个非 0xA5 即得到峰值水位。
    若 pxStack[0..3]（栈底 4 个字）已不是 0xA5A5A5A5 → 已溢出，必须加大 stack_size。
"""

import os
import struct

TEMP = os.environ.get("TEMP", r"C:\Users\KAZUMA\AppData\Local\Temp")
A5 = 0xA5A5A5A5

# (任务名, 栈大小字节数) —— 与 tools/dump_task_stacks.cfg 里的 sizes 保持一致
TASKS = [
    ("Fan", 512),
    ("Warm", 512),
    ("Humidifier", 512),
    ("Bluetooth", 2048),
    ("DHT11", 512),
    ("LCD", 1024),
    ("LED", 512),
    ("Init", 512),
]


def main() -> None:
    print(f"{'任务':<12}{'栈大小':>8}{'峰值用':>9}{'占用率':>9}{'剩余':>8}   状态")
    print("-" * 64)
    overflow = []
    for name, size in TASKS:
        path = os.path.join(TEMP, f"st_{name}.bin")
        if not os.path.exists(path):
            print(f"{name:<12}  (缺少 {path})")
            continue

        data = open(path, "rb").read()
        words = struct.unpack("<%dI" % (len(data) // 4), data)

        idx = next((i for i, v in enumerate(words) if v != A5), None)
        if idx is None:
            print(f"{name:<12}{len(data):>8}{0:>9}{0.0:>8.1f}%{len(data):>8}   全 0xA5（未使用）")
            continue

        used = len(data) - idx * 4
        remain = idx * 4
        if idx < 4:
            state = "!! 栈底标记被覆盖 → 已溢出"
            overflow.append(name)
        elif remain < 64:
            state = "! 余量偏小"
        else:
            state = "正常"
        print(f"{name:<12}{len(data):>8}{used:>9}{100 * used / len(data):>8.1f}%{remain:>8}   {state}")

    print()
    if overflow:
        print("溢出的任务：" + "、".join(overflow))
        print("→ 在 Core/Src/freertos.c 里加大对应 osThreadAttr_t 的 .stack_size（同步改 .ioc）")
    else:
        print("所有任务栈都有余量。建议保持剩余 ≥ 25%，sprintf/sscanf 这类 newlib 函数很吃栈。")


if __name__ == "__main__":
    main()
