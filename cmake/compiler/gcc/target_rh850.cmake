# Copyright (c) 2026 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

if(NOT CONFIG_PICOLIBC)
  list(APPEND TOOLCHAIN_LD_FLAGS -lm)
endif()

if(NOT CONFIG_FPU)
  list(APPEND TOOLCHAIN_C_FLAGS  -msoft-float)
  list(APPEND TOOLCHAIN_LD_FLAGS -msoft-float)
endif()

list(APPEND TOOLCHAIN_C_FLAGS  -mv850e3v5 -mrh850-abi)
list(APPEND TOOLCHAIN_LD_FLAGS -mv850e3v5 -mrh850-abi)
