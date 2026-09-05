/*
 * Copyright 2025 NXP
 * Copyright (c) 2026 CogniPilot Foundation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <fsl_clock.h>
#include <fsl_iomuxc.h>
#include <fsl_gpio.h>
#include <fsl_xbara.h>
#include <soc.h>

#include <stdint.h>

/*
 * Simple early delay using the DWT cycle counter. Safe in
 * board_early_init_hook because it does not depend on Zephyr timers.
 */
static void early_delay_us(uint32_t us)
{
	/* SystemCoreClock is usually set very early on i.MX RT */
	uint32_t cycles = (SystemCoreClock / 1000000U) * us;

	/* Enable DWT CYCCNT */
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

	uint32_t start = DWT->CYCCNT;

	while ((DWT->CYCCNT - start) < cycles) {
		/* busy wait */
	}
}

#define STRAP_PAD_CFG_INPUT_NOPULL (0x10B0u)
#define STRAP_PAD_CFG_OUTPUT       (0x10B0u)

void board_early_init_hook(void)
{
	/*
	 * Errata 50235 requires the LPUART clock to be enabled before
	 * selecting CAN_CLK_SELECT 2.
	 */
	CLOCK_EnableClock(kCLOCK_Lpuart1);
	CLOCK_EnableClock(kCLOCK_Timer3);
	CLOCK_EnableClock(kCLOCK_Xbar1);

	/* GPIO_B1_11 -> GPIO2_IO27  (Open / input) */
	IOMUXC_SetPinMux(IOMUXC_GPIO_B1_11_GPIO2_IO27, 0U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_11_GPIO2_IO27, STRAP_PAD_CFG_INPUT_NOPULL);

	/* GPIO_B1_05 -> GPIO2_IO21  (Drive High) */
	IOMUXC_SetPinMux(IOMUXC_GPIO_B1_05_GPIO2_IO21, 0U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_05_GPIO2_IO21, STRAP_PAD_CFG_OUTPUT);

	/* GPIO_B1_04 -> GPIO2_IO20  (Open / input) */
	IOMUXC_SetPinMux(IOMUXC_GPIO_B1_04_GPIO2_IO20, 0U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_04_GPIO2_IO20, STRAP_PAD_CFG_INPUT_NOPULL);

	/* GPIO_B1_06 -> GPIO2_IO22  (Drive Low) */
	IOMUXC_SetPinMux(IOMUXC_GPIO_B1_06_GPIO2_IO22, 0U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_06_GPIO2_IO22, STRAP_PAD_CFG_OUTPUT);

	/* Reset pin: GPIO_B0_14 -> GPIO2_IO14 */
	IOMUXC_SetPinMux(IOMUXC_GPIO_B0_14_GPIO2_IO14, 0U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B0_14_GPIO2_IO14, STRAP_PAD_CFG_OUTPUT);

	gpio_pin_config_t cfg_in = {.direction = kGPIO_DigitalInput,
				    .outputLogic = 0U,
				    .interruptMode = kGPIO_NoIntmode};

	gpio_pin_config_t cfg_out_hi = {.direction = kGPIO_DigitalOutput,
					.outputLogic = 1U,
					.interruptMode = kGPIO_NoIntmode};

	gpio_pin_config_t cfg_out_lo = {.direction = kGPIO_DigitalOutput,
					.outputLogic = 0U,
					.interruptMode = kGPIO_NoIntmode};

	/* Strap pins */
	GPIO_PinInit(GPIO2, 27U, &cfg_in);     /* Open */
	GPIO_PinInit(GPIO2, 21U, &cfg_out_hi); /* High */
	GPIO_PinInit(GPIO2, 20U, &cfg_in);     /* Open */
	GPIO_PinInit(GPIO2, 22U, &cfg_out_lo); /* Low */
	GPIO_PinInit(GPIO2, 14U, &cfg_out_lo); /* assert reset (low) */

	early_delay_us(1000U); /* 1ms assert */

	/* Release reset */
	GPIO_WritePinOutput(GPIO2, 14U, 1U);

	early_delay_us(1000U); /* 1ms settle */
}
