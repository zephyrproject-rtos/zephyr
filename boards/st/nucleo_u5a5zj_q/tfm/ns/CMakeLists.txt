#-------------------------------------------------------------------------------
# Copyright (c) 2023-2024, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# This file is exported to NS side during CMake installation phase and renamed
# to CMakeLists.txt. It instructs how to build a platform on non-secture side.
# The structure and sources list are fully platform specific.

add_library(platform_ns)

target_sources(platform_ns
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/Device/Source/startup_stm32u5xx_ns.c
    PRIVATE
        hal/Src/stm32u5xx_hal.c
        Device/Source/system_stm32u5xx.c
        CMSIS_Driver/low_level_com.c
        hal/Src/stm32u5xx_hal_dma.c
        hal/Src/stm32u5xx_hal_dma_ex.c
        hal/Src/stm32u5xx_hal_pwr.c
        hal/Src/stm32u5xx_hal_pwr_ex.c
        hal/Src/stm32u5xx_hal_rcc.c
        hal/Src/stm32u5xx_hal_gpio.c
        hal/Src/stm32u5xx_hal_uart.c
        hal/Src/stm32u5xx_hal_uart_ex.c
        hal/Src/stm32u5xx_hal_cortex.c
        hal/Src/stm32u5xx_hal_rcc_ex.c
)

target_include_directories(platform_ns
    PUBLIC
        include
        ext/cmsis/Include
        ext/cmsis/Include/m-profile
        ext/common
        Device/Include
        hal/Inc
)

# Include region_defs.h and flash_layout.h
target_include_directories(platform_region_defs
    INTERFACE
        partition
)
