/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/sys/reboot.h>
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

/* PINRST stays 0, or the pads stay in reset (erratum SMM_TC.006) */
#define SMM_BASE            0xF0060000UL
#define SMM_SWRSTCON        (SMM_BASE + 0xC0UL)
#define SMM_RSTTRIGCTRLA    (SMM_BASE + 0x114UL)
#define RSTTRIGCTRLA_SW     GENMASK(26, 24)
#define RSTTRIGCTRLA_PINRST BIT(30)
#define RSTTRIG_SW_SYSTEM   1U
#define RSTTRIG_SW_APP      2U
#define SWRSTCON_SWRSTREQ   BIT(1)

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

/* Warm reboot is an application reset, cold reboot a system reset */
void sys_arch_reboot(int type)
{
	uint32_t sw = (type == SYS_REBOOT_COLD) ? RSTTRIG_SW_SYSTEM : RSTTRIG_SW_APP;
	uint32_t val = sys_read32(SMM_RSTTRIGCTRLA);

	val &= ~(RSTTRIGCTRLA_SW | RSTTRIGCTRLA_PINRST);
	sys_write32(val | FIELD_PREP(RSTTRIGCTRLA_SW, sw), SMM_RSTTRIGCTRLA);
	sys_write32(SWRSTCON_SWRSTREQ, SMM_SWRSTCON);

	for (;;) {
		/* The reset takes effect shortly after the request */
	}
}
