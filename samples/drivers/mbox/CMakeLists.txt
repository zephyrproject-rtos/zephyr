#
# Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

if(("${BOARD}" STREQUAL "nrf5340dk_nrf5340_cpuapp") OR
   ("${BOARD}" STREQUAL "nrf5340bsim_nrf5340_cpuapp") OR
   ("${BOARD}" STREQUAL "adp_xc7k_ae350") OR
   ("${BOARD}" STREQUAL "mimxrt595_evk_cm33"))
  message(STATUS "${BOARD} compile as Main in this sample")
else()
  message(FATAL_ERROR "${BOARD} is not supported for this sample")
endif()

project(mbox_ipc)

enable_language(C ASM)

target_sources(app PRIVATE src/main.c)
