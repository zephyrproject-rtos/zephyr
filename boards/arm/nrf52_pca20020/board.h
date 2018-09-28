/*
 * Copyright (c) 2018 Aapo Vienamo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* Push button switch 0 */
#define SW0_GPIO_PIN 11
#define SW0_GPIO_NAME CONFIG_GPIO_P0_DEV_NAME

#define LIGHTWELL_PORT CONFIG_GPIO_SX1509B_DEV_NAME
#define LIGHTWELL_R_PIN	7
#define LIGHTWELL_G_PIN	5
#define LIGHTWELL_B_PIN	6

#define LED0_GPIO_PORT  LIGHTWELL_PORT
#define LED0_GPIO_PIN	LIGHTWELL_G_PIN

#endif /* __INC_BOARD_H */
