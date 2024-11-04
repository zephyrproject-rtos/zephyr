# SPDX-License-Identifier: Apache-2.0

if(CONFIG_RX)
  list(APPEND TOOLCHAIN_C_FLAGS)
  list(APPEND TOOLCHAIN_C_FLAGS  -mlittle-endian-data -ffunction-sections -fdata-sections)

  list(APPEND TOOLCHAIN_LD_FLAGS)
  list(APPEND TOOLCHAIN_LD_FLAGS -mlittle-endian-data)

  if(NOT CONFIG_FPU)
    list(APPEND TOOLCHAIN_C_FLAGS -nofpu)
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

endif()
