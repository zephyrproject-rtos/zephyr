/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <limits.h>

#include <zephyr/drivers/precision_clock_output.h>
#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/precision_timing/precision_clock_ptp.h>
#include <zephyr/sys/clock.h>

static const struct ptp_clock_driver_api *ptp_api(const struct precision_clock *precision_clk)
{
	const struct precision_clock_ptp_adapter *adapter = precision_clk->data;

	return DEVICE_API_GET(ptp_clock, adapter->dev);
}

static int precision_clock_ptp_read(const struct precision_clock *precision_clk,
				    precision_time_t *time_ns)
{
	const struct precision_clock_ptp_adapter *adapter = precision_clk->data;
	const struct ptp_clock_driver_api *api = ptp_api(precision_clk);
	struct net_ptp_time ptp_time;
	uint64_t seconds_limit = PRECISION_TIME_MAX / NSEC_PER_SEC;
	uint32_t nanoseconds_limit = PRECISION_TIME_MAX % NSEC_PER_SEC;
	int ret;

	ret = api->get(adapter->dev, &ptp_time);
	if (ret < 0) {
		return ret;
	}

	if (ptp_time.nanosecond >= NSEC_PER_SEC || ptp_time.second > seconds_limit ||
	    (ptp_time.second == seconds_limit && ptp_time.nanosecond > nanoseconds_limit)) {
		return -ERANGE;
	}

	*time_ns = (precision_time_t)ptp_time.second * NSEC_PER_SEC + ptp_time.nanosecond;

	return 0;
}

static int precision_clock_ptp_set(const struct precision_clock *precision_clk,
				   precision_time_t time_ns)
{
	const struct precision_clock_ptp_adapter *adapter = precision_clk->data;
	const struct ptp_clock_driver_api *api = ptp_api(precision_clk);
	struct net_ptp_time ptp_time;

	if (time_ns < 0) {
		return -ERANGE;
	}

	ptp_time.second = (uint64_t)(time_ns / NSEC_PER_SEC);
	ptp_time.nanosecond = (uint32_t)(time_ns % NSEC_PER_SEC);

	return api->set(adapter->dev, &ptp_time);
}

static int precision_clock_ptp_adjust_phase(const struct precision_clock *precision_clk,
					    precision_time_t phase_ns)
{
	const struct precision_clock_ptp_adapter *adapter = precision_clk->data;
	const struct ptp_clock_driver_api *api = ptp_api(precision_clk);

	if (phase_ns < INT_MIN || phase_ns > INT_MAX) {
		return -ERANGE;
	}

	return api->adjust(adapter->dev, (int)phase_ns);
}

static int precision_clock_ptp_adjust_rate(const struct precision_clock *precision_clk,
					   int64_t scaled_ppm)
{
	const struct precision_clock_ptp_adapter *adapter = precision_clk->data;
	const struct ptp_clock_driver_api *api = ptp_api(precision_clk);
	/* The existing PTP clock API expresses the adjustment as a rate ratio. */
	double rate_ratio =
		1.0 + (double)scaled_ppm / (1000000.0 * PRECISION_CLOCK_SCALED_PPM_ONE);

	return api->rate_adjust(adapter->dev, rate_ratio);
}

#ifdef CONFIG_PRECISION_CLOCK_OUTPUT
static const struct precision_clock_output_provider *
precision_clock_ptp_output_provider(const struct precision_clock_ptp_adapter *adapter)
{
	const struct ptp_clock_driver_api *api;

	if (adapter == NULL || adapter->dev == NULL) {
		return NULL;
	}

	api = DEVICE_API_GET(ptp_clock, adapter->dev);
	return api->output;
}

static int precision_clock_ptp_get_output_caps(const struct precision_clock *precision_clk,
					       uint32_t channel,
					       struct precision_clock_output_caps *caps)
{
	const struct precision_clock_ptp_adapter *adapter = precision_clk->data;
	const struct precision_clock_output_provider *provider;

	if (adapter == NULL || caps == NULL) {
		return -EINVAL;
	}

	provider = precision_clock_ptp_output_provider(adapter);
	if (provider == NULL || provider->get_caps == NULL) {
		return -ENOTSUP;
	}

	return provider->get_caps(adapter->dev, channel, caps);
}

static int precision_clock_ptp_output_start_waveform(
	const struct precision_clock *precision_clk, uint32_t channel,
	const struct precision_clock_output_waveform_config *config)
{
	const struct precision_clock_ptp_adapter *adapter = precision_clk->data;
	const struct precision_clock_output_provider *provider;
	struct precision_clock_output_raw_waveform_config raw_config;

	if (adapter == NULL || config == NULL) {
		return -EINVAL;
	}

	provider = precision_clock_ptp_output_provider(adapter);
	if (provider == NULL || provider->start_waveform == NULL) {
		return -ENOTSUP;
	}

	raw_config = (struct precision_clock_output_raw_waveform_config){
		.first_rising_time = config->first_rising_time,
		.period_ns = config->period_ns,
		.width_policy = config->width_policy,
		.pulse_width_ns = config->pulse_width_ns,
	};

	return provider->start_waveform(adapter->dev, channel, &raw_config);
}

static int precision_clock_ptp_output_stop(const struct precision_clock *precision_clk,
					   uint32_t channel)
{
	const struct precision_clock_ptp_adapter *adapter = precision_clk->data;
	const struct precision_clock_output_provider *provider;

	if (adapter == NULL) {
		return -EINVAL;
	}

	provider = precision_clock_ptp_output_provider(adapter);
	if (provider == NULL || provider->stop == NULL) {
		return -ENOTSUP;
	}

	return provider->stop(adapter->dev, channel);
}

static int precision_clock_ptp_get_output_status(const struct precision_clock *precision_clk,
						 uint32_t channel,
						 struct precision_clock_output_status *status)
{
	const struct precision_clock_ptp_adapter *adapter = precision_clk->data;
	const struct precision_clock_output_provider *provider;
	struct precision_clock_output_raw_status raw_status;
	int ret;

	if (adapter == NULL || status == NULL) {
		return -EINVAL;
	}

	provider = precision_clock_ptp_output_provider(adapter);
	if (provider == NULL || provider->get_status == NULL) {
		return -ENOTSUP;
	}

	ret = provider->get_status(adapter->dev, channel, &raw_status);
	if (ret < 0) {
		return ret;
	}

	*status = (struct precision_clock_output_status){
		.configured = raw_status.configured,
		.hardware_active_valid = raw_status.hardware_active_valid,
		.hardware_active = raw_status.hardware_active,
	};

	if (!raw_status.configured) {
		return 0;
	}

	status->kind = raw_status.kind;

	switch (raw_status.kind) {
	case PRECISION_CLOCK_OUTPUT_KIND_WAVEFORM:
		status->config.waveform.first_rising_time =
			raw_status.config.waveform.first_rising_time;
		status->config.waveform.period_ns = raw_status.config.waveform.period_ns;
		status->config.waveform.width_policy = raw_status.config.waveform.width_policy;
		status->config.waveform.pulse_width_ns = raw_status.config.waveform.pulse_width_ns;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
#endif /* CONFIG_PRECISION_CLOCK_OUTPUT */

static const struct precision_clock_api precision_clock_ptp_api = {
	.read = precision_clock_ptp_read,
	.set = precision_clock_ptp_set,
	.adjust_phase = precision_clock_ptp_adjust_phase,
	.adjust_rate = precision_clock_ptp_adjust_rate,
#ifdef CONFIG_PRECISION_CLOCK_OUTPUT
	.get_output_caps = precision_clock_ptp_get_output_caps,
	.output_start_waveform = precision_clock_ptp_output_start_waveform,
	.output_stop = precision_clock_ptp_output_stop,
	.get_output_status = precision_clock_ptp_get_output_status,
#endif /* CONFIG_PRECISION_CLOCK_OUTPUT */
};

void precision_clock_ptp_init(struct precision_clock_ptp_adapter *adapter,
			      const struct device *dev)
{
	adapter->dev = dev;
	adapter->precision_clk.api = &precision_clock_ptp_api;
	adapter->precision_clk.data = adapter;
}
