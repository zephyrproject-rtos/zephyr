# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: Apache-2.0

zephyr_get(HEXAGON_TOOLCHAIN_PATH)

if(NOT HEXAGON_TOOLCHAIN_PATH)
  message(FATAL_ERROR
    "HEXAGON_TOOLCHAIN_PATH is not set. Point it at the Hexagon LLVM "
    "cross-toolchain, for example "
    "/opt/toolchains/clang+llvm-23.1.0-cross-hexagon-unknown-linux-musl."
  )
endif()

set(LLVM_TOOLCHAIN_PATH ${HEXAGON_TOOLCHAIN_PATH} CACHE PATH
    "clang install directory" FORCE
)

include(${ZEPHYR_BASE}/cmake/toolchain/host/llvm/generic.cmake)

set(TOOLCHAIN_HAS_LIBCXX OFF CACHE BOOL
    "True if toolchain supports libc++" FORCE
)

set(TOOLCHAIN_KCONFIG_DIR ${ZEPHYR_BASE}/cmake/toolchain/host/llvm)

message(STATUS "Found toolchain: hexagon (${HEXAGON_TOOLCHAIN_PATH})")
