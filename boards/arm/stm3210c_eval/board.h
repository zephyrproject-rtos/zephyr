/*
 * Copyright (c) 2016 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* User Button. */
#define BTN_USER_GPIO_NAME	"GPIOB"
#define BTN_USER_GPIO_PIN	9

/* Push button switch 0. Create an alias to tamper button
 * to make the basic button sample work.
 */
#define SW0_GPIO_NAME	BTN_USER_GPIO_NAME
#define SW0_GPIO_PIN	BTN_USER_GPIO_PIN

/* LED 2 */
#define LED2_GPIO_NAME		"GPIOD"
#define LED2_GPIO_PIN		13

/* LED0. Create an alias to the LED2 to make the basic
 * blinky sample work.
 */
#define LED0_GPIO_PORT	LED2_GPIO_NAME
#define LED0_GPIO_PIN	LED2_GPIO_PIN

#endif /* __INC_BOARD_H */
