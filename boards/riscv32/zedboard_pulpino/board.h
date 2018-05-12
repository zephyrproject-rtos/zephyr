/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>
#include <gpio.h>

/* LEDS configuration */
#define LED0_GPIO_PORT        "gpio0"
#define LED1_GPIO_PORT        "gpio0"
#define LED2_GPIO_PORT        "gpio0"
#define LED3_GPIO_PORT        "gpio0"
#define LED4_GPIO_PORT        "gpio0"
#define LED5_GPIO_PORT        "gpio0"
#define LED6_GPIO_PORT        "gpio0"
#define LED7_GPIO_PORT        "gpio0"

#define LED0_GPIO_PIN         8
#define LED1_GPIO_PIN         9
#define LED2_GPIO_PIN         10
#define LED3_GPIO_PIN         11
#define LED4_GPIO_PIN         12
#define LED5_GPIO_PIN         13
#define LED6_GPIO_PIN         14
#define LED7_GPIO_PIN         15

/* Push buttons configuration */
#define SW0_GPIO_NAME         "gpio0"
#define SW1_GPIO_NAME         "gpio0"
#define SW2_GPIO_NAME         "gpio0"
#define SW3_GPIO_NAME         "gpio0"
#define SW4_GPIO_NAME         "gpio0"

#define SW0_GPIO_PIN	      16
#define SW1_GPIO_PIN	      17
#define SW2_GPIO_PIN	      18
#define SW3_GPIO_PIN	      19
#define SW4_GPIO_PIN	      20

/*
 * GPIO PULL-UP config does not exist in pulpino
 * set all to 1 for buttons to compile against apps.
 */
#define SW0_GPIO_PULL_UP      1
#define SW1_GPIO_PULL_UP      1
#define SW2_GPIO_PULL_UP      1
#define SW3_GPIO_PULL_UP      1
#define SW4_GPIO_PULL_UP      1

/* Interrupt config for push buttons */
#define SW0_GPIO_FLAGS     (GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH)
#define SW1_GPIO_FLAGS     SW0_GPIO_FLAGS
#define SW2_GPIO_FLAGS     SW0_GPIO_FLAGS
#define SW3_GPIO_FLAGS     SW0_GPIO_FLAGS
#define SW4_GPIO_FLAGS     SW0_GPIO_FLAGS

#endif /* __INC_BOARD_H */
