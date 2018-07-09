/*
 * Copyright (c) 2016-2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* Aliases to make the basic samples (e.g. LED, button) work */

/* Push button switch 0
 * This is the only button available to the application,
 * the other button on the board is used to reset.
 */
#define SW0_GPIO_PIN     GPIO_KEYS_0_GPIO_PIN
#define SW0_GPIO_NAME    GPIO_KEYS_0_GPIO_CONTROLLER
#define SW0_GPIO_PIN_PUD GPIO_KEYS_0_GPIO_FLAGS

/* LED 0 is a green LED */
#define LED0_GPIO_PIN  GPIO_LEDS_LED_0_GREEN_GPIO_PIN
#define LED0_GPIO_PORT GPIO_LEDS_LED_0_GREEN_GPIO_CONTROLLER

/* LED 1 is RGB */
#define LED1_GPIO_PIN  GPIO_LEDS_LED_1_RED_GPIO_PIN
#define LED1_GPIO_PORT GPIO_LEDS_LED_1_RED_GPIO_CONTROLLER
#define LED2_GPIO_PIN  GPIO_LEDS_LED_1_GREEN_GPIO_PIN
#define LED2_GPIO_PORT GPIO_LEDS_LED_1_GREEN_GPIO_CONTROLLER
#define LED3_GPIO_PIN  GPIO_LEDS_LED_1_BLUE_GPIO_PIN
#define LED3_GPIO_PORT GPIO_LEDS_LED_1_BLUE_GPIO_CONTROLLER

#endif /* __INC_BOARD_H */
