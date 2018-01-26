/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* Red LED */
#define RED_GPIO_NAME	CONFIG_GPIO_MCUX_LPC_PORT0_NAME
#define RED_GPIO_PIN    29

/* Green LED */
#define GREEN_GPIO_NAME CONFIG_GPIO_MCUX_LPC_PORT1_NAME
#define GREEN_GPIO_PIN  10

/* Blue LED */
#define BLUE_GPIO_NAME  CONFIG_GPIO_MCUX_LPC_PORT1_NAME
#define BLUE_GPIO_PIN   9

/* LED0. There is no physical LED on the board with this name, so create an
 * alias to the green LED to make various samples work.
 */
#define LED0_GPIO_PORT  GREEN_GPIO_NAME
#define LED0_GPIO_PIN   GREEN_GPIO_PIN

/* LED1. There is no physical LED on the board with this name, so create an
 * alias to the blue LED to make various samples work.
 */
#define LED1_GPIO_PORT	BLUE_GPIO_NAME
#define LED1_GPIO_PIN	BLUE_GPIO_PIN

#endif /* __INC_BOARD_H */
