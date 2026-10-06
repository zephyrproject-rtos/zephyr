/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_STM32_HAL_PTP_OUTPUT_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_STM32_HAL_PTP_OUTPUT_H_

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>

struct eth_stm32_ptp_output_rearm_timing {
	uint64_t target_ns;
	uint64_t safe_after_ns;
	uint64_t safe_after_monotonic_ns;
	bool monotonic_guard_active;
};

struct eth_stm32_ptp_output_rearm_limits {
	uint64_t period_ns;
	uint64_t min_lead_time_ns;
	uint64_t falling_edge_guard_ns;
	uint64_t scheduling_margin_ns;
	uint64_t hardware_max_width_ns;
	uint32_t resolution_ns;
	uint32_t min_rate_pct;
	uint32_t max_rate_pct;
};

static inline int eth_stm32_ptp_output_real_duration_ceil(uint64_t clock_duration_ns,
							  uint32_t rate_pct,
							  uint64_t *real_duration_ns)
{
	uint64_t scaled_remainder;
	uint64_t scaled_whole;
	uint64_t whole;

	if (rate_pct == 0U || real_duration_ns == NULL) {
		return -EINVAL;
	}

	whole = clock_duration_ns / rate_pct;
	if (u64_mul_overflow(whole, 100U, &scaled_whole)) {
		return -ERANGE;
	}

	scaled_remainder = DIV_ROUND_UP((clock_duration_ns % rate_pct) * 100U, rate_pct);
	if (u64_add_overflow(scaled_whole, scaled_remainder, real_duration_ns)) {
		return -ERANGE;
	}

	return 0;
}

static inline int eth_stm32_ptp_output_real_duration_floor(uint64_t clock_duration_ns,
							   uint32_t rate_pct,
							   uint64_t *real_duration_ns)
{
	uint64_t scaled_remainder;
	uint64_t scaled_whole;
	uint64_t whole;

	if (rate_pct == 0U || real_duration_ns == NULL) {
		return -EINVAL;
	}

	whole = clock_duration_ns / rate_pct;
	if (u64_mul_overflow(whole, 100U, &scaled_whole)) {
		return -ERANGE;
	}

	scaled_remainder = ((clock_duration_ns % rate_pct) * 100U) / rate_pct;
	if (u64_add_overflow(scaled_whole, scaled_remainder, real_duration_ns)) {
		return -ERANGE;
	}

	return 0;
}

static inline int eth_stm32_ptp_output_clock_duration_floor(uint64_t real_duration_ns,
							    uint32_t rate_pct,
							    uint64_t *clock_duration_ns)
{
	uint64_t scaled_remainder;
	uint64_t scaled_whole;
	uint64_t whole;

	if (rate_pct == 0U || clock_duration_ns == NULL) {
		return -EINVAL;
	}

	whole = real_duration_ns / 100U;
	if (u64_mul_overflow(whole, rate_pct, &scaled_whole)) {
		return -ERANGE;
	}

	scaled_remainder = ((real_duration_ns % 100U) * rate_pct) / 100U;
	if (u64_add_overflow(scaled_whole, scaled_remainder, clock_duration_ns)) {
		return -ERANGE;
	}

	return 0;
}

static inline int
eth_stm32_ptp_output_max_pulse_width(const struct eth_stm32_ptp_output_rearm_limits *limits,
				     uint64_t *max_width_ns)
{
	uint64_t available_real_ns;
	uint64_t trailing_clock_ns;
	uint64_t width_ns;
	int ret;

	if (limits == NULL || max_width_ns == NULL || limits->resolution_ns == 0U ||
	    limits->min_rate_pct == 0U || limits->max_rate_pct == 0U ||
	    limits->period_ns <= limits->min_lead_time_ns) {
		return -EINVAL;
	}

	ret = eth_stm32_ptp_output_real_duration_floor(limits->period_ns - limits->min_lead_time_ns,
						       limits->max_rate_pct, &available_real_ns);
	if (ret < 0) {
		return ret;
	}
	if (available_real_ns <= limits->scheduling_margin_ns) {
		return -ERANGE;
	}
	available_real_ns -= limits->scheduling_margin_ns;

	ret = eth_stm32_ptp_output_clock_duration_floor(available_real_ns, limits->min_rate_pct,
							&trailing_clock_ns);
	if (ret < 0) {
		return ret;
	}
	if (trailing_clock_ns <= limits->falling_edge_guard_ns) {
		return -ERANGE;
	}

	width_ns = trailing_clock_ns - limits->falling_edge_guard_ns;
	width_ns = MIN(width_ns, limits->hardware_max_width_ns);
	width_ns -= width_ns % limits->resolution_ns;
	if (width_ns < limits->resolution_ns) {
		return -ERANGE;
	}

	*max_width_ns = width_ns;

	return 0;
}

static inline int
eth_stm32_ptp_output_rearm_timing_init(struct eth_stm32_ptp_output_rearm_timing *timing,
				       uint64_t target_ns, uint64_t trailing_ns)
{
	uint64_t safe_after_ns;

	if (timing == NULL) {
		return -EINVAL;
	}
	if (u64_add_overflow(target_ns, trailing_ns, &safe_after_ns)) {
		return -ERANGE;
	}

	*timing = (struct eth_stm32_ptp_output_rearm_timing){
		.target_ns = target_ns,
		.safe_after_ns = safe_after_ns,
	};

	return 0;
}

static inline int
eth_stm32_ptp_output_rearm_guard_start(struct eth_stm32_ptp_output_rearm_timing *timing,
				       uint64_t now_monotonic_ns, uint32_t min_rate_pct,
				       uint64_t tick_ns)
{
	uint64_t real_duration_ns;
	uint64_t trailing_ns;
	int ret;

	if (timing == NULL || timing->safe_after_ns < timing->target_ns) {
		return -EINVAL;
	}

	trailing_ns = timing->safe_after_ns - timing->target_ns;
	ret = eth_stm32_ptp_output_real_duration_ceil(trailing_ns, min_rate_pct, &real_duration_ns);
	if (ret < 0 || u64_add_overflow(real_duration_ns, tick_ns, &real_duration_ns) ||
	    u64_add_overflow(now_monotonic_ns, real_duration_ns,
			     &timing->safe_after_monotonic_ns)) {
		return -ERANGE;
	}

	timing->monotonic_guard_active = true;

	return 0;
}

static inline int
eth_stm32_ptp_output_rearm_observe(struct eth_stm32_ptp_output_rearm_timing *timing,
				   uint64_t now_ns, uint64_t now_monotonic_ns,
				   uint32_t min_rate_pct, uint64_t tick_ns)
{
	if (timing == NULL) {
		return -EINVAL;
	}
	if (timing->monotonic_guard_active || now_ns < timing->target_ns) {
		return 0;
	}

	return eth_stm32_ptp_output_rearm_guard_start(timing, now_monotonic_ns, min_rate_pct,
						      tick_ns);
}

static inline bool
eth_stm32_ptp_output_rearm_ready(const struct eth_stm32_ptp_output_rearm_timing *timing,
				 uint64_t now_ns, uint64_t now_monotonic_ns)
{
	return timing != NULL && timing->monotonic_guard_active &&
	       now_ns >= timing->safe_after_ns &&
	       now_monotonic_ns >= timing->safe_after_monotonic_ns;
}

static inline int
eth_stm32_ptp_output_rearm_delay(const struct eth_stm32_ptp_output_rearm_timing *timing,
				 uint64_t now_ns, uint64_t now_monotonic_ns, uint32_t max_rate_pct,
				 uint64_t tick_ns, uint64_t max_delay_ns, uint64_t *delay_ns)
{
	uint64_t monotonic_delay_ns = 0U;
	uint64_t phc_delay_ns = 0U;
	uint64_t phc_remaining_ns;
	uint64_t wait_ns;
	int ret;

	if (timing == NULL || delay_ns == NULL || max_delay_ns == 0U) {
		return -EINVAL;
	}

	if (timing->monotonic_guard_active) {
		phc_remaining_ns =
			timing->safe_after_ns > now_ns ? timing->safe_after_ns - now_ns : 0U;
		if (timing->safe_after_monotonic_ns > now_monotonic_ns) {
			monotonic_delay_ns = timing->safe_after_monotonic_ns - now_monotonic_ns;
		}
	} else {
		phc_remaining_ns = timing->target_ns > now_ns ? timing->target_ns - now_ns : 0U;
	}

	ret = eth_stm32_ptp_output_real_duration_floor(phc_remaining_ns, max_rate_pct,
						       &phc_delay_ns);
	if (ret < 0) {
		return ret;
	}

	wait_ns = MAX(phc_delay_ns, monotonic_delay_ns);
	if ((phc_remaining_ns > 0U || monotonic_delay_ns > 0U) && wait_ns < tick_ns) {
		wait_ns = tick_ns;
	}
	*delay_ns = MIN(wait_ns, max_delay_ns);

	return 0;
}

#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_STM32_HAL_PTP_OUTPUT_H_ */
