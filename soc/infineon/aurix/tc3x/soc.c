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

#include <aurix_endinit.h>

/* WDTxCON1 follows WDTxCON0; DR requests the watchdog to be disabled */
#define WDTCON1_OFFSET 4
#define WDTCON1_DR     BIT(3)

void z_tricore_wdt_boot(void);

static void wdt_disable(mem_addr_t con1)
{
	sys_write32(sys_read32(con1) | WDTCON1_DR, con1);
}

/* Zephyr does not service the watchdogs, so disable them */
void z_tricore_wdt_boot(void)
{
	unsigned int key;

	key = aurix_endinit_cpu_unlock();
	wdt_disable(aurix_wdtcpu_con0() + WDTCON1_OFFSET);
	aurix_endinit_cpu_lock(key);

	if ((cr_read(TRICORE_CORE_ID) & GENMASK(2, 0)) == 0U) {
		key = aurix_endinit_safety_unlock();
		wdt_disable(AURIX_SCU_BASE + AURIX_SCU_WDTSCON0 + WDTCON1_OFFSET);
		aurix_endinit_safety_lock(key);
	}
}
