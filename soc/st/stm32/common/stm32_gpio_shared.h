/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ST_STM32_COMMON_STM32_GPIO_SHARED_H_
#define ZEPHYR_SOC_ST_STM32_COMMON_STM32_GPIO_SHARED_H_

#include <zephyr/drivers/clock_control/stm32_clock_control.h>
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
 * STM32 GPIO port configuration block and data block structures
 */
struct gpio_stm32_config {
#if defined(CONFIG_GPIO_STM32)
	struct gpio_driver_config common;
#endif /* CONFIG_GPIO_STM32 */

	/* GPIO port base address */
	void *base;

	/* GPIO port index (`STM32_PORTx`) */
	uint32_t port;

	/* Clock configuration */
	struct stm32_pclken pclken;
};

struct gpio_stm32_data {
#if defined(CONFIG_GPIO_STM32)
	struct gpio_driver_data common;

	/*
	 * Keeps track of pins which are used as GPIOs
	 * and need the GPIO port clock to remain enabled.
	 */
	gpio_port_pins_t pin_has_clock_enabled;
#endif /* CONFIG_GPIO_STM32 */

	/* User callbacks list */
	sys_slist_t cb;
};


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

/*
 * Exports from the GPIO port manager module
 */

/**
 * @brief Get GPIO port device by index
 *
 * @param port_index GPIO port index `STM32_PORTx`
 * (c.f., `include/zephyr/dt-bindings/pinctrl/stm32-pinctrl-common.h`)
 *
 * @return Non-NULL pointer to requested GPIO port device on success,
 * NULL if the requested GPIO port does not exist or is not ready.
 */
const struct device *stm32_gpioport_get(uint32_t port_index);

/**
 * @brief Configure specific pin on GPIO port
 *
 * @param port GPIO port device
 * @param pin Pin to configure
 *
 * @return 0 on success, negative errno value otherwise
 */
int stm32_gpioport_configure_pin(
	const struct device *port, gpio_pin_t pin, uint32_t config, uint32_t func);

/*
 * GPIO port device API
 *
 * Implementation of the Zephyr GPIO driver API
 * exported by the GPIO driver when enabled.
 */
#if defined(CONFIG_GPIO_STM32)
extern const struct gpio_driver_api gpio_stm32_driver;
#endif /* CONFIG_GPIO_STM32 */

#endif /* ZEPHYR_SOC_ST_STM32_COMMON_STM32_GPIO_SHARED_H_ */
