# SPDX-License-Identifier: Apache-2.0

if(CONFIG_USERSPACE)
  # Andes Clang defaults to 8; keep 8-byte constants out of user-inaccessible .sdata
  list(APPEND TOOLCHAIN_C_FLAGS -msmall-data-limit=4)
endif()

include(${ZEPHYR_BASE}/cmake/compiler/clang/compiler_flags.cmake)
