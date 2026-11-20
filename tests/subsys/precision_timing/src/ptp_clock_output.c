/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/precision_clock_output.h>
#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/precision_timing/precision_clock.h>
#include <zephyr/precision_timing/precision_clock_ptp.h>
#include <zephyr/ztest.h>

struct ptp_output_fake {
	struct precision_clock_output_caps caps;
	struct precision_clock_output_channel_caps channels[2];
	struct precision_clock_output_status status;
	struct net_ptp_time now;
	int caps_error;
	int start_error;
	int stop_error;
	int status_error;
};

static struct ptp_output_fake fake_output;

static void ptp_output_fake_mark_active(uint32_t channel)
{
	fake_output.status.configured = true;
	fake_output.status.hardware_active_valid =
		(fake_output.channels[channel].flags &
		 PRECISION_CLOCK_OUTPUT_CAP_HARDWARE_ACTIVE) != 0U;
	fake_output.status.hardware_active = true;
}

static int ptp_output_fake_get(const struct device *dev, struct net_ptp_time *tm)
{
	ARG_UNUSED(dev);

	*tm = fake_output.now;

	return 0;
}

static int ptp_output_fake_get_caps(const struct device *dev,
				    struct precision_clock_output_caps *caps)
{
	ARG_UNUSED(dev);

	if (fake_output.caps_error != 0) {
		return fake_output.caps_error;
	}

	*caps = fake_output.caps;

	return 0;
}

static int ptp_output_fake_start(const struct device *dev, uint32_t channel,
				 const struct precision_clock_output_config *cfg)
{
	ARG_UNUSED(dev);

	if (channel >= fake_output.caps.channel_count) {
		return -ENOTSUP;
	}

	if (fake_output.start_error != 0) {
		return fake_output.start_error;
	}

	ptp_output_fake_mark_active(channel);
	fake_output.status.config = *cfg;

	return 0;
}

static int ptp_output_fake_stop(const struct device *dev, uint32_t channel)
{
	ARG_UNUSED(dev);

	if (channel >= fake_output.caps.channel_count) {
		return -ENOTSUP;
	}

	if (fake_output.stop_error != 0) {
		return fake_output.stop_error;
	}

	fake_output.status.configured = false;

	return 0;
}

static int ptp_output_fake_get_status(const struct device *dev, uint32_t channel,
				      struct precision_clock_output_status *status)
{
	ARG_UNUSED(dev);

	if (channel >= fake_output.caps.channel_count) {
		return -ENOTSUP;
	}

	if (fake_output.status_error != 0) {
		return fake_output.status_error;
	}

	*status = fake_output.status;

	return 0;
}

static const struct precision_clock_output_provider ptp_output_full_provider = {
	.get_caps = ptp_output_fake_get_caps,
	.start = ptp_output_fake_start,
	.stop = ptp_output_fake_stop,
	.get_status = ptp_output_fake_get_status,
};

static const struct precision_clock_output_provider ptp_output_missing_start_provider = {
	.get_caps = ptp_output_fake_get_caps,
	.stop = ptp_output_fake_stop,
	.get_status = ptp_output_fake_get_status,
};

static const struct precision_clock_output_provider ptp_output_missing_caps_provider = {
	.start = ptp_output_fake_start,
	.stop = ptp_output_fake_stop,
	.get_status = ptp_output_fake_get_status,
};

static const struct precision_clock_output_provider ptp_output_missing_stop_provider = {
	.get_caps = ptp_output_fake_get_caps,
	.start = ptp_output_fake_start,
	.get_status = ptp_output_fake_get_status,
};

static const struct precision_clock_output_provider ptp_output_missing_status_provider = {
	.get_caps = ptp_output_fake_get_caps,
	.start = ptp_output_fake_start,
	.stop = ptp_output_fake_stop,
};

static DEVICE_API(ptp_clock, ptp_output_full_api) = {
	.get = ptp_output_fake_get,
	.output = &ptp_output_full_provider,
};

DEVICE_DEFINE(ptp_output_full, "ptp_output_full", NULL, NULL, NULL, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ptp_output_full_api);

static DEVICE_API(ptp_clock, ptp_output_no_extension_api) = {
	.get = ptp_output_fake_get,
};

DEVICE_DEFINE(ptp_output_no_extension, "ptp_output_no_extension", NULL, NULL, NULL, NULL,
	      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ptp_output_no_extension_api);

static DEVICE_API(ptp_clock, ptp_output_missing_start_api) = {
	.get = ptp_output_fake_get,
	.output = &ptp_output_missing_start_provider,
};

DEVICE_DEFINE(ptp_output_missing_start, "ptp_output_missing_start", NULL, NULL, NULL, NULL,
	      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ptp_output_missing_start_api);

static DEVICE_API(ptp_clock, ptp_output_missing_caps_api) = {
	.get = ptp_output_fake_get,
	.output = &ptp_output_missing_caps_provider,
};

DEVICE_DEFINE(ptp_output_missing_caps, "ptp_output_missing_caps", NULL, NULL, NULL, NULL,
	      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ptp_output_missing_caps_api);

static DEVICE_API(ptp_clock, ptp_output_missing_stop_api) = {
	.get = ptp_output_fake_get,
	.output = &ptp_output_missing_stop_provider,
};

DEVICE_DEFINE(ptp_output_missing_stop, "ptp_output_missing_stop", NULL, NULL, NULL, NULL,
	      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ptp_output_missing_stop_api);

static DEVICE_API(ptp_clock, ptp_output_missing_status_api) = {
	.get = ptp_output_fake_get,
	.output = &ptp_output_missing_status_provider,
};

DEVICE_DEFINE(ptp_output_missing_status, "ptp_output_missing_status", NULL, NULL, NULL, NULL,
	      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ptp_output_missing_status_api);

static void ptp_output_fake_init(void)
{
	fake_output = (struct ptp_output_fake){0};
	fake_output.channels[0] = (struct precision_clock_output_channel_caps){
		.flags = PRECISION_CLOCK_OUTPUT_CAP_WAVEFORM |
			 PRECISION_CLOCK_OUTPUT_CAP_PROGRAMMABLE_WIDTH |
			 PRECISION_CLOCK_OUTPUT_CAP_HARDWARE_ACTIVE,
		.resolution_ns = 10,
		.min_lead_time_ns = 100,
		.min_period_ns = 100,
		.max_period_ns = 2000000000LL,
		.min_pulse_width_ns = 20,
		.max_pulse_width_ns = 1000000000LL,
	};
	fake_output.channels[1] = fake_output.channels[0];
	fake_output.channels[1].resolution_ns = 20;
	fake_output.caps = (struct precision_clock_output_caps){
		.channel_count = ARRAY_SIZE(fake_output.channels),
		.channels = fake_output.channels,
	};
	fake_output.now = (struct net_ptp_time){
		.second = 1,
		.nanosecond = 20,
	};
}

static struct precision_clock_output_config valid_output_config(void)
{
	return (struct precision_clock_output_config){
		.kind = PRECISION_CLOCK_OUTPUT_KIND_WAVEFORM,
		.waveform = {
			.first_rising_time = 3LL * NSEC_PER_SEC + 40,
			.period_ns = NSEC_PER_SEC,
			.width_policy = PRECISION_CLOCK_OUTPUT_WIDTH_EXACT,
			.pulse_width_ns = 100 * NSEC_PER_MSEC,
		},
	};
}

ZTEST(precision_timing, test_ptp_clock_output_propagates_errors)
{
	struct precision_clock_ptp_adapter adapter;
	struct precision_clock_output_caps caps;
	struct precision_clock_output_status status;
	struct precision_clock_output_config waveform = valid_output_config();
	const struct precision_clock *precision_clk;

	ptp_output_fake_init();
	precision_clock_ptp_init(&adapter, DEVICE_GET(ptp_output_full));
	precision_clk = precision_clock_ptp_get(&adapter);

	fake_output.caps_error = -EIO;
	zassert_equal(precision_clock_output_get_caps(precision_clk, &caps), -EIO);
	fake_output.caps_error = 0;

	fake_output.start_error = -ENOSPC;
	zassert_equal(precision_clock_output_start(precision_clk, 0, &waveform), -ENOSPC);
	fake_output.start_error = 0;

	fake_output.stop_error = -EIO;
	zassert_equal(precision_clock_output_stop(precision_clk, 0), -EIO);
	fake_output.stop_error = 0;

	fake_output.status_error = -EAGAIN;
	zassert_equal(precision_clock_output_get_status(precision_clk, 0, &status), -EAGAIN);
}

ZTEST(precision_timing, test_ptp_clock_output_reports_historical_status_without_caps)
{
	struct precision_clock_ptp_adapter adapter;
	struct precision_clock_output_status status;
	struct precision_clock_output_config config = valid_output_config();
	const struct precision_clock *precision_clk;

	ptp_output_fake_init();
	precision_clock_ptp_init(&adapter, DEVICE_GET(ptp_output_full));
	precision_clk = precision_clock_ptp_get(&adapter);
	zassert_ok(precision_clock_output_start(precision_clk, 1, &config));
	fake_output.now.second = 10;
	fake_output.caps_error = -EIO;
	zassert_ok(precision_clock_output_get_status(precision_clk, 1, &status));
	zassert_true(status.configured);
	zassert_equal(status.config.kind, config.kind);
	zassert_equal(status.config.waveform.first_rising_time, config.waveform.first_rising_time);
	zassert_equal(status.config.waveform.period_ns, config.waveform.period_ns);
	zassert_equal(status.config.waveform.width_policy, config.waveform.width_policy);
	zassert_equal(status.config.waveform.pulse_width_ns, config.waveform.pulse_width_ns);
	zassert_true(status.hardware_active_valid);
	zassert_true(status.hardware_active);
	zassert_ok(precision_clock_output_stop(precision_clk, 1));
	zassert_ok(precision_clock_output_get_status(precision_clk, 1, &status));
	zassert_false(status.configured);
}

ZTEST(precision_timing, test_ptp_clock_output_discovers_channels)
{
	struct precision_clock_ptp_adapter adapter;
	struct precision_clock_output_caps caps;
	struct precision_clock_output_config config = valid_output_config();
	struct precision_clock_output_status status;
	const struct precision_clock *precision_clk;

	ptp_output_fake_init();
	precision_clock_ptp_init(&adapter, DEVICE_GET(ptp_output_full));
	precision_clk = precision_clock_ptp_get(&adapter);
	zassert_ok(precision_clock_output_get_caps(precision_clk, &caps));
	zassert_equal(caps.channel_count, 2);
	zassert_equal(caps.channels[0].resolution_ns, 10);
	zassert_equal(caps.channels[1].resolution_ns, 20);
	zassert_equal(precision_clock_output_start(precision_clk, 2, &config), -ENOTSUP);
	zassert_equal(precision_clock_output_stop(precision_clk, 2), -ENOTSUP);
	zassert_equal(precision_clock_output_get_status(precision_clk, 2, &status), -ENOTSUP);

	config.waveform.first_rising_time += 10;
	zassert_equal(precision_clock_output_start(precision_clk, 1, &config), -ERANGE);
	zassert_ok(precision_clock_output_start(precision_clk, 0, &config));
	zassert_ok(precision_clock_output_stop(precision_clk, 0));
}

ZTEST(precision_timing, test_ptp_clock_output_discovers_no_channels)
{
	struct precision_clock_ptp_adapter adapter;
	struct precision_clock_output_caps caps;
	struct precision_clock_output_config config = valid_output_config();
	const struct precision_clock *precision_clk;

	ptp_output_fake_init();
	fake_output.caps = (struct precision_clock_output_caps){0};
	precision_clock_ptp_init(&adapter, DEVICE_GET(ptp_output_full));
	precision_clk = precision_clock_ptp_get(&adapter);
	zassert_ok(precision_clock_output_get_caps(precision_clk, &caps));
	zassert_equal(caps.channel_count, 0);
	zassert_is_null(caps.channels);
	zassert_equal(precision_clock_output_start(precision_clk, 0, &config), -ENOTSUP);
}

static void assert_no_scheduled_output(const struct device *dev)
{
	struct precision_clock_ptp_adapter adapter;
	struct precision_clock_output_caps output_caps;
	struct precision_clock_output_status status;
	struct precision_clock_output_config waveform = valid_output_config();
	const struct precision_clock *precision_clk;

	precision_clock_ptp_init(&adapter, dev);
	precision_clk = precision_clock_ptp_get(&adapter);
	zassert_equal(precision_clock_output_get_caps(precision_clk, &output_caps), -ENOTSUP);
	zassert_equal(precision_clock_output_start(precision_clk, 0, &waveform), -ENOTSUP);
	zassert_equal(precision_clock_output_stop(precision_clk, 0), -ENOTSUP);
	zassert_equal(precision_clock_output_get_status(precision_clk, 0, &status), -ENOTSUP);
}

ZTEST(precision_timing, test_ptp_clock_output_requires_common_callbacks)
{
	struct precision_clock_ptp_adapter adapter;
	struct precision_clock_output_caps caps;
	struct precision_clock_output_status status;
	struct precision_clock_output_config waveform = valid_output_config();
	const struct precision_clock *precision_clk;

	ptp_output_fake_init();
	assert_no_scheduled_output(DEVICE_GET(ptp_output_no_extension));

	precision_clock_ptp_init(&adapter, DEVICE_GET(ptp_output_missing_start));
	precision_clk = precision_clock_ptp_get(&adapter);
	zassert_ok(precision_clock_output_get_caps(precision_clk, &caps));
	zassert_equal(precision_clock_output_start(precision_clk, 0, &waveform), -ENOTSUP);
	zassert_ok(precision_clock_output_stop(precision_clk, 0));
	zassert_ok(precision_clock_output_get_status(precision_clk, 0, &status));

	precision_clock_ptp_init(&adapter, DEVICE_GET(ptp_output_missing_caps));
	precision_clk = precision_clock_ptp_get(&adapter);
	zassert_equal(precision_clock_output_get_caps(precision_clk, &caps), -ENOTSUP);
	zassert_equal(precision_clock_output_start(precision_clk, 0, &waveform), -ENOTSUP);
	zassert_ok(precision_clock_output_stop(precision_clk, 0));
	zassert_ok(precision_clock_output_get_status(precision_clk, 0, &status));
	zassert_false(status.configured);

	precision_clock_ptp_init(&adapter, DEVICE_GET(ptp_output_missing_stop));
	precision_clk = precision_clock_ptp_get(&adapter);
	zassert_equal(precision_clock_output_stop(precision_clk, 0), -ENOTSUP);

	precision_clock_ptp_init(&adapter, DEVICE_GET(ptp_output_missing_status));
	precision_clk = precision_clock_ptp_get(&adapter);
	zassert_equal(precision_clock_output_get_status(precision_clk, 0, &status), -ENOTSUP);
}
