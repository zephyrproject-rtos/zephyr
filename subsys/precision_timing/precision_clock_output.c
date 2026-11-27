/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>

#include <zephyr/precision_timing/precision_clock_output.h>

int precision_clock_output_get_caps(const struct precision_clock *precision_clk,
				    struct precision_clock_output_caps *caps)
{
	if (precision_clk == NULL || caps == NULL || precision_clk->api == NULL) {
		return -EINVAL;
	}

	if (precision_clk->api->output_get_caps == NULL) {
		return -ENOTSUP;
	}

	return precision_clk->api->output_get_caps(precision_clk, caps);
}

static int start_prerequisite_error(int ret)
{
	/* Neither a capability query nor a clock read attempted to start the output. */
	return (ret == -EINPROGRESS || ret == -ETIME) ? -EIO : ret;
}

static int precision_clock_output_check_waveform_config(
	const struct precision_clock *precision_clk,
	const struct precision_clock_output_channel_caps *caps,
	const struct precision_clock_output_waveform_config *config)
{
	precision_time_t now;
	precision_time_t earliest;
	bool exact_width;
	int ret;

	if ((caps->flags & PRECISION_CLOCK_OUTPUT_CAP_WAVEFORM) == 0U) {
		return -ENOTSUP;
	}

	switch (config->width_policy) {
	case PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT:
		exact_width = false;
		break;
	case PRECISION_CLOCK_OUTPUT_WIDTH_EXACT:
		exact_width = true;
		break;
	default:
		return -EINVAL;
	}

	if (config->period_ns <= 0 ||
	    (exact_width &&
	     (config->pulse_width_ns <= 0 || config->pulse_width_ns >= config->period_ns))) {
		return -EINVAL;
	}

	if (exact_width && (caps->flags & PRECISION_CLOCK_OUTPUT_CAP_PROGRAMMABLE_WIDTH) == 0U) {
		return -ENOTSUP;
	}

	if (caps->resolution_ns <= 0) {
		return -EINVAL;
	}

	if (config->period_ns < caps->min_period_ns || config->period_ns > caps->max_period_ns ||
	    (config->period_ns % caps->resolution_ns) != 0 ||
	    (config->first_rising_time % caps->resolution_ns) != 0) {
		return -ERANGE;
	}

	if (exact_width &&
	    (config->pulse_width_ns < caps->min_pulse_width_ns ||
	     config->pulse_width_ns > caps->max_pulse_width_ns ||
	     (config->pulse_width_ns % caps->resolution_ns) != 0)) {
		return -ERANGE;
	}

	ret = precision_clock_read(precision_clk, &now);
	if (ret != 0) {
		return start_prerequisite_error(ret);
	}

	if (config->first_rising_time < now) {
		return -ETIME;
	}

	ret = precision_time_add(now, caps->min_lead_time_ns, &earliest);
	if (ret != 0) {
		return ret;
	}

	if (config->first_rising_time < earliest) {
		return -ETIME;
	}

	return 0;
}

int precision_clock_output_next_start_time(precision_time_t now, precision_time_t period_ns,
					   precision_time_t min_lead_time_ns,
					   precision_time_t *start_time)
{
	precision_time_t boundary;
	precision_time_t remainder;
	int ret;

	if (start_time == NULL || period_ns <= 0 || min_lead_time_ns < 0) {
		return -EINVAL;
	}

	ret = precision_time_add(now, min_lead_time_ns, &boundary);
	if (ret != 0) {
		return ret;
	}

	remainder = boundary % period_ns;
	if (remainder > 0) {
		ret = precision_time_add(boundary, period_ns - remainder, &boundary);
	} else if (remainder < 0) {
		ret = precision_time_sub(boundary, remainder, &boundary);
	} else {
		/* The boundary already meets the alignment requirement. */
	}
	if (ret != 0) {
		return ret;
	}

	*start_time = boundary;

	return 0;
}

int precision_clock_output_start(const struct precision_clock *precision_clk, uint32_t channel,
				 const struct precision_clock_output_config *config)
{
	struct precision_clock_output_caps caps;
	int ret;

	if (precision_clk == NULL || config == NULL || precision_clk->api == NULL) {
		return -EINVAL;
	}

	if (precision_clk->api->output_start == NULL) {
		return -ENOTSUP;
	}

	ret = precision_clock_output_get_caps(precision_clk, &caps);
	if (ret != 0) {
		return start_prerequisite_error(ret);
	}

	if (channel >= caps.channel_count) {
		return -ENOTSUP;
	}

	switch (config->kind) {
	case PRECISION_CLOCK_OUTPUT_KIND_WAVEFORM:
		ret = precision_clock_output_check_waveform_config(
			precision_clk, &caps.channels[channel], &config->waveform);
		break;
	default:
		return -EINVAL;
	}
	if (ret != 0) {
		return ret;
	}

	return precision_clk->api->output_start(precision_clk, channel, config);
}

int precision_clock_output_stop(const struct precision_clock *precision_clk, uint32_t channel)
{
	if (precision_clk == NULL || precision_clk->api == NULL) {
		return -EINVAL;
	}

	if (precision_clk->api->output_stop == NULL) {
		return -ENOTSUP;
	}

	return precision_clk->api->output_stop(precision_clk, channel);
}

int precision_clock_output_get_status(const struct precision_clock *precision_clk, uint32_t channel,
				      struct precision_clock_output_status *status)
{
	if (precision_clk == NULL || status == NULL || precision_clk->api == NULL) {
		return -EINVAL;
	}

	if (precision_clk->api->output_get_status == NULL) {
		return -ENOTSUP;
	}

	return precision_clk->api->output_get_status(precision_clk, channel, status);
}
