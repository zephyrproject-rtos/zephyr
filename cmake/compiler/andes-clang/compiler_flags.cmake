# SPDX-License-Identifier: Apache-2.0

if(CONFIG_USERSPACE)
  # Andes Clang defaults to -msmall-data-limit=8, which may materialize
  # large immediates (e.g. 0x0F0F0F0F0F0F0F0F) in .sdata section. This can
  # cause user threads to access the inaccessible .sdata section.
  # Reduce the small data limit to 4 to prevent such accesses.
  list(APPEND TOOLCHAIN_C_FLAGS -msmall-data-limit=4)
endif()

include(${ZEPHYR_BASE}/cmake/compiler/clang/compiler_flags.cmake)
