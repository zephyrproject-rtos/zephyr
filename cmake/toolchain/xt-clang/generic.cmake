# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/cmake/toolchain/xcc/common.cmake)

set(COMPILER xt-clang)
set(CC clang)
set(C++ clang++)
set(LINKER xt-ld)

# xt-clang uses GNU Assembler (xt-as) based on binutils.
# However, CMake doesn't recognize it when invoking through xt-clang.
# This results in CMake going through all possible combinations of
# command line arguments while invoking xt-clang to determine
# assembler vendor. This multiple invocation of xt-clang unnecessarily
# lengthens the CMake phase of build, especially when xt-clang needs to
# obtain license information from remote licensing servers. So here
# forces the assembler ID to be GNU to speed things up a bit.
set(CMAKE_ASM_COMPILER_ID "GNU")

message(STATUS "Found toolchain: xt-clang (${XTENSA_TOOLCHAIN_PATH})")
