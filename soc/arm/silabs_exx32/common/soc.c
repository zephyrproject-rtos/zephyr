/*
 * Copyright (c) 2018, Christian Taedcke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common SoC initialization for the EXX32
 */

#include <kernel.h>
#include <init.h>
#include <soc.h>
#include <em_cmu.h>
#include <em_emu.h>
#include <em_chip.h>
#include <arch/cpu.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

#ifdef CONFIG_CMU_HFCLK_HFXO
/**
 * @brief Initialization parameters for the external high frequency oscillator
 */
static const CMU_HFXOInit_TypeDef hfxoInit = CMU_HFXOINIT_DEFAULT;
#elif (defined CONFIG_CMU_HFCLK_LFXO)
/**
 * @brief Initialization parameters for the external low frequency oscillator
 */
static const CMU_LFXOInit_TypeDef lfxoInit = CMU_LFXOINIT_DEFAULT;
#endif

/**
 * @brief Initialize the system clock
 *
 * @return N/A
 *
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
	if (CMU_ClockSelectGet(cmuClock_HF) != cmuSelect_LFXO) {
		CMU_LFXOInit(&lfxoInit);
		CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
		CMU_ClockSelectSet(cmuClock_HF, cmuSelect_LFXO);
	}
	SystemLFXOClockSet(CONFIG_CMU_LFXO_FREQ);
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

	/* Select HFCLK as the debug trace clock */
	CMU->DBGCLKSEL = CMU_DBGCLKSEL_DBG_HFCLK;

#if defined(_GPIO_ROUTEPEN_MASK)
	/* Enable Serial wire output pin */
	GPIO->ROUTEPEN |= GPIO_ROUTEPEN_SWVPEN;
	/* Set SWO location */
	GPIO->ROUTELOC0 =
		DT_GPIO_GECKO_SWO_LOCATION << _GPIO_ROUTELOC0_SWVLOC_SHIFT;
#else
	GPIO->ROUTE = GPIO_ROUTE_SWOPEN | (DT_GPIO_GECKO_SWO_LOCATION << 8);
#endif
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
static int silabs_exx32_init(struct device *arg)
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
