/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* Push button switch 2 */
#define SW2_GPIO_NAME	CONFIG_GPIO_MCUX_PORTC_NAME
#define SW2_GPIO_PIN	6

/* Push button switch 3 */
#define SW3_GPIO_NAME	CONFIG_GPIO_MCUX_PORTA_NAME
#define SW3_GPIO_PIN	4

/* Red LED */
#define RED_GPIO_NAME	CONFIG_GPIO_MCUX_PORTB_NAME
#define RED_GPIO_PIN	22

/* Green LED */
#define GREEN_GPIO_NAME	CONFIG_GPIO_MCUX_PORTE_NAME
#define GREEN_GPIO_PIN	26

/* Blue LED */
#define BLUE_GPIO_NAME	CONFIG_GPIO_MCUX_PORTB_NAME
#define BLUE_GPIO_PIN	21

/* LED0. There is no physical LED on the board with this name, so create an
 * alias to the green LED to make various samples work.
 */
#define LED0_GPIO_PORT	GREEN_GPIO_NAME
#define LED0_GPIO_PIN	GREEN_GPIO_PIN

/* LED1. There is no physical LED on the board with this name, so create an
 * alias to the blue LED to make various samples work.
 */
#define LED1_GPIO_PORT	BLUE_GPIO_NAME
#define LED1_GPIO_PIN 	BLUE_GPIO_PIN

/* Push button switch 0. There is no physical switch on the board with this
 * name, so create an alias to SW3 to make the basic button sample work.
 */
#define SW0_GPIO_NAME	SW3_GPIO_NAME
#define SW0_GPIO_PIN	SW3_GPIO_PIN

#endif /* __INC_BOARD_H */
