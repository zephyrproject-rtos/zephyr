/*
 * Copyright (c) 2016 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* Push button switch 0 */
#define DT_ALIAS_SW0_GPIOS_PIN	11
#define SW0_GPIO_NAME	DT_GPIO_P0_DEV_NAME

/* Push button switch 1 */
#define DT_ALIAS_SW1_GPIOS_PIN	12
#define SW1_GPIO_NAME	DT_GPIO_P0_DEV_NAME

/* Push button switch 2 */
#define DT_ALIAS_SW2_GPIOS_PIN	24
#define SW2_GPIO_NAME	DT_GPIO_P0_DEV_NAME

/* Push button switch 3 */
#define DT_ALIAS_SW3_GPIOS_PIN	25
#define SW3_GPIO_NAME	DT_GPIO_P0_DEV_NAME

/* Onboard GREEN LED 0 */
#define DT_ALIAS_LED0_GPIOS_PIN   13
#define LED0_GPIO_PORT  DT_GPIO_P0_DEV_NAME

/* Onboard GREEN LED 1 */
#define DT_ALIAS_LED1_GPIOS_PIN   14
#define LED1_GPIO_PORT  DT_GPIO_P0_DEV_NAME

/* Onboard GREEN LED 2 */
#define DT_ALIAS_LED2_GPIOS_PIN   15
#define LED2_GPIO_PORT  DT_GPIO_P0_DEV_NAME

/* Onboard GREEN LED 3 */
#define DT_ALIAS_LED3_GPIOS_PIN   16
#define LED3_GPIO_PORT  DT_GPIO_P0_DEV_NAME

#endif /* __INC_BOARD_H */
