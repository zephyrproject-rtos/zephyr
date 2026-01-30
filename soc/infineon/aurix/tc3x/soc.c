/*
 * Copyright (c) 2024 Infineon Technologies AG
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#include "soc.h"
#include "IfxScu_regdef.h"

#define CPU_BASE_ADDR(i) (0xF8800000 + i * 0x20000)

#define CPU_PC     0x1FE08
#define CPU_SYSCON 0x1FE14

static inline void wdt_disable(mm_reg_t base)
{
	volatile Ifx_SCU_WDTCPU *wdt = (Ifx_SCU_WDTCPU *)base;
	Ifx_SCU_WDTCPU_CON0 wtu_ctrla;
	Ifx_SCU_WDTCPU_CON1 wtu_ctrlb = {.B.DR = 1};

	wtu_ctrla = wdt->CON0;
	wtu_ctrla.B.PW ^= 0x003F;
	/* Unlock watchdog */
	if (wtu_ctrla.B.LCK) {
		wtu_ctrla.B.ENDINIT = 1;
		wtu_ctrla.B.LCK = 0;
		wdt->CON0 = wtu_ctrla;
	}
	/* Clear endinit */
	wtu_ctrla.B.ENDINIT = 0;
	wtu_ctrla.B.LCK = 1;
	wdt->CON0 = wtu_ctrla;
	while (wdt->CON0.B.ENDINIT == 1) {
		;
	}

	/* Disable watchdog */
	wdt->CON1 = wtu_ctrlb;

	/* Unlock watchdog */
	wtu_ctrla.B.ENDINIT = 1;
	wtu_ctrla.B.LCK = 0;
	wdt->CON0 = wtu_ctrla;
	/* Set endinit */
	wtu_ctrla.B.ENDINIT = 1;
	wtu_ctrla.B.LCK = 1;
	wdt->CON0 = wtu_ctrla;
	while (wdt->CON0.B.ENDINIT == 0) {
		;
	}
}

#define WDT_DISABLE(n) wdt_disable(DT_REG_ADDR(n))
#define WDT_DISABLE_IF_UNUSED(node_id)                                                             \
	IF_ENABLED(DT_NODE_HAS_STATUS(node_id, disabled), (wdt_disable(DT_REG_ADDR(node_id))))
#define WDT_CPU_DISABLE_BY_ID(idx)     WDT_DISABLE(DT_NODELABEL(DT_CAT3(cpu, idx, _wdt)))
#define WDT_CPU_DISABLE_IF_UNUSED(idx) WDT_DISABLE_IF_UNUSED(DT_NODELABEL(DT_CAT3(cpu, i, _wdt)))
#define WDT_SAFETY_DISABLE_IF_UNUSED   WDT_DISABLE_IF_UNUSED(DT_NODELABEL(safety_wdt))

void z_tricore_wdt_boot(void)
{
#if !IS_ENABLED(CONFIG_WATCHDOG)
#if CONFIG_TRICORE_CORE_ID == 0
	WDT_DISABLE(DT_NODELABEL(safety_wdt));
#endif
	WDT_CPU_DISABLE_BY_ID(CONFIG_TRICORE_CORE_ID);
#elif IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT)
#if CONFIG_TRICORE_CORE_ID == 0
	WDT_SAFETY_DISABLE_IF_UNUSED;
#endif
	WDT_CPU_DISABLE(CONFIG_TRICORE_CORE_ID)
#else
#if CONFIG_TRICORE_CORE_ID == 0
	WDT_DISABLE(DT_NODELABEL(safety_wdt));
#endif
	WDT_CPU_DISABLE_IF_UNUSED(CONFIG_TRICORE_CORE_ID);
#endif
}

void aurix_cpu_endinit_enable(bool enabled)
{
	uintptr_t wdtcpucon0 = 0xf0036000 + 0x24C + 12 * arch_proc_id();
	uint32_t t = sys_read32(wdtcpucon0);
	uint32_t pw = (t & 0xFFFF) >> 2;

	pw ^= 0x003F;
	if (t & BIT(1)) {
		sys_write32((t & GENMASK(31, 16)) | (pw << 2) | BIT(0), wdtcpucon0);
	}
	sys_write32((t & GENMASK(31, 16)) | (pw << 2) | BIT(1) | (enabled ? BIT(0) : 0),
		    wdtcpucon0);
	WAIT_FOR((sys_read32(wdtcpucon0) & BIT(0)) == enabled, 1000, k_busy_wait(1));
}

void aurix_safety_endinit_enable(bool enabled)
{
	uintptr_t wdtscon0 = 0xf0036000 + 0x02A8;
	uint32_t t = sys_read32(wdtscon0);
	uint32_t pw = (t & 0xFFFF) >> 2;

	pw ^= 0x003F;
	if (t & BIT(1)) {
		sys_write32((t & GENMASK(31, 16)) | (pw << 2) | BIT(0), wdtscon0);
	}
	sys_write32((t & GENMASK(31, 16)) | (pw << 2) | BIT(1) | (enabled ? BIT(0) : 0), wdtscon0);
	WAIT_FOR((sys_read32(wdtscon0) & BIT(0)) == enabled, 10, k_busy_wait(1));
}

void __memcpy_assume_aligned(void *dst, const void *src, size_t n)
{
	if (n == 4) {
		*(uint32_t *)dst = *(uint32_t *)src;
		return;
	}
	memcpy(dst, src, n);
}
