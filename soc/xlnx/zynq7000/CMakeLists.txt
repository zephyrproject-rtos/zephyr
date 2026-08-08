#
# Copyright (c) 2021 Weidmueller Interface GmbH & Co. KG
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources(common/soc.c)
zephyr_include_directories(common)

# soc_reset_hook() calls the arch-internal PL310 early-enable entry point
# declared in <cortex_a_r/outercache.h>.
zephyr_include_directories_ifdef(CONFIG_ARM_PL310 ${ZEPHYR_BASE}/arch/arm/include)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "")
