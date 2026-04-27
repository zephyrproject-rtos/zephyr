# Copyright (c) 2026 Silicon Signals Pvt. Ltd.
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_LV_NEMA_HAL_CUSTOM)
  zephyr_library_sources_ifdef(CONFIG_LV_USE_NEMA_GFX zephyr_nemagfx_hal.c)
endif() # CONFIG_LV_NEMA_HAL_CUSTOM

if(CONFIG_LV_USE_NEMA_LIB EQUAL 1)
  set(NEMA_LIB_DIR cortex_m33_revC/gcc)
  zephyr_include_directories(${LVGL_DIR}/libs/nema_gfx/include/cortex_m33)
elseif(CONFIG_LV_USE_NEMA_LIB EQUAL 2)
  set(NEMA_LIB_DIR cortex_m33_NemaPVG/gcc)
  zephyr_include_directories(${LVGL_DIR}/libs/nema_gfx/include/cortex_m33)
elseif(CONFIG_LV_USE_NEMA_LIB EQUAL 3)
  set(NEMA_LIB_DIR cortex_m55/gcc)
  zephyr_include_directories(${LVGL_DIR}/libs/nema_gfx/include/cortex_m55_m7)
elseif(CONFIG_LV_USE_NEMA_LIB EQUAL 4)
  set(NEMA_LIB_DIR cortex_m7/gcc)
  zephyr_include_directories(${LVGL_DIR}/libs/nema_gfx/include/cortex_m55_m7)
else()
  message(FATAL_ERROR "Unsupported NemaGFX lib")
endif()

zephyr_library_link_libraries(
  nemagfx-float-abi-hard
  -L${LVGL_DIR}/libs/nema_gfx/lib/core/${NEMA_LIB_DIR}
)
