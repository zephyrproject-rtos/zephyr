/*
 * Copyright (c) 2024 Infineon Technologies AG
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/sys_io.h>

#include "IfxWtu_reg.h"
#include "IfxWtu_regdef.h"
#include "soc.h"
#include "soc_prot.h"

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

#define WDT_DISABLE(n) wdt_disable(DT_REG_ADDR(n))
#define WDT_DISABLE_IF_UNUSED(node_id)                                                             \
	IF_ENABLED(DT_NODE_HAS_STATUS(node_id, disabled), (wdt_disable(DT_REG_ADDR(node_id))))
#define WDT_CPU_DISABLE_BY_ID(idx)     WDT_DISABLE(DT_NODELABEL(DT_CAT3(cpu, idx, _wdt)))
#define WDT_CPU_DISABLE_IF_UNUSED(idx) WDT_DISABLE_IF_UNUSED(DT_NODELABEL(DT_CAT3(cpu, i, _wdt)))
#define WDT_SAFETY_DISABLE_IF_UNUSED   WDT_DISABLE_IF_UNUSED(DT_NODELABEL(safety_wdt))
#define WDT_CPU_IDX(i)                 COND_CODE_1(IS_EQ(i, 6), (cs), (i))

void z_tricore_wdt_boot(void)
{
#if !IS_ENABLED(CONFIG_WATCHDOG)
#if CONFIG_TRICORE_CORE_ID == 0
	WDT_DISABLE(DT_NODELABEL(safety_wdt));
#endif
	WDT_CPU_DISABLE_BY_ID(WDT_CPU_IDX(CONFIG_TRICORE_CORE_ID));
#elif IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT)
#if CONFIG_TRICORE_CORE_ID == 0
	WDT_SAFETY_DISABLE_IF_UNUSED;
#endif
	WDT_CPU_DISABLE(WDT_CPU_IDX(CONFIG_TRICORE_CORE_ID))
#else
#if CONFIG_TRICORE_CORE_ID == 0
	WDT_DISABLE(DT_NODELABEL(safety_wdt));
#endif
	WDT_CPU_DISABLE_IF_UNUSED(WDT_CPU_IDX(CONFIG_TRICORE_CORE_ID));
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
