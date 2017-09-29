/*
 * Copyright (c) 2017 I-SENSE group of ICCS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* USER push button */
#define USER_PB_GPIO_PORT	"GPIOA"
#define USER_PB_GPIO_PIN	0

/* LD3 red LED */
#define LD3_GPIO_PORT	"GPIOE"
#define LD3_GPIO_PIN	9

/* LD4 blue LED */
#define LD4_GPIO_PORT	"GPIOE"
#define LD4_GPIO_PIN	8

/* LD5 orange LED */
#define LD5_GPIO_PORT	"GPIOE"
#define LD5_GPIO_PIN	10

/* LD6 green LED */
#define LD6_GPIO_PORT	"GPIOE"
#define LD6_GPIO_PIN	15

/* LD7 green LED */
#define LD7_GPIO_PORT	"GPIOE"
#define LD7_GPIO_PIN	11

/* LD8 orange LED */
#define LD8_GPIO_PORT	"GPIOE"
#define LD8_GPIO_PIN	14

/* LD9 blue LED */
#define LD9_GPIO_PORT	"GPIOE"
#define LD9_GPIO_PIN	12

/* LD10 red LED */
#define LD10_GPIO_PORT	"GPIOE"
#define LD10_GPIO_PIN	13

/* Create aliases to make the basic samples work */
#define SW0_GPIO_NAME	USER_PB_GPIO_PORT
#define SW0_GPIO_PIN	USER_PB_GPIO_PIN
#define LED0_GPIO_PORT	LD3_GPIO_PORT
#define LED0_GPIO_PIN	LD3_GPIO_PIN

#endif /* __INC_BOARD_H */
