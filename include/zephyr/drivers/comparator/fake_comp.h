/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_COMPARATOR_FAKE_H_
#define ZEPHYR_INCLUDE_DRIVERS_COMPARATOR_FAKE_H_

#include <zephyr/drivers/comparator.h>
#include <zephyr/fff.h>

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_FAKE_VALUE_FUNC(int,
			comp_fake_comp_get_output,
			const struct device *);

DECLARE_FAKE_VALUE_FUNC(int,
			comp_fake_comp_set_trigger,
			const struct device *,
			enum comparator_trigger);

DECLARE_FAKE_VALUE_FUNC(int,
			comp_fake_comp_set_trigger_callback,
			const struct device *,
			comparator_callback_t,
			void *);

DECLARE_FAKE_VALUE_FUNC(int,
			comp_fake_comp_trigger_is_pending,
			const struct device *);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_COMPARATOR_FAKE_H_ */
