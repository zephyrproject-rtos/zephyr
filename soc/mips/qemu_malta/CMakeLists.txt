# Copyright (c) 2020 Antony Pavlov <antonynpavlov@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0

zephyr_compile_options(
  -G0
  -mips32
  -mabi=32
  -mno-abicalls
  -fno-pic
  -pipe
  ${TOOLCHAIN_C_FLAGS}
)

zephyr_ld_options(
  -mips32
  ${TOOLCHAIN_LD_FLAGS}
)
