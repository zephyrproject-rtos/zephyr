/*
 * Copyright (c) 2021 Argentum Systems Ltd.
 * Copyright (c) 2023-2025 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Atmel SAML MCU series initialization code
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <cmsis_core.h>

/* clang-format off */

/* the SAML21 currently operates only in Performance Level 2... sleep
 * and low-power operation are not currently supported by the BSP
 *
 * the CPU clock will be configured to 48 MHz, and run via DFLL48M.
 *
 *   Reference -> GCLK Gen 1 -> DFLL48M -> GCLK Gen 0 -> GCLK_MAIN
 *
 * GCLK Gen 0 -> GCLK_MAIN @ 48 Mhz
 * GCLK Gen 1 -> DFLL48M (variable)
 * GCLK Gen 2 -> USB @ 48 MHz
 * GCLK Gen 3 -> ADC @ 24 MHz (further /2 in the ADC peripheral)
 * GCLK Gen 4 -> RTC @ reserved
 */

static inline void gclk_reset(void)
{
	/* by default, OSC16M will be enabled at 4 MHz, and the CPU will
	 * run from it. to permit initialization, the CPU is temporarily
	 * clocked from OSCULP32K, and OSC16M is disabled
	 */
	GCLK->GENCTRL[0].bit.SRC = GCLK_GENCTRL_SRC_OSCULP32K_Val;
	OSCCTRL->OSC16MCTRL.bit.ENABLE = 0;
}

#if !CONFIG_SOC_ATMEL_SAML_OSC32K
#define osc32k_init()
#else
static inline void osc32k_init(void)
{
	uint32_t cal;

	/* OSC32KCAL is in NVMCTRL_OTP5[12:6] */
	cal = *((uint32_t *)NVMCTRL_OTP5);
	cal >>= 6;
	cal &= (1 << 7) - 1;

	OSC32KCTRL->OSC32K.reg = 0
		|  OSC32KCTRL_OSC32K_CALIB(cal)
		|  OSC32KCTRL_OSC32K_STARTUP(0x5) /* 34 cycles / ~1.038ms */
		| !OSC32KCTRL_OSC32K_ONDEMAND
		|  OSC32KCTRL_OSC32K_RUNSTDBY
		|  OSC32KCTRL_OSC32K_EN32K
		|  OSC32KCTRL_OSC32K_EN1K
		|  OSC32KCTRL_OSC32K_ENABLE;

	/* wait for ready */
	while (!OSC32KCTRL->STATUS.bit.OSC32KRDY) {
	}
}
#endif

#if !CONFIG_SOC_ATMEL_SAML_XOSC32K
#define xosc32k_init()
#else
static inline void xosc32k_init(void)
{
	OSC32KCTRL->XOSC32K.reg = 0
		|  OSC32KCTRL_XOSC32K_STARTUP(0x1) /* 4096 cycles / ~0.13s */
		| !OSC32KCTRL_XOSC32K_ONDEMAND
		|  OSC32KCTRL_XOSC32K_RUNSTDBY
		|  OSC32KCTRL_XOSC32K_EN32K
		|  OSC32KCTRL_XOSC32K_EN1K
#if CONFIG_SOC_ATMEL_SAML_XOSC32K_CRYSTAL
		|  OSC32KCTRL_XOSC32K_XTALEN
#endif
		|  OSC32KCTRL_XOSC32K_ENABLE;

	/* wait for ready */
	while (!OSC32KCTRL->STATUS.bit.XOSC32KRDY) {
	}
}
#endif

#if !CONFIG_SOC_ATMEL_SAML_OSC16M
#define osc16m_init()
#else
static inline void osc16m_init(void)
{
	OSCCTRL->OSC16MCTRL.reg = 0
		| !OSCCTRL_OSC16MCTRL_ONDEMAND
		|  OSCCTRL_OSC16MCTRL_RUNSTDBY
		|  OSCCTRL_OSC16MCTRL_FSEL_16
		|  OSCCTRL_OSC16MCTRL_ENABLE;

	/* wait for ready */
	while (!OSCCTRL->STATUS.bit.OSC16MRDY) {
	}
}
#endif

/* TODO: use CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC ?? */
static inline void dfll48m_init(void)
{
	uint32_t cal;

	/* setup the reference clock (if any) */
	GCLK->GENCTRL[1].reg = 0
#if CONFIG_SOC_ATMEL_SAML_OSC32K_AS_MAIN
		|  GCLK_GENCTRL_SRC_OSC32K
#elif CONFIG_SOC_ATMEL_SAML_XOSC32K_AS_MAIN
		|  GCLK_GENCTRL_SRC_XOSC32K
#elif CONFIG_SOC_ATMEL_SAML_OSC16M_AS_MAIN
		/* configure Fout = Fin / 2^(DIV+1) = 31.25 kHz
		 * Fgclk_dfll48m_ref max is 33 kHz
		 */
		|  GCLK_GENCTRL_DIV(8)
		|  GCLK_GENCTRL_DIVSEL
		|  GCLK_GENCTRL_SRC_OSC16M
#endif
#if !CONFIG_SOC_ATMEL_SAML_OPENLOOP_AS_MAIN
		|  GCLK_GENCTRL_RUNSTDBY
		|  GCLK_GENCTRL_GENEN
#endif
		;

#if !CONFIG_SOC_ATMEL_SAML_OPENLOOP_AS_MAIN
	/* configure and enable the generator & peripheral channel */
	GCLK->PCHCTRL[0].reg = 0
		|  GCLK_PCHCTRL_CHEN
		|  GCLK_PCHCTRL_GEN_GCLK1;
#endif

	/* --- */

	/* if the target frequency is 48 MHz, then the calibration value can be used to
	 * decrease the time until the coarse lock is acquired. this is loaded from
	 * NVMCTRL_OTP5[31:26]
	 */
	cal = *((uint32_t *)NVMCTRL_OTP5);
	cal >>= 26;
	cal &= (1 << 6) - 1;

	OSCCTRL->DFLLCTRL.reg = 0
		|  OSCCTRL_DFLLCTRL_QLDIS
		| !OSCCTRL_DFLLCTRL_ONDEMAND
		|  OSCCTRL_DFLLCTRL_RUNSTDBY
#if !CONFIG_SOC_ATMEL_SAML_OPENLOOP_AS_MAIN
		|  OSCCTRL_DFLLCTRL_MODE
#endif
		;

	OSCCTRL->DFLLVAL.reg = 0
		|  OSCCTRL_DFLLVAL_COARSE(cal)
		|  OSCCTRL_DFLLVAL_FINE(512) /* use 50% */
		;

	OSCCTRL->DFLLMUL.reg = 0
		/* use 25% of maximum value for the coarse and fine step
		 * ... I couldn't find details on the inner workings of the DFLL, or any
		 * example values for these - I have seen others using ~50%. hopefully these
		 * values will provide a good balance between startup time and overshoot
		 */
		|  OSCCTRL_DFLLMUL_CSTEP(16)
		|  OSCCTRL_DFLLMUL_FSTEP(256)
#if CONFIG_SOC_ATMEL_SAML_OSC32K_AS_MAIN || CONFIG_SOC_ATMEL_SAML_XOSC32K_AS_MAIN
		/* use a 32.768 kHz reference ... 48e6 / 32,768 = 1,464.843... */
		|  OSCCTRL_DFLLMUL_MUL(1465)
#elif CONFIG_SOC_ATMEL_SAML_OSC16M_AS_MAIN
		/* use a 16 MHz -> 31.25 kHz reference... 48e6 / 31,250 = 1,536
		 * a small value can make the DFLL unstable, hence not using the
		 * 16 MHz source directly
		 */
		|  OSCCTRL_DFLLMUL_MUL(1536)
#endif
		;

	/* --- */

	/* enable */
	while (!OSCCTRL->STATUS.bit.DFLLRDY) {
	}
	OSCCTRL->DFLLCTRL.bit.ENABLE = 1;

#if !CONFIG_SOC_ATMEL_SAML_OPENLOOP_AS_MAIN
	/* wait for ready... note in open loop mode, we won't get a lock */
	while (!OSCCTRL->STATUS.bit.DFLLLCKC || !OSCCTRL->STATUS.bit.DFLLLCKF) {
	}
#endif
}

static inline void flash_waitstates_init(void)
{
	/* PL2, >= 2.7v, 48MHz = 2 wait states */
	NVMCTRL->CTRLB.bit.RWS = 2;
}

static inline void pm_init(void)
{
	PM->PLCFG.bit.PLDIS = 0;
	PM->PLCFG.bit.PLSEL = 2;
}

static inline void gclk_main_configure(void)
{
	/* finally, switch the CPU over to run from DFLL48M */
	GCLK->GENCTRL[0].bit.SRC = GCLK_GENCTRL_SRC_DFLL48M_Val;
}

#if !CONFIG_USB_DC_SAM0
#define gclk_usb_configure()
#else
static inline void gclk_usb_configure(void)
{
	GCLK->GENCTRL[2].reg = 0
		| GCLK_GENCTRL_SRC_DFLL48M
		| GCLK_GENCTRL_DIV(1)
		| GCLK_GENCTRL_GENEN;
}
#endif

#if !CONFIG_ADC_SAM0
#define gclk_adc_configure()
#else
static inline void gclk_adc_configure(void)
{
	GCLK->GENCTRL[3].reg = 0
		| GCLK_GENCTRL_SRC_DFLL48M
		| GCLK_GENCTRL_DIV(2)
		| GCLK_GENCTRL_GENEN;
}
#endif

#if CONFIG_SOC_ATMEL_SAML_DEBUG_PAUSE
static inline void pause_for_debug(void)
{
	/* for some reason, when attempting to flash / debug the target, the operations
	 * will time out... I suspect this is due to clock configuration, so instead of
	 * doing this immediately, we defer startup for a while to permit the debugger
	 * to jump in and interrupt us. ick
	 */
	for (uint32_t i = 0; i < 10000; i += 1) {
		__asm__ volatile ("nop\n");
	}
}
#else
static inline void pause_for_debug(void) {}
#endif

void soc_reset_hook(void)
{
	pause_for_debug();

	gclk_reset();
	osc32k_init();
	xosc32k_init();
	osc16m_init();
	dfll48m_init();
	flash_waitstates_init();
	pm_init();
	gclk_main_configure();
	gclk_usb_configure();
	gclk_adc_configure();
}

/* clang-format on */
