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

#include <aurix_endinit.h>

/* WDTxCON1 follows WDTxCON0; DR requests the watchdog to be disabled */
#define WDTCON1_OFFSET 4
#define WDTCON1_DR     BIT(3)

/* RSTCON.SW: 01b system, 10b application reset (TC3xx UM v2.0.0, p. 9-16, 9-17) */
#define SCU_RSTCON        0x58UL
#define SCU_SWRSTCON      0x60UL
#define RSTCON_SW         GENMASK(9, 8)
#define RSTCON_SW_SYSTEM  1U
#define RSTCON_SW_APP     2U
#define SWRSTCON_SWRSTREQ BIT(1)

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

/* Warm reboot is an application reset, cold reboot a system reset */
void sys_arch_reboot(int type)
{
	mem_addr_t rstcon = AURIX_SCU_BASE + SCU_RSTCON;
	mem_addr_t swrstcon = AURIX_SCU_BASE + SCU_SWRSTCON;
	uint32_t sw = (type == SYS_REBOOT_COLD) ? RSTCON_SW_SYSTEM : RSTCON_SW_APP;
	unsigned int key;

	/* RSTCON is Safety ENDINIT protected, SWRSTCON CPU ENDINIT protected */
	key = aurix_endinit_safety_unlock();
	sys_write32((sys_read32(rstcon) & ~RSTCON_SW) | FIELD_PREP(RSTCON_SW, sw), rstcon);
	aurix_endinit_safety_lock(key);

	key = aurix_endinit_cpu_unlock();
	sys_write32(sys_read32(swrstcon) | SWRSTCON_SWRSTREQ, swrstcon);
	aurix_endinit_cpu_lock(key);

	for (;;) {
		/* The reset takes effect shortly after the request */
	}
}
