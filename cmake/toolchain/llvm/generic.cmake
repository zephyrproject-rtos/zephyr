# SPDX-License-Identifier: Apache-2.0

# Purpose of the generic.cmake is to define a generic C compiler which can be
# used for devicetree pre-processing and other pre-processing tasks which must
# be performed before the target can be determined.

# Todo: deprecate CLANG_ROOT_DIR
set_ifndef(LLVM_TOOLCHAIN_PATH "$ENV{CLANG_ROOT_DIR}")
zephyr_get(LLVM_TOOLCHAIN_PATH)

if(LLVM_TOOLCHAIN_PATH)
  set(TOOLCHAIN_HOME ${LLVM_TOOLCHAIN_PATH}/bin/)
endif()

set(LLVM_TOOLCHAIN_PATH ${CLANG_ROOT_DIR} CACHE PATH "clang install directory")

set(COMPILER clang)
set(BINTOOLS llvm)

set(TOOLCHAIN_HAS_NEWLIB OFF CACHE BOOL "True if toolchain supports newlib")

list(APPEND TOOLCHAIN_C_FLAGS --config ${ZEPHYR_BASE}/cmake/toolchain/llvm/clang.cfg)
list(APPEND TOOLCHAIN_LD_FLAGS --config ${ZEPHYR_BASE}/cmake/toolchain/llvm/clang.cfg)

message(STATUS "Found toolchain: host (clang/ld)")
