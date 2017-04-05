/*
 * Copyright (c) 2017, Christian Taedcke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SoC initialization for the EFM32WG
 */

#include <kernel.h>
#include <init.h>
#include <soc.h>
#include <em_cmu.h>
#include <em_chip.h>
#include <arch/cpu.h>
#include <cortex_m/exc.h>

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
static ALWAYS_INLINE void clkInit(void)
{
#ifdef CONFIG_CMU_HFCLK_HFXO
	CMU_HFXOInit(&hfxoInit);
	CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
	CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
	CMU_OscillatorEnable(cmuOsc_HFRCO, false, false);
	SystemHFXOClockSet(CONFIG_CMU_HFXO_FREQ);
#elif (defined CONFIG_CMU_HFCLK_LFXO)
	CMU_LFXOInit(&lfxoInit);
	CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
	CMU_ClockSelectSet(cmuClock_HF, cmuSelect_LFXO);
	CMU_OscillatorEnable(cmuOsc_HFRCO, false, false);
	SystemLFXOClockSet(CONFIG_CMU_LFXO_FREQ);
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

#ifdef CONFIG_GPIO_GECKO
	CMU_ClockEnable(cmuClock_GPIO, true);
#endif
}

/**
 * @brief Perform basic hardware initialization
 *
 * Initialize the interrupt controller device drivers.
 * Also initialize the timer device driver, if required.
 *
 * @return 0
 */
static int silabs_efm32wg_init(struct device *arg)
{
	ARG_UNUSED(arg);

	int oldLevel; /* old interrupt lock level */

	/* disable interrupts */
	oldLevel = irq_lock();

	/* handle chip errata */
	CHIP_Init();

	_ClearFaults();

	/* Initialize system clock according to CONFIG_CMU settings */
	clkInit();

	/*
	 * install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	/* restore interrupt state */
	irq_unlock(oldLevel);
	return 0;
}

SYS_INIT(silabs_efm32wg_init, PRE_KERNEL_1, 0);
