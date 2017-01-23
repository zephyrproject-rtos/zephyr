/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* Onboard USR1 GREEN LED */
#define USR1_GPIO_PIN     29
#define USR1_GPIO_PORT    CONFIG_GPIO_NRF5_P0_DEV_NAME

/* Onboard BT BLUE LED */
#define BT_GPIO_PIN       28
#define BT_GPIO_PORT      CONFIG_GPIO_NRF5_P0_DEV_NAME

/* USER push button */
#define USER_PB_GPIO_PIN  27
#define USER_PB_GPIO_PORT CONFIG_GPIO_NRF5_P0_DEV_NAME

/* Aliases to make the basic samples (e.g. LED, button) work */
#define LED0_GPIO_PIN     USR1_GPIO_PIN
#define LED0_GPIO_PORT    USR1_GPIO_PORT
#define LED1_GPIO_PIN     BT_GPIO_PIN
#define LED1_GPIO_PORT    BT_GPIO_PORT
#define SW0_GPIO_PIN      USER_PB_GPIO_PIN
#define SW0_GPIO_NAME     USER_PB_GPIO_PORT


#endif /* __INC_BOARD_H */
