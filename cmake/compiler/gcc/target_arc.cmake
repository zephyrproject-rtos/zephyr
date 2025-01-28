# SPDX-License-Identifier: Apache-2.0

if(EXISTS ${SOC_FULL_DIR}/tune_build_ops.cmake)
  include(${SOC_FULL_DIR}/tune_build_ops.cmake)
endif()

if(NOT DEFINED GCC_ARC_TUNED_CPU)
  set(GCC_ARC_TUNED_CPU ${GCC_M_CPU})
endif()

# Flags not supported by llext linker
# (regexps are supported and match whole word)
set(LLEXT_REMOVE_FLAGS
  -fno-pic
  -fno-pie
  -ffunction-sections
  -fdata-sections
  -Os
)

set(LLEXT_APPEND_FLAGS
  -mcpu=${GCC_ARC_TUNED_CPU} # Force compiler and linker match
)

list(APPEND TOOLCHAIN_C_FLAGS -mcpu=${GCC_ARC_TUNED_CPU})
list(APPEND TOOLCHAIN_LD_FLAGS -mcpu=${GCC_ARC_TUNED_CPU})
