# Copyright 2026
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(utick_demo)

target_sources(app PRIVATE src/main.c)

# The MCUX SDK UTICK driver has no Kconfig symbol in this Zephyr tree yet.
# Compile its small HAL source directly for this standalone demonstration.
target_sources(app PRIVATE
  ${ZEPHYR_BASE}/../modules/hal/nxp/mcux/mcux-sdk-ng/drivers/utick/fsl_utick.c
)
target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/../modules/hal/nxp/mcux/mcux-sdk-ng/drivers/utick
)
