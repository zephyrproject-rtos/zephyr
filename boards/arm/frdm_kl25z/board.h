/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* Push button switch for test purposes */
#define SW0_TEST_GPIO_NAME	CONFIG_GPIO_MCUX_PORTA_NAME
#define SW0_TEST_GPIO_PIN	16

/* Push button switch for test purposes */
#define SW1_TEST_GPIO_NAME	CONFIG_GPIO_MCUX_PORTA_NAME
#define SW1_TEST_GPIO_PIN	17

/* Red LED */
#define RED_GPIO_NAME	CONFIG_GPIO_MCUX_PORTB_NAME
#define RED_GPIO_PIN	18

/* Green LED */
#define GREEN_GPIO_NAME	CONFIG_GPIO_MCUX_PORTB_NAME
#define GREEN_GPIO_PIN	19

/* Blue LED */
#define BLUE_GPIO_NAME	CONFIG_GPIO_MCUX_PORTD_NAME
#define BLUE_GPIO_PIN	1

/* LED0. There is no physical LED on the board with this name, so create an
 * alias to the green LED to make various samples work.
 */
#define LED0_GPIO_PORT	GREEN_GPIO_NAME
#define LED0_GPIO_PIN	GREEN_GPIO_PIN

/* LED1. There is no physical LED on the board with this name, so create an
 * alias to the blue LED to make various samples work.
 */
#define LED1_GPIO_PORT	BLUE_GPIO_NAME
#define LED1_GPIO_PIN	BLUE_GPIO_PIN

/* Push button switch 0. There is no physical switch on the board,
 * so an push button must be added to such pins for basic button sample work.
 */
#define SW0_GPIO_NAME	SW0_TEST_GPIO_NAME
#define SW0_GPIO_PIN	SW0_TEST_GPIO_PIN

#endif /* __INC_BOARD_H */
