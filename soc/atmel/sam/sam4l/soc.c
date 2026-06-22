/*
 * Copyright (c) 2020-2025 Gerson Fernando Budke <nandojve@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Atmel SAM4L MCU series initialization code
 *
 * This module provides routines to initialize and support board-level hardware
 * for the Atmel SAM4L series processor.
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/sys/util.h>

#define XTAL_FREQ 12000000
#define NR_PLLS 1
#define PLL_MAX_STARTUP_CYCLES (SCIF_PLL_PLLCOUNT_Msk >> SCIF_PLL_PLLCOUNT_Pos)

/**
 * Fcpu = 48MHz
 * Fpll = (Fclk * PLL_mul) / PLL_div
 */
#define PLL0_MUL        (192000000 / XTAL_FREQ)
#define PLL0_DIV         4

static inline bool pll_is_locked(uint32_t pll_id)
{
	return !!(SCIF->PCLKSR & (1U << (6 + pll_id)));
}

static inline bool osc_is_ready(uint8_t id)
{
	switch (id) {
	case OSC_ID_OSC0:
		return !!(SCIF->PCLKSR & SCIF_PCLKSR_OSC0RDY);
	case OSC_ID_OSC32:
		return !!(BSCIF->PCLKSR & BSCIF_PCLKSR_OSC32RDY);
	case OSC_ID_RC32K:
		return !!(BSCIF->RC32KCR & (BSCIF_RC32KCR_EN));
	case OSC_ID_RC80M:
		return !!(SCIF->RC80MCR & (SCIF_RC80MCR_EN));
	case OSC_ID_RCFAST:
		return !!(SCIF->RCFASTCFG & (SCIF_RCFASTCFG_EN));
	case OSC_ID_RC1M:
		return !!(BSCIF->RC1MCR & (BSCIF_RC1MCR_CLKOE));
	case OSC_ID_RCSYS:
		/* RCSYS is always ready */
		return true;
	default:
		/* unhandled_case(id); */
		return false;
	}
}

/**
 * Enable Backup System Control Oscilator RC32K
 */
static inline void osc_priv_enable_rc32k(void)
{
	uint32_t temp = BSCIF->RC32KCR;
	uint32_t addr = (uint32_t)&BSCIF->RC32KCR - (uint32_t)BSCIF;

	BSCIF->UNLOCK = BSCIF_UNLOCK_KEY(0xAAu)
		      | BSCIF_UNLOCK_ADDR(addr);
	BSCIF->RC32KCR = temp | BSCIF_RC32KCR_EN32K | BSCIF_RC32KCR_EN;
}

/**
 * The PLL options #PLL_OPT_VCO_RANGE_HIGH and #PLL_OPT_OUTPUT_DIV will
 * be set automatically based on the calculated target frequency.
 */
static inline uint32_t pll_config_init(uint32_t divide, uint32_t mul)
{
#define SCIF0_PLL_VCO_RANGE1_MAX_FREQ   240000000
#define SCIF_PLL0_VCO_RANGE1_MIN_FREQ   160000000
#define SCIF_PLL0_VCO_RANGE0_MAX_FREQ   180000000
#define SCIF_PLL0_VCO_RANGE0_MIN_FREQ    80000000
/* VCO frequency range is 160-240 MHz (80-180 MHz if unset) */
#define PLL_OPT_VCO_RANGE_HIGH    0
/* Divide output frequency by two */
#define PLL_OPT_OUTPUT_DIV        1
/* The threshold above which to set the #PLL_OPT_VCO_RANGE_HIGH option */
#define PLL_VCO_LOW_THRESHOLD     ((SCIF_PLL0_VCO_RANGE1_MIN_FREQ \
	+ SCIF_PLL0_VCO_RANGE0_MAX_FREQ) / 2)
#define PLL_MIN_HZ 40000000
#define PLL_MAX_HZ 240000000
#define MUL_MIN    2
#define MUL_MAX    16
#define DIV_MIN    0
#define DIV_MAX    15

	uint32_t pll_value;
	uint32_t vco_hz;

	/* Calculate internal VCO frequency */
	vco_hz = XTAL_FREQ * mul;
	vco_hz /= divide;

	pll_value = 0;

	/* Bring the internal VCO frequency up to the minimum value */
	if ((vco_hz < PLL_MIN_HZ * 2) && (mul <= 8)) {
		mul *= 2;
		vco_hz *= 2;
		pll_value |= (1U << (SCIF_PLL_PLLOPT_Pos +
				     PLL_OPT_OUTPUT_DIV));
	}

	/* Set VCO frequency range according to calculated value */
	if (vco_hz >= PLL_VCO_LOW_THRESHOLD) {
		pll_value |= 1U << (SCIF_PLL_PLLOPT_Pos +
				    PLL_OPT_VCO_RANGE_HIGH);
	}

	pll_value |= ((mul - 1) << SCIF_PLL_PLLMUL_Pos) |
		      (divide << SCIF_PLL_PLLDIV_Pos) |
		      (PLL_MAX_STARTUP_CYCLES << SCIF_PLL_PLLCOUNT_Pos);

	return pll_value;
}

static inline void flashcalw_set_wait_state(uint32_t wait_state)
{
	HFLASHC->FCR = (HFLASHC->FCR & ~FLASHCALW_FCR_FWS) |
			(wait_state ?
			 FLASHCALW_FCR_FWS_1 :
			 FLASHCALW_FCR_FWS_0);
}

static inline bool flashcalw_is_ready(void)
{
	return ((HFLASHC->FSR & FLASHCALW_FSR_FRDY) != 0);
}

static inline void flashcalw_issue_command(uint32_t command, int page_number)
{
	uint32_t time;

	flashcalw_is_ready();
	time = HFLASHC->FCMD;

	/* Clear the command bitfield. */
	time &= ~FLASHCALW_FCMD_CMD_Msk;
	if (page_number >= 0) {
		time = (FLASHCALW_FCMD_KEY_KEY |
			FLASHCALW_FCMD_PAGEN(page_number) | command);
	} else {
		time |= (FLASHCALW_FCMD_KEY_KEY | command);
	}

	HFLASHC->FCMD = time;
	flashcalw_is_ready();
}

/**
 * @brief Setup various clock on SoC at boot time.
 *
 * Setup the SoC clocks according to section 28.12 in datasheet.
 *
 * Setup Slow, Main, PLLA, Processor and Master clocks during the device boot.
 * It is assumed that the relevant registers are at their reset value.
 */
static ALWAYS_INLINE void clock_init(void)
{
	uint32_t gen_clk_conf;

	/* Disable PicoCache and Enable HRAMC1 as extended RAM */
	soc_pmc_peripheral_enable(
		PM_CLOCK_MASK(PM_CLK_GRP_HSB, SYSCLK_HRAMC1_DATA));
	soc_pmc_peripheral_enable(
		PM_CLOCK_MASK(PM_CLK_GRP_PBB, SYSCLK_HRAMC1_REGS));

	HCACHE->CTRL = HCACHE_CTRL_CEN_NO;

	while (HCACHE->SR & HCACHE_SR_CSTS_EN) {
		;
	}

	/* Enable PLL */
	if (!pll_is_locked(0)) {
		/* This assumes external 12MHz Crystal */
		SCIF->UNLOCK = SCIF_UNLOCK_KEY(0xAAu) |
				SCIF_UNLOCK_ADDR((uint32_t)&SCIF->OSCCTRL0 -
						 (uint32_t)SCIF);
		SCIF->OSCCTRL0 = SCIF_OSCCTRL0_STARTUP(2) |
					SCIF_OSCCTRL0_GAIN(3) |
					SCIF_OSCCTRL0_MODE |
					SCIF_OSCCTRL0_OSCEN;

		while (!osc_is_ready(OSC_ID_OSC0)) {
			;
		}
		uint32_t pll_config = pll_config_init(PLL0_DIV,
						      PLL0_MUL);

		SCIF->UNLOCK = SCIF_UNLOCK_KEY(0xAAu) |
			       SCIF_UNLOCK_ADDR((uint32_t)&SCIF->PLL[0] -
						(uint32_t)SCIF);
		SCIF->PLL[0] = pll_config | SCIF_PLL_PLLEN;

		while (!pll_is_locked(0)) {
			;
		}
	}

	/** Set a flash wait state depending on the new cpu frequency.
	 */
	flashcalw_set_wait_state(1);
	flashcalw_issue_command(FLASHCALW_FCMD_CMD_HSEN, -1);

	/** Set Clock CPU/BUS dividers
	 */
	PM->UNLOCK = PM_UNLOCK_KEY(0xAAu) |
		     PM_UNLOCK_ADDR((uint32_t)&PM->CPUSEL - (uint32_t)PM);
	PM->CPUSEL = PM_CPUSEL_CPUSEL(0);

	PM->UNLOCK = PM_UNLOCK_KEY(0xAAu) |
		     PM_UNLOCK_ADDR((uint32_t)&PM->PBASEL - (uint32_t)PM);
	PM->PBASEL = PM_PBASEL_PBSEL(0);

	PM->UNLOCK = PM_UNLOCK_KEY(0xAAu) |
		     PM_UNLOCK_ADDR((uint32_t)&PM->PBBSEL - (uint32_t)PM);
	PM->PBBSEL = PM_PBBSEL_PBSEL(0);

	PM->UNLOCK = PM_UNLOCK_KEY(0xAAu) |
		     PM_UNLOCK_ADDR((uint32_t)&PM->PBCSEL - (uint32_t)PM);
	PM->PBCSEL = PM_PBCSEL_PBSEL(0);

	PM->UNLOCK = PM_UNLOCK_KEY(0xAAu) |
		     PM_UNLOCK_ADDR((uint32_t)&PM->PBDSEL - (uint32_t)PM);
	PM->PBDSEL = PM_PBDSEL_PBSEL(0);

	/** Set PLL0 as source clock
	 */
	PM->UNLOCK = PM_UNLOCK_KEY(0xAAu) |
		     PM_UNLOCK_ADDR((uint32_t)&PM->MCCTRL - (uint32_t)PM);
	PM->MCCTRL = OSC_SRC_PLL0;

	/** Enable RC32K Oscilator */
	osc_priv_enable_rc32k();
	while (!osc_is_ready(OSC_ID_RC32K)) {
		;
	}

	/** Enable Generic Clock 5
	 *   Source: RC32K
	 *      Div: 32
	 *      Clk: 1024 Hz
	 *  The GCLK-5 can be used by GLOC, TC0 and RC32KIFB_REF
	 */
	gen_clk_conf = SCIF_GCCTRL_RESETVALUE;
	gen_clk_conf |= SCIF_GCCTRL_OSCSEL(GEN_CLK_SRC_RC32K);
	gen_clk_conf |= SCIF_GCCTRL_DIVEN;
	gen_clk_conf |= SCIF_GCCTRL_DIV(((32 + 1) / 2) - 1);
	SCIF->GCCTRL[GEN_CLK_TC0_GLOC_RC32] = gen_clk_conf | SCIF_GCCTRL_CEN;
}

void soc_reset_hook(void)
{
	/* Setup system clocks. */
	clock_init();
}
