/*
 * Copyright (c) 2024 Infineon Technologies AG
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>

#include <IfxScu_reg.h>
#include <IfxScu_regdef.h>
#include "soc.h"

static inline void wdt_set_endinit(mm_reg_t base, bool endinit)
{
	volatile Ifx_SCU_WDTCPU *wdt = (Ifx_SCU_WDTCPU *)base;
	Ifx_SCU_WDTCPU_CON0 wtu_ctrla = wdt->CON0;

	/* Unlock watchdog */
	wtu_ctrla.B.PW ^= 0x003F;
	if (wtu_ctrla.B.LCK) {
		wtu_ctrla.B.ENDINIT = 1;
		wtu_ctrla.B.LCK = 0;
		wdt->CON0 = wtu_ctrla;
	}

	/* Set endinit */
	wtu_ctrla.B.ENDINIT = endinit;
	wtu_ctrla.B.LCK = 1;
	wdt->CON0 = wtu_ctrla;
}

static inline void wdt_disable(mm_reg_t base)
{
	volatile Ifx_SCU_WDTCPU *wdt = (Ifx_SCU_WDTCPU *)base;
	Ifx_SCU_WDTCPU_CON1 wtu_ctrlb = {.B.DR = 1};

	/* Clear endinit */
	wdt_set_endinit(base, false);

	/* Disable watchdog */
	wdt->CON1 = wtu_ctrlb;

	/* Set endinit */
	wdt_set_endinit(base, true);
}

#define WDT_DISABLE(n) wdt_disable(DT_REG_ADDR(DT_NODELABEL(n)))
#define WDT_DISABLED(n)                                                                            \
	(!IS_ENABLED(CONFIG_WATCHDOG) || IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT) ||                 \
	 DT_NODE_HAS_STATUS(DT_NODELABEL(n), disabled))

void z_tricore_wdt_boot(void)
{
#if IS_ENABLED(CONFIG_AURIX_STARTUP_CPU)
	/* Initially set safety endinit */
	aurix_safety_endinit_enable(true);
#endif

#if WDT_DISABLED(wdts) && IS_ENABLED(CONFIG_AURIX_STARTUP_CPU)
	WDT_DISABLE(wdts);
#endif
#if WDT_DISABLED(wdt)
	WDT_DISABLE(wdt);
#endif
}

void aurix_cpu_endinit_enable(bool enabled)
{
	mm_reg_t wdtcpucon0 = (uintptr_t)&MODULE_SCU.WDTCPU[arch_proc_id()];

	wdt_set_endinit(wdtcpucon0, enabled);
}

void aurix_safety_endinit_enable(bool enabled)
{
	uintptr_t wdtscon0 = (uintptr_t)&MODULE_SCU.WDTS;

	wdt_set_endinit(wdtscon0, enabled);
}

void __memcpy_assume_aligned(void *dst, const void *src, size_t n)
{
	if (n == 4) {
		*(uint32_t *)dst = *(const uint32_t *)src;
		return;
	}
	memcpy(dst, src, n);
}
