/*
 * Copyright (c) 2018, Christian Taedcke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common SoC initialization for the Silabs products
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <em_chip.h>
#include <em_cmu.h>
#include <em_emu.h>
#include <soc.h>
#include <cmsis_core.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

#ifdef CONFIG_CMU_HFCLK_HFXO
/**
 * @brief Initialization parameters for the external high frequency oscillator
 */
static CMU_HFXOInit_TypeDef hfxoInit = CMU_HFXOINIT_DEFAULT;
#endif

#ifdef CONFIG_CMU_NEED_LFXO
/**
 * @brief Initialization parameters for the external low frequency oscillator
 */
static CMU_LFXOInit_TypeDef lfxoInit = CMU_LFXOINIT_DEFAULT;

static void init_lfxo(void)
{
	/*
	 * Configuring LFXO disables it, so we can do that only if it's not
	 * used as a SYSCLK/HFCLK source.
	 */
	if (CMU_ClockSelectGet(cmuClock_HF) != cmuSelect_LFXO) {
		CMU_LFXOInit(&lfxoInit);
		CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
	}

	SystemLFXOClockSet(CONFIG_CMU_LFXO_FREQ);
}

#endif /* CONFIG_CMU_NEED_LFXO */

/**
 * @brief Initialize the system clock
 */
static ALWAYS_INLINE void clock_init(void)
{
#ifdef CONFIG_CMU_HFCLK_HFXO
	if (CMU_ClockSelectGet(cmuClock_HF) != cmuSelect_HFXO) {
		CMU_HFXOInit(&hfxoInit);
		CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
		CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
	}
	SystemHFXOClockSet(CONFIG_CMU_HFXO_FREQ);
	CMU_OscillatorEnable(cmuOsc_HFRCO, false, false);
#elif (defined CONFIG_CMU_HFCLK_LFXO)
	/* LFXO should've been already brought up by init_lfxo() */
	CMU_ClockSelectSet(cmuClock_HF, cmuSelect_LFXO);
	CMU_OscillatorEnable(cmuOsc_HFRCO, false, false);
#elif (defined CONFIG_CMU_HFCLK_HFRCO)
	/*
	 * This is the default clock, the controller starts with
	 */

#ifdef CONFIG_SOC_GECKO_HAS_HFRCO_FREQRANGE
	if (CONFIG_CMU_HFRCO_FREQ) {
		/* Setting system HFRCO frequency */
		CMU_HFRCOBandSet(CONFIG_CMU_HFRCO_FREQ);

		/* Using HFRCO as high frequency clock, HFCLK */
		CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFRCO);
	}
#endif
#else
#error "Unsupported clock source for HFCLK selected"
#endif

	/* Enable the High Frequency Peripheral Clock */
	CMU_ClockEnable(cmuClock_HFPER, true);

#if defined(CONFIG_GPIO_GECKO) || defined(CONFIG_LOG_BACKEND_SWO)
	CMU_ClockEnable(cmuClock_GPIO, true);
#endif
}

#ifdef CONFIG_SILABS_GECKO_EMU_DCDC
static ALWAYS_INLINE void dcdc_init(void)
{
#if defined(CONFIG_SILABS_GECKO_EMU_DCDC_MODE_UNCONFIGURED)
	/* Nothing to do, leave DC/DC converter in unconfigured, safe state. */
#elif defined(CONFIG_SILABS_GECKO_EMU_DCDC_MODE_ON) ||                                             \
	defined(CONFIG_SILABS_GECKO_EMU_DCDC_MODE_BYPASS)
	EMU_DCDCInit_TypeDef init_cfg = EMU_DCDCINIT_DEFAULT;
#if defined(CONFIG_SILABS_GECKO_EMU_DCDC_MODE_BYPASS)
	init_cfg.dcdcMode = emuDcdcMode_Bypass;
#endif
	EMU_DCDCInit(&init_cfg);
#elif defined(CONFIG_SILABS_GECKO_EMU_DCDC_MODE_OFF)
	EMU_DCDCPowerOff();
#else
#error "Unsupported power configuration mode of the on chip DC/DC converter."
#endif
}
#endif

#ifdef CONFIG_LOG_BACKEND_SWO
static void swo_init(void)
{
	struct soc_gpio_pin pin_swo = PIN_SWO;

	/* Select HFCLK as the debug trace clock */
	CMU->DBGCLKSEL = CMU_DBGCLKSEL_DBG_HFCLK;

#if defined(_GPIO_ROUTEPEN_MASK)
	/* Enable Serial wire output pin */
	GPIO->ROUTEPEN |= GPIO_ROUTEPEN_SWVPEN;
	/* Set SWO location */
	GPIO->ROUTELOC0 = SWO_LOCATION << _GPIO_ROUTELOC0_SWVLOC_SHIFT;
#else
	GPIO->ROUTE = GPIO_ROUTE_SWOPEN | (SWO_LOCATION << 8);
#endif

	GPIO_PinModeSet(pin_swo.port, pin_swo.pin, pin_swo.mode, pin_swo.out);
}
#endif /* CONFIG_LOG_BACKEND_SWO */

/**
 * @brief Perform basic hardware initialization
 *
 * Initialize the interrupt controller device drivers.
 * Also initialize the timer device driver, if required.
 *
 * @return 0
 */
void soc_early_init_hook(void)
{
	/* handle chip errata */
	CHIP_Init();

#ifdef CONFIG_CMU_NEED_LFXO
	init_lfxo();
#endif

#ifdef CONFIG_SILABS_GECKO_EMU_DCDC
	dcdc_init();
#endif

	/* Initialize system clock according to CONFIG_CMU settings */
	clock_init();

#ifdef CONFIG_LOG_BACKEND_SWO
	/* Configure SWO debug output */
	swo_init();
#endif
}
