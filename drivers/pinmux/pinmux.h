/* pinmux.h - the private pinmux driver header */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_PINMUX_PINMUX_H_
#define ZEPHYR_DRIVERS_PINMUX_PINMUX_H_

#include <zephyr/types.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pin_config {
	uint8_t pin_num;
	uint32_t mode;
};

struct pinmux_config {
	uint32_t	base_address;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_PINMUX_PINMUX_H_ */
