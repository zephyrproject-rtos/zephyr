# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if (CONFIG_CPU_HAS_FPU STREQUAL "y")
  zephyr_compile_definitions(ICP101XX_DRV_USE_FLOATS)
endif()

zephyr_library_sources(icp101xx_drv.c)
zephyr_library_include_directories(.)
