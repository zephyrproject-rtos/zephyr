/*
 * Copyright (c) 2017 Clage GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* USER push button */
#define USER_PB_GPIO_PORT	"GPIOA"
#define USER_PB_GPIO_PIN	0

/* LD3 red/upper LED*/
#define LD3_GPIO_PORT		"GPIOC"
#define LD3_GPIO_PIN		6

/* LD4 yellow/left LED  */
#define LD4_GPIO_PORT		"GPIOC"
#define LD4_GPIO_PIN		8

/* LD5 green/right LED */
#define LD5_GPIO_PORT		"GPIOC"
#define LD5_GPIO_PIN		9

/* LD6 blue/lower LED */
#define LD6_GPIO_PORT		"GPIOC"
#define LD6_GPIO_PIN		7

/* Create aliases to make the basic samples work */
#define SW0_GPIO_NAME	USER_PB_GPIO_PORT
#define SW0_GPIO_PIN	USER_PB_GPIO_PIN
#define LED0_GPIO_PORT	LD3_GPIO_PORT
#define LED0_GPIO_PIN	LD3_GPIO_PIN
#define LED1_GPIO_PORT	LD2_GPIO_PORT
#define LED1_GPIO_PIN	LD2_GPIO_PIN

#endif /* __INC_BOARD_H */
