# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

# Same test-pwm-loopback binding and wiring as the pwm_loopback test
list(APPEND DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../pwm_loopback)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pwm_pm)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_PM_POLICY_DEVICE_CONSTRAINTS app PRIVATE src/policy.c)

# PM_DEVICE_SYSTEM_MANAGED defaults on with PM_DEVICE alone; the sweep only
# exists with system PM.
if(CONFIG_PM AND CONFIG_PM_DEVICE_SYSTEM_MANAGED)
  target_sources(app PRIVATE src/sysmanaged.c)
endif()
