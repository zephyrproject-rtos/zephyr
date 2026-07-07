# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rt700_dual_core_pm_remote)

if(NOT CONFIG_BOARD_MIMXRT700_EVK_MIMXRT798S_CM33_CPU1)
  message(FATAL_ERROR "${BOARD}/${BOARD_QUALIFIERS} is not supported by this sample")
endif()

target_sources(app PRIVATE src/main.c)
target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/../modules/hal/nxp/mcux/mcux-sdk-ng/drivers/mu1)
