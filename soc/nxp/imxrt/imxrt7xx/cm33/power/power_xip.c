/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "power_xip.h"
#include <fsl_common.h>

/*
 * XIP detection: the address of the flash-resident anchor below identifies
 * which XSPI interface is hosting the executing image.
 *
 * The suspend/resume helpers run from RAM (CodeQuickAccess), so their
 * addresses can no longer be used for this purpose. power_xip_anchor is
 * deliberately left in XIP flash; only its link-time address (not its
 * code) is referenced, so the comparison stays valid even while the
 * XSPI interface is held in reset.
 */
static void power_xip_anchor(void)
{
	__asm__ volatile("");
}

#define POWER_XIP_ANCHOR ((uint32_t)(uintptr_t)power_xip_anchor)

/*
 * Each XSPI instance's memory-mapped window comes from devicetree: reg[0] is the
 * controller's register block, reg[1] the AHB window the image executes from.
 * RT7xx exposes every window twice, at a secure and a non-secure alias that
 * differ only in bit 28, and the image may be linked against either, so both
 * are accepted.
 */
#define POWER_XIP_NS_ALIAS_BIT 0x10000000U

#define POWER_XIP_WINDOW_BASE(node) DT_REG_ADDR_BY_IDX(DT_NODELABEL(node), 1)
#define POWER_XIP_WINDOW_SIZE(node) DT_REG_SIZE_BY_IDX(DT_NODELABEL(node), 1)

#define POWER_XIP_IN_WINDOW(base, size)								\
	((POWER_XIP_ANCHOR >= (uint32_t)(base)) &&						\
	 (POWER_XIP_ANCHOR < (uint32_t)((base) + (size))))

#define POWER_IS_XIP(node)									\
	(POWER_XIP_IN_WINDOW(POWER_XIP_WINDOW_BASE(node), POWER_XIP_WINDOW_SIZE(node)) ||	\
	 POWER_XIP_IN_WINDOW(POWER_XIP_WINDOW_BASE(node) & ~POWER_XIP_NS_ALIAS_BIT,		\
			     POWER_XIP_WINDOW_SIZE(node)))

/*
 * State preserved across suspend/resume. Lives in QUICKACCESS SRAM
 * so it remains readable while the XSPI interface is held in reset.
 */
AT_QUICKACCESS_SECTION_DATA(static bool xspi_cache_enabled);
AT_QUICKACCESS_SECTION_DATA(static bool xcache0_was_enabled);
AT_QUICKACCESS_SECTION_DATA(static bool xcache1_was_enabled);

/*
 * XCACHE (L1 code/system) helpers
 *
 * All helpers below run while the hosting XSPI flash is held in reset, so they
 * must execute from RAM, so they are placed in the .ramfunc region with __ramfunc.
 */
static __ramfunc void disable_xcache(XCACHE_Type *base)
{
	/* Push modified contents */
	base->CCR |= XCACHE_CCR_PUSHW0_MASK | XCACHE_CCR_PUSHW1_MASK | XCACHE_CCR_GO_MASK;
	while ((base->CCR & XCACHE_CCR_GO_MASK) != 0x00U) {
	}
	base->CCR &= ~(XCACHE_CCR_PUSHW0_MASK | XCACHE_CCR_PUSHW1_MASK);
	/* Disable cache */
	base->CCR &= ~XCACHE_CCR_ENCACHE_MASK;
}

static __ramfunc void enable_xcache(XCACHE_Type *base)
{
	/* Invalidate all lines */
	base->CCR |= XCACHE_CCR_INVW0_MASK | XCACHE_CCR_INVW1_MASK | XCACHE_CCR_GO_MASK;
	while ((base->CCR & XCACHE_CCR_GO_MASK) != 0x00U) {
	}
	/* Enable cache */
	base->CCR |= XCACHE_CCR_ENCACHE_MASK;
	__ISB();
	__DSB();
}

/*
 * CACHE64 (XSPI inline) helpers
 */
static __ramfunc void enable_xspi_cache(CACHE64_CTRL_Type *cache)
{
	/* Invalidate entire cache */
	cache->CCR |= CACHE64_CTRL_CCR_INVW0_MASK | CACHE64_CTRL_CCR_INVW1_MASK |
		      CACHE64_CTRL_CCR_GO_MASK;
	while ((cache->CCR & CACHE64_CTRL_CCR_GO_MASK) != 0x00U) {
	}
	cache->CCR &= ~(CACHE64_CTRL_CCR_INVW0_MASK | CACHE64_CTRL_CCR_INVW1_MASK);
	/* Enable cache */
	cache->CCR |= CACHE64_CTRL_CCR_ENCACHE_MASK;
}

static __ramfunc void disable_xspi_cache(CACHE64_CTRL_Type *cache)
{
	/* Push (clean) cache */
	cache->CCR |= CACHE64_CTRL_CCR_PUSHW0_MASK | CACHE64_CTRL_CCR_PUSHW1_MASK |
		      CACHE64_CTRL_CCR_GO_MASK;
	while ((cache->CCR & CACHE64_CTRL_CCR_GO_MASK) != 0x00U) {
	}
	cache->CCR &= ~(CACHE64_CTRL_CCR_PUSHW0_MASK | CACHE64_CTRL_CCR_PUSHW1_MASK);
	/* Disable cache */
	cache->CCR &= ~CACHE64_CTRL_CCR_ENCACHE_MASK;
}

/*
 * XSPI interface helpers
 *
 * The reset part of this sequence mirrors the SDK's
 * XSPI_ResetSfmAndAhbDomain(): the module must be enabled to assert the
 * serial-flash and AHB domain resets, disabled to de-assert them, and each
 * transition needs a short settling delay -- six NOPs, the same count the SDK
 * uses. On top of that, this path also pulses IPS_TG_RST and clears the AHB
 * buffer, because it resumes an interface that lost power rather than
 * initialising a live one.
 *
 * MCR and SPTRCLR are Device memory, so writes to them are already ordered
 * against each other and need no barrier. Explicit barriers are only used
 * where a write must have completed before an untimed NOP delay is counted,
 * and once at the end before instruction fetch is handed back to the XSPI.
 */
static __ramfunc void init_xspi(XSPI_Type *base, CACHE64_CTRL_Type *cache)
{
	base->MCR |= XSPI_MCR_MDIS_MASK;
	base->MCR |= XSPI_MCR_IPS_TG_RST_MASK;

	base->MCR &= ~XSPI_MCR_MDIS_MASK;

	base->MCR |= XSPI_MCR_SWRSTSD_MASK | XSPI_MCR_SWRSTHD_MASK;
	__DSB();
	for (uint32_t i = 0U; i < 6U; i++) {
		__NOP();
	}
	base->MCR |= XSPI_MCR_MDIS_MASK;
	base->MCR &= ~(XSPI_MCR_SWRSTSD_MASK | XSPI_MCR_SWRSTHD_MASK);
	__DSB();
	for (uint32_t i = 0U; i < 6U; i++) {
		__NOP();
	}
	base->MCR &= ~XSPI_MCR_MDIS_MASK;

	base->MCR |= XSPI_MCR_MDIS_MASK;

	/* Clear AHB buffer */
	base->SPTRCLR |= XSPI_SPTRCLR_ABRT_CLR_MASK;
	while ((base->SPTRCLR & XSPI_SPTRCLR_ABRT_CLR_MASK) != 0UL) {
	}

	/* Clear AHB access sequence pointer */
	base->SPTRCLR |= XSPI_SPTRCLR_BFPTRC_MASK;

	/* Enable XSPI module */
	base->MCR &= ~XSPI_MCR_MDIS_MASK;

	if (xspi_cache_enabled) {
		enable_xspi_cache(cache);
	}

	__DSB();
	__ISB();
}

static __ramfunc void deinit_xspi(XSPI_Type *base, CACHE64_CTRL_Type *cache)
{
	xspi_cache_enabled = false;
	base->MCR &= ~XSPI_MCR_MDIS_MASK;

	while ((base->SR & XSPI_SR_BUSY_MASK) != 0U) {
	}

	if ((cache->CCR & CACHE64_CTRL_CCR_ENCACHE_MASK) != 0x00U) {
		xspi_cache_enabled = true;
		disable_xspi_cache(cache);
	}

	base->MCR |= XSPI_MCR_MDIS_MASK;
}

/*
 * Only the interface the running image executes from is torn down and rebuilt
 * here. Every other XSPI instance is an ordinary peripheral whose driver owns
 * its own suspend/resume; this hand-over exists solely because the CPU fetches
 * instructions through this one, which is also why it has to run from RAM.
 */
static __ramfunc void deinit_xip(void)
{
	if (POWER_IS_XIP(xspi0)) {
		CLKCTL0->PSCCTL1_SET = CLKCTL0_PSCCTL1_SET_XSPI0_MASK;
		deinit_xspi(XSPI0, CACHE64_CTRL0);
		CLKCTL0->PSCCTL1_CLR = CLKCTL0_PSCCTL1_SET_XSPI0_MASK;
	} else if (POWER_IS_XIP(xspi1)) {
		CLKCTL0->PSCCTL1_SET = CLKCTL0_PSCCTL1_SET_XSPI1_MASK;
		deinit_xspi(XSPI1, CACHE64_CTRL1);
		CLKCTL0->PSCCTL1_CLR = CLKCTL0_PSCCTL1_SET_XSPI1_MASK;
	}
}

static __ramfunc void init_xip(void)
{
	if (POWER_IS_XIP(xspi0)) {
		CLKCTL0->PSCCTL1_SET = CLKCTL0_PSCCTL1_SET_XSPI0_MASK;
		init_xspi(XSPI0, CACHE64_CTRL0);
	} else if (POWER_IS_XIP(xspi1)) {
		CLKCTL0->PSCCTL1_SET = CLKCTL0_PSCCTL1_SET_XSPI1_MASK;
		init_xspi(XSPI1, CACHE64_CTRL1);
	}
}

/*
 * Power control API
 *
 * Multi-level cache ordering: going down, the inner level (XCACHE, closest to
 * the core) is disabled first, so its dirty lines drain outwards while the XSPI
 * inline cache and the interface behind it are still live; the outer level goes
 * last. Coming back up the order is reversed, outer level first. Tearing down
 * outer-first would aim XCACHE write-backs at an already-disabled XSPI.
 */
__ramfunc void power_xip_suspend(bool flush_xcache0, bool flush_xcache1)
{
	xcache1_was_enabled = false;
	xcache0_was_enabled = false;

	if (flush_xcache1) {
		xcache1_was_enabled = ((XCACHE1->CCR & XCACHE_CCR_ENCACHE_MASK) != 0U);
		if (xcache1_was_enabled) {
			disable_xcache(XCACHE1);
		}
	}
	if (flush_xcache0) {
		xcache0_was_enabled = ((XCACHE0->CCR & XCACHE_CCR_ENCACHE_MASK) != 0U);
		if (xcache0_was_enabled) {
			disable_xcache(XCACHE0);
		}
	}

	deinit_xip();
}

__ramfunc void power_xip_resume(void)
{
	init_xip();

	if (xcache1_was_enabled) {
		enable_xcache(XCACHE1);
	}
	if (xcache0_was_enabled) {
		enable_xcache(XCACHE0);
	}
}
