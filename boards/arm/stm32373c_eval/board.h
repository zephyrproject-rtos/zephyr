/*
 * Copyright (c) 2016 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* Key button. */
#define BTN_KEY_GPIO_NAME	"GPIOA"
#define BTN_KEY_GPIO_PIN	2

/* LED 2 */
#define LED2_GPIO_NAME		"GPIOC"
#define LED2_GPIO_PIN		1

/* Push button switch 0. Create an alias to key button
 * to make the basic button sample work.
 */
#define SW0_GPIO_NAME	BTN_KEY_GPIO_NAME
#define SW0_GPIO_PIN	BTN_KEY_GPIO_PIN

/* LED0. Create an alias to the LED2 to make the basic
 * blinky sample work.
 */
#define LED0_GPIO_PORT	LED2_GPIO_NAME
#define LED0_GPIO_PIN	LED2_GPIO_PIN

#endif /* __INC_BOARD_H */
