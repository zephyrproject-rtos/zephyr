#
# Copyright (c) 2026 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources(
  ${ZEPHYR_BASE}/soc/common/riscv-privileged/soc_irq.S
  ${ZEPHYR_BASE}/soc/common/riscv-privileged/vector.S
)

zephyr_library()
zephyr_library_sources(soc.c)
zephyr_library_include_directories(${ZEPHYR_BASE}/arch/common/include)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")
