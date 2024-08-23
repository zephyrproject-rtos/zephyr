/*
 * Copyright (c) 2023-2024 MUNIC SA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Renesas RA specific pinctrl interface
 *
 */

#ifndef __ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_RA2_H__
#define __ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_RA2_H__

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>

/* This is an extension to the pinctrl API: returns the PIN config.
 *
 * Actually used by the GPIO driver.
 */
int pinctrl_ra_get_pin(unsigned int port_id, gpio_pin_t pin_id,
		pinctrl_soc_pin_t *pin);

#endif /* __ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_RA2_H__ */
