/*
 * Copyright (c) 2022 Kamil Serwus
 * Copyright (c) 2023-2025 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Atmel SAMC MCU series initialization code
 */

/* GCLK Gen 0 -> GCLK_MAIN @ OSC48M
 * GCLK Gen 2 -> WDT       @ reserved
 * GCLK Gen 0 -> ADC       @ OSC48M
 * GCLK Gen 4 -> RTC       @ reserved
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <soc.h>

/* clang-format off */

static void flash_waitstates_init(void)
{
	/* Two wait state at 48 MHz. */
	NVMCTRL->CTRLB.bit.RWS = NVMCTRL_CTRLB_RWS_DUAL_Val;
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

void soc_reset_hook(void)
{
	flash_waitstates_init();
	osc48m_init();
	mclk_init();
	gclks_init();
}

/* clang-format on */
