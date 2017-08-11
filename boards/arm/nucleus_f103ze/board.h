/*
 * Copyright (c) 2016 Matthias Boesl
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* USER1 push button */
#define USER_PB1_GPIO_PORT	"GPIOE"
#define USER_PB1_GPIO_PIN	4

/* USER2 push button*/
#define USER_PB2_GPIO_PORT      "GPIOE"
#define USER_PB2_GPIO_PIN       3

/* On board user1 green LED */
#define LD1_GPIO_PORT	"GPIOB"
#define LD1_GPIO_PIN	5

/* On board user2  green LED */
#define LD2_GPIO_PORT   "GPIOE"
#define LD2_GPIO_PIN    5

/* Create aliases to make the basic samples work */
#define SW0_GPIO_NAME	USER_PB1_GPIO_PORT
#define SW0_GPIO_PIN	USER_PB1_GPIO_PIN
#define LED0_GPIO_PORT	LD1_GPIO_PORT
#define LED0_GPIO_PIN	LD1_GPIO_PIN
#define LED1_GPIO_PORT  LD2_GPIO_PORT
#define LED1_GPIO_PIN   LD2_GPIO_PIN

#endif /* __INC_BOARD_H */
