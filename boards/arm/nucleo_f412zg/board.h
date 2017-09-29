/*
 * Copyright (c) 2017 Florian Vaussard, HEIG-VD
 *
 * Based on nucleo_f411re:
 *
 * Copyright (c) 2016 Matthias Boesl
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* USER push button */
#define USER_PB_GPIO_PORT	"GPIOC"
#define USER_PB_GPIO_PIN	13

/* LD1 green LED */
#define LD1_GPIO_PORT		"GPIOB"
#define LD1_GPIO_PIN		0

/* LD2 blue LED */
#define LD2_GPIO_PORT		"GPIOB"
#define LD2_GPIO_PIN		7

/* LD3 red LED */
#define LD3_GPIO_PORT		"GPIOB"
#define LD3_GPIO_PIN		14

/* Create aliases to make the basic samples work */
#define SW0_GPIO_NAME		USER_PB_GPIO_PORT
#define SW0_GPIO_PIN		USER_PB_GPIO_PIN
#define LED0_GPIO_PORT		LD1_GPIO_PORT
#define LED0_GPIO_PIN		LD1_GPIO_PIN
#define LED1_GPIO_PORT		LD2_GPIO_PORT
#define LED1_GPIO_PIN		LD2_GPIO_PIN
#define LED2_GPIO_PORT		LD3_GPIO_PORT
#define LED2_GPIO_PIN		LD3_GPIO_PIN

#endif /* __INC_BOARD_H */
