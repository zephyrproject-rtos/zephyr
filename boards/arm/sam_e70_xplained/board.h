/*
 * Copyright (c) 2018 Justin Watson
 * Copyright (c) 2016 Piotr Mienkowski
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _INC_BOARD_H_
#define _INC_BOARD_H_

#include <soc.h>

#define BOARD_GREEN_LED_GPIO_PORT	CONFIG_GPIO_SAM_PORTC_LABEL
#define BOARD_GREEN_LED_GPIO_PIN	8

/* The switch is labeled SW300 in the schematic, and labeled SW0 on the
 * board.
 */
#define BOARD_SW0_GPIO_PORT	CONFIG_GPIO_SAM_PORTA_LABEL
#define BOARD_SW0_GPIO_PIN	11

/* Aliases to make the basic samples work. */
#define LED0_GPIO_PORT		BOARD_GREEN_LED_GPIO_PORT
#define LED0_GPIO_PIN		BOARD_GREEN_LED_GPIO_PIN
#define SW0_GPIO_NAME		BOARD_SW0_GPIO_PORT
#define SW0_GPIO_PIN		BOARD_SW0_GPIO_PIN
#define SW0_GPIO_PIN_PUD	GPIO_PUD_PULL_UP

#endif /* _INC_BOARD_H_ */
