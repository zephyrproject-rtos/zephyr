# Copyright (c) 2024 Michael Hope
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(common)
add_subdirectory(${CONFIG_SOC_SERIES})

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")
