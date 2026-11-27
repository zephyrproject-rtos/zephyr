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
#include <zephyr/precision_timing/precision_clock_pps.h>
#include <zephyr/precision_timing/precision_clock_ptp.h>

LOG_MODULE_REGISTER(pps_out, LOG_LEVEL_INF);

#define PHC_WAIT_ATTEMPTS 100U
#define PHC_WAIT_INTERVAL K_MSEC(100)
#define PPS_HEALTH_PERIOD K_SECONDS(5)

static struct precision_clock_ptp_adapter phc_adapter;
static struct precision_clock_pps pps_service;

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

static void pps_output_callback(struct precision_clock_pps *pps)
{
	struct precision_clock_pps_state state;
	int ret;

	ret = precision_clock_pps_state_get(pps, &state);
	if (ret < 0) {
		LOG_ERR("Failed to query PPS output state: %d", ret);
		return;
	}

	if ((state.events & PRECISION_CLOCK_PPS_EVENT_HARD_STEP) != 0U) {
		LOG_WRN("Clock step detected");
	}
	if ((state.events & PRECISION_CLOCK_PPS_EVENT_OUTPUT_INACTIVE) != 0U) {
		LOG_WRN("PPS output inactive or reconfigured");
	}
	if ((state.events & PRECISION_CLOCK_PPS_EVENT_READ_ERROR) != 0U) {
		LOG_ERR("PHC read failed");
	}
	if ((state.events & PRECISION_CLOCK_PPS_EVENT_READ_RECOVERED) != 0U) {
		LOG_INF("PHC read recovered");
	}
	if ((state.events & PRECISION_CLOCK_PPS_EVENT_STATUS_ERROR) != 0U) {
		LOG_ERR("PPS output status query failed");
	}
	if ((state.events & PRECISION_CLOCK_PPS_EVENT_STATUS_RECOVERED) != 0U) {
		LOG_INF("PPS output status query recovered");
	}
	if ((state.events & PRECISION_CLOCK_PPS_EVENT_STOP_ERROR) != 0U) {
		LOG_ERR("PPS output stop failed");
	}
	if ((state.events & PRECISION_CLOCK_PPS_EVENT_START_ERROR) != 0U) {
		LOG_ERR("PPS output start failed");
	}
	if ((state.events & PRECISION_CLOCK_PPS_EVENT_RECOVERED) != 0U) {
		LOG_INF("PPS service rearmed channel %u", CONFIG_PRECISION_CLOCK_PPS_CHANNEL);
	}
	if ((state.events & PRECISION_CLOCK_PPS_EVENT_ARMED) != 0U &&
	    state.phase == PRECISION_CLOCK_PPS_ACTIVE) {
		LOG_INF("PPS service armed channel %u: first_rising_ns=%lld period_ns=%lld",
			CONFIG_PRECISION_CLOCK_PPS_CHANNEL, (long long)state.config.first_rising_time,
			(long long)state.config.period_ns);
		if (state.config.width_policy == PRECISION_CLOCK_OUTPUT_WIDTH_EXACT) {
			LOG_INF("width_policy=exact pulse_width_ns=%lld",
				(long long)state.config.pulse_width_ns);
		} else {
			LOG_INF("width_policy=provider_default");
		}
	}
}

int main(void)
{
	struct precision_clock_pps_config config = PRECISION_CLOCK_PPS_CONFIG_DEFAULTS;
	struct precision_clock_pps_state state;
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

	config.callback = pps_output_callback;
	ret = precision_clock_pps_init(&pps_service, precision_clk, &config);
	if (ret < 0) {
		LOG_ERR("Failed to initialize PPS output service: %d", ret);
		return ret;
	}

	ret = precision_clock_pps_start(&pps_service);
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

		ret = precision_clock_pps_state_get(&pps_service, &state);
		if (ret < 0) {
			LOG_ERR("Failed to query PPS output state: %d", ret);
			continue;
		}

		LOG_INF("PPS service state: service_active=%u",
			state.phase == PRECISION_CLOCK_PPS_ACTIVE);
	}

	return 0;
}
