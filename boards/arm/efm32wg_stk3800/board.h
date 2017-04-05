/*
 * Copyright (c) 2017 Christian Taedcke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* Push button PB0 */
#define PB0_GPIO_NAME	CONFIG_GPIO_GECKO_PORTB_NAME
#define PB0_GPIO_PIN	9

/* Push button PB1 */
#define PB1_GPIO_NAME	CONFIG_GPIO_GECKO_PORTB_NAME
#define PB1_GPIO_PIN	10

/* LED 0 */
#define LED0_GPIO_NAME	CONFIG_GPIO_GECKO_PORTE_NAME
#define LED0_GPIO_PORT  LED0_GPIO_NAME
#define LED0_GPIO_PIN	2

/* LED 1 */
#define LED1_GPIO_NAME	CONFIG_GPIO_GECKO_PORTE_NAME
#define LED1_GPIO_PIN	3

/* Push button switch 0. There is no physical switch on the board with this
 * name, so create an alias to SW3 to make the basic button sample work.
 */
#define SW0_GPIO_NAME	PB0_GPIO_NAME
#define SW0_GPIO_PIN	PB0_GPIO_PIN

/* This pin is used to enable the serial port using the board controller */
#define BC_ENABLE_GPIO_NAME  CONFIG_GPIO_GECKO_PORTF_NAME
#define BC_ENABLE_GPIO_PIN   7

#endif /* __INC_BOARD_H */
