/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* Onboard USR1 GREEN LED */
#define USR1_GPIO_PIN     2
#define USR1_GPIO_PORT    "GPIOD"

/* Onboard USR2 GREEN LED */
#define USR2_GPIO_PIN     15
#define USR2_GPIO_PORT    "GPIOA"

/* Onboard BT BLUE LED */
#define BT_GPIO_PIN       5
#define BT_GPIO_PORT      "GPIOB"

/* Aliases to make the basic LED samples work */
#define LED0_GPIO_PIN     USR1_GPIO_PIN
#define LED0_GPIO_PORT    USR1_GPIO_PORT
#define LED1_GPIO_PIN     USR2_GPIO_PIN
#define LED1_GPIO_PORT    USR2_GPIO_PORT
#define LED2_GPIO_PIN     BT_GPIO_PIN
#define LED2_GPIO_PORT    BT_GPIO_PORT

#endif /* __INC_BOARD_H */
