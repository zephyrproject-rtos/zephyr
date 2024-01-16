# Copyright (c) 2023 Meta
# SPDX-License-Identifier: Apache-2.0

zephyr_sources()

zephyr_include_directories(${ZEPHYR_BASE}/soc/v2/riscv/common/riscv-privileged)
zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")
