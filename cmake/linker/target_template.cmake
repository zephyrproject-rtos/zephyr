# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2024, Nordic Semiconductor ASA

# Template file for optional Zephyr linker macros.
#
# This file will define optional linker macros for toolchains that are not
# defining these macros themselves.

if(NOT COMMAND toolchain_linker_finalize)
  macro(toolchain_linker_finalize)
  endmacro()
endif()
