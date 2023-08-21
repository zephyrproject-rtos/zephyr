# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2023, Nordic Semiconductor ASA

# Set Zephyr roots to support unittest mode. This is an optional CMake module,
# which allows the build system to locate the dedicated unit testing board.
#
# Outcome:
# The following variables will be defined when this CMake module completes:
#
# - BOARD_ROOT: Board root containing the unit_testing board
# - ARCH_ROOT:  Arch root containing the unit_testing arch
# - SOC_ROOT:   SoC root containing the unit_testing SoC
#
# Note:
# This module will override previous values of the ROOT variables named above.
# The root.cmake module may be loaded next, to add more items to each ROOT list.

set(BOARD_ROOT ${ZEPHYR_BASE}/subsys/testsuite)
set(ARCH_ROOT  ${ZEPHYR_BASE}/subsys/testsuite)
set(SOC_ROOT   ${ZEPHYR_BASE}/subsys/testsuite)
