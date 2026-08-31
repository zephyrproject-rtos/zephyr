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
 * The fake comparator driver implements every comparator API callback as a
 * Fake Function Framework (FFF) fake. It is enabled by
 * @kconfig{CONFIG_COMPARATOR_FAKE_COMP} and instantiated from a
 * @dtcompatible{zephyr,fake-comp} devicetree node.
 *
 * Each fake is named after the API function it backs, prefixed with
 * `comp_fake_comp_` (`comp_fake_comp_get_output()` for `comparator_get_output()`,
 * and so on), and is paired with an FFF control structure carrying an
 * additional `_fake` suffix (`comp_fake_comp_get_output_fake`). Test suites
 * include this header to set return values, install custom fakes, or inspect
 * call counts and captured arguments. See @rstref{mocking-fff}.
 *
 * When @kconfig{CONFIG_ZTEST} is enabled, a ztest rule resets all fakes before
 * each test case.
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

/** @cond INTERNAL_HIDDEN */

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

/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_COMPARATOR_FAKE_H_ */
