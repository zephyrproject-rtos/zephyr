# SPDX-License-Identifier: Apache-2.0

if(CONFIG_LLVM_USE_LD)
  set(LINKER ld)
elseif(CONFIG_LLVM_USE_LLD)
  set(LINKER lld)
endif()
