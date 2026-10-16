# Copyright (c) 2020, 2021 Antony Pavlov <antonynpavlov@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0

zephyr_compile_options(
  -mips32
  ${TOOLCHAIN_C_FLAGS}
)

zephyr_sources(
  vector.S
)

zephyr_ld_options(
  -mips32
  ${TOOLCHAIN_LD_FLAGS}
)

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/mips/linker.ld CACHE INTERNAL "")
