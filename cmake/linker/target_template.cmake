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

if(NOT COMMAND toolchain_linker_add_compiler_options)

  # If the linker doesn't provide a method for mapping compiler options
  # to linker options, then assume we can't. This matters when the linker
  # is using additional flags when computing toolchain library paths.

  function(toolchain_linker_add_compiler_options)
  endfunction()
endif()
