/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* USER push button */
#define USER_PB_GPIO_PORT "GPIOB"
#define USER_PB_GPIO_PIN   2

/* Onboard USR0 GREEN LED */
#define USR0_GPIO_PIN     12
#define USR0_GPIO_PORT    "GPIOB"

/* Onboard USR1 GREEN LED */
#define USR1_GPIO_PIN     13
#define USR1_GPIO_PORT    "GPIOB"

/* Onboard USR2 GREEN LED */
#define USR2_GPIO_PIN     14
#define USR2_GPIO_PORT    "GPIOB"

/* Onboard USR3 GREEN LED */
#define USR3_GPIO_PIN     15
#define USR3_GPIO_PORT    "GPIOB"

/* Aliases to make the basic LED samples work */
#define SW0_GPIO_NAME     USER_PB_GPIO_PORT
#define SW0_GPIO_PIN      USER_PB_GPIO_PIN
#define LED0_GPIO_PIN     USR0_GPIO_PIN
#define LED0_GPIO_PORT    USR0_GPIO_PORT
#define LED1_GPIO_PIN     USR1_GPIO_PIN
#define LED1_GPIO_PORT    USR1_GPIO_PORT
#define LED2_GPIO_PIN     USR2_GPIO_PIN
#define LED2_GPIO_PORT    USR2_GPIO_PORT
#define LED3_GPIO_PIN     USR3_GPIO_PIN
#define LED3_GPIO_PORT    USR3_GPIO_PORT

#endif /* __INC_BOARD_H */
