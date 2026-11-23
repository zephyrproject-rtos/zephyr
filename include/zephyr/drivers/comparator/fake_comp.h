/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fake comparator driver API functions.
 * @ingroup comparator_fake
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_COMPARATOR_FAKE_H_
#define ZEPHYR_INCLUDE_DRIVERS_COMPARATOR_FAKE_H_

#include <zephyr/drivers/comparator.h>
#include <zephyr/fff.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fake comparator driver
 * @defgroup comparator_fake Fake comparator
 * @ingroup io_emulators
 * @ingroup comparator_interface
 *
 * @driver_fake{comparator_interface,CONFIG_COMPARATOR_FAKE_COMP,zephyr\,fake-comp}
 *
 * @code{.c}
 * const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(fake_comp));
 *
 * comp_fake_comp_get_output_fake.return_val = 1;
 *
 * zassert_equal(1, comparator_get_output(dev));
 * zassert_equal(1, comp_fake_comp_get_output_fake.call_count);
 * zassert_equal(dev, comp_fake_comp_get_output_fake.arg0_val);
 * @endcode
 *
 * @{
 */

/** @fake_of{comparator_driver_api::get_output} */
DECLARE_FAKE_VALUE_FUNC(int,
			comp_fake_comp_get_output,
			const struct device *);

/** @fake_of{comparator_driver_api::set_trigger} */
DECLARE_FAKE_VALUE_FUNC(int,
			comp_fake_comp_set_trigger,
			const struct device *,
			enum comparator_trigger);

/** @fake_of{comparator_driver_api::set_trigger_callback} */
DECLARE_FAKE_VALUE_FUNC(int,
			comp_fake_comp_set_trigger_callback,
			const struct device *,
			comparator_callback_t,
			void *);

/** @fake_of{comparator_driver_api::trigger_is_pending} */
DECLARE_FAKE_VALUE_FUNC(int,
			comp_fake_comp_trigger_is_pending,
			const struct device *);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_COMPARATOR_FAKE_H_ */
