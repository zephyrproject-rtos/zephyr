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

/* LD3 orange LED */
#define LD3_GPIO_PORT	"GPIOF"
#define LD3_GPIO_PIN	9

/* LD4 green LED */
#define LD4_GPIO_PORT	"GPIOF"
#define LD4_GPIO_PIN	10

/* Create aliases to make the basic samples work */
#define SW0_GPIO_NAME	USER_PB_GPIO_PORT
#define SW0_GPIO_PIN	USER_PB_GPIO_PIN
#define LED0_GPIO_PORT	LD3_GPIO_PORT
#define LED0_GPIO_PIN	LD3_GPIO_PIN
#define LED1_GPIO_PORT	LD4_GPIO_PORT
#define LED1_GPIO_PIN	LD4_GPIO_PIN

#endif /* __INC_BOARD_H */
