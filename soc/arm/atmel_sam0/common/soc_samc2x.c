/*
 * Copyright (c) 2022 Kamil Serwus
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Atmel SAMC MCU series initialization code
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <soc.h>

static void flash_waitstates_init(void)
{
	/* One wait state at 48 MHz. */
	NVMCTRL->CTRLB.bit.RWS = NVMCTRL_CTRLB_RWS_HALF_Val;
}

static void osc48m_init(void)
{
	/* Turn off the prescaler */
	OSCCTRL->OSC48MDIV.bit.DIV = 0;
	while (OSCCTRL->OSC48MSYNCBUSY.bit.OSC48MDIV) {
	}
	while (!OSCCTRL->STATUS.bit.OSC48MRDY) {
	}
}

static void mclk_init(void)
{
	MCLK->CPUDIV.reg = MCLK_CPUDIV_CPUDIV_DIV1_Val;
}

static void gclks_init(void)
{
	GCLK->GENCTRL[0].reg = GCLK_GENCTRL_SRC(GCLK_GENCTRL_SRC_OSC48M)
			     | GCLK_GENCTRL_DIV(1)
			     | GCLK_GENCTRL_GENEN;
}

static int atmel_samc_init(const struct device *arg)
{
	uint32_t key;

	ARG_UNUSED(arg);

	key = irq_lock();

	flash_waitstates_init();
	osc48m_init();
	mclk_init();
	gclks_init();

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	irq_unlock(key);

	return 0;
}

SYS_INIT(atmel_samc_init, PRE_KERNEL_1, 0);
