/*
 * Copyright (c) 2025 Andrei-Edward Popa <andrei.popa105@yahoo.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>

#include <ch32fun.h>

#define AFIO_SWCFG_MASK    0x07000000
#define AFIO_SWCFG_SWD_OFF 0x04000000

void board_late_init_hook(void)
{
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpiod))
	const struct device *gpiod = DEVICE_DT_GET(DT_NODELABEL(gpiod));
	uint32_t pcfr1;

	/*
	 * PD1 is wired to led and to SWDIO pin.
	 * If PD1 is not put to ground, change the function to GPIO to use
	 * the led as a user led.
	 * If PD1 is put to ground, let the pin as SWDIO pin (default) to be
	 * able to program the board.
	 */
	if (gpio_pin_get(gpiod, 1) == 1) {
		RCC->APB2PCENR |= RCC_AFIOEN;
		pcfr1 = AFIO->PCFR1;
		pcfr1 = (pcfr1 & ~AFIO_SWCFG_MASK) | (AFIO_SWCFG_SWD_OFF & AFIO_SWCFG_MASK);
		AFIO->PCFR1 = pcfr1;
	}
#endif
}
