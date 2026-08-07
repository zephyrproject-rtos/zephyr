# SPDX-License-Identifier: Apache-2.0

list(APPEND TOOLCHAIN_C_FLAGS)
list(APPEND TOOLCHAIN_C_FLAGS  -mlittle-endian-data -ffunction-sections -fdata-sections -m64bit-doubles)

list(APPEND TOOLCHAIN_LD_FLAGS)
list(APPEND TOOLCHAIN_LD_FLAGS -mlittle-endian-data -ffunction-sections -fdata-sections -m64bit-doubles)

if(NOT CONFIG_PICOLIBC)
  list(APPEND TOOLCHAIN_LD_FLAGS -lm)
endif()

if(CONFIG_FPU)
  if(CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION)
    list(APPEND TOOLCHAIN_C_FLAGS -mdfpu)
    list(APPEND TOOLCHAIN_LD_FLAGS -mdfpu)
  endif()
else()
  list(APPEND TOOLCHAIN_C_FLAGS -nofpu)
  list(APPEND TOOLCHAIN_LD_FLAGS -nofpu)
endif()

if(CONFIG_CPU_RXV1)
  list(APPEND TOOLCHAIN_C_FLAGS -misa=v1)
  list(APPEND TOOLCHAIN_LD_FLAGS -misa=v1)
elseif(CONFIG_CPU_RXV2)
  list(APPEND TOOLCHAIN_C_FLAGS -misa=v2)
  list(APPEND TOOLCHAIN_LD_FLAGS -misa=v2)
else()
  list(APPEND TOOLCHAIN_C_FLAGS -misa=v3)
  list(APPEND TOOLCHAIN_LD_FLAGS -misa=v3)
endif()
