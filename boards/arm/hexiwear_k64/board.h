/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

#ifdef CONFIG_PWM_3

#define RED_PWM_NAME		CONFIG_FTM_3_NAME
#define RED_PWM_CHANNEL		4

#define GREEN_PWM_NAME		CONFIG_FTM_3_NAME
#define GREEN_PWM_CHANNEL	0

#define BLUE_PWM_NAME		CONFIG_FTM_3_NAME
#define BLUE_PWM_CHANNEL	5

#else

/* Red LED */
#define RED_GPIO_NAME		CONFIG_GPIO_MCUX_PORTC_NAME
#define RED_GPIO_PIN		8

/* Green LED */
#define GREEN_GPIO_NAME		CONFIG_GPIO_MCUX_PORTD_NAME
#define GREEN_GPIO_PIN		0

/* Blue LED */
#define BLUE_GPIO_NAME		CONFIG_GPIO_MCUX_PORTC_NAME
#define BLUE_GPIO_PIN		9

/* LED0. There is no physical LED on the board with this name, so create an
 * alias to the green LED to make the basic blinky sample work.
 */
#define LED0_GPIO_PORT	GREEN_GPIO_NAME
#define LED0_GPIO_PIN	GREEN_GPIO_PIN

#endif /* CONFIG_PWM_3 */

#endif /* __INC_BOARD_H */
