/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/sys/util.h>
#include <zephyr/precision_timing/precision_clock.h>
#include <zephyr/precision_timing/precision_timing_shell.h>
#include <zephyr/ztest.h>

struct fake_clock {
	struct precision_clock clock;
	struct precision_clock_output_caps output_caps;
	struct precision_clock_output_status output_status;
	struct precision_clock_output_waveform_config last_waveform_config;
	precision_time_t time;
	int read_error;
	int output_caps_error;
	int start_waveform_error;
	int start_waveform_error_once;
	bool retain_start_error_config;
	int output_stop_error;
	int output_status_error;
	precision_time_t read_advance_ns;
	uint32_t last_output_channel;
	uint32_t output_caps_calls;
	uint32_t start_waveform_calls;
	uint32_t output_stop_calls;
	uint32_t output_status_calls;
};

static const struct shell *test_shell;

static struct fake_clock clock_fake;
static struct fake_clock uninitialized_fake;

static struct fake_clock *fake_clock_from_precision(const struct precision_clock *precision_clk)
{
	return (struct fake_clock *)precision_clk->data;
}

static int fake_clock_read(const struct precision_clock *precision_clk, precision_time_t *tm)
{
	struct fake_clock *fake = fake_clock_from_precision(precision_clk);

	if (fake->read_error < 0) {
		return fake->read_error;
	}

	*tm = fake->time;
	fake->time += fake->read_advance_ns;
	return 0;
}

static int fake_clock_get_output_caps(const struct precision_clock *precision_clk, uint32_t channel,
				      struct precision_clock_output_caps *caps)
{
	struct fake_clock *fake = fake_clock_from_precision(precision_clk);

	fake->output_caps_calls++;
	fake->last_output_channel = channel;
	if (fake->output_caps_error < 0) {
		return fake->output_caps_error;
	}

	*caps = fake->output_caps;
	return 0;
}

static void fake_clock_mark_configured(struct fake_clock *fake,
				       enum precision_clock_output_kind kind)
{
	fake->output_status.configured = true;
	fake->output_status.kind = kind;
	fake->output_status.hardware_active_valid =
		(fake->output_caps.flags & PRECISION_CLOCK_OUTPUT_CAP_HARDWARE_ACTIVE) != 0U;
	fake->output_status.hardware_active = true;
}

static int
fake_clock_output_start_waveform(const struct precision_clock *precision_clk, uint32_t channel,
				 const struct precision_clock_output_waveform_config *config)
{
	struct fake_clock *fake = fake_clock_from_precision(precision_clk);

	fake->start_waveform_calls++;
	fake->last_output_channel = channel;
	fake->last_waveform_config = *config;
	if (fake->start_waveform_error_once < 0) {
		int error = fake->start_waveform_error_once;

		fake->start_waveform_error_once = 0;
		if (fake->retain_start_error_config) {
			fake_clock_mark_configured(fake, PRECISION_CLOCK_OUTPUT_KIND_WAVEFORM);
			fake->output_status.config.waveform = *config;
		}
		return error;
	}
	if (fake->start_waveform_error < 0) {
		return fake->start_waveform_error;
	}
	if (fake->output_status.configured) {
		return -EBUSY;
	}

	fake_clock_mark_configured(fake, PRECISION_CLOCK_OUTPUT_KIND_WAVEFORM);
	fake->output_status.config.waveform = *config;
	return 0;
}

static int fake_clock_output_stop(const struct precision_clock *precision_clk, uint32_t channel)
{
	struct fake_clock *fake = fake_clock_from_precision(precision_clk);

	fake->last_output_channel = channel;
	if (channel >= fake->output_caps.channel_count) {
		return -ENOTSUP;
	}

	fake->output_stop_calls++;
	if (fake->output_stop_error < 0) {
		return fake->output_stop_error;
	}

	fake->output_status.configured = false;
	return 0;
}

static int fake_clock_get_output_status(const struct precision_clock *precision_clk,
					uint32_t channel,
					struct precision_clock_output_status *status)
{
	struct fake_clock *fake = fake_clock_from_precision(precision_clk);

	fake->output_status_calls++;
	fake->last_output_channel = channel;
	if (fake->output_status_error < 0) {
		return fake->output_status_error;
	}

	*status = fake->output_status;
	return 0;
}

static const struct precision_clock_api fake_clock_api = {
	.read = fake_clock_read,
	.get_output_caps = fake_clock_get_output_caps,
	.output_start_waveform = fake_clock_output_start_waveform,
	.output_stop = fake_clock_output_stop,
	.get_output_status = fake_clock_get_output_status,
};

static void fake_clock_init(struct fake_clock *fake, precision_time_t tm)
{
	memset(fake, 0, sizeof(*fake));
	fake->clock.api = &fake_clock_api;
	fake->clock.data = fake;
	fake->time = tm;
}

#if defined(CONFIG_PRECISION_CLOCK_OUTPUT)
static void fake_clock_enable_output(struct fake_clock *fake)
{
	fake->output_caps = (struct precision_clock_output_caps){
		.flags = PRECISION_CLOCK_OUTPUT_CAP_WAVEFORM |
			 PRECISION_CLOCK_OUTPUT_CAP_PROGRAMMABLE_WIDTH |
			 PRECISION_CLOCK_OUTPUT_CAP_HARDWARE_ACTIVE,
		.channel_count = 2,
		.resolution_ns = 10,
		.min_lead_time_ns = NSEC_PER_MSEC,
		.min_period_ns = 100,
		.max_period_ns = 2 * (precision_time_t)NSEC_PER_SEC,
		.min_pulse_width_ns = 10,
		.max_pulse_width_ns = NSEC_PER_SEC - 10,
	};
}
#endif /* CONFIG_PRECISION_CLOCK_OUTPUT */

/* Statically register the clocks used by the shell tests. */
PRECISION_TIMING_SHELL_REGISTER(clock, &clock_fake.clock);
/* A clock whose api is left NULL to exercise the uninitialized-clock path. */
PRECISION_TIMING_SHELL_REGISTER(uninit, &uninitialized_fake.clock);

static int execute(const char *command)
{
	shell_backend_dummy_clear_output(test_shell);
	return shell_execute_cmd(test_shell, command);
}

static const char *output_get(void)
{
	size_t output_size;

	return shell_backend_dummy_get_output(test_shell, &output_size);
}

static void assert_output_contains(const char *output, const char *expected)
{
	zassert_not_null(strstr(output, expected), "output '%s' does not contain '%s'", output,
			 expected);
}

#if defined(CONFIG_PRECISION_CLOCK_OUTPUT)
static void assert_output_not_contains(const char *output, const char *unexpected)
{
	zassert_is_null(strstr(output, unexpected), "output '%s' unexpectedly contains '%s'",
			output, unexpected);
}
#endif /* CONFIG_PRECISION_CLOCK_OUTPUT */

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);

#if defined(CONFIG_PRECISION_CLOCK_OUTPUT)
	fake_clock_init(&clock_fake, NSEC_PER_SEC);
	fake_clock_enable_output(&clock_fake);
#else
	fake_clock_init(&clock_fake, 0);
#endif
	memset(&uninitialized_fake, 0, sizeof(uninitialized_fake));
	shell_backend_dummy_clear_output(test_shell);
}

static void *setup(void)
{
	test_shell = shell_backend_dummy_get_ptr();
	WAIT_FOR(shell_ready(test_shell), 20000, k_msleep(1));
	zassert_true(shell_ready(test_shell), "timed out waiting for dummy shell backend");

	return NULL;
}

ZTEST(precision_timing_shell, test_list_includes_registered_clocks)
{
	const char *output;

	zassert_ok(execute("precision_clock list"));
	output = output_get();
	assert_output_contains(output, "clock");
	assert_output_contains(output, "uninit");
}

ZTEST(precision_timing_shell, test_get_clock_and_provider_errors)
{
	const char *output;

	clock_fake.time = -123;
	zassert_ok(execute("precision_clock get clock"));
	output = output_get();
	assert_output_contains(output, "time_ns=-123");

	clock_fake.read_error = -EIO;
	zassert_equal(execute("precision_clock get clock"), -EIO);
	assert_output_contains(output_get(), "precision clock 'clock': -5");

	/* An uninitialized clock (api == NULL) is rejected before calling read. */
	zassert_equal(execute("precision_clock get uninit"), -ENODEV);
	assert_output_contains(output_get(), "precision clock 'uninit': -19");

	zassert_equal(execute("precision_clock get missing"), -ENOENT);
	assert_output_contains(output_get(), "precision clock 'missing': -2");
}

#if defined(CONFIG_PRECISION_CLOCK_OUTPUT)
ZTEST(precision_timing_shell, test_output_caps_and_status_text)
{
	const char *output;

	clock_fake.time = NSEC_PER_SEC;
	fake_clock_enable_output(&clock_fake);

	zassert_ok(execute("precision_clock output caps clock 1"));
	output = output_get();
	assert_output_contains(output,
			       "waveform=yes programmable_width=yes channel_count=2");
	assert_output_contains(output, "hardware_active=yes");
	assert_output_contains(output, "resolution_ns=10 min_lead_time_ns=1000000");
	assert_output_contains(output, "min_period_ns=100 max_period_ns=2000000000");
	assert_output_contains(output, "min_pulse_width_ns=10 max_pulse_width_ns=999999990");
	zassert_equal(clock_fake.last_output_channel, 1);

	zassert_ok(execute("precision_clock output get clock 0"));
	output = output_get();
	assert_output_contains(output, "configured=no");
	assert_output_contains(output, "hardware_active=unknown");
	assert_output_not_contains(output, "kind=");
	assert_output_not_contains(output, "first_rising_time_ns=");

	/* An out-of-range channel is rejected by the generic capability check. */
	zassert_equal(execute("precision_clock output caps clock 2"), -ENOTSUP);
	assert_output_contains(output_get(), "precision clock 'clock':");
	zassert_equal(execute("precision_clock output caps clock 4294967296"), -ERANGE);
	assert_output_contains(output_get(), "invalid channel '4294967296': -34");
	zassert_equal(execute("precision_clock output caps missing 0"), -ENOENT);
	assert_output_contains(output_get(), "precision clock 'missing': -2");

	clock_fake.output_caps_error = -EIO;
	zassert_equal(execute("precision_clock output caps clock 0"), -EIO);
	assert_output_contains(output_get(), "precision clock 'clock': -5");
	clock_fake.output_caps_error = 0;
	clock_fake.output_status_error = -ESTALE;
	zassert_equal(execute("precision_clock output get clock 0"), -ESTALE);
	assert_output_contains(output_get(), "precision clock 'clock': -116");
}

ZTEST(precision_timing_shell, test_output_waveform_start_forwarding_and_errors)
{
	const char *output;

	clock_fake.time = NSEC_PER_SEC;
	fake_clock_enable_output(&clock_fake);

	/* An exact-width waveform forwards the width policy and pulse width. */
	zassert_ok(
		execute("precision_clock output waveform clock 1 5000000000 1000000000 100000000"));
	zassert_equal(clock_fake.start_waveform_calls, 1);
	zassert_equal(clock_fake.last_output_channel, 1);
	zassert_equal(clock_fake.last_waveform_config.first_rising_time, 5000000000LL);
	zassert_equal(clock_fake.last_waveform_config.period_ns, NSEC_PER_SEC);
	zassert_equal(clock_fake.last_waveform_config.width_policy,
		      PRECISION_CLOCK_OUTPUT_WIDTH_EXACT);
	zassert_equal(clock_fake.last_waveform_config.pulse_width_ns, 100 * NSEC_PER_MSEC);

	zassert_ok(execute("precision_clock output get clock 1"));
	output = output_get();
	assert_output_contains(output, "configured=yes");
	assert_output_contains(output, "kind=waveform");
	assert_output_contains(output, "first_rising_time_ns=5000000000");
	assert_output_contains(output,
			       "period_ns=1000000000 width_policy=exact pulse_width_ns=100000000");
	assert_output_contains(output, "hardware_active=yes");

	zassert_equal(execute("precision_clock output waveform clock 1 6000000000 1000000000 "
			      "100000000"),
		      -EBUSY);
	assert_output_contains(output_get(), "precision clock 'clock': -16");
	zassert_ok(execute("precision_clock output stop clock 1"));

	/* Omitting the pulse width selects the provider-default width policy. */
	zassert_ok(execute("precision_clock output waveform clock 0 5000000000 1000000000"));
	zassert_equal(clock_fake.last_waveform_config.width_policy,
		      PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT);
	zassert_ok(execute("precision_clock output get clock 0"));
	output = output_get();
	assert_output_contains(output, "period_ns=1000000000 width_policy=default");
	assert_output_not_contains(output, "pulse_width_ns=");

	/* Hardware activity prints as unknown when the provider cannot observe it. */
	clock_fake.output_status.hardware_active_valid = false;
	zassert_ok(execute("precision_clock output get clock 0"));
	assert_output_contains(output_get(), "hardware_active=unknown");
	zassert_ok(execute("precision_clock output stop clock 0"));

	/* Negative absolute times remain valid in the owning clock's timescale. */
	clock_fake.time = -5 * (precision_time_t)NSEC_PER_SEC;
	zassert_ok(execute("precision_clock output waveform clock 0 -2000000000 1000000000"));
	zassert_equal(clock_fake.last_waveform_config.first_rising_time, -2000000000LL);
	zassert_ok(execute("precision_clock output stop clock 0"));
	clock_fake.time = NSEC_PER_SEC;

	/* Argument and provider errors. */
	zassert_equal(execute("precision_clock output waveform clock 0 9223372036854775808 "
			      "1000000000"), -ERANGE);
	assert_output_contains(output_get(), "invalid first rising time '9223372036854775808'");
	zassert_equal(execute("precision_clock output waveform missing 0 5000000000 1000000000"),
		      -ENOENT);
	assert_output_contains(output_get(), "precision clock 'missing': -2");
	zassert_equal(execute("precision_clock output waveform clock 0 5000000000 -1"), -EINVAL);
	assert_output_contains(output_get(), "invalid period '-1': -22");
	clock_fake.start_waveform_error = -ETIME;
	zassert_equal(execute("precision_clock output waveform clock 0 5000000000 1000000000 "
			      "100000000"),
		      -ETIME);
	assert_output_contains(output_get(), "precision clock 'clock': -62");
}

ZTEST(precision_timing_shell, test_output_stop_forwarding_and_errors)
{
	clock_fake.time = NSEC_PER_SEC;
	fake_clock_enable_output(&clock_fake);

	zassert_ok(execute("precision_clock output stop clock 1"));
	zassert_equal(clock_fake.output_stop_calls, 1);
	zassert_equal(clock_fake.last_output_channel, 1);

	/* Out-of-range channels are rejected by the provider. */
	zassert_equal(execute("precision_clock output stop clock 2"), -ENOTSUP);

	clock_fake.output_stop_error = -EBUSY;
	zassert_equal(execute("precision_clock output stop clock 0"), -EBUSY);
	assert_output_contains(output_get(), "precision clock 'clock': -16");

	zassert_equal(execute("precision_clock output stop clock 4294967296"), -ERANGE);
	assert_output_contains(output_get(), "invalid channel '4294967296': -34");
	zassert_equal(execute("precision_clock output stop missing 0"), -ENOENT);
	assert_output_contains(output_get(), "precision clock 'missing': -2");
}

ZTEST(precision_timing_shell, test_pps_helper_scheduling_width_and_errors)
{
	uint32_t start_waveform_calls;
	const char *output;

	clock_fake.time = 3500000000LL;
	fake_clock_enable_output(&clock_fake);

	zassert_ok(execute("precision_clock pps start clock 0"));
	output = output_get();
	assert_output_contains(output, "first_rising_time_ns=6000000000 period_ns=1000000000 "
				       "width_policy=exact pulse_width_ns=200000000");
	zassert_equal(clock_fake.last_waveform_config.first_rising_time, 6000000000LL);
	zassert_equal(clock_fake.last_waveform_config.period_ns, NSEC_PER_SEC);
	zassert_equal(clock_fake.last_waveform_config.width_policy,
		      PRECISION_CLOCK_OUTPUT_WIDTH_EXACT);
	zassert_equal(clock_fake.last_waveform_config.pulse_width_ns, 200 * NSEC_PER_MSEC);

	zassert_ok(execute("precision_clock pps stop clock 0"));
	clock_fake.time = 4 * (precision_time_t)NSEC_PER_SEC;
	clock_fake.read_advance_ns = NSEC_PER_MSEC;
	zassert_ok(execute("precision_clock pps start clock 1 250000000"));
	zassert_equal(clock_fake.last_waveform_config.first_rising_time, 7000000000LL);
	zassert_equal(clock_fake.last_waveform_config.pulse_width_ns, 250 * NSEC_PER_MSEC);
	zassert_ok(execute("precision_clock pps stop clock 1"));
	clock_fake.read_advance_ns = 0;

	clock_fake.output_caps.flags &= ~PRECISION_CLOCK_OUTPUT_CAP_PROGRAMMABLE_WIDTH;
	zassert_ok(execute("precision_clock pps start clock 0"));
	zassert_equal(clock_fake.last_waveform_config.width_policy,
		      PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT);
	assert_output_contains(output_get(), "width_policy=provider_default");
	zassert_ok(execute("precision_clock pps stop clock 0"));
	zassert_equal(execute("precision_clock pps start clock 0 200000000"), -ENOTSUP);
	clock_fake.output_caps.flags |= PRECISION_CLOCK_OUTPUT_CAP_PROGRAMMABLE_WIDTH;

	clock_fake.output_caps.min_lead_time_ns = 3 * (precision_time_t)NSEC_PER_SEC;
	clock_fake.time = 4 * (precision_time_t)NSEC_PER_SEC;
	clock_fake.read_advance_ns = NSEC_PER_MSEC;
	zassert_ok(execute("precision_clock pps start clock 0"));
	zassert_equal(clock_fake.last_waveform_config.first_rising_time, 9000000000LL);
	zassert_ok(execute("precision_clock pps stop clock 0"));
	clock_fake.output_caps.min_lead_time_ns = NSEC_PER_MSEC;

	clock_fake.time = 3500000000LL;
	clock_fake.read_advance_ns = NSEC_PER_SEC;
	clock_fake.start_waveform_error_once = -ETIME;
	start_waveform_calls = clock_fake.start_waveform_calls;
	zassert_ok(execute("precision_clock pps start clock 0"));
	zassert_equal(clock_fake.start_waveform_calls, start_waveform_calls + 2);
	zassert_equal(clock_fake.last_waveform_config.first_rising_time, 8000000000LL);
	zassert_ok(execute("precision_clock pps stop clock 0"));
	clock_fake.read_advance_ns = 0;

	start_waveform_calls = clock_fake.start_waveform_calls;
	clock_fake.time = PRECISION_TIME_MAX - NSEC_PER_SEC;
	zassert_equal(execute("precision_clock pps start clock 0"), -ERANGE);
	assert_output_contains(output_get(), "precision clock 'clock': -34");
	zassert_equal(clock_fake.start_waveform_calls, start_waveform_calls);

	clock_fake.time = 3500000000LL;
	clock_fake.read_error = -EIO;
	zassert_equal(execute("precision_clock pps start clock 0"), -EIO);
	assert_output_contains(output_get(), "precision clock 'clock': -5");
	clock_fake.read_error = 0;
	clock_fake.start_waveform_error = -ETIME;
	zassert_equal(execute("precision_clock pps start clock 0"), -ETIME);
	assert_output_contains(output_get(), "precision clock 'clock': -62");

	zassert_equal(execute("precision_clock pps start clock 0 -1"), -EINVAL);
	assert_output_contains(output_get(), "invalid pulse width '-1': -22");
}

ZTEST(precision_timing_shell, test_pps_start_preserves_existing_waveform)
{
	clock_fake.time = NSEC_PER_SEC;
	fake_clock_enable_output(&clock_fake);
	zassert_ok(execute("precision_clock output waveform clock 0 5000000000 1000000000"));

	/* A fresh target would expire during generic validation before reaching the provider. */
	clock_fake.time = 3500000000LL;
	clock_fake.read_advance_ns = 3 * (precision_time_t)NSEC_PER_SEC;
	zassert_equal(execute("precision_clock pps start clock 0"), -EBUSY);
	zassert_true(clock_fake.output_status.configured);
	zassert_equal(clock_fake.output_status.config.waveform.first_rising_time, 5000000000LL);
	zassert_equal(clock_fake.output_status.config.waveform.period_ns, NSEC_PER_SEC);
	zassert_equal(clock_fake.output_status.config.waveform.width_policy,
		      PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT);
	zassert_equal(clock_fake.output_stop_calls, 0);

	zassert_ok(execute("precision_clock pps stop clock 0"));
}

ZTEST(precision_timing_shell, test_pps_start_preserves_output_when_status_fails)
{
	clock_fake.time = NSEC_PER_SEC;
	fake_clock_enable_output(&clock_fake);
	zassert_ok(execute("precision_clock output waveform clock 0 5000000000 1000000000"));

	clock_fake.time = 3500000000LL;
	clock_fake.read_advance_ns = 3 * (precision_time_t)NSEC_PER_SEC;
	clock_fake.output_status_error = -EIO;
	zassert_equal(execute("precision_clock pps start clock 0"), -EIO);
	zassert_true(clock_fake.output_status.configured);
	zassert_equal(clock_fake.output_status.config.waveform.first_rising_time, 5000000000LL);
	zassert_equal(clock_fake.output_status.config.waveform.period_ns, NSEC_PER_SEC);
	zassert_equal(clock_fake.output_status.config.waveform.width_policy,
		      PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT);
	zassert_equal(clock_fake.output_stop_calls, 0);

	zassert_ok(execute("precision_clock pps stop clock 0"));
}

ZTEST(precision_timing_shell, test_pps_start_releases_retained_error_configuration)
{
	clock_fake.time = 3500000000LL;
	fake_clock_enable_output(&clock_fake);
	clock_fake.read_advance_ns = NSEC_PER_SEC;
	clock_fake.start_waveform_error_once = -ETIME;
	clock_fake.retain_start_error_config = true;

	zassert_ok(execute("precision_clock pps start clock 0"));
	zassert_equal(clock_fake.start_waveform_calls, 2);
	zassert_equal(clock_fake.output_stop_calls, 1);
	zassert_true(clock_fake.output_status.configured);
	zassert_equal(clock_fake.output_status.config.waveform.first_rising_time, 8000000000LL);
	zassert_ok(execute("precision_clock pps stop clock 0"));
	zassert_false(clock_fake.output_status.configured);
}

ZTEST(precision_timing_shell, test_pps_start_cleanup_failure_prevents_retry)
{
	clock_fake.time = 3500000000LL;
	fake_clock_enable_output(&clock_fake);
	clock_fake.start_waveform_error_once = -ETIME;
	clock_fake.retain_start_error_config = true;
	clock_fake.output_stop_error = -EIO;

	zassert_equal(execute("precision_clock pps start clock 0"), -EIO);
	zassert_equal(clock_fake.start_waveform_calls, 1);
	zassert_equal(clock_fake.output_stop_calls, 1);
	zassert_true(clock_fake.output_status.configured);
	clock_fake.output_stop_error = 0;
	zassert_ok(execute("precision_clock pps stop clock 0"));
	zassert_false(clock_fake.output_status.configured);
}

ZTEST(precision_timing_shell, test_pps_read_etime_does_not_stop_output)
{
	clock_fake.time = 3500000000LL;
	fake_clock_enable_output(&clock_fake);
	clock_fake.read_error = -ETIME;

	zassert_equal(execute("precision_clock pps start clock 0"), -ETIME);
	zassert_equal(clock_fake.start_waveform_calls, 0);
	zassert_equal(clock_fake.output_stop_calls, 0);
}

#endif /* CONFIG_PRECISION_CLOCK_OUTPUT */

ZTEST_SUITE(precision_timing_shell, NULL, setup, before_each, NULL, NULL);
