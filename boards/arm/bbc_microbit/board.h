/*
 * Copyright (c) 2016 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* Push button switch 0 (BTN_A) */
#define SW0_GPIO_PIN	17
#define SW0_GPIO_NAME	CONFIG_GPIO_NRF5_P0_DEV_NAME

/* Push button switch 1 (BTN_B) */
#define SW1_GPIO_PIN	26
#define SW1_GPIO_NAME	CONFIG_GPIO_NRF5_P0_DEV_NAME

/* Onboard GREEN LED Row 1 */
#define LED_ROW1_GPIO_PIN   13
#define LED_ROW1_GPIO_PORT  CONFIG_GPIO_NRF5_P0_DEV_NAME

/* Onboard GREEN LED Row 2 */
#define LED_ROW2_GPIO_PIN   14
#define LED_ROW2_GPIO_PORT  CONFIG_GPIO_NRF5_P0_DEV_NAME

/* Onboard GREEN LED Row 3 */
#define LED_ROW3_GPIO_PIN   15
#define LED_ROW3_GPIO_PORT  CONFIG_GPIO_NRF5_P0_DEV_NAME

/* Onboard GREEN LED Column 1 */
#define LED_COL1_GPIO_PIN   4
#define LED_COL1_GPIO_PORT  CONFIG_GPIO_NRF5_P0_DEV_NAME

/* Onboard GREEN LED Column 2 */
#define LED_COL2_GPIO_PIN   5
#define LED_COL2_GPIO_PORT  CONFIG_GPIO_NRF5_P0_DEV_NAME

/* Onboard GREEN LED Column 3 */
#define LED_COL3_GPIO_PIN   6
#define LED_COL3_GPIO_PORT  CONFIG_GPIO_NRF5_P0_DEV_NAME

/* Onboard GREEN LED Column 4 */
#define LED_COL4_GPIO_PIN   7
#define LED_COL4_GPIO_PORT  CONFIG_GPIO_NRF5_P0_DEV_NAME

/* Onboard GREEN LED Column 5 */
#define LED_COL5_GPIO_PIN   8
#define LED_COL5_GPIO_PORT  CONFIG_GPIO_NRF5_P0_DEV_NAME

/* Onboard GREEN LED Column 6 */
#define LED_COL6_GPIO_PIN   9
#define LED_COL6_GPIO_PORT  CONFIG_GPIO_NRF5_P0_DEV_NAME

/* Onboard GREEN LED Column 7 */
#define LED_COL7_GPIO_PIN   10
#define LED_COL7_GPIO_PORT  CONFIG_GPIO_NRF5_P0_DEV_NAME

/* Onboard GREEN LED Column 8 */
#define LED_COL8_GPIO_PIN   11
#define LED_COL8_GPIO_PORT  CONFIG_GPIO_NRF5_P0_DEV_NAME

/* Onboard GREEN LED Column 9 */
#define LED_COL9_GPIO_PIN   12
#define LED_COL9_GPIO_PORT  CONFIG_GPIO_NRF5_P0_DEV_NAME

#endif /* __INC_BOARD_H */
