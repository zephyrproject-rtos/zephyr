/*
 * Copyright (c) 2018, Christian Taedcke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common SoC initialization for the EXX32
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <soc.h>
#include <em_cmu.h>
#include <em_emu.h>
#include <em_chip.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

#ifdef CONFIG_CMU_HFCLK_HFXO
/**
 * @brief Initialization parameters for the external high frequency oscillator
 */
static CMU_HFXOInit_TypeDef hfxoInit = CMU_HFXOINIT_DEFAULT;
#elif (defined CONFIG_CMU_HFCLK_LFXO)
/**
 * @brief Initialization parameters for the external low frequency oscillator
 */
static CMU_LFXOInit_TypeDef lfxoInit = CMU_LFXOINIT_DEFAULT;
#endif

/**
 * @brief Initialize the system clock
 */
static ALWAYS_INLINE void clock_init(void)
{
#ifdef CONFIG_CMU_HFCLK_HFXO
#if defined(_SILICON_LABS_32B_SERIES_2)
	if (CMU_ClockSelectGet(cmuClock_SYSCLK) != cmuSelect_HFXO) {
		/*
		 * Check if device has HFXO configuration info in DEVINFO
		 * See AN0016.2
		 */
		if ((DEVINFO->MODULEINFO & DEVINFO_MODULEINFO_HFXOCALVAL) ==
				DEVINFO_MODULEINFO_HFXOCALVAL_VALID) {
			hfxoInit.ctuneXoAna = (DEVINFO->MODXOCAL
				& _DEVINFO_MODXOCAL_HFXOCTUNEXOANA_MASK)
				>> _DEVINFO_MODXOCAL_HFXOCTUNEXOANA_SHIFT;
			hfxoInit.ctuneXiAna = (DEVINFO->MODXOCAL
				& _DEVINFO_MODXOCAL_HFXOCTUNEXIANA_MASK)
				>> _DEVINFO_MODXOCAL_HFXOCTUNEXIANA_SHIFT;
		}

		CMU_HFXOInit(&hfxoInit);
		CMU_ClockSelectSet(cmuClock_SYSCLK, cmuSelect_HFXO);
	}

	SystemHFXOClockSet(CONFIG_CMU_HFXO_FREQ);
#else
	if (CMU_ClockSelectGet(cmuClock_HF) != cmuSelect_HFXO) {
		CMU_HFXOInit(&hfxoInit);
		CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
		CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
	}
	SystemHFXOClockSet(CONFIG_CMU_HFXO_FREQ);
	CMU_OscillatorEnable(cmuOsc_HFRCO, false, false);
#endif /* _SILICON_LABS_32B_SERIES_2 */
#elif (defined CONFIG_CMU_HFCLK_LFXO)
#if defined(_SILICON_LABS_32B_SERIES_2)
	if (CMU_ClockSelectGet(cmuClock_SYSCLK) != cmuSelect_LFXO) {
		/*
		 * Start the LFXO Oscillator as well (use by RTCC)
		 * Check if device has HFXO configuration info in DEVINFO
		 * See AN0016.2
		 */
		if ((DEVINFO->MODULEINFO & DEVINFO_MODULEINFO_LFXOCALVAL) ==
				DEVINFO_MODULEINFO_LFXOCALVAL_VALID) {
			lfxoInit.capTune = (DEVINFO->MODXOCAL
				& _DEVINFO_MODXOCAL_LFXOCAPTUNE_MASK)
				>> _DEVINFO_MODXOCAL_LFXOCAPTUNE_SHIFT;
		}
	}

	SystemLFXOClockSet(CONFIG_CMU_LFXO_FREQ);
#else
	if (CMU_ClockSelectGet(cmuClock_HF) != cmuSelect_LFXO) {
		CMU_LFXOInit(&lfxoInit);
		CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
		CMU_ClockSelectSet(cmuClock_HF, cmuSelect_LFXO);
	}
	SystemLFXOClockSet(CONFIG_CMU_LFXO_FREQ);
	CMU_OscillatorEnable(cmuOsc_HFRCO, false, false);
#endif /* _SILICON_LABS_32B_SERIES_2 */
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

#if defined(_SILICON_LABS_32B_SERIES_2)
		/* Enable the High Frequency Peripheral Clock */
		CMU_ClockEnable(cmuClock_PCLK, true);
#else
		/* Enable the High Frequency Peripheral Clock */
		CMU_ClockEnable(cmuClock_HFPER, true);
#endif /* _SILICON_LABS_32B_SERIES_2 */

#if defined(CONFIG_GPIO_GECKO) || defined(CONFIG_LOG_BACKEND_SWO)
	CMU_ClockEnable(cmuClock_GPIO, true);
#endif
}

#ifdef CONFIG_SOC_GECKO_EMU_DCDC
static ALWAYS_INLINE void dcdc_init(void)
{
#if defined(CONFIG_SOC_GECKO_EMU_DCDC_MODE_UNCONFIGURED)
	/* Nothing to do, leave DC/DC converter in unconfigured, safe state. */
#elif defined(CONFIG_SOC_GECKO_EMU_DCDC_MODE_ON) || defined(CONFIG_SOC_GECKO_EMU_DCDC_MODE_BYPASS)
	EMU_DCDCInit_TypeDef init_cfg = EMU_DCDCINIT_DEFAULT;
#if defined(CONFIG_SOC_GECKO_EMU_DCDC_MODE_BYPASS)
	init_cfg.dcdcMode = emuDcdcMode_Bypass;
#endif
	EMU_DCDCInit(&init_cfg);
#elif defined(CONFIG_SOC_GECKO_EMU_DCDC_MODE_OFF)
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

#if defined(_SILICON_LABS_32B_SERIES_2)
	GPIO->TRACEROUTEPEN = GPIO_TRACEROUTEPEN_SWVPEN;
#else
	/* Select HFCLK as the debug trace clock */
	CMU->DBGCLKSEL = CMU_DBGCLKSEL_DBG_HFCLK;

#if defined(_GPIO_ROUTEPEN_MASK)
	/* Enable Serial wire output pin */
	GPIO->ROUTEPEN |= GPIO_ROUTEPEN_SWVPEN;
	/* Set SWO location */
	GPIO->ROUTELOC0 =
		SWO_LOCATION << _GPIO_ROUTELOC0_SWVLOC_SHIFT;
#else
	GPIO->ROUTE = GPIO_ROUTE_SWOPEN | (SWO_LOCATION << 8);
#endif
#endif /* _SILICON_LABS_32B_SERIES_2 */

	soc_gpio_configure(&pin_swo);
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
static int silabs_exx32_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	unsigned int oldLevel; /* old interrupt lock level */

	/* disable interrupts */
	oldLevel = irq_lock();

	/* handle chip errata */
	CHIP_Init();

#ifdef CONFIG_SOC_GECKO_EMU_DCDC
	dcdc_init();
#endif

	/* Initialize system clock according to CONFIG_CMU settings */
	clock_init();

	/*
	 * install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

#ifdef CONFIG_LOG_BACKEND_SWO
	/* Configure SWO debug output */
	swo_init();
#endif

	/* restore interrupt state */
	irq_unlock(oldLevel);
	return 0;
}

SYS_INIT(silabs_exx32_init, PRE_KERNEL_1, 0);
