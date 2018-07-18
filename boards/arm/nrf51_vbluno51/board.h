/*
 * Copyright (c) 2017 VNG IoT Lab Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* Button*/
#define BUT_GPIO_PIN	15
#define BUT_GPIO_NAME	CONFIG_GPIO_P0_DEV_NAME
#define SW0_GPIO_PIN	BUT_GPIO_PIN
#define SW0_GPIO_NAME	BUT_GPIO_NAME

/* Onboard GREEN LED 0 */
#define LED0_GPIO_PIN   7
#define LED0_GPIO_PORT  CONFIG_GPIO_P0_DEV_NAME

#define LED_GPIO_PIN    LED0_GPIO_PIN
#define LED_GPIO_PORT   LED0_GPIO_PORT

#endif /* __INC_BOARD_H */
