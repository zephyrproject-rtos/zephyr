/* a4988.h - Public API for the a4988 stepper motor controller */
/*
 * Copyright (c) 2023 Kindhome Sp. z o.o <kindhome.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_MISC_A4988_H_
#define ZEPHYR_INCLUDE_MISC_A4988_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum a4988_microstep {
	a4988_full_step,
	a4988_half_step,
	a4988_quarter_step,
	a4988_eigth_step,
	a4988_sixteenth_step,
};

int a4988_step(const struct device *dev, enum a4988_microstep microstep, bool clockwise);

int a4988_sleep(const struct device *dev, bool sleep);

int a4988_enable(const struct device *dev, bool enable);

int a4988_reset(const struct device *dev, bool reset);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MISC_A4988_H_ */
