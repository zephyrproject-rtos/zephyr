/*
 * Copyright (c) 2016 Matthias Boesl
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* USER push button */
#define USER_PB1_GPIO_PORT	"GPIOE"
#define USER_PB1_GPIO_PIN	4

/* USER push button */
#define USER_PB2_GPIO_PORT       "GPIOE"
#define USER_PB2_GPIO_PIN        3

/* LD2 green LED */
#define LD2_GPIO_PORT	"GPIOA"
#define LD2_GPIO_PIN	6

/* LD3 green LED */
#define LD3_GPIO_PORT   "GPIOA"
#define LD3_GPIO_PIN    7


/* Create aliases to make the basic samples work */
#define SW0_GPIO_NAME	USER_PB1_GPIO_PORT
#define SW0_GPIO_PIN	USER_PB1_GPIO_PIN
#define SW1_GPIO_NAME   USER_PB2_GPIO_PORT
#define SW1_GPIO_PIN    USER_PB2_GPIO_PIN
#define LED0_GPIO_PORT	LD2_GPIO_PORT
#define LED0_GPIO_PIN	LD2_GPIO_PIN
#define LED1_GPIO_PORT  LD3_GPIO_PORT
#define LED1_GPIO_PIN   LD3_GPIO_PIN

/* pwm dev and channal */

#if defined(CONFIG_PWM_STM32)
#define PWM_DRIVER CONFIG_PWM_STM32_2_DEV_NAME
#define PWM_CHANNEL 1
#endif

#endif /* __INC_BOARD_H */
