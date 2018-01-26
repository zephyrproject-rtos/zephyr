/*
 * Copyright (c) 2017 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* LED on PA17 */
#define LED0_GPIO_PORT	CONFIG_GPIO_SAM0_PORTA_LABEL
#define LED0_GPIO_PIN	17

/* TX on PA27 */
#define LED1_GPIO_PORT	CONFIG_GPIO_SAM0_PORTA_LABEL
#define LED1_GPIO_PIN	27

/* RX on PB3 */
#define LED2_GPIO_PORT	CONFIG_GPIO_SAM0_PORTB_LABEL
#define LED2_GPIO_PIN	3

#endif /* __INC_BOARD_H */
