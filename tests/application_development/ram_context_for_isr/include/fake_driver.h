/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_FAKE_DRIVER_H_
#define ZEPHYR_INCLUDE_DRIVERS_FAKE_DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*fake_driver_irq_callback_t)(const struct device *dev, void *user_data);

struct fake_driver_config {
	void (*irq_config_func)(void);
	uint8_t irq_num;
	uint8_t irq_priority;
};

struct fake_driver_data {
	fake_driver_irq_callback_t irq_callback;
	void *user_data;
};

__subsystem struct fake_driver_api {
	int (*configure)(const struct device *dev, int config);
	int (*register_irq_callback)(const struct device *dev, fake_driver_irq_callback_t cb,
				     void *user_data);
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_FAKE_DRIVER_H_ */
