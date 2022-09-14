/*
 * Copyright (c) 2016 Piotr Mienkowski
 * Copyright (c) 2023 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM MCU family Power Management Controller (PMC) module
 * HAL driver.
 */

#ifndef _ATMEL_SAM_SOC_PMC_H_
#define _ATMEL_SAM_SOC_PMC_H_

#include <stdbool.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>
#include <soc.h>

/**
 * @brief Enable the clock of specified peripheral module.
 *
 * @param id   peripheral module id, as defined in data sheet.
 */
void soc_pmc_peripheral_enable(uint32_t id);

/**
 * @brief Disable the clock of specified peripheral module.
 *
 * @param id   peripheral module id, as defined in data sheet.
 */
void soc_pmc_peripheral_disable(uint32_t id);

/**
 * @brief Check if specified peripheral module is enabled.
 *
 * @param id   peripheral module id, as defined in data sheet.
 * @return     1 if peripheral is enabled, 0 otherwise
 */
uint32_t soc_pmc_peripheral_is_enabled(uint32_t id);

#if !defined(CONFIG_SOC_SERIES_SAM4L)

enum soc_pmc_fast_rc_freq {
#if defined(CKGR_MOR_MOSCRCF_4_MHz)
	SOC_PMC_FAST_RC_FREQ_4MHZ = CKGR_MOR_MOSCRCF_4_MHz,
#endif
#if defined(CKGR_MOR_MOSCRCF_8_MHz)
	SOC_PMC_FAST_RC_FREQ_8MHZ = CKGR_MOR_MOSCRCF_8_MHz,
#endif
#if defined(CKGR_MOR_MOSCRCF_12_MHz)
	SOC_PMC_FAST_RC_FREQ_12MHZ = CKGR_MOR_MOSCRCF_12_MHz,
#endif
};

enum soc_pmc_mck_src {
#if defined(PMC_MCKR_CSS_SLOW_CLK)
	SOC_PMC_MCK_SRC_SLOW_CLK = PMC_MCKR_CSS_SLOW_CLK,
#endif
#if defined(PMC_MCKR_CSS_MAIN_CLK)
	SOC_PMC_MCK_SRC_MAIN_CLK = PMC_MCKR_CSS_MAIN_CLK,
#endif
#if defined(PMC_MCKR_CSS_PLLA_CLK)
	SOC_PMC_MCK_SRC_PLLA_CLK = PMC_MCKR_CSS_PLLA_CLK,
#endif
#if defined(PMC_MCKR_CSS_PLLB_CLK)
	SOC_PMC_MCK_SRC_PLLB_CLK = PMC_MCKR_CSS_PLLB_CLK,
#endif
#if defined(PMC_MCKR_CSS_UPLL_CLK)
	SOC_PMC_MCK_SRC_UPLL_CLK = PMC_MCKR_CSS_UPLL_CLK,
#endif
};

/**
 * @brief Set the prescaler of the Master clock.
 *
 * @param prescaler the prescaler value.
 */
static ALWAYS_INLINE void soc_pmc_mck_set_prescaler(uint32_t prescaler)
{
	uint32_t reg_val;

	switch (prescaler) {
	case 1:
		reg_val = PMC_MCKR_PRES_CLK_1;
		break;
	case 2:
		reg_val = PMC_MCKR_PRES_CLK_2;
		break;
	case 4:
		reg_val = PMC_MCKR_PRES_CLK_4;
		break;
	case 8:
		reg_val = PMC_MCKR_PRES_CLK_8;
		break;
	case 16:
		reg_val = PMC_MCKR_PRES_CLK_16;
		break;
	case 32:
		reg_val = PMC_MCKR_PRES_CLK_32;
		break;
	case 64:
		reg_val = PMC_MCKR_PRES_CLK_64;
		break;
	case 3:
		reg_val = PMC_MCKR_PRES_CLK_3;
		break;
	default:
		__ASSERT(false, "Invalid MCK prescaler");
		reg_val = PMC_MCKR_PRES_CLK_1;
		break;
	}

	PMC->PMC_MCKR = (PMC->PMC_MCKR & (~PMC_MCKR_PRES_Msk)) | reg_val;

	while (!(PMC->PMC_SR & PMC_SR_MCKRDY)) {
	}
}

#if defined(CONFIG_SOC_SERIES_SAME70) || defined(CONFIG_SOC_SERIES_SAMV71)

/**
 * @brief Set the divider of the Master clock.
 *
 * @param divider the divider value.
 */
static ALWAYS_INLINE void soc_pmc_mck_set_divider(uint32_t divider)
{
	uint32_t reg_val;

	switch (divider) {
	case 1:
		reg_val = PMC_MCKR_MDIV_EQ_PCK;
		break;
	case 2:
		reg_val = PMC_MCKR_MDIV_PCK_DIV2;
		break;
	case 3:
		reg_val = PMC_MCKR_MDIV_PCK_DIV3;
		break;
	case 4:
		reg_val = PMC_MCKR_MDIV_PCK_DIV4;
		break;
	default:
		__ASSERT(false, "Invalid MCK divider");
		reg_val = PMC_MCKR_MDIV_EQ_PCK;
		break;
	}

	PMC->PMC_MCKR = (PMC->PMC_MCKR & (~PMC_MCKR_MDIV_Msk)) | reg_val;

	while (!(PMC->PMC_SR & PMC_SR_MCKRDY)) {
	}
}

#endif /* CONFIG_SOC_SERIES_SAME70 || CONFIG_SOC_SERIES_SAMV71 */

/**
 * @brief Set the source of the Master clock.
 *
 * @param source the source identifier.
 */
static ALWAYS_INLINE void soc_pmc_mck_set_source(enum soc_pmc_mck_src source)
{
	PMC->PMC_MCKR = (PMC->PMC_MCKR & (~PMC_MCKR_CSS_Msk)) | (uint32_t)source;

	while (!(PMC->PMC_SR & PMC_SR_MCKRDY)) {
	}
}

/**
 * @brief Switch main clock source selection to internal fast RC.
 *
 * @param freq the internal fast RC desired frequency 4/8/12MHz.
 */
static ALWAYS_INLINE void soc_pmc_switch_mainck_to_fastrc(enum soc_pmc_fast_rc_freq freq)
{
	/* Enable Fast RC oscillator but DO NOT switch to RC now */
	PMC->CKGR_MOR |= CKGR_MOR_KEY_PASSWD | CKGR_MOR_MOSCRCEN;

	/* Wait for the Fast RC to stabilize */
	while (!(PMC->PMC_SR & PMC_SR_MOSCRCS)) {
	}

	/* Change Fast RC oscillator frequency */
	PMC->CKGR_MOR = (PMC->CKGR_MOR & ~CKGR_MOR_MOSCRCF_Msk)
		      | CKGR_MOR_KEY_PASSWD
		      | (uint32_t)freq;

	/* Wait for the Fast RC to stabilize */
	while (!(PMC->PMC_SR & PMC_SR_MOSCRCS)) {
	}

	/* Switch to Fast RC */
	PMC->CKGR_MOR = (PMC->CKGR_MOR & ~CKGR_MOR_MOSCSEL)
		      | CKGR_MOR_KEY_PASSWD;
}

/**
 * @brief Enable internal fast RC oscillator.
 *
 * @param freq the internal fast RC desired frequency 4/8/12MHz.
 */
static ALWAYS_INLINE void soc_pmc_osc_enable_fastrc(enum soc_pmc_fast_rc_freq freq)
{
	/* Enable Fast RC oscillator but DO NOT switch to RC */
	PMC->CKGR_MOR |= CKGR_MOR_KEY_PASSWD | CKGR_MOR_MOSCRCEN;

	/* Wait for the Fast RC to stabilize */
	while (!(PMC->PMC_SR & PMC_SR_MOSCRCS)) {
	}

	/* Change Fast RC oscillator frequency */
	PMC->CKGR_MOR = (PMC->CKGR_MOR & ~CKGR_MOR_MOSCRCF_Msk)
		      | CKGR_MOR_KEY_PASSWD
		      | (uint32_t)freq;

	/* Wait for the Fast RC to stabilize */
	while (!(PMC->PMC_SR & PMC_SR_MOSCRCS)) {
	}
}

/**
 * @brief Disable internal fast RC oscillator.
 */
static ALWAYS_INLINE void soc_pmc_osc_disable_fastrc(void)
{
	/* Disable Fast RC oscillator */
	PMC->CKGR_MOR = (PMC->CKGR_MOR & ~CKGR_MOR_MOSCRCEN & ~CKGR_MOR_MOSCRCF_Msk)
		      | CKGR_MOR_KEY_PASSWD;
}

/**
 * @brief Check if the internal fast RC is ready.
 *
 * @return true if internal fast RC is ready, false otherwise
 */
static ALWAYS_INLINE bool soc_pmc_osc_is_ready_fastrc(void)
{
	return (PMC->PMC_SR & PMC_SR_MOSCRCS);
}

/**
 * @brief Enable the external crystal oscillator.
 *
 * @param xtal_startup_time crystal start-up time, in number of slow clocks.
 */
static ALWAYS_INLINE void soc_pmc_osc_enable_main_xtal(uint32_t xtal_startup_time)
{
	uint32_t mor = PMC->CKGR_MOR;

	mor &= ~(CKGR_MOR_MOSCXTBY | CKGR_MOR_MOSCXTEN);
	mor |= CKGR_MOR_KEY_PASSWD | CKGR_MOR_MOSCXTEN | CKGR_MOR_MOSCXTST(xtal_startup_time);

	PMC->CKGR_MOR = mor;

	/* Wait the main Xtal to stabilize */
	while (!(PMC->PMC_SR & PMC_SR_MOSCXTS)) {
	}
}

/**
 * @brief Bypass the external crystal oscillator.
 */
static ALWAYS_INLINE void soc_pmc_osc_bypass_main_xtal(void)
{
	uint32_t mor = PMC->CKGR_MOR;

	mor &= ~(CKGR_MOR_MOSCXTBY | CKGR_MOR_MOSCXTEN);
	mor |= CKGR_MOR_KEY_PASSWD | CKGR_MOR_MOSCXTBY;

	/* Enable Crystal oscillator but DO NOT switch now. Keep MOSCSEL to 0 */
	PMC->CKGR_MOR = mor;
	/* The MOSCXTS in PMC_SR is automatically set */
}

/**
 * @brief Disable the external crystal oscillator.
 */
static ALWAYS_INLINE void soc_pmc_osc_disable_main_xtal(void)
{
	uint32_t mor = PMC->CKGR_MOR;

	mor &= ~(CKGR_MOR_MOSCXTBY | CKGR_MOR_MOSCXTEN);

	PMC->CKGR_MOR = CKGR_MOR_KEY_PASSWD | mor;
}

/**
 * @brief Check if the external crystal oscillator is bypassed.
 *
 * @return true if external crystal oscillator is bypassed, false otherwise
 */
static ALWAYS_INLINE bool soc_pmc_osc_is_bypassed_main_xtal(void)
{
	return (PMC->CKGR_MOR & CKGR_MOR_MOSCXTBY);
}

/**
 * @brief Check if the external crystal oscillator is ready.
 *
 * @return true if external crystal oscillator is ready, false otherwise
 */
static ALWAYS_INLINE bool soc_pmc_osc_is_ready_main_xtal(void)
{
	return (PMC->PMC_SR & PMC_SR_MOSCXTS);
}

/**
 * @brief Switch main clock source selection to external crystal oscillator.
 *
 * @param bypass select bypass or xtal
 * @param xtal_startup_time crystal start-up time, in number of slow clocks
 */
static ALWAYS_INLINE void soc_pmc_switch_mainck_to_xtal(bool bypass, uint32_t xtal_startup_time)
{
	soc_pmc_osc_enable_main_xtal(xtal_startup_time);

	/* Enable Main Xtal oscillator */
	if (bypass) {
		PMC->CKGR_MOR = (PMC->CKGR_MOR & ~CKGR_MOR_MOSCXTEN)
			      | CKGR_MOR_KEY_PASSWD
			      | CKGR_MOR_MOSCXTBY
			      | CKGR_MOR_MOSCSEL;
	} else {
		PMC->CKGR_MOR = (PMC->CKGR_MOR & ~CKGR_MOR_MOSCXTBY)
			      | CKGR_MOR_KEY_PASSWD
			      | CKGR_MOR_MOSCXTEN
			      | CKGR_MOR_MOSCXTST(xtal_startup_time);

		/* Wait for the Xtal to stabilize */
		while (!(PMC->PMC_SR & PMC_SR_MOSCXTS)) {
		}

		PMC->CKGR_MOR |= CKGR_MOR_KEY_PASSWD | CKGR_MOR_MOSCSEL;
	}
}

/**
 * @brief Disable the external crystal oscillator.
 *
 * @param bypass select bypass or xtal
 */
static ALWAYS_INLINE void soc_pmc_osc_disable_xtal(bool bypass)
{
	/* Disable xtal oscillator */
	if (bypass) {
		PMC->CKGR_MOR = (PMC->CKGR_MOR & ~CKGR_MOR_MOSCXTBY)
			      | CKGR_MOR_KEY_PASSWD;
	} else {
		PMC->CKGR_MOR = (PMC->CKGR_MOR & ~CKGR_MOR_MOSCXTEN)
			      | CKGR_MOR_KEY_PASSWD;
	}
}

/**
 * @brief Check if the main clock is ready. Depending on MOSCEL, main clock can be one
 * of external crystal, bypass or internal RC.
 *
 * @return true if main clock is ready, false otherwise
 */
static ALWAYS_INLINE bool soc_pmc_osc_is_ready_mainck(void)
{
	return PMC->PMC_SR & PMC_SR_MOSCSELS;
}

/**
 * @brief Enable Wait Mode.
 */
static ALWAYS_INLINE void soc_pmc_enable_waitmode(void)
{
	PMC->PMC_FSMR |= PMC_FSMR_LPM;
}

/**
 * @brief Enable Clock Failure Detector.
 */
static ALWAYS_INLINE void soc_pmc_enable_clock_failure_detector(void)
{
	uint32_t mor = PMC->CKGR_MOR;

	PMC->CKGR_MOR = CKGR_MOR_KEY_PASSWD | CKGR_MOR_CFDEN | mor;
}

/**
 * @brief Disable Clock Failure Detector.
 */
static ALWAYS_INLINE void soc_pmc_disable_clock_failure_detector(void)
{
	uint32_t mor = PMC->CKGR_MOR & (~CKGR_MOR_CFDEN);

	PMC->CKGR_MOR = CKGR_MOR_KEY_PASSWD | mor;
}

#if defined(PMC_MCKR_CSS_PLLA_CLK)

/**
 * @brief Disable the PLLA clock.
 */
static ALWAYS_INLINE void soc_pmc_disable_pllack(void)
{
	PMC->CKGR_PLLAR = CKGR_PLLAR_ONE | CKGR_PLLAR_MULA(0);
}

/**
 * @brief Enable the PLLA clock.
 *
 * @param mula PLLA multiplier
 * @param pllacount PLLA lock counter, in number of slow clocks
 * @param diva PLLA Divider
 */
static ALWAYS_INLINE void soc_pmc_enable_pllack(uint32_t mula, uint32_t pllacount, uint32_t diva)
{
	__ASSERT(diva > 0, "Invalid PLLA divider");

	/* first disable the PLL to unlock the lock */
	soc_pmc_disable_pllack();

	PMC->CKGR_PLLAR = CKGR_PLLAR_ONE
			| CKGR_PLLAR_DIVA(diva)
			| CKGR_PLLAR_PLLACOUNT(pllacount)
			| CKGR_PLLAR_MULA(mula);

	while ((PMC->PMC_SR & PMC_SR_LOCKA) == 0) {
	}
}

/**
 * @brief Check if the PLLA is locked.
 *
 * @return true if PLLA is locked, false otherwise
 */
static ALWAYS_INLINE bool soc_pmc_is_locked_pllack(void)
{
	return (PMC->PMC_SR & PMC_SR_LOCKA);
}

#endif /* PMC_MCKR_CSS_PLLA_CLK */

#if defined(PMC_MCKR_CSS_PLLB_CLK)

/**
 * @brief Disable the PLLB clock.
 */
static ALWAYS_INLINE void soc_pmc_disable_pllbck(void)
{
	PMC->CKGR_PLLBR = CKGR_PLLBR_MULB(0);
}

/**
 * @brief Enable the PLLB clock.
 *
 * @param mulb PLLB multiplier
 * @param pllbcount PLLB lock counter, in number of slow clocks
 * @param divb PLLB Divider
 */
static ALWAYS_INLINE void soc_pmc_enable_pllbck(uint32_t mulb, uint32_t pllbcount, uint32_t divb)
{
	__ASSERT(divb > 0, "Invalid PLLB divider");

	/* first disable the PLL to unlock the lock */
	soc_pmc_disable_pllbck();

	PMC->CKGR_PLLBR = CKGR_PLLBR_DIVB(divb)
			| CKGR_PLLBR_PLLBCOUNT(pllbcount)
			| CKGR_PLLBR_MULB(mulb);

	while ((PMC->PMC_SR & PMC_SR_LOCKB) == 0) {
	}
}

/**
 * @brief Check if the PLLB is locked.
 *
 * @return true if PLLB is locked, false otherwise
 */
static ALWAYS_INLINE bool soc_pmc_is_locked_pllbck(void)
{
	return (PMC->PMC_SR & PMC_SR_LOCKB);
}

#endif /* PMC_MCKR_CSS_PLLB_CLK */

#if defined(PMC_MCKR_CSS_UPLL_CLK)

/**
 * @brief Enable the UPLL clock.
 */
static ALWAYS_INLINE void soc_pmc_enable_upllck(uint32_t upllcount)
{
	PMC->CKGR_UCKR = CKGR_UCKR_UPLLCOUNT(upllcount)
		       | CKGR_UCKR_UPLLEN;

	/* Wait UTMI PLL Lock Status */
	while (!(PMC->PMC_SR & PMC_SR_LOCKU)) {
	}
}

/**
 * @brief Disable the UPLL clock.
 */
static ALWAYS_INLINE void soc_pmc_disable_upllck(void)
{
	PMC->CKGR_UCKR &= ~CKGR_UCKR_UPLLEN;
}

/**
 * @brief Check if the UPLL is locked.
 *
 * @return true if UPLL is locked, false otherwise
 */
static ALWAYS_INLINE bool soc_pmc_is_locked_upllck(void)
{
	return (PMC->PMC_SR & PMC_SR_LOCKU);
}

#endif /* PMC_MCKR_CSS_UPLL_CLK */

#endif /* !CONFIG_SOC_SERIES_SAM4L */

#endif /* _ATMEL_SAM_SOC_PMC_H_ */
