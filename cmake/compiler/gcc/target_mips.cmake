# Copyright (c) 2021 Antony Pavlov <antonynpavlov@gmail.com>
# Copyright (c) 2021 Remy Luisant <remy@luisant.ca>
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BIG_ENDIAN)
  list(APPEND TOOLCHAIN_C_FLAGS -EB)
  list(APPEND TOOLCHAIN_LD_FLAGS -EB)
else()
  list(APPEND TOOLCHAIN_C_FLAGS -EL)
  list(APPEND TOOLCHAIN_LD_FLAGS -EL)
endif()

list(APPEND TOOLCHAIN_C_FLAGS  -msoft-float)
list(APPEND TOOLCHAIN_LD_FLAGS -msoft-float)

list(APPEND TOOLCHAIN_C_FLAGS -G0 -mno-gpopt -fno-pic)

list(APPEND TOOLCHAIN_C_FLAGS -pipe)
