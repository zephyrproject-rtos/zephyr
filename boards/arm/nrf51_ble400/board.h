/*
 * Copyright (c) 2018 Roman Tataurov <diytronic@yandex.ru>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* Push button switch 0 KEY1 */
#define SW0_GPIO_PIN     16
#define SW0_GPIO_NAME    CONFIG_GPIO_P0_DEV_NAME
#define SW0_GPIO_PIN_PUD GPIO_PUD_PULL_UP

/* Push button switch 1 KEY2 */
#define SW1_GPIO_PIN     17
#define SW1_GPIO_NAME    CONFIG_GPIO_P0_DEV_NAME
#define SW1_GPIO_PIN_PUD GPIO_PUD_PULL_UP

/* Onboard RED LED 0 */
#define LED0_GPIO_PIN   18
#define LED0_GPIO_PORT  CONFIG_GPIO_P0_DEV_NAME

/* Onboard RED LED 1 */
#define LED1_GPIO_PIN   19
#define LED1_GPIO_PORT  CONFIG_GPIO_P0_DEV_NAME

/* Onboard RED LED 2 */
#define LED2_GPIO_PIN   20
#define LED2_GPIO_PORT  CONFIG_GPIO_P0_DEV_NAME

/* Onboard RED LED 3 */
#define LED3_GPIO_PIN   21
#define LED3_GPIO_PORT  CONFIG_GPIO_P0_DEV_NAME

/* Onboard RED LED 4 */
#define LED4_GPIO_PIN   22
#define LED4_GPIO_PORT  CONFIG_GPIO_P0_DEV_NAME

#endif /* __INC_BOARD_H */
