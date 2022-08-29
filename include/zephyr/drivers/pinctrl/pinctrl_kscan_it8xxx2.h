/*
 * Copyright (c) 2022 ITE Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_KSCAN_IT8XXX2_H_
#define ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_KSCAN_IT8XXX2_H_

#include <pinctrl_kscan_soc.h>

/**
 * @brief Configure a set of pins.
 *
 * This function will configure the necessary hardware blocks to make the
 * configuration immediately effective.
 *
 * @param pins List of pins to be configured.
 * @param pin_cnt Number of pins.
 *
 * @retval 0 If succeeded
 * @retval -errno Negative errno for other failures.
 */
int pinctrl_kscan_configure_pins(const struct pinctrl_kscan_soc_pin *pins,
				 uint8_t pin_cnt);

#endif /* ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_KSCAN_IT8XXX2_H_ */
