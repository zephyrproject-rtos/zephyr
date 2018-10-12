/*
 * Copyright (c) 2018 Marcio Montenegro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* This pin is used to enable the serial port using the board controller */
#define BC_ENABLE_GPIO_NAME  CONFIG_GPIO_GECKO_PORTA_NAME
#define BC_ENABLE_GPIO_PIN   9

#endif /* __INC_BOARD_H */
