/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ACTUATOR_FAKE_H_
#define ZEPHYR_INCLUDE_DRIVERS_ACTUATOR_FAKE_H_

#include <zephyr/drivers/actuator.h>
#include <zephyr/fff.h>

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_FAKE_VALUE_FUNC(int,
			actuator_fake_set_setpoint,
			const struct device *,
			q31_t);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_ACTUATOR_FAKE_H_ */
