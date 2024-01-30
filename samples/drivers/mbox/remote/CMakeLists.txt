#
# Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
# Copyright 2023-2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

if(("${BOARD}" STREQUAL "nrf5340dk_nrf5340_cpunet") OR
  ("${BOARD}" STREQUAL "nrf5340bsim_nrf5340_cpunet") OR
  ("${BOARD}" STREQUAL "mimxrt1170_evkb_cm4") OR
  ("${BOARD}" STREQUAL "mimxrt1170_evk_cm4") OR
  ("${BOARD}" STREQUAL "mimxrt1160_evk_cm4") OR
  ("${BOARD}" STREQUAL "lpcxpresso55s69_cpu1") OR
  ("${BOARD}" STREQUAL "adp_xc7k_ae350"))
  message(STATUS "${BOARD} compile as remote in this sample")
else()
  message(FATAL_ERROR "${BOARD} is not supported for this sample")
endif()

project(mbox_ipc_remote)

target_sources(app PRIVATE src/main.c)
