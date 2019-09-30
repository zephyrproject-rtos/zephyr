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
#include <cortex_m/exc.h>

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
	 * This is the default clock, the controller starts with, so nothing to
	 * do here.
	 */
#else
#error "Unsupported clock source for HFCLK selected"
#endif

	/* Enable the High Frequency Peripheral Clock */
	CMU_ClockEnable(cmuClock_HFPER, true);

#ifdef CONFIG_LOG_BACKEND_SWO
	/* Select HFCLK as the debug trace clock */
	CMU->DBGCLKSEL = CMU_DBGCLKSEL_DBG_HFCLK;
#endif

#ifdef CONFIG_GPIO_GECKO
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

	/* restore interrupt state */
	irq_unlock(oldLevel);
	return 0;
}

SYS_INIT(silabs_exx32_init, PRE_KERNEL_1, 0);
