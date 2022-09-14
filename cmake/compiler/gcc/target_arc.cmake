# SPDX-License-Identifier: Apache-2.0

if(EXISTS ${SOC_FULL_DIR}/tune_build_ops.cmake)
  include(${SOC_FULL_DIR}/tune_build_ops.cmake)
endif()

if(NOT DEFINED GCC_ARC_TUNED_CPU)
  set(GCC_ARC_TUNED_CPU ${GCC_M_CPU})
endif()

list(APPEND TOOLCHAIN_C_FLAGS -mcpu=${GCC_ARC_TUNED_CPU})
list(APPEND TOOLCHAIN_LD_FLAGS -mcpu=${GCC_ARC_TUNED_CPU})
