/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Button assignments
 *
 * Button mappings are listed here.
 *
 */

#ifndef _BUTTON_ASSIGNMENTS_H_
#define _BUTTON_ASSIGNMENTS_H_

#include <zephyr/drivers/gpio.h>

/** @brief List of buttons and associated metadata
 */
enum button_pin_names {
	BUTTON_VOLUME_DOWN = DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
	BUTTON_VOLUME_UP = DT_GPIO_PIN(DT_ALIAS(sw1), gpios),
	BUTTON_PLAY_PAUSE = DT_GPIO_PIN(DT_ALIAS(sw2), gpios),
	BUTTON_4 = DT_GPIO_PIN(DT_ALIAS(sw3), gpios),
	BUTTON_5 = DT_GPIO_PIN(DT_ALIAS(sw4), gpios),
};

#endif /* _BUTTON_ASSIGNMENTS_H_ */
