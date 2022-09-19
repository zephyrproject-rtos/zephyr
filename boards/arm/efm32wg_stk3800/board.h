/*
 * Copyright (c) 2017 Christian Taedcke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

/* This pin is used to enable the serial port using the board controller */
#define BC_ENABLE_GPIO_NODE  DT_NODELABEL(gpiof)
#define BC_ENABLE_GPIO_PIN   7

#endif /* __INC_BOARD_H */
