# SPDX-License-Identifier: Apache-2.0

list(APPEND TOOLCHAIN_C_FLAGS  -msoft-float)
list(APPEND TOOLCHAIN_LD_FLAGS -msoft-float)

if(CONFIG_SPARC_CASA)
  # SPARC V8, mul/div, casa
  list(APPEND TOOLCHAIN_C_FLAGS  -mcpu=leon3)
  list(APPEND TOOLCHAIN_LD_FLAGS -mcpu=leon3)
else()
  # SPARC V8, mul/div, no casa
  list(APPEND TOOLCHAIN_C_FLAGS  -mcpu=leon)
  list(APPEND TOOLCHAIN_LD_FLAGS -mcpu=leon)
endif()
