# ====== 分层目录支持：App / Service / Device / Bsp ======
#
# 依赖方向（严格单向，禁止反向）：
#     App  ──►  Service  ──►  Device  ──►  Bsp  ──►  HAL/LL
#
#   App     : 业务层。任务函数、控制策略、界面排版、共享数据。
#   Service : 服务层。与业务无关的可复用能力（成帧、蓝牙协议与 ACK）。
#   Device  : 设备驱动层。器件时序与命令序列（DHT11 / ST7789 / TB6612 ...）。
#   Bsp     : 板级支持层。引脚坐标、微秒延时、外设启动、板级专属初始化。
#
# RTOS 不单独成层：各层直接调 CMSIS-RTOS2（cmsis_os.h）。临界区用 FreeRTOS 原生
# taskENTER_CRITICAL —— 工程里所有中断优先级都 ≥ configLIBRARY_MAX_SYSCALL_
# INTERRUPT_PRIORITY(5)，BASEPRI 能挡住全部中断，与 PRIMASK 等效且自带嵌套计数。
#
# 唯一的反向依赖例外：App/Src/app_tasks.c 直接 include "bsp_tim.h" 调 Bsp_Tim_Start()。
# 原因：开 TIM1 只能由 Bsp 做，而它必须在调度器起来之后（ISR 里要 osSemaphoreRelease），
# 调用点被锁死在 Init 任务里（App）。只此一处，别在别的 App 文件再开口子。
#
# 目录约定：每层分 Inc/ 与 Src/，头文件和源文件分开存放：
#     Device/Inc/Led.h
#     Device/Src/Led.c
#
# 头文件名在工程内唯一，所以只需把各层 Inc/ 加进 include path，
# 源码里照旧写 #include "Led.h" 即可，不用写成 "Device/Inc/Led.h"。
#
# 本文件被 CMakeLists.txt 与 CMakeLists_template.txt 同时 include，
# 所以 CubeMX 重新生成 CMakeLists.txt 后分层规则依然生效。

set(LAYER_LIST App Service Device Bsp)

foreach(layer ${LAYER_LIST})
    # ---- 头文件路径 ----
    if(EXISTS ${CMAKE_SOURCE_DIR}/${layer}/Inc)
        target_include_directories(${PROJECT_NAME}.elf PRIVATE ${CMAKE_SOURCE_DIR}/${layer}/Inc)
    endif()

    # ---- 源文件 ----
    # CONFIGURE_DEPENDS：新增 .c 时自动重新运行 Cmake，避免"加了文件却编不到"
    if(EXISTS ${CMAKE_SOURCE_DIR}/${layer}/Src)
        file(GLOB _layer_src CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/${layer}/Src/*.c)
        if(_layer_src)
            target_sources(${PROJECT_NAME}.elf PRIVATE ${_layer_src})
        endif()

        # .h 也加进 target，IDE 项目树里能看到
        file(GLOB _layer_hdr CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/${layer}/Inc/*.h)
        if(_layer_hdr)
            target_sources(${PROJECT_NAME}.elf PRIVATE ${_layer_hdr})
        endif()
    endif()

    unset(_layer_src)
    unset(_layer_hdr)
endforeach()
