/*
 * Copyright (c) 2017 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* USER push button */
#define USER_PB_GPIO_PORT	"GPIOC"
#define USER_PB_GPIO_PIN	13

/* LD2 green LED */
#define LD2_GPIO_PORT		"GPIOA"
#define LD2_GPIO_PIN		5

/* Create aliases to make the basic samples work */
#define SW0_GPIO_NAME		USER_PB_GPIO_PORT
#define SW0_GPIO_PIN		USER_PB_GPIO_PIN
#define LED0_GPIO_PORT		LD2_GPIO_PORT
#define LED0_GPIO_PIN		LD2_GPIO_PIN

#endif /* __INC_BOARD_H */
