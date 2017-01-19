/*
 * Copyright (c) 2016, Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __INC_BOARD_H
#define __INC_BOARD_H

/* Push button switch 2 */
#define SW2_GPIO_PIN	6    /* GPIO22/Pin15 */
#define SW2_GPIO_NAME	"GPIO_A2"

/* Push button switch 3 */
#define SW3_GPIO_PIN	5    /* GPIO13/Pin4 */
#define SW3_GPIO_NAME	"GPIO_A1"

/* Push button switch 0: Map to SW2 so zephyr button example works */
#define SW0_GPIO_PIN	SW2_GPIO_PIN
#define SW0_GPIO_NAME	SW2_GPIO_NAME

/* Onboard GREEN LED */
#define LED0_GPIO_PIN   3  /*GPIO11/Pin2 */
#define LED0_GPIO_PORT  "GPIO_A1"

#endif /* __INC_BOARD_H */
