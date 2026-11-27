/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/precision_timing/precision_clock_output.h>
#include <zephyr/ztest.h>

struct output_fake {
	struct precision_clock clock;
	struct precision_clock_output_caps caps;
	struct precision_clock_output_channel_caps channels[2];
	struct precision_clock_output_status status;
	precision_time_t now_ns;
	uint32_t last_channel;
	int read_error;
	bool provider_configured;
	int caps_error;
	int start_error;
	int stop_error;
	int status_error;
};

static int output_fake_read(const struct precision_clock *precision_clk, precision_time_t *tm)
{
	struct output_fake *fake = precision_clk->data;

	if (fake->read_error != 0) {
		return fake->read_error;
	}

	*tm = fake->now_ns;

	return 0;
}

static int output_fake_get_caps(const struct precision_clock *precision_clk,
				struct precision_clock_output_caps *caps)
{
	struct output_fake *fake = precision_clk->data;

	if (fake->caps_error != 0) {
		return fake->caps_error;
	}

	*caps = fake->caps;

	return 0;
}

static void output_fake_mark_active(struct output_fake *fake)
{
	fake->provider_configured = true;
	fake->status.configured = true;
	fake->status.hardware_active_valid =
		(fake->channels[fake->last_channel].flags &
		 PRECISION_CLOCK_OUTPUT_CAP_HARDWARE_ACTIVE) != 0U;
	fake->status.hardware_active = true;
}

static int output_fake_start(const struct precision_clock *precision_clk, uint32_t channel,
			     const struct precision_clock_output_config *config)
{
	struct output_fake *fake = precision_clk->data;

	fake->last_channel = channel;
	if (fake->provider_configured) {
		return -EBUSY;
	}
	if (fake->start_error != 0 && fake->start_error != -EINPROGRESS) {
		return fake->start_error;
	}

	output_fake_mark_active(fake);
	fake->status.config = *config;

	return fake->start_error;
}

static int output_fake_stop(const struct precision_clock *precision_clk, uint32_t channel)
{
	struct output_fake *fake = precision_clk->data;

	fake->last_channel = channel;
	if (channel >= fake->caps.channel_count) {
		return -ENOTSUP;
	}

	if (fake->stop_error != 0) {
		return fake->stop_error;
	}

	fake->provider_configured = false;
	fake->status.configured = false;

	return 0;
}

static int output_fake_get_status(const struct precision_clock *precision_clk, uint32_t channel,
				  struct precision_clock_output_status *status)
{
	struct output_fake *fake = precision_clk->data;

	fake->last_channel = channel;
	if (channel >= fake->caps.channel_count) {
		return -ENOTSUP;
	}
	if (fake->status_error != 0) {
		return fake->status_error;
	}

	*status = fake->status;

	return 0;
}

static const struct precision_clock_api output_fake_api = {
	.read = output_fake_read,
	.output_get_caps = output_fake_get_caps,
	.output_start = output_fake_start,
	.output_stop = output_fake_stop,
	.output_get_status = output_fake_get_status,
};

static const struct precision_clock_api output_unsupported_api = {
	.read = output_fake_read,
};

static void output_fake_init(struct output_fake *fake)
{
	*fake = (struct output_fake){0};
	fake->clock = (struct precision_clock){
		.api = &output_fake_api,
		.data = fake,
	};
	fake->channels[0] = (struct precision_clock_output_channel_caps){
		.flags = PRECISION_CLOCK_OUTPUT_CAP_WAVEFORM |
			 PRECISION_CLOCK_OUTPUT_CAP_PROGRAMMABLE_WIDTH |
			 PRECISION_CLOCK_OUTPUT_CAP_HARDWARE_ACTIVE,
		.resolution_ns = 10,
		.min_lead_time_ns = 100,
		.min_period_ns = 100,
		.max_period_ns = 10000,
		.min_pulse_width_ns = 20,
		.max_pulse_width_ns = 5000,
	};
	fake->channels[1] = fake->channels[0];
	fake->channels[1].resolution_ns = 20;
	fake->channels[1].min_period_ns = 200;
	fake->channels[1].min_lead_time_ns = 200;
	fake->caps = (struct precision_clock_output_caps){
		.channel_count = ARRAY_SIZE(fake->channels),
		.channels = fake->channels,
	};
	fake->now_ns = 1000;
}

static struct precision_clock_output_config valid_output_config(void)
{
	return (struct precision_clock_output_config){
		.kind = PRECISION_CLOCK_OUTPUT_KIND_WAVEFORM,
		.waveform = {
			.first_rising_time = 2000,
			.period_ns = 1000,
			.width_policy = PRECISION_CLOCK_OUTPUT_WIDTH_EXACT,
			.pulse_width_ns = 100,
		},
	};
}

ZTEST(precision_timing, test_clock_output_rejects_invalid_arguments)
{
	struct output_fake fake;
	struct precision_clock_output_caps caps;
	struct precision_clock_output_status status;
	struct precision_clock_output_config waveform = valid_output_config();
	struct precision_clock invalid_clock = {0};

	output_fake_init(&fake);
	zassert_equal(precision_clock_output_get_caps(NULL, &caps), -EINVAL);
	zassert_equal(precision_clock_output_get_caps(&fake.clock, NULL), -EINVAL);
	zassert_equal(precision_clock_output_get_caps(&invalid_clock, &caps), -EINVAL);
	zassert_equal(precision_clock_output_start(NULL, 0, &waveform), -EINVAL);
	zassert_equal(precision_clock_output_start(&fake.clock, 0, NULL), -EINVAL);
	zassert_equal(precision_clock_output_stop(NULL, 0), -EINVAL);
	zassert_equal(precision_clock_output_get_status(NULL, 0, &status), -EINVAL);
	zassert_equal(precision_clock_output_get_status(&fake.clock, 0, NULL), -EINVAL);
}

ZTEST(precision_timing, test_clock_output_reports_unsupported_operations)
{
	struct output_fake fake;
	struct precision_clock_output_caps caps;
	struct precision_clock_output_status status;
	struct precision_clock_output_config waveform = valid_output_config();

	output_fake_init(&fake);
	fake.clock.api = &output_unsupported_api;
	zassert_equal(precision_clock_output_get_caps(&fake.clock, &caps), -ENOTSUP);
	zassert_equal(precision_clock_output_start(&fake.clock, 0, &waveform), -ENOTSUP);
	zassert_equal(precision_clock_output_stop(&fake.clock, 0), -ENOTSUP);
	zassert_equal(precision_clock_output_get_status(&fake.clock, 0, &status), -ENOTSUP);
}

ZTEST(precision_timing, test_clock_output_calculates_next_aligned_start)
{
	precision_time_t start;
	precision_time_t now = 1999LL * NSEC_PER_MSEC;

	zassert_ok(
		precision_clock_output_next_start_time(now, NSEC_PER_SEC, NSEC_PER_MSEC, &start));
	zassert_equal(start, 2LL * NSEC_PER_SEC);

	now++;
	zassert_ok(
		precision_clock_output_next_start_time(now, NSEC_PER_SEC, NSEC_PER_MSEC, &start));
	zassert_equal(start, 3LL * NSEC_PER_SEC);

	now = -1500LL * NSEC_PER_MSEC;
	zassert_ok(precision_clock_output_next_start_time(now, NSEC_PER_SEC, 0, &start));
	zassert_equal(start, -(precision_time_t)NSEC_PER_SEC);
	now = -2LL * NSEC_PER_SEC;
	zassert_ok(precision_clock_output_next_start_time(now, NSEC_PER_SEC, 0, &start));
	zassert_equal(start, now);

	zassert_equal(precision_clock_output_next_start_time(now, 0, 0, &start), -EINVAL);
	zassert_equal(precision_clock_output_next_start_time(now, NSEC_PER_SEC, -1, &start),
		      -EINVAL);
	zassert_equal(precision_clock_output_next_start_time(now, NSEC_PER_SEC, 0, NULL), -EINVAL);

	now = PRECISION_TIME_MAX;
	zassert_equal(precision_clock_output_next_start_time(now, NSEC_PER_SEC, 1, &start),
		      -ERANGE);
	zassert_equal(precision_clock_output_next_start_time(now, NSEC_PER_SEC, 0, &start),
		      -ERANGE);
}

ZTEST(precision_timing, test_clock_output_discovers_channels)
{
	struct output_fake fake;
	struct precision_clock_output_caps caps;

	output_fake_init(&fake);
	zassert_ok(precision_clock_output_get_caps(&fake.clock, &caps));
	zassert_equal(caps.channel_count, 2);
	zassert_equal(caps.channels[0].resolution_ns, 10);
	zassert_equal(caps.channels[1].resolution_ns, 20);
	zassert_equal(caps.channels[1].min_period_ns, 200);

	fake.caps_error = -EIO;
	zassert_equal(precision_clock_output_get_caps(&fake.clock, &caps), -EIO);
}

ZTEST(precision_timing, test_clock_output_uses_selected_channel_constraints)
{
	struct output_fake fake;
	struct precision_clock_output_config config = valid_output_config();

	output_fake_init(&fake);
	config.waveform.period_ns = 100;
	config.waveform.pulse_width_ns = 20;
	zassert_equal(precision_clock_output_start(&fake.clock, 1, &config), -ERANGE);
	zassert_ok(precision_clock_output_start(&fake.clock, 0, &config));
	zassert_ok(precision_clock_output_stop(&fake.clock, 0));
	config = valid_output_config();
	config.waveform.first_rising_time = 2010;
	zassert_equal(precision_clock_output_start(&fake.clock, 1, &config), -ERANGE);
	zassert_ok(precision_clock_output_start(&fake.clock, 0, &config));
	zassert_ok(precision_clock_output_stop(&fake.clock, 0));
	config.waveform.first_rising_time = 1100;
	zassert_equal(precision_clock_output_start(&fake.clock, 1, &config), -ETIME);
	zassert_ok(precision_clock_output_start(&fake.clock, 0, &config));
	zassert_ok(precision_clock_output_stop(&fake.clock, 0));
	zassert_equal(precision_clock_output_start(&fake.clock, 2, &config), -ENOTSUP);
}

ZTEST(precision_timing, test_clock_output_discovers_no_channels)
{
	struct output_fake fake;
	struct precision_clock_output_caps caps;
	struct precision_clock_output_config config = valid_output_config();

	output_fake_init(&fake);
	fake.caps = (struct precision_clock_output_caps){0};
	zassert_ok(precision_clock_output_get_caps(&fake.clock, &caps));
	zassert_equal(caps.channel_count, 0);
	zassert_is_null(caps.channels);
	zassert_equal(precision_clock_output_start(&fake.clock, 0, &config), -ENOTSUP);
}

ZTEST(precision_timing, test_clock_output_starts_waveform_exact_width)
{
	struct output_fake fake;
	struct precision_clock_output_status status;
	struct precision_clock_output_config waveform = valid_output_config();

	output_fake_init(&fake);
	zassert_ok(precision_clock_output_start(&fake.clock, 1, &waveform));
	zassert_equal(precision_clock_output_start(&fake.clock, 1, &waveform), -EBUSY);

	zassert_ok(precision_clock_output_get_status(&fake.clock, 1, &status));
	zassert_true(status.configured);
	zassert_equal(status.config.kind, PRECISION_CLOCK_OUTPUT_KIND_WAVEFORM);
	zassert_equal(status.config.waveform.first_rising_time,
		      waveform.waveform.first_rising_time);
	zassert_equal(status.config.waveform.period_ns, waveform.waveform.period_ns);
	zassert_equal(status.config.waveform.width_policy, PRECISION_CLOCK_OUTPUT_WIDTH_EXACT);
	zassert_equal(status.config.waveform.pulse_width_ns, waveform.waveform.pulse_width_ns);
	zassert_true(status.hardware_active_valid);
	zassert_true(status.hardware_active);
}

ZTEST(precision_timing, test_clock_output_requires_waveform_capability)
{
	struct output_fake fake;
	struct precision_clock_output_config waveform = valid_output_config();

	output_fake_init(&fake);
	fake.channels[0].flags &= ~PRECISION_CLOCK_OUTPUT_CAP_WAVEFORM;
	zassert_equal(precision_clock_output_start(&fake.clock, 0, &waveform), -ENOTSUP);
}

ZTEST(precision_timing, test_clock_output_starts_waveform_provider_default_width)
{
	struct output_fake fake;
	struct precision_clock_output_config waveform = valid_output_config();

	output_fake_init(&fake);
	fake.channels[0].flags = PRECISION_CLOCK_OUTPUT_CAP_WAVEFORM;
	fake.channels[0].min_pulse_width_ns = 0;
	fake.channels[0].max_pulse_width_ns = 0;
	waveform.waveform.width_policy = PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT;
	waveform.waveform.pulse_width_ns = 0;
	zassert_ok(precision_clock_output_start(&fake.clock, 0, &waveform));
	zassert_ok(precision_clock_output_stop(&fake.clock, 0));

	waveform = valid_output_config();
	zassert_equal(precision_clock_output_start(&fake.clock, 0, &waveform), -ENOTSUP);
}

ZTEST(precision_timing, test_clock_output_rejects_invalid_waveform_arguments)
{
	struct output_fake fake;
	struct precision_clock_output_config waveform = valid_output_config();

	output_fake_init(&fake);
	waveform.kind = (enum precision_clock_output_kind)5;
	zassert_equal(precision_clock_output_start(&fake.clock, 0, &waveform), -EINVAL);
	waveform = valid_output_config();
	waveform.waveform.width_policy = (enum precision_clock_output_width_policy)5;
	zassert_equal(precision_clock_output_start(&fake.clock, 0, &waveform), -EINVAL);
	waveform = valid_output_config();
	waveform.waveform.period_ns = 0;
	zassert_equal(precision_clock_output_start(&fake.clock, 0, &waveform), -EINVAL);
	waveform = valid_output_config();
	waveform.waveform.pulse_width_ns = 0;
	zassert_equal(precision_clock_output_start(&fake.clock, 0, &waveform), -EINVAL);
	waveform = valid_output_config();
	waveform.waveform.pulse_width_ns = waveform.waveform.period_ns;
	zassert_equal(precision_clock_output_start(&fake.clock, 0, &waveform), -EINVAL);
}

ZTEST(precision_timing, test_clock_output_rejects_nonpositive_resolution)
{
	struct output_fake fake;
	struct precision_clock_output_config config = valid_output_config();

	output_fake_init(&fake);
	fake.channels[0].resolution_ns = 0;
	fake.channels[1].resolution_ns = -1;
	zassert_equal(precision_clock_output_start(&fake.clock, 0, &config), -EINVAL);
	zassert_equal(precision_clock_output_start(&fake.clock, 1, &config), -EINVAL);
	zassert_false(fake.provider_configured);
}

ZTEST(precision_timing, test_clock_output_validates_waveform_limits_and_resolution)
{
	struct output_fake fake;
	struct precision_clock_output_config waveform = valid_output_config();

	output_fake_init(&fake);
	waveform.waveform.period_ns =
		fake.channels[0].min_period_ns - fake.channels[0].resolution_ns;
	waveform.waveform.pulse_width_ns = fake.channels[0].min_pulse_width_ns;
	zassert_equal(precision_clock_output_start(&fake.clock, 0, &waveform), -ERANGE);
	waveform = valid_output_config();
	waveform.waveform.period_ns =
		fake.channels[0].max_period_ns + fake.channels[0].resolution_ns;
	zassert_equal(precision_clock_output_start(&fake.clock, 0, &waveform), -ERANGE);
	waveform = valid_output_config();
	waveform.waveform.pulse_width_ns =
		fake.channels[0].min_pulse_width_ns - fake.channels[0].resolution_ns;
	zassert_equal(precision_clock_output_start(&fake.clock, 0, &waveform), -ERANGE);
	waveform = valid_output_config();
	waveform.waveform.period_ns = 6000;
	waveform.waveform.pulse_width_ns =
		fake.channels[0].max_pulse_width_ns + fake.channels[0].resolution_ns;
	zassert_equal(precision_clock_output_start(&fake.clock, 0, &waveform), -ERANGE);

	waveform = valid_output_config();
	waveform.waveform.first_rising_time++;
	zassert_equal(precision_clock_output_start(&fake.clock, 0, &waveform), -ERANGE);
	waveform = valid_output_config();
	waveform.waveform.period_ns++;
	zassert_equal(precision_clock_output_start(&fake.clock, 0, &waveform), -ERANGE);
	waveform = valid_output_config();
	waveform.waveform.pulse_width_ns++;
	zassert_equal(precision_clock_output_start(&fake.clock, 0, &waveform), -ERANGE);
}

ZTEST(precision_timing, test_clock_output_enforces_minimum_lead_time)
{
	struct output_fake fake;
	struct precision_clock_output_config config;
	const enum precision_clock_output_width_policy policies[] = {
		PRECISION_CLOCK_OUTPUT_WIDTH_EXACT,
		PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT,
	};

	for (size_t i = 0; i < ARRAY_SIZE(policies); i++) {
		output_fake_init(&fake);
		config = valid_output_config();
		config.waveform.width_policy = policies[i];
		if (policies[i] == PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT) {
			config.waveform.pulse_width_ns = 0;
		}
		config.waveform.first_rising_time = fake.now_ns - fake.channels[0].resolution_ns;
		zassert_equal(precision_clock_output_start(&fake.clock, 0, &config), -ETIME);
		config.waveform.first_rising_time = fake.now_ns +
			fake.channels[0].min_lead_time_ns - fake.channels[0].resolution_ns;
		zassert_equal(precision_clock_output_start(&fake.clock, 0, &config), -ETIME);
		config.waveform.first_rising_time = fake.now_ns + fake.channels[0].min_lead_time_ns;
		zassert_ok(precision_clock_output_start(&fake.clock, 0, &config));
		zassert_ok(precision_clock_output_stop(&fake.clock, 0));

		fake.now_ns = PRECISION_TIME_MAX - 57;
		config.waveform.first_rising_time = PRECISION_TIME_MAX - 7;
		zassert_equal(precision_clock_output_start(&fake.clock, 0, &config), -ERANGE);
	}
}

ZTEST(precision_timing, test_clock_output_start_distinguishes_validation_and_provider_results)
{
	const int reserved_errors[] = {-ETIME, -EINPROGRESS};
	struct precision_clock_output_config config = valid_output_config();
	struct precision_clock_output_status status;
	struct output_fake fake;

	output_fake_init(&fake);
	for (size_t i = 0; i < ARRAY_SIZE(reserved_errors); i++) {
		fake.caps_error = reserved_errors[i];
		zassert_equal(precision_clock_output_start(&fake.clock, 0, &config), -EIO);
		fake.caps_error = 0;
		fake.read_error = reserved_errors[i];
		zassert_equal(precision_clock_output_start(&fake.clock, 0, &config), -EIO);
		fake.read_error = 0;
		zassert_false(fake.provider_configured);
	}

	fake.start_error = -ETIME;
	zassert_equal(precision_clock_output_start(&fake.clock, 0, &config), -ETIME);
	zassert_false(fake.provider_configured);

	fake.start_error = -EINPROGRESS;
	zassert_equal(precision_clock_output_start(&fake.clock, 0, &config), -EINPROGRESS);
	zassert_ok(precision_clock_output_get_status(&fake.clock, 0, &status));
	zassert_true(status.configured);
	zassert_equal(status.config.waveform.first_rising_time, config.waveform.first_rising_time);
	zassert_ok(precision_clock_output_stop(&fake.clock, 0));
	zassert_false(fake.provider_configured);
}

ZTEST(precision_timing, test_clock_output_reports_historical_status_without_clock_or_caps)
{
	struct output_fake fake;
	struct precision_clock_output_status status;
	struct precision_clock_output_config config = valid_output_config();
	struct precision_clock_api api = output_fake_api;

	output_fake_init(&fake);
	zassert_ok(precision_clock_output_start(&fake.clock, 0, &config));
	fake.now_ns = config.waveform.first_rising_time + NSEC_PER_SEC;
	fake.read_error = -EIO;
	fake.caps_error = -EIO;
	zassert_ok(precision_clock_output_get_status(&fake.clock, 0, &status));
	zassert_true(status.configured);
	zassert_equal(status.config.kind, config.kind);
	zassert_equal(status.config.waveform.first_rising_time, config.waveform.first_rising_time);

	api.output_get_caps = NULL;
	api.read = NULL;
	fake.clock.api = &api;
	zassert_ok(precision_clock_output_get_status(&fake.clock, 0, &status));
	zassert_true(status.configured);
	zassert_equal(status.config.waveform.first_rising_time, config.waveform.first_rising_time);
	zassert_ok(precision_clock_output_stop(&fake.clock, 0));
	zassert_ok(precision_clock_output_get_status(&fake.clock, 0, &status));
	zassert_false(status.configured);
}

ZTEST(precision_timing, test_clock_output_stop_and_propagates_errors)
{
	struct output_fake fake;
	struct precision_clock_output_status status;
	struct precision_clock_output_config waveform = valid_output_config();

	output_fake_init(&fake);
	zassert_ok(precision_clock_output_stop(&fake.clock, 0));
	zassert_equal(precision_clock_output_stop(&fake.clock, 2), -ENOTSUP);
	zassert_equal(precision_clock_output_get_status(&fake.clock, 2, &status), -ENOTSUP);

	fake.start_error = -EIO;
	zassert_equal(precision_clock_output_start(&fake.clock, 0, &waveform), -EIO);
	fake.stop_error = -EBUSY;
	zassert_equal(precision_clock_output_stop(&fake.clock, 0), -EBUSY);
	fake.status_error = -EAGAIN;
	zassert_equal(precision_clock_output_get_status(&fake.clock, 0, &status), -EAGAIN);
}
