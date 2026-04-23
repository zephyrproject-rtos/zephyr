/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/kernel.h>

extern char _vector_start[];

#ifdef CONFIG_SRAM_DEPRECATED_KCONFIG_SET
#define RAM_BASE CONFIG_SRAM_BASE_ADDRESS
#else
#define RAM_BASE DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#endif

void relocate_vector_table(void)
{
#if defined(CONFIG_XIP) && (CONFIG_FLASH_BASE_ADDRESS != 0) ||                                     \
	!defined(CONFIG_XIP) && (RAM_BASE != 0)
	write_sctlr(read_sctlr() & ~HIVECS);
#elif defined(CONFIG_SW_VECTOR_RELAY) || defined(CONFIG_SW_VECTOR_RELAY_CLIENT)
	_vector_table_pointer = _vector_start;
#endif
	__set_VBAR(POINTER_TO_UINT(_vector_start));
}
