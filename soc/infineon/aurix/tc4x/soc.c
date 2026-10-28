/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

/* Watchdog timer unit (TC4Dx UM v1.1, chapter 42) */
#define WTU_BASE        0xF0000000UL
#define WDTCPU_CTRLA(y) (WTU_BASE + 0x3CUL + (y) * 0x30UL)
#define WDTSYS_CTRLA    (WTU_BASE + 0x1A8UL)
#define CTRLB_OFFSET    4

#define CTRLA_LCK    BIT(0)
/* Bits 7:1 of the password read back inverted */
#define CTRLA_PW_INV GENMASK(7, 1)
#define CTRLB_DR     BIT(0)

#define CORE_ID_MASK GENMASK(2, 0)
#define CPU_WDT_MAX  5

void z_tricore_wdt_boot(void);

/* Unlock access, disable request, then configuration update and lock access */
static void wdt_disable(mem_addr_t ctrla)
{
	uint32_t val = sys_read32(ctrla);

	if ((val & CTRLA_LCK) != 0U) {
		sys_write32((val ^ CTRLA_PW_INV) & ~CTRLA_LCK, ctrla);
	}

	sys_write32(sys_read32(ctrla + CTRLB_OFFSET) | CTRLB_DR, ctrla + CTRLB_OFFSET);
	sys_write32(sys_read32(ctrla) | CTRLA_LCK, ctrla);
}

/* Zephyr does not service the watchdogs, so disable them */
void z_tricore_wdt_boot(void)
{
	uint32_t core = cr_read(TRICORE_CORE_ID) & CORE_ID_MASK;

	if (core <= CPU_WDT_MAX) {
		wdt_disable(WDTCPU_CTRLA(core));
	}

	if (core == 0U) {
		wdt_disable(WDTSYS_CTRLA);
	}
}
