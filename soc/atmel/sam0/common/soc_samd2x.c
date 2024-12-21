/*
 * Copyright (c) 2017 Google LLC.
 * Copyright (c) 2023 Ionut Catalin Pavel <iocapa@iocapa.com>
 * Copyright (c) 2023 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Atmel SAMD MCU series initialization code
 */

/* The CPU clock will be configured to the DT requested value,
 * and run via DFLL48M.
 *
 * Reference -> GCLK Gen 1 -> DFLL48M -> GCLK Gen 0 -> GCLK_MAIN
 *
 * GCLK Gen 0 -> GCLK_MAIN
 * GCLK Gen 1 -> DFLL48M (variable)
 * GCLK Gen 2 -> WDT @ 32768 Hz
 * GCLK Gen 3 -> ADC @ 8 MHz
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <soc.h>
#include <cmsis_core.h>

/**
 * Fix different naming conventions for SAMD20
 */
#ifdef FUSES_OSC32KCAL_ADDR
#define FUSES_OSC32K_CAL_ADDR		FUSES_OSC32KCAL_ADDR
#define FUSES_OSC32K_CAL_Pos		FUSES_OSC32KCAL_Pos
#define FUSES_OSC32K_CAL_Msk		FUSES_OSC32KCAL_Msk
#endif

static inline void osc8m_init(void)
{
	uint32_t reg;

	/* Save calibration */
	reg = SYSCTRL->OSC8M.reg
	    & (SYSCTRL_OSC8M_FRANGE_Msk | SYSCTRL_OSC8M_CALIB_Msk);

	SYSCTRL->OSC8M.reg = reg
			   | SYSCTRL_OSC8M_RUNSTDBY
			   | SYSCTRL_OSC8M_PRESC(0) /* 8MHz (/1) */
			   | SYSCTRL_OSC8M_ENABLE;

	while (!SYSCTRL->PCLKSR.bit.OSC8MRDY) {
	}

	/* Use 8Mhz clock as gclk_main to allow switching between clocks
	 * when using bootloaders
	 */
	GCLK->GENDIV.reg = GCLK_GENDIV_ID(0)
			 | GCLK_GENDIV_DIV(0);

	while (GCLK->STATUS.bit.SYNCBUSY) {
	}

	GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(0)
			  | GCLK_GENCTRL_SRC_OSC8M
			  | GCLK_GENCTRL_IDC
			  | GCLK_GENCTRL_GENEN;

	while (GCLK->STATUS.bit.SYNCBUSY) {
	}
}

#if !CONFIG_SOC_ATMEL_SAMD_OSC32K || CONFIG_SOC_ATMEL_SAMD_DEFAULT_AS_MAIN
#define osc32k_init()
#else
static inline void osc32k_init(void)
{
	uint32_t cal;

	/* Get calibration value */
	cal = (*((uint32_t *)FUSES_OSC32K_CAL_ADDR)
	    & FUSES_OSC32K_CAL_Msk) >> FUSES_OSC32K_CAL_Pos;

	SYSCTRL->OSC32K.reg = SYSCTRL_OSC32K_CALIB(cal)
			    | SYSCTRL_OSC32K_STARTUP(0x5) /* 34 cycles / ~1ms */
			    | SYSCTRL_OSC32K_RUNSTDBY
			    | SYSCTRL_OSC32K_EN32K
			    | SYSCTRL_OSC32K_ENABLE;

	while (!SYSCTRL->PCLKSR.bit.OSC32KRDY) {
	}
}
#endif

#if !CONFIG_SOC_ATMEL_SAMD_XOSC || CONFIG_SOC_ATMEL_SAMD_DEFAULT_AS_MAIN
#define xosc_init()
#else
static inline void xosc_init(void)
{
	SYSCTRL->XOSC.reg = SYSCTRL_XOSC_STARTUP(0x5) /* 32 cycles / ~1ms */
			  | SYSCTRL_XOSC_RUNSTDBY
			  | SYSCTRL_XOSC_AMPGC
#if CONFIG_SOC_ATMEL_SAMD_XOSC_FREQ_HZ <= 2000000
			  | SYSCTRL_XOSC_GAIN(0x0)
#elif CONFIG_SOC_ATMEL_SAMD_XOSC_FREQ_HZ <= 4000000
			  | SYSCTRL_XOSC_GAIN(0x1)
#elif CONFIG_SOC_ATMEL_SAMD_XOSC_FREQ_HZ <= 8000000
			  | SYSCTRL_XOSC_GAIN(0x2)
#elif CONFIG_SOC_ATMEL_SAMD_XOSC_FREQ_HZ <= 16000000
			  | SYSCTRL_XOSC_GAIN(0x3)
#elif CONFIG_SOC_ATMEL_SAMD_XOSC_FREQ_HZ <= 32000000
			  | SYSCTRL_XOSC_GAIN(0x4)
#endif
#if CONFIG_SOC_ATMEL_SAMD_XOSC_CRYSTAL
			  | SYSCTRL_XOSC_XTALEN
#endif
			  | SYSCTRL_XOSC_ENABLE;

	while (!SYSCTRL->PCLKSR.bit.XOSCRDY) {
	}
}
#endif

#if !CONFIG_SOC_ATMEL_SAMD_XOSC32K || CONFIG_SOC_ATMEL_SAMD_DEFAULT_AS_MAIN
#define xosc32k_init()
#else
static inline void xosc32k_init(void)
{
	SYSCTRL->XOSC32K.reg = SYSCTRL_XOSC32K_STARTUP(0x1) /* 4096 cycles / ~0.13s */
			     | SYSCTRL_XOSC32K_RUNSTDBY
			     | SYSCTRL_XOSC32K_EN32K
			     | SYSCTRL_XOSC32K_AAMPEN
#if CONFIG_SOC_ATMEL_SAMD_XOSC32K_CRYSTAL
			     | SYSCTRL_XOSC32K_XTALEN
#endif
			     | SYSCTRL_XOSC32K_ENABLE;

	while (!SYSCTRL->PCLKSR.bit.XOSC32KRDY) {
	}
}
#endif

#if CONFIG_SOC_ATMEL_SAMD_DEFAULT_AS_MAIN
#define dfll48m_init()
#else
static inline void dfll48m_init(void)
{
	uint32_t fcal, ccal;

	GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(1)
#if CONFIG_SOC_ATMEL_SAMD_OSC32K_AS_MAIN
			  | GCLK_GENCTRL_SRC_OSC32K
#elif CONFIG_SOC_ATMEL_SAMD_XOSC32K_AS_MAIN
			  | GCLK_GENCTRL_SRC_XOSC32K
#elif CONFIG_SOC_ATMEL_SAMD_OSC8M_AS_MAIN
			  | GCLK_GENCTRL_SRC_OSC8M
#elif CONFIG_SOC_ATMEL_SAMD_XOSC_AS_MAIN
			  | GCLK_GENCTRL_SRC_XOSC
#endif
			  | GCLK_GENCTRL_IDC
			  | GCLK_GENCTRL_RUNSTDBY
			  | GCLK_GENCTRL_GENEN;

	while (GCLK->STATUS.bit.SYNCBUSY) {
	}

	GCLK->GENDIV.reg = GCLK_GENDIV_ID(1)
			 | GCLK_GENDIV_DIV(SOC_ATMEL_SAM0_GCLK1_DIV);

	while (GCLK->STATUS.bit.SYNCBUSY) {
	}

	/* Route multiplexer 0 to DFLL48M */
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(0)
			  | GCLK_CLKCTRL_GEN_GCLK1
			  | GCLK_CLKCTRL_CLKEN;


	SYSCTRL->DFLLCTRL.reg = SYSCTRL_DFLLCTRL_MODE
			      | SYSCTRL_DFLLCTRL_QLDIS
			      | SYSCTRL_DFLLCTRL_RUNSTDBY;

	/* Get calibration values */
	ccal = (*((uint32_t *)FUSES_DFLL48M_COARSE_CAL_ADDR)
	     & FUSES_DFLL48M_COARSE_CAL_Msk) >> FUSES_DFLL48M_COARSE_CAL_Pos;

	fcal = (*((uint32_t *)FUSES_DFLL48M_FINE_CAL_ADDR)
	     & FUSES_DFLL48M_FINE_CAL_Msk) >> FUSES_DFLL48M_FINE_CAL_Pos;

	SYSCTRL->DFLLVAL.reg = SYSCTRL_DFLLVAL_COARSE(ccal)
			     | SYSCTRL_DFLLVAL_FINE(fcal);

	/* Use half of maximum for both */
	SYSCTRL->DFLLMUL.reg = SYSCTRL_DFLLMUL_CSTEP(31)
			     | SYSCTRL_DFLLMUL_FSTEP(511)
			     | SYSCTRL_DFLLMUL_MUL(SOC_ATMEL_SAM0_DFLL48M_MUL);

	/* Enable */
	while (!SYSCTRL->PCLKSR.bit.DFLLRDY) {
	}
	SYSCTRL->DFLLCTRL.bit.ENABLE = 1;

	/* Wait for synchronization. */
	while (!SYSCTRL->PCLKSR.bit.DFLLLCKC || !SYSCTRL->PCLKSR.bit.DFLLLCKF) {
	}
}
#endif

#if CONFIG_SOC_ATMEL_SAMD_DEFAULT_AS_MAIN
#define flash_waitstates_init()
#else
static inline void flash_waitstates_init(void)
{
	NVMCTRL->CTRLB.bit.RWS = NVMCTRL_CTRLB_RWS(CONFIG_SOC_ATMEL_SAMD_NVM_WAIT_STATES);
}
#endif

#if CONFIG_SOC_ATMEL_SAMD_DEFAULT_AS_MAIN
#define gclk_main_configure()
#else
static inline void gclk_main_configure(void)
{
	GCLK->GENDIV.reg = GCLK_GENDIV_ID(0)
			 | GCLK_GENDIV_DIV(SOC_ATMEL_SAM0_GCLK0_DIV);

	while (GCLK->STATUS.bit.SYNCBUSY) {
	}

	GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(0)
			  | GCLK_GENCTRL_SRC_DFLL48M
			  | GCLK_GENCTRL_IDC
			  | GCLK_GENCTRL_GENEN;

	while (GCLK->STATUS.bit.SYNCBUSY) {
	}
}
#endif

#if !CONFIG_ADC_SAM0 || CONFIG_SOC_ATMEL_SAMD_DEFAULT_AS_MAIN
#define gclk_adc_configure()
#else
static inline void gclk_adc_configure(void)
{
	GCLK->GENDIV.reg = GCLK_GENDIV_ID(3)
			 | GCLK_GENDIV_DIV(SOC_ATMEL_SAM0_GCLK3_DIV);

	while (GCLK->STATUS.bit.SYNCBUSY) {
	}

	GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(3)
			  | GCLK_GENCTRL_SRC_DFLL48M
			  | GCLK_GENCTRL_IDC
			  | GCLK_GENCTRL_GENEN;

	while (GCLK->STATUS.bit.SYNCBUSY) {
	}
}
#endif

#if !CONFIG_WDT_SAM0
#define gclk_wdt_configure()
#else
static inline void gclk_wdt_configure(void)
{
	GCLK->GENDIV.reg = GCLK_GENDIV_ID(2)
			 | GCLK_GENDIV_DIV(4);

	GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(2)
			  | GCLK_GENCTRL_GENEN
			  | GCLK_GENCTRL_SRC_OSCULP32K
			  | GCLK_GENCTRL_DIVSEL;

	while (GCLK->STATUS.bit.SYNCBUSY) {
	}
}
#endif

#if CONFIG_SOC_ATMEL_SAMD_OSC8M || CONFIG_SOC_ATMEL_SAMD_DEFAULT_AS_MAIN
#define osc8m_disable()
#else
static inline void osc8m_disable(void)
{
	SYSCTRL->OSC8M.bit.ENABLE = 0;
}
#endif

void soc_reset_hook(void)
{
	osc8m_init();
	osc32k_init();
	xosc_init();
	xosc32k_init();
	dfll48m_init();
	flash_waitstates_init();
	gclk_main_configure();
	gclk_adc_configure();
	gclk_wdt_configure();
	osc8m_disable();
}
