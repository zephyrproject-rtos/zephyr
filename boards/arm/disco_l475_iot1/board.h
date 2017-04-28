/*
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
#define LD1_GPIO_PORT	"GPIOA"
#define LD1_GPIO_PIN	5

/* LD2 green LED */
#define LD2_GPIO_PORT	"GPIOB"
#define LD2_GPIO_PIN	14

/* LD3 & LD4 are mounted on PC_9 */
/* PC_9 output: 1: LD2 off, LD3 on */
/* PC_9 output: 0: LD2 on, LD3 off */
/* PC_9 input: LD3 off, LD3 off */
/* LD3 yelllow: Wifi activity */
#define LD3_GPIO_PORT	"GPIOC"
#define LD3_GPIO_PIN	9
/* LD4 blue: BT activity */
#define LD4_GPIO_PORT	"GPIOC"
#define LD4_GPIO_PIN	9

/* Create aliases to make the basic samples work */
#define SW0_GPIO_NAME	USER_PB_GPIO_PORT
#define SW0_GPIO_PIN	USER_PB_GPIO_PIN
#define LED0_GPIO_PORT	LD2_GPIO_PORT
#define LED0_GPIO_PIN	LD2_GPIO_PIN

#endif /* __INC_BOARD_H */
