/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* USER push button BUT */
#define USER_PB_GPIO_PORT	"GPIOA"
#define USER_PB_GPIO_PIN	0

/* LED1 green LED */
#define LED1_GPIO_PORT	"GPIOC"
#define LED1_GPIO_PIN	13

/* Define aliases to make the basic samples work */
#define SW0_GPIO_NAME	USER_PB_GPIO_PORT
#define SW0_GPIO_PIN	USER_PB_GPIO_PIN
#define LED0_GPIO_PORT	LED1_GPIO_PORT
#define LED0_GPIO_PIN	LED1_GPIO_PIN

#endif /* __INC_BOARD_H */
