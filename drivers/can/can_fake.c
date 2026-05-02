/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_fake.h>
#include <zephyr/fff.h>
#include <zephyr/sys/util.h>

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

DEFINE_FAKE_VALUE_FUNC(int, fake_can_get_core_clock, const struct device *, uint32_t *);

static int fake_can_get_core_clock_delegate(const struct device *dev, uint32_t *rate)
{
	ARG_UNUSED(dev);

	/* Recommended CAN clock from CiA 601-3 */
	*rate = MHZ(80);

	return 0;
}

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
	RESET_FAKE(fake_can_get_core_clock);

	/* Re-install default delegate for reporting the core clock */
	fake_can_get_core_clock_fake.custom_fake = fake_can_get_core_clock_delegate;
}

ZTEST_RULE(fake_can_reset_rule, fake_can_reset_rule_before, NULL);
#endif /* CONFIG_ZTEST */

static int fake_can_init(const struct device *dev)
{
	/* Install default delegate for reporting the core clock */
	fake_can_get_core_clock_fake.custom_fake = fake_can_get_core_clock_delegate;

	return 0;
}

static DEVICE_API(can, fake_can_driver_api) = {
	.start = fake_can_start,
	.stop = fake_can_stop,
	.get_capabilities = fake_can_get_capabilities,
	.set_mode = fake_can_set_mode,
	.set_timing = fake_can_set_timing,
	.send = fake_can_send,
	.add_rx_filter = fake_can_add_rx_filter,
	.remove_rx_filter = fake_can_remove_rx_filter,
	.get_state = fake_can_get_state,
#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
	.recover = fake_can_recover,
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */
	.set_state_change_callback = fake_can_set_state_change_callback,
	.get_core_clock = fake_can_get_core_clock,
	.get_max_filters = fake_can_get_max_filters,
	/* Recommended configuration ranges from CiA 601-2 */
	.timing_min = {
		.sjw = 1,
		.prop_seg = 0,
		.phase_seg1 = 2,
		.phase_seg2 = 2,
		.prescaler = 1
	},
	.timing_max = {
		.sjw = 128,
		.prop_seg = 0,
		.phase_seg1 = 256,
		.phase_seg2 = 128,
		.prescaler = 32
	},
#ifdef CONFIG_CAN_FD_MODE
	.set_timing_data = fake_can_set_timing_data,
	/* Recommended configuration ranges from CiA 601-2 */
	.timing_data_min = {
		.sjw = 1,
		.prop_seg = 0,
		.phase_seg1 = 1,
		.phase_seg2 = 1,
		.prescaler = 1
	},
	.timing_data_max = {
		.sjw = 16,
		.prop_seg = 0,
		.phase_seg1 = 32,
		.phase_seg2 = 16,
		.prescaler = 32
	},
#endif /* CONFIG_CAN_FD_MODE */
};

#ifdef CONFIG_CAN_FD_MODE
#define FAKE_CAN_MAX_BITRATE 8000000
#else /* CONFIG_CAN_FD_MODE */
#define FAKE_CAN_MAX_BITRATE 1000000
#endif /* !CONFIG_CAN_FD_MODE */

#define FAKE_CAN_INIT(inst)						                \
	static const struct fake_can_config fake_can_config_##inst = {	                \
		.common = CAN_DT_DRIVER_CONFIG_INST_GET(inst, 0, FAKE_CAN_MAX_BITRATE), \
	};								                \
									                \
	static struct fake_can_data fake_can_data_##inst;		                \
									                \
	CAN_DEVICE_DT_INST_DEFINE(inst, fake_can_init, NULL, &fake_can_data_##inst,     \
				  &fake_can_config_##inst, POST_KERNEL,	                \
				  CONFIG_CAN_INIT_PRIORITY,                             \
				  &fake_can_driver_api);

DT_INST_FOREACH_STATUS_OKAY(FAKE_CAN_INIT)
