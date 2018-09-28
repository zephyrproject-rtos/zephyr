/*
 * Copyright (c) 2016 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* Push button switch 0 */
#define SW0_GPIO_PIN	11
#define SW0_GPIO_NAME	CONFIG_GPIO_P0_DEV_NAME

/* Push button switch 1 */
#define SW1_GPIO_PIN	12
#define SW1_GPIO_NAME	CONFIG_GPIO_P0_DEV_NAME

/* Push button switch 2 */
#define SW2_GPIO_PIN	24
#define SW2_GPIO_NAME	CONFIG_GPIO_P0_DEV_NAME

/* Push button switch 3 */
#define SW3_GPIO_PIN	25
#define SW3_GPIO_NAME	CONFIG_GPIO_P0_DEV_NAME

/* Onboard GREEN LED 0 */
#define LED0_GPIO_PIN   13
#define LED0_GPIO_PORT  CONFIG_GPIO_P0_DEV_NAME

/* Onboard GREEN LED 1 */
#define LED1_GPIO_PIN   14
#define LED1_GPIO_PORT  CONFIG_GPIO_P0_DEV_NAME

/* Onboard GREEN LED 2 */
#define LED2_GPIO_PIN   15
#define LED2_GPIO_PORT  CONFIG_GPIO_P0_DEV_NAME

/* Onboard GREEN LED 3 */
#define LED3_GPIO_PIN   16
#define LED3_GPIO_PORT  CONFIG_GPIO_P0_DEV_NAME

#endif /* __INC_BOARD_H */
