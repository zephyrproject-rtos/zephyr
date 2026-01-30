/*
 * Copyright (c) 2024 Infineon Technologies AG
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/sys_io.h>

#include "IfxWtu_regdef.h"

static inline void wdt_disable(mm_reg_t base)
{
	volatile Ifx_WTU_WDTCPU *wdt = (Ifx_WTU_WDTCPU *)base;
	Ifx_WTU_CTRLA wtu_ctrla;
	Ifx_WTU_CTRLB wtu_ctrlb = {.B.DR = 1};

	wtu_ctrla = wdt->CTRLA;
	if (wtu_ctrla.B.LCK) {
		wtu_ctrla.B.LCK = 0;
		wtu_ctrla.B.PW ^= 0x007F;

		wdt->CTRLA = wtu_ctrla;
	}
	wdt->CTRLB = wtu_ctrlb;

	wtu_ctrla.B.LCK = 1;
	wdt->CTRLA = wtu_ctrla;
}

#define WDT_DISABLE(n) wdt_disable(DT_REG_ADDR(DT_NODELABEL(n)))
#define WDT_DISABLED(n)                                                                            \
	(!IS_ENABLED(CONFIG_WATCHDOG) || IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT) ||                 \
	 DT_NODE_HAS_STATUS(DT_NODELABEL(n), disabled))

void z_tricore_wdt_boot(void)
{
#if WDT_DISABLED(wdtsys) && IS_ENABLED(CONFIG_AURIX_STARTUP_CPU)
	WDT_DISABLE(wdtsys);
#endif
#if WDT_DISABLED(wdt)
	WDT_DISABLE(wdt);
#endif
}

/* Function for Hightec LLVM */
void __memcpy_assume_aligned(void *dst, const void *src, size_t n)
{
	if (n == 4) {
		*(uint32_t *)dst = *(uint32_t *)src;
		return;
	}
	memcpy(dst, src, n);
}
