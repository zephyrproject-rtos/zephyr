/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/precision_timing/precision_clock_ptp.h>
#include <zephyr/precision_timing/precision_pps_output.h>

LOG_MODULE_REGISTER(pps_out, LOG_LEVEL_INF);

#define PHC_WAIT_ATTEMPTS 100U
#define PHC_WAIT_INTERVAL K_MSEC(100)
#define PPS_HEALTH_PERIOD K_SECONDS(5)

#define PPS_CHANNEL CONFIG_PRECISION_PPS_OUTPUT_CHANNEL

static struct precision_clock_ptp_adapter phc_adapter;
static struct precision_pps_output pps_service;

static struct precision_pps_output_config pps_config = PRECISION_PPS_OUTPUT_CONFIG_DEFAULTS;

static const struct device *find_ptp_clock(void)
{
	for (uint32_t attempt = 0U; attempt < PHC_WAIT_ATTEMPTS; attempt++) {
		struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
		const struct device *ptp_clock;

		if (iface != NULL) {
			ptp_clock = net_eth_get_ptp_clock(iface);
			if (ptp_clock != NULL && device_is_ready(ptp_clock)) {
				return ptp_clock;
			}
		}

		k_sleep(PHC_WAIT_INTERVAL);
	}

	return NULL;
}

static void pps_output_callback(struct precision_pps_output *pps, uint32_t events,
				const struct precision_pps_output_state *state, void *user_data)
{
	const uint32_t error_events =
		PRECISION_PPS_OUTPUT_EVENT_READ_ERROR | PRECISION_PPS_OUTPUT_EVENT_STATUS_ERROR |
		PRECISION_PPS_OUTPUT_EVENT_STOP_ERROR | PRECISION_PPS_OUTPUT_EVENT_START_ERROR;

	ARG_UNUSED(pps);
	ARG_UNUSED(user_data);

	if ((events & PRECISION_PPS_OUTPUT_EVENT_HARD_STEP) != 0U) {
		LOG_WRN("Clock step detected: continuity_error_ns=%lld generation=%u",
			(long long)state->continuity_error_ns, state->generation);
	}
	if ((events & PRECISION_PPS_OUTPUT_EVENT_OUTPUT_INACTIVE) != 0U) {
		LOG_WRN("PPS output inactive or reconfigured: generation=%u", state->generation);
	}
	if ((events & PRECISION_PPS_OUTPUT_EVENT_READ_ERROR) != 0U) {
		LOG_ERR("PHC read failed");
	}
	if ((events & PRECISION_PPS_OUTPUT_EVENT_READ_RECOVERED) != 0U) {
		LOG_INF("PHC read recovered");
	}
	if ((events & PRECISION_PPS_OUTPUT_EVENT_STATUS_ERROR) != 0U) {
		LOG_ERR("PPS output status query failed");
	}
	if ((events & PRECISION_PPS_OUTPUT_EVENT_STATUS_RECOVERED) != 0U) {
		LOG_INF("PPS output status query recovered");
	}
	if ((events & PRECISION_PPS_OUTPUT_EVENT_STOP_ERROR) != 0U) {
		LOG_ERR("PPS output stop failed");
	}
	if ((events & PRECISION_PPS_OUTPUT_EVENT_START_ERROR) != 0U) {
		LOG_ERR("PPS output start failed");
	}
	if ((events & error_events) != 0U) {
		LOG_ERR("Most recent PPS service error: %d", state->last_error);
	}
	if ((events & PRECISION_PPS_OUTPUT_EVENT_ARMED) != 0U) {
		if (state->config.width_policy == PRECISION_CLOCK_OUTPUT_WIDTH_EXACT) {
			LOG_INF("PPS service armed channel %u: first_rising_ns=%lld period_ns=%lld "
				"width_policy=exact pulse_width_ns=%lld generation=%u (service "
				"state "
				"only; not a physical waveform check)",
				pps_config.channel, (long long)state->config.first_rising_time,
				(long long)state->config.period_ns,
				(long long)state->config.pulse_width_ns, state->generation);
		} else {
			LOG_INF("PPS service armed channel %u: first_rising_ns=%lld period_ns=%lld "
				"width_policy=provider_default generation=%u (service state only; "
				"not a physical waveform check)",
				pps_config.channel, (long long)state->config.first_rising_time,
				(long long)state->config.period_ns, state->generation);
		}
	}
	if ((events & PRECISION_PPS_OUTPUT_EVENT_RECOVERED) != 0U) {
		LOG_INF("PPS service rearmed channel %u: generation=%u rearm_count=%u",
			pps_config.channel, state->generation, state->rearm_count);
	}
}

int main(void)
{
	struct precision_pps_output_state state;
	struct precision_clock_output_caps output_caps;
	const struct precision_clock *precision_clk;
	const struct device *ptp_clock;
	int ret;

	ptp_clock = find_ptp_clock();
	if (ptp_clock == NULL) {
		LOG_ERR("No ready Ethernet PTP clock");
		return -ENODEV;
	}

	precision_clock_ptp_init(&phc_adapter, ptp_clock);
	precision_clk = precision_clock_ptp_get(&phc_adapter);

	ret = precision_clock_output_get_caps(precision_clk, PPS_CHANNEL, &output_caps);
	if (ret < 0) {
		LOG_ERR("Failed to query PPS output capabilities: %d", ret);
		return ret;
	}

	ret = precision_pps_output_init(&pps_service, precision_clk, &pps_config,
				       pps_output_callback, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to initialize PPS output service: %d", ret);
		return ret;
	}

	ret = precision_pps_output_start(&pps_service);
	if (ret < 0) {
		LOG_ERR("Failed to start PPS output service: %d", ret);
		return ret;
	}

	/*
	 * The service arms and maintains the output autonomously from here on.
	 * Arming only reflects that the provider accepted the configuration; it
	 * does not verify a physical waveform on the output pin. This loop
	 * reports a periodic snapshot of the service state for visibility.
	 */
	while (1) {
		k_sleep(PPS_HEALTH_PERIOD);

		ret = precision_pps_output_state_get(&pps_service, &state);
		if (ret < 0) {
			LOG_ERR("Failed to query PPS output state: %d", ret);
			continue;
		}

		LOG_INF("PPS service state: service_active=%u phc_time_ns=%lld generation=%u "
			"rearm_count=%u last_error=%d",
			state.active, (long long)state.phc_time_ns, state.generation,
			state.rearm_count, state.last_error);
	}

	return 0;
}
