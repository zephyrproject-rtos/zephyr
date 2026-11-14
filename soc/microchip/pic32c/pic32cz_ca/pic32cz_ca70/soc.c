/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cache.h>
#include <zephyr/cache.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <cmsis_core.h>
#include <soc.h>

#define PMC_NODE       DT_NODELABEL(pmc)
#define MAIN_XTAL_NODE DT_CLOCKS_CTLR_BY_NAME(PMC_NODE, main_xtal)

#define MAIN_XTAL_HZ DT_PROP(MAIN_XTAL_NODE, clock_frequency)
#define PLLA_MUL     DT_PROP(PMC_NODE, microchip_plla_mul)
#define PLLA_DIV     DT_PROP(PMC_NODE, microchip_plla_div)
#define MCK_PRES     DT_PROP(PMC_NODE, microchip_mck_prescaler)
#define MCK_DIV      DT_PROP(PMC_NODE, microchip_mck_divider)

#define PLLA_IN_HZ  (MAIN_XTAL_HZ / PLLA_DIV)
#define PLLA_OUT_HZ ((uint64_t)MAIN_XTAL_HZ * PLLA_MUL / PLLA_DIV)
#define HCLK_HZ     (PLLA_OUT_HZ / MCK_PRES)
#define MCK_HZ      (HCLK_HZ / MCK_DIV)

/* Data sheet: PLLA_1, PLLA_3, processor and MCK maximum frequencies. */
BUILD_ASSERT(PLLA_IN_HZ >= MHZ(8) && PLLA_IN_HZ <= MHZ(32), "PLLA input must be 8 to 32 MHz");
BUILD_ASSERT(PLLA_OUT_HZ >= MHZ(160) && PLLA_OUT_HZ <= MHZ(500),
	     "PLLA output must be 160 to 500 MHz");
BUILD_ASSERT(HCLK_HZ <= MHZ(300), "processor clock must not exceed 300 MHz");
BUILD_ASSERT(MCK_HZ <= MHZ(150), "MCK must not exceed 150 MHz");
BUILD_ASSERT(HCLK_HZ == DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency),
	     "cpu clock-frequency does not match the PMC configuration");

/* PMC_MCKR.PRES encoding: 1, 2, 4, 8, 16, 32, 64 -> 0..6 and 3 -> 7. */
#define MCK_PRES_VAL                                                                               \
	((MCK_PRES == 3)    ? 7U                                                                   \
	 : (MCK_PRES == 64) ? 6U                                                                   \
	 : (MCK_PRES == 32) ? 5U                                                                   \
	 : (MCK_PRES == 16) ? 4U                                                                   \
	 : (MCK_PRES == 8)  ? 3U                                                                   \
	 : (MCK_PRES == 4)  ? 2U                                                                   \
	 : (MCK_PRES == 2)  ? 1U                                                                   \
			    : 0U)

/* PMC_MCKR.MDIV encoding: 1, 2, 4, 3 -> 0..3. */
#define MCK_DIV_VAL ((MCK_DIV == 3) ? 3U : (MCK_DIV == 4) ? 2U : (MCK_DIV - 1U))

/*
 * Flash wait states (data sheet NVM_3/NVM_5, VDDIO 2.5 V to 3.6 V): each wait
 * state adds 23 MHz, up to 150 MHz with 6 wait states.
 */
#define FLASH_FWS MIN((MCK_HZ - 1U) / MHZ(23), 6U)

#define MOSCXT_STARTUP 0xFFU
#define PLLA_COUNT     0x3FU

static ALWAYS_INLINE void mck_wait_ready(pmc_registers_t *pmc)
{
	while ((pmc->PMC_SR & PMC_SR_MCKRDY_Msk) == 0U) {
	}
}

static ALWAYS_INLINE void mck_update(pmc_registers_t *pmc, uint32_t mask, uint32_t val)
{
	pmc->PMC_MCKR = (pmc->PMC_MCKR & ~mask) | val;
	mck_wait_ready(pmc);
}

static ALWAYS_INLINE void clock_init(void)
{
	pmc_registers_t *pmc = PMC_REGS;
	uint32_t mor;

	/* Run from the main clock while reconfiguring, e.g. after a warm reset. */
	if ((pmc->PMC_MCKR & PMC_MCKR_CSS_Msk) != PMC_MCKR_CSS_MAIN_CLK) {
		mck_update(pmc, PMC_MCKR_CSS_Msk, PMC_MCKR_CSS_MAIN_CLK);
	}

	/* Worst case wait states until MCK reaches its final frequency. */
	EFC_REGS->EEFC_FMR = EEFC_FMR_FWS(6) | EEFC_FMR_CLOE_Msk;

	/* Start the main crystal oscillator and select it as main clock. */
	mor = pmc->CKGR_MOR & ~CKGR_MOR_KEY_Msk;
	mor &= ~CKGR_MOR_MOSCXTBY_Msk;
	mor |= CKGR_MOR_KEY_PASSWD | CKGR_MOR_MOSCXTST(MOSCXT_STARTUP) | CKGR_MOR_MOSCRCEN_Msk |
	       CKGR_MOR_MOSCXTEN_Msk;
	pmc->CKGR_MOR = mor;
	while ((pmc->PMC_SR & PMC_SR_MOSCXTS_Msk) == 0U) {
	}

	pmc->CKGR_MOR = mor | CKGR_MOR_MOSCSEL_Msk;
	while ((pmc->PMC_SR & PMC_SR_MOSCSELS_Msk) == 0U) {
	}
	mck_wait_ready(pmc);

	/* Stop PLLA, then program and lock it. */
	pmc->CKGR_PLLAR = CKGR_PLLAR_ONE_Msk | CKGR_PLLAR_MULA(0U);
	pmc->CKGR_PLLAR = CKGR_PLLAR_ONE_Msk | CKGR_PLLAR_MULA(PLLA_MUL - 1U) |
			  CKGR_PLLAR_PLLACOUNT(PLLA_COUNT) | CKGR_PLLAR_DIVA(PLLA_DIV);
	while ((pmc->PMC_SR & PMC_SR_LOCKA_Msk) == 0U) {
	}

	/* Prescaler and divider first, PLLA as source last (data sheet PMC switching). */
	mck_update(pmc, PMC_MCKR_PRES_Msk, PMC_MCKR_PRES(MCK_PRES_VAL));
	mck_update(pmc, PMC_MCKR_MDIV_Msk, PMC_MCKR_MDIV(MCK_DIV_VAL));
	mck_update(pmc, PMC_MCKR_CSS_Msk, PMC_MCKR_CSS_PLLA_CLK);

	EFC_REGS->EEFC_FMR = EEFC_FMR_FWS(FLASH_FWS) | EEFC_FMR_CLOE_Msk;

	/* The main RC oscillator is no longer needed. */
	pmc->CKGR_MOR =
		(pmc->CKGR_MOR & ~(CKGR_MOR_KEY_Msk | CKGR_MOR_MOSCRCEN_Msk)) | CKGR_MOR_KEY_PASSWD;
}

void soc_reset_hook(void)
{
	/*
	 * The watchdog runs from reset and WDT_MR can be written only once, so
	 * leave it to the watchdog driver when one is enabled.
	 */
	if (!IS_ENABLED(CONFIG_WATCHDOG) || IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT)) {
		WDT_REGS->WDT_MR = WDT_MR_WDDIS_Msk;
	}
	RSWDT_REGS->RSWDT_MR = RSWDT_MR_WDDIS_Msk;

	/*
	 * The D-cache may be enabled by a bootloader or debugger; disable it so
	 * that the cache API can enable both caches from a known state.
	 */
	SCB_InvalidateDCache();
	SCB_DisableDCache();

	sys_cache_instr_enable();
	sys_cache_data_enable();

	clock_init();
}
