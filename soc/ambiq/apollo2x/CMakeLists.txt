# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2025 Linumiz GmbH
# Author : Sri Surya  <srisurya@linumiz.com>

zephyr_sources(soc.c)
zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
