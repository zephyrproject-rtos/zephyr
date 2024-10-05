# Copyright (c) 2021 Yonatan Schachter
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_linker_sources_ifdef(CONFIG_RP2_REQUIRES_SECOND_STAGE_BOOT
	SECTIONS sections.ld
)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
