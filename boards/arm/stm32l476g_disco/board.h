/*
 * Copyright (c) 2017 Arthur Sfez
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* LD4 red LED */
#define LD4_GPIO_PORT	"GPIOB"
#define LD4_GPIO_PIN	2

/* LD5 green LED */
#define LD5_GPIO_PORT	"GPIOE"
#define LD5_GPIO_PIN	8

/* Create aliases to make the basic samples work */
#define LED0_GPIO_PORT	LD4_GPIO_PORT
#define LED0_GPIO_PIN	LD4_GPIO_PIN
#define LED1_GPIO_PORT	LD5_GPIO_PORT
#define LED1_GPIO_PIN	LD5_GPIO_PIN

#endif /* __INC_BOARD_H */
