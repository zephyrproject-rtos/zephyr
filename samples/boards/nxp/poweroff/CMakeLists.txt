# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nxp_mcu_poweroff)

target_sources_ifdef(CONFIG_COUNTER_WAKEUP_ENABLE app PRIVATE src/counter_wakeup.c)
target_sources_ifdef(CONFIG_BUTTON_WAKEUP_ENABLE app PRIVATE src/button_wakeup.c)
