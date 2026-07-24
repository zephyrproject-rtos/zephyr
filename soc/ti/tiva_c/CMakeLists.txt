# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2026 Linumiz
# Author: Sri Surya <srisurya@linumiz.com>


add_subdirectory(${SOC_SERIES})

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
