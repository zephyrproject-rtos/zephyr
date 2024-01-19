/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_fake.h>
#include <zephyr/fff.h>

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>
#endif /* CONFIG_ZTEST */

#define DT_DRV_COMPAT zephyr_fake_can

struct fake_can_config {
	const struct can_driver_config common;
};

struct fake_can_data {
	struct can_driver_data common;
};

DEFINE_FAKE_VALUE_FUNC(int, fake_can_start, const struct device *);

DEFINE_FAKE_VALUE_FUNC(int, fake_can_stop, const struct device *);

DEFINE_FAKE_VALUE_FUNC(int, fake_can_set_timing, const struct device *, const struct can_timing *);

DEFINE_FAKE_VALUE_FUNC(int, fake_can_set_timing_data, const struct device *,
		       const struct can_timing *);

DEFINE_FAKE_VALUE_FUNC(int, fake_can_get_capabilities, const struct device *, can_mode_t *);

DEFINE_FAKE_VALUE_FUNC(int, fake_can_set_mode, const struct device *, can_mode_t);

DEFINE_FAKE_VALUE_FUNC(int, fake_can_send, const struct device *, const struct can_frame *,
		       k_timeout_t, can_tx_callback_t, void *);

DEFINE_FAKE_VALUE_FUNC(int, fake_can_add_rx_filter, const struct device *, can_rx_callback_t,
		       void *, const struct can_filter *);

DEFINE_FAKE_VOID_FUNC(fake_can_remove_rx_filter, const struct device *, int);

DEFINE_FAKE_VALUE_FUNC(int, fake_can_recover, const struct device *, k_timeout_t);

DEFINE_FAKE_VALUE_FUNC(int, fake_can_get_state, const struct device *, enum can_state *,
		       struct can_bus_err_cnt *);

DEFINE_FAKE_VOID_FUNC(fake_can_set_state_change_callback, const struct device *,
		      can_state_change_callback_t, void *);

DEFINE_FAKE_VALUE_FUNC(int, fake_can_get_max_filters, const struct device *, bool);

#ifdef CONFIG_ZTEST
static void fake_can_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	ARG_UNUSED(test);
	ARG_UNUSED(fixture);

	RESET_FAKE(fake_can_start);
	RESET_FAKE(fake_can_stop);
	RESET_FAKE(fake_can_get_capabilities);
	RESET_FAKE(fake_can_set_mode);
	RESET_FAKE(fake_can_set_timing);
	RESET_FAKE(fake_can_set_timing_data);
	RESET_FAKE(fake_can_send);
	RESET_FAKE(fake_can_add_rx_filter);
	RESET_FAKE(fake_can_remove_rx_filter);
	RESET_FAKE(fake_can_get_state);
	RESET_FAKE(fake_can_recover);
	RESET_FAKE(fake_can_set_state_change_callback);
	RESET_FAKE(fake_can_get_max_filters);
}

ZTEST_RULE(fake_can_reset_rule, fake_can_reset_rule_before, NULL);
#endif /* CONFIG_ZTEST */

static int fake_can_get_core_clock(const struct device *dev, uint32_t *rate)
{
	ARG_UNUSED(dev);

	*rate = 16000000;

	return 0;
}

static const struct can_driver_api fake_can_driver_api = {
	.start = fake_can_start,
	.stop = fake_can_stop,
	.get_capabilities = fake_can_get_capabilities,
	.set_mode = fake_can_set_mode,
	.set_timing = fake_can_set_timing,
	.send = fake_can_send,
	.add_rx_filter = fake_can_add_rx_filter,
	.remove_rx_filter = fake_can_remove_rx_filter,
	.get_state = fake_can_get_state,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = fake_can_recover,
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */
	.set_state_change_callback = fake_can_set_state_change_callback,
	.get_core_clock = fake_can_get_core_clock,
	.get_max_filters = fake_can_get_max_filters,
	.timing_min = {
		.sjw = 0x01,
		.prop_seg = 0x01,
		.phase_seg1 = 0x01,
		.phase_seg2 = 0x01,
		.prescaler = 0x01
	},
	.timing_max = {
		.sjw = 0x0f,
		.prop_seg = 0x0f,
		.phase_seg1 = 0x0f,
		.phase_seg2 = 0x0f,
		.prescaler = 0xffff
	},
#ifdef CONFIG_CAN_FD_MODE
	.set_timing_data = fake_can_set_timing_data,
	.timing_data_min = {
		.sjw = 0x01,
		.prop_seg = 0x01,
		.phase_seg1 = 0x01,
		.phase_seg2 = 0x01,
		.prescaler = 0x01
	},
	.timing_data_max = {
		.sjw = 0x0f,
		.prop_seg = 0x0f,
		.phase_seg1 = 0x0f,
		.phase_seg2 = 0x0f,
		.prescaler = 0xffff
	},
#endif /* CONFIG_CAN_FD_MODE */
};

#define FAKE_CAN_INIT(inst)						     \
	static const struct fake_can_config fake_can_config_##inst = {	     \
		.common = CAN_DT_DRIVER_CONFIG_INST_GET(inst, 0U),	     \
	};								     \
									     \
	static struct fake_can_data fake_can_data_##inst;		     \
									     \
	CAN_DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &fake_can_data_##inst,   \
				  &fake_can_config_##inst, POST_KERNEL,	     \
				  CONFIG_CAN_INIT_PRIORITY,                  \
				  &fake_can_driver_api);

DT_INST_FOREACH_STATUS_OKAY(FAKE_CAN_INIT)
