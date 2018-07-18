/*
 * Copyright (c) 2017 Shawn Nock <shawn@monadnock.ca>
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* Onboard LED P0_07 */
#define LED0_GPIO_PIN	7
#define LED0_GPIO_PORT	CONFIG_GPIO_P0_DEV_NAME

/* Onboard BUTTON P0_06 */
#define SW0_GPIO_PIN 6
#define SW0_GPIO_PORT CONFIG_GPIO_P0_DEV_NAME

#endif /* __INC_BOARD_H */
