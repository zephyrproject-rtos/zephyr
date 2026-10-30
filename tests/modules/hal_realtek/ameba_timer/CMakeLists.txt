# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ameba_timer)

set(timer_dir "${ZEPHYR_BASE}/modules/hal_realtek/ameba/os_wrapper")
target_sources(app PRIVATE
  src/main.c
  "${timer_dir}/os_wrapper_timer.c"
  "${timer_dir}/ameba_timer_core.c"
)
target_include_directories(app PRIVATE
  "${ZEPHYR_HAL_REALTEK_MODULE_DIR}/ameba/common/os_wrapper/include"
  "${ZEPHYR_HAL_REALTEK_MODULE_DIR}/ameba/common/include"
)
