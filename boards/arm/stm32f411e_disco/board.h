/*
 * Copyright (c) 2017 Fenix Engineering Solutions
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* USER push button */
#define USER_PB_GPIO_PORT "GPIOA"
#define USER_PB_GPIO_PIN  0

/* LD3 orange LED */
#define LD3_GPIO_PORT "GPIOD"
#define LD3_GPIO_PIN  13

/* LD4 green LED */
#define LD4_GPIO_PORT	"GPIOD"
#define LD4_GPIO_PIN	12

/* LD5 red LED */
#define LD5_GPIO_PORT	"GPIOD"
#define LD5_GPIO_PIN	14

/* LD6 blue LED */
#define LD6_GPIO_PORT	"GPIOD"
#define LD6_GPIO_PIN	15

/* Create aliases to make the basic samples work */
#define SW0_GPIO_NAME USER_PB_GPIO_PORT
#define SW0_GPIO_PIN  USER_PB_GPIO_PIN
#define LED0_GPIO_PORT	LD3_GPIO_PORT
#define LED0_GPIO_PIN	LD3_GPIO_PIN
#define LED1_GPIO_PORT	LD4_GPIO_PORT
#define LED1_GPIO_PIN	LD4_GPIO_PIN
#define LED2_GPIO_PORT	LD5_GPIO_PORT
#define LED2_GPIO_PIN	LD5_GPIO_PIN
#define LED3_GPIO_PORT	LD6_GPIO_PORT
#define LED3_GPIO_PIN	LD6_GPIO_PIN

#endif /* __INC_BOARD_H */
