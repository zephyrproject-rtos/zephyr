/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2020 Linumiz
 */
MEMORY
{
	SRAM_NC (wx) : ORIGIN = CONFIG_SRAM_NC_BASE_ADDRESS, LENGTH = CONFIG_SRAM_NC_SIZE * 1024
}
#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>
