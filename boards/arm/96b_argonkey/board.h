/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* USER push button */
#define USER_PB_GPIO_PORT "GPIOA"
#define USER_PB_GPIO_PIN   2

/* LED 1 */
#define USR1_GPIO_PIN     2
#define USR1_GPIO_PORT    "GPIOB"

/* LED 0 */
#define USR0_GPIO_PIN     13
#define USR0_GPIO_PORT    "GPIOC"

/* Aliases to make the basic LED samples work */
#define SW0_GPIO_NAME     USER_PB_GPIO_PORT
#define SW0_GPIO_PIN      USER_PB_GPIO_PIN
#define LED0_GPIO_PIN     USR0_GPIO_PIN
#define LED0_GPIO_PORT    USR0_GPIO_PORT
#define LED1_GPIO_PIN     USR1_GPIO_PIN
#define LED1_GPIO_PORT    USR1_GPIO_PORT

#endif /* __INC_BOARD_H */
