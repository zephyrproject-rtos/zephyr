# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/cmake/toolchain/xcc/common.cmake)

set(COMPILER xcc)
set(OPTIMIZE_FOR_DEBUG_FLAG "-O0")
set(CC xcc)
set(C++ xc++)

list(APPEND TOOLCHAIN_C_FLAGS
  -imacros${ZEPHYR_BASE}/include/zephyr/toolchain/xcc_missing_defs.h
  )

# GCC-based XCC uses GNU Assembler (xt-as).
# However, CMake doesn't recognize it when invoking through xt-xcc.
# This results in CMake going through all possible combinations of
# command line arguments while invoking xt-xcc to determine
# assembler vendor. This multiple invocation of xt-xcc unnecessarily
# lengthens the CMake phase of build, especially when XCC needs to
# obtain license information from remote licensing servers. So here
# forces the assembler ID to be GNU to speed things up a bit.
set(CMAKE_ASM_COMPILER_ID "GNU")

message(STATUS "Found toolchain: xcc (${XTENSA_TOOLCHAIN_PATH})")
