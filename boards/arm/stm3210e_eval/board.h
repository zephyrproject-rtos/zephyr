/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* User Button. */
#define BTN_USER_GPIO_NAME	"GPIOG"
#define BTN_USER_GPIO_PIN	8

/* Push button switch 0. Create an alias to user button
 * to make the basic button sample work.
 */
#define SW0_GPIO_NAME	BTN_USER_GPIO_NAME
#define SW0_GPIO_PIN	BTN_USER_GPIO_PIN

/* LED 1 */
#define LED1_GPIO_NAME		"GPIOF"
#define LED1_GPIO_PIN		6

/* LED0. Create an alias to the LED2 to make the basic
 * blinky sample work.
 */
#define LED0_GPIO_PORT	LED1_GPIO_NAME
#define LED0_GPIO_PIN	LED1_GPIO_PIN

#endif /* __INC_BOARD_H */
