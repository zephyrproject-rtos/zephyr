/*
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

/* pin used to enable the buffer power */
#define SERIAL_BUFFER_ENABLE_GPIO_NAME  DT_INST_0_NORDIC_NRF_GPIO_LABEL
#define SERIAL_BUFFER_ENABLE_GPIO_PIN   25
#define SERIAL_BUFFER_ENABLE_GPIO_FLAGS GPIO_ACTIVE_LOW

/* pin used to detect V_INT (buffer power) */
#define V_INT_DETECT_GPIO_PIN	2
#define V_INT_DETECT_GPIO_FLAGS	GPIO_ACTIVE_HIGH

/* SKYWORKS SKY13351 antenna selection settings (only use vctl1) */
#define ANT_UFLn_GPIO_NAME	DT_INST_0_SKYWORKS_SKY13351_VCTL1_GPIOS_CONTROLLER
#define ANT_UFLn_GPIO_FLAGS	DT_INST_0_SKYWORKS_SKY13351_VCTL1_GPIOS_FLAGS
#define ANT_UFLn_GPIO_PIN	DT_INST_0_SKYWORKS_SKY13351_VCTL1_GPIOS_PIN

#endif /* __INC_BOARD_H */
