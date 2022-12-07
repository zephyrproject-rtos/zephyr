/*
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

/* pin used to enable the buffer power */
#define SERIAL_BUFFER_ENABLE_GPIO_NODE  DT_INST(0, nordic_nrf_gpio)
#define SERIAL_BUFFER_ENABLE_GPIO_PIN   25
#define SERIAL_BUFFER_ENABLE_GPIO_FLAGS GPIO_ACTIVE_LOW

/* pin used to detect V_INT (buffer power) */
#define V_INT_DETECT_GPIO_PIN	2
#define V_INT_DETECT_GPIO_FLAGS	GPIO_ACTIVE_HIGH

#endif /* __INC_BOARD_H */
