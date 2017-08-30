/*
 * Copyright (c) 2017 Powersoft.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* LD1 green LED */
#define LD1_GPIO_PORT	"GPIOE"
#define LD1_GPIO_PIN	0

/* LD2 orange LED */
#define LD2_GPIO_PORT	"GPIOE"
#define LD2_GPIO_PIN	1

/* LD3 red LED */
#define LD3_GPIO_PORT	"GPIOE"
#define LD3_GPIO_PIN	2

/* LD4 blue LED */
#define LD4_GPIO_PORT	"GPIOE"
#define LD4_GPIO_PIN	4

/* Create aliases to make the basic samples work */
#define LED0_GPIO_PORT	LD1_GPIO_PORT
#define LED0_GPIO_PIN	LD1_GPIO_PIN
#define LED1_GPIO_PORT	LD2_GPIO_PORT
#define LED1_GPIO_PIN	LD2_GPIO_PIN
#define LED2_GPIO_PORT	LD3_GPIO_PORT
#define LED2_GPIO_PIN	LD3_GPIO_PIN
#define LED3_GPIO_PORT	LD4_GPIO_PORT
#define LED3_GPIO_PIN	LD4_GPIO_PIN

#endif /* __INC_BOARD_H */
