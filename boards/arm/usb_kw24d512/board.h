/*
 * Copyright (c) 2017 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* Push button switch (SW1) */
#define SW0_GPIO_NAME	CONFIG_GPIO_MCUX_PORTC_NAME
#define SW0_GPIO_PIN	4

/* LED0 (D2) */
#define LED0_GPIO_PORT	CONFIG_GPIO_MCUX_PORTD_NAME
#define LED0_GPIO_PIN	4

/* LED1 (D3) */
#define LED1_GPIO_PORT	CONFIG_GPIO_MCUX_PORTD_NAME
#define LED1_GPIO_PIN	5

#endif /* __INC_BOARD_H */
