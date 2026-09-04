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
 * @driver_fake{can_interface,CONFIG_CAN_FAKE,zephyr\,fake-can}
 *
 * `fake_can_get_capabilities()`, `fake_can_get_state()` and
 * `fake_can_get_core_clock()` are given a default `custom_fake` that fills in
 * their output parameters with usable values, re-installed on every reset.
 * Install your own `custom_fake` in the test case to change what they report.
 *
 * @{
 */

/** @fake_of{can_driver_api::start} */
DECLARE_FAKE_VALUE_FUNC(int, fake_can_start, const struct device *);

/** @fake_of{can_driver_api::stop} */
DECLARE_FAKE_VALUE_FUNC(int, fake_can_stop, const struct device *);

/** @fake_of{can_driver_api::set_timing} */
DECLARE_FAKE_VALUE_FUNC(int, fake_can_set_timing, const struct device *, const struct can_timing *);

/** @fake_of{can_driver_api::set_timing_data} */
DECLARE_FAKE_VALUE_FUNC(int, fake_can_set_timing_data, const struct device *,
			const struct can_timing *);

/** @fake_of{can_driver_api::get_capabilities} */
DECLARE_FAKE_VALUE_FUNC(int, fake_can_get_capabilities, const struct device *, can_mode_t *);

/** @fake_of{can_driver_api::set_mode} */
DECLARE_FAKE_VALUE_FUNC(int, fake_can_set_mode, const struct device *, can_mode_t);

/** @fake_of{can_driver_api::send} */
DECLARE_FAKE_VALUE_FUNC(int, fake_can_send, const struct device *, const struct can_frame *,
			k_timeout_t, can_tx_callback_t, void *);

/** @fake_of{can_driver_api::add_rx_filter} */
DECLARE_FAKE_VALUE_FUNC(int, fake_can_add_rx_filter, const struct device *, can_rx_callback_t,
			void *, const struct can_filter *);

/** @fake_of{can_driver_api::remove_rx_filter} */
DECLARE_FAKE_VOID_FUNC(fake_can_remove_rx_filter, const struct device *, int);

/** @fake_of{can_driver_api::recover} */
DECLARE_FAKE_VALUE_FUNC(int, fake_can_recover, const struct device *, k_timeout_t);

/** @fake_of{can_driver_api::get_state} */
DECLARE_FAKE_VALUE_FUNC(int, fake_can_get_state, const struct device *, enum can_state *,
			struct can_bus_err_cnt *);

/** @fake_of{can_driver_api::set_state_change_callback} */
DECLARE_FAKE_VOID_FUNC(fake_can_set_state_change_callback, const struct device *,
		       can_state_change_callback_t, void *);

/** @fake_of{can_driver_api::get_max_filters} */
DECLARE_FAKE_VALUE_FUNC(int, fake_can_get_max_filters, const struct device *, bool);

/** @fake_of{can_driver_api::get_core_clock} */
DECLARE_FAKE_VALUE_FUNC(int, fake_can_get_core_clock, const struct device *, uint32_t *);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CAN_CAN_FAKE_H_ */
