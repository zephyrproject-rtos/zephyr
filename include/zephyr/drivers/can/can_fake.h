/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CAN_CAN_FAKE_H_
#define ZEPHYR_INCLUDE_DRIVERS_CAN_CAN_FAKE_H_

#include <zephyr/drivers/can.h>
#include <zephyr/fff.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CAN_CAN_FAKE_H_ */
