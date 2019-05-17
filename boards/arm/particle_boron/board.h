/*
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

/* pin used to enable the buffer power */
#define SERIAL_BUFFER_ENABLE_GPIO_NAME  DT_NORDIC_NRF_GPIO_0_LABEL
#define SERIAL_BUFFER_ENABLE_GPIO_PIN   25

/* pin used to detect V_INT (buffer power) */
#define V_INT_DETECT_GPIO_PIN		2

#endif /* __INC_BOARD_H */
