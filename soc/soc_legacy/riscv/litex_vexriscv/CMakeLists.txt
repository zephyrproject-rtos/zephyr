#
# Copyright (c) 2018 - 2019 Antmicro <www.antmicro.com>
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources(
    ${ZEPHYR_BASE}/soc/common/riscv/riscv-privileged/soc_irq.S
    ${ZEPHYR_BASE}/soc/common/riscv/riscv-privileged/vector.S
)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")
