/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* USER push button */
#define USER_PB_GPIO_PORT	"GPIOA"
#define USER_PB_GPIO_PIN	0

/* LD1 green LED */
#define LD1_GPIO_PORT	"GPIOG"
#define LD1_GPIO_PIN	6

/* LD2 orange LED */
#define LD2_GPIO_PORT	"GPIOD"
#define LD2_GPIO_PIN	4

/* LD3 red LED */
#define LD3_GPIO_PORT	"GPIOD"
#define LD3_GPIO_PIN	5

/* LD4 blue LED */
#define LD4_GPIO_PORT	"GPIOK"
#define LD4_GPIO_PIN	3

/* Create aliases to make the basic samples work */
#define SW0_GPIO_NAME	USER_PB_GPIO_PORT
#define SW0_GPIO_PIN	USER_PB_GPIO_PIN
#define LED0_GPIO_PORT	LD1_GPIO_PORT
#define LED0_GPIO_PIN	LD1_GPIO_PIN

#endif /* __INC_BOARD_H */
