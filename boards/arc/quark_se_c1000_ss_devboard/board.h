/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* Push button switch 0 */
#define SW0_GPIO_PIN	4
#define SW0_GPIO_NAME	CONFIG_GPIO_QMSI_1_NAME

/* Push button switch 1 */
#define SW1_GPIO_PIN	5
#define SW1_GPIO_NAME	CONFIG_GPIO_QMSI_0_NAME


/* Onboard LED */
#define LED0_GPIO_PORT  CONFIG_GPIO_QMSI_0_NAME
#define LED0_GPIO_PIN   25

#endif /* __INC_BOARD_H */
