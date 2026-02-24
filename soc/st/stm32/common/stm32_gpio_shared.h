/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ST_STM32_COMMON_STM32_GPIO_SHARED_H_
#define ZEPHYR_SOC_ST_STM32_COMMON_STM32_GPIO_SHARED_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/types.h>

/*
 * Name of all GPIO ports that may exist (without the `gpio` prefix).
 *
 * This should be the only thing that needs updating when adding support
 * for new GPIO ports if need arises.
 *
 * WARNING: make sure both list are kept in sync when modifying!
 */
#define STM32_GPIO_PORTS_LIST_LWR \
	a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z

#define STM32_GPIO_PORTS_LIST_UPR \
	A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z

/*
 * Utility functions
 */

/**
 * @brief Translate pin to pinval that the LL library needs
 */
static inline uint32_t stm32_gpiomgr_pinnum_to_ll_val(gpio_pin_t pinnum)
{
	uint32_t pinval;

#ifdef CONFIG_SOC_SERIES_STM32F1X
	/*
	 * Avoid pulling <stm32_ll_gpio.h> by hardcoding
	 * the value of the GPIO_PIN_MASK_POS macro here
	 */
	const uint32_t gpio_pin_mask_pos = 8u;

	pinval = (1u << pinnum) << gpio_pin_mask_pos;
	if (pinnum <= 7) {
		pinval |= 1u << pinnum;
	} else {
		pinval |= (1u << (pinnum % 8)) | 0x04000000u;
	}
#else
	pinval = 1u << pinnum;
#endif
	return pinval;
}

#endif /* ZEPHYR_SOC_ST_STM32_COMMON_STM32_GPIO_SHARED_H_ */
