# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/cmake/toolchain/andes/target.cmake)

if(CONFIG_LLVM_USE_LLD)
  set(LINKER lld)
else()
  set(LINKER ld)
endif()
