# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_compile_definitions_ifdef(CONFIG_CPU_HAS_FPU ICP101XX_DRV_USE_FLOATS)

zephyr_library_sources(icp101xx_drv.c)
zephyr_library_include_directories(.)
