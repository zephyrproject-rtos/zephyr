/*
 * Copyright (c) 2018 Nathan Tsoi <nathan@vertile.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* USER push button */
#define USER_PB_GPIO_PORT   "GPIOA"
#define USER_PB_GPIO_PIN    0

/* LD3 green LED */
#define GREEN_LED_GPIO_PORT "GPIOC"
#define GREEN_LED_GPIO_PIN  9

/* LD4 blue LED */
#define BLUE_LED_GPIO_PORT  "GPIOC"
#define BLUE_LED_GPIO_PIN   8

/* Create aliases to make the basic samples work */
#define SW0_GPIO_NAME       USER_PB_GPIO_PORT
#define SW0_GPIO_PIN        USER_PB_GPIO_PIN
#define LED0_GPIO_PORT      GREEN_LED_GPIO_PORT
#define LED0_GPIO_PIN       GREEN_LED_GPIO_PIN
#define LED1_GPIO_PORT      BLUE_LED_GPIO_PORT
#define LED1_GPIO_PIN       BLUE_LED_GPIO_PIN

#endif /* __INC_BOARD_H */
