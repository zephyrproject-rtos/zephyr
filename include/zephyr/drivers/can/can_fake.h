/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fake CAN controller driver API functions.
 * @ingroup can_fake
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CAN_CAN_FAKE_H_
#define ZEPHYR_INCLUDE_DRIVERS_CAN_CAN_FAKE_H_

#include <zephyr/drivers/can.h>
#include <zephyr/fff.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fake CAN controller driver
 * @defgroup can_fake Fake CAN controller
 * @ingroup io_emulators
 * @ingroup can_interface
 *
 * The fake CAN controller driver implements every CAN controller API callback
 * as a Fake Function Framework (FFF) fake. It is enabled by
 * @kconfig{CONFIG_CAN_FAKE} and instantiated from
 * @dtcompatible{zephyr,fake-can} devicetree nodes.
 *
 * Each fake is named after the API function it backs (`fake_can_send()` for
 * `can_send()`, and so on) and is paired with an FFF control structure carrying
 * an additional `_fake` suffix (`fake_can_send_fake`). Test suites include
 * this header to set return values, install custom fakes, or inspect call
 * counts and captured arguments. See @rstref{mocking-fff}.
 *
 * When @kconfig{CONFIG_ZTEST} is enabled, a ztest rule resets all fakes before
 * each test case. The reset also re-installs a default `custom_fake` for
 * `fake_can_get_capabilities()`, `fake_can_get_state()` and
 * `fake_can_get_core_clock()`, which fill in their output parameters with
 * usable values. FFF gives `custom_fake` precedence over `return_val`, so clear
 * it before setting a return value for those three.
 *
 * @code{.c}
 * const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(fake_can));
 * enum can_state state;
 *
 * // Drop the default delegate, then report an error instead.
 * fake_can_get_state_fake.custom_fake = NULL;
 * fake_can_get_state_fake.return_val = -EIO;
 *
 * zassert_equal(-EIO, can_get_state(dev, &state, NULL));
 * zassert_equal(1, fake_can_get_state_fake.call_count);
 * zassert_equal(dev, fake_can_get_state_fake.arg0_val);
 * @endcode
 *
 * @{
 */

/** @cond INTERNAL_HIDDEN */

DECLARE_FAKE_VALUE_FUNC(int, fake_can_start, const struct device *);

DECLARE_FAKE_VALUE_FUNC(int, fake_can_stop, const struct device *);

DECLARE_FAKE_VALUE_FUNC(int, fake_can_set_timing, const struct device *, const struct can_timing *);

DECLARE_FAKE_VALUE_FUNC(int, fake_can_set_timing_data, const struct device *,
			const struct can_timing *);

DECLARE_FAKE_VALUE_FUNC(int, fake_can_get_capabilities, const struct device *, can_mode_t *);

DECLARE_FAKE_VALUE_FUNC(int, fake_can_set_mode, const struct device *, can_mode_t);

DECLARE_FAKE_VALUE_FUNC(int, fake_can_send, const struct device *, const struct can_frame *,
			k_timeout_t, can_tx_callback_t, void *);

DECLARE_FAKE_VALUE_FUNC(int, fake_can_add_rx_filter, const struct device *, can_rx_callback_t,
			void *, const struct can_filter *);

DECLARE_FAKE_VOID_FUNC(fake_can_remove_rx_filter, const struct device *, int);

DECLARE_FAKE_VALUE_FUNC(int, fake_can_recover, const struct device *, k_timeout_t);

DECLARE_FAKE_VALUE_FUNC(int, fake_can_get_state, const struct device *, enum can_state *,
			struct can_bus_err_cnt *);

DECLARE_FAKE_VOID_FUNC(fake_can_set_state_change_callback, const struct device *,
		       can_state_change_callback_t, void *);

DECLARE_FAKE_VALUE_FUNC(int, fake_can_get_max_filters, const struct device *, bool);

DECLARE_FAKE_VALUE_FUNC(int, fake_can_get_core_clock, const struct device *, uint32_t *);

/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CAN_CAN_FAKE_H_ */
