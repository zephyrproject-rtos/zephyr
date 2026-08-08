/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/timing/precision_ptp_clock.h>

#include "gptp_clock.h"

void gptp_clock_servo_fault(void)
{
	precision_pi_fault(&gptp_clock.discipline);
	precision_time_mapping_invalidate(&gptp_clock.mapping);
	precision_deadline_cancel(&gptp_clock.source_timeout);
}

int gptp_clock_set(const struct precision_clock *precision_clk,
		   const struct precision_time_point *target)
{
	int ret;

	ret = precision_clock_set(precision_clk, target);
	if (ret == -ENOTSUP) {
		return 0;
	}

	if (ret < 0) {
		gptp_clock_servo_fault();
	}

	return ret;
}

int gptp_clock_adjust_rate(const struct precision_clock *precision_clk, int32_t rate_ppb)
{
	int ret;

	ret = precision_clock_adjust_rate(precision_clk, rate_ppb);
	if (ret == -ENOTSUP) {
		return 0;
	}

	if (ret < 0) {
		gptp_clock_servo_fault();
	}

	return ret;
}

int gptp_clock_servo_reset(const struct precision_clock *precision_clk)
{
	precision_pi_reset(&gptp_clock.discipline);
	precision_time_mapping_invalidate(&gptp_clock.mapping);
	precision_deadline_cancel(&gptp_clock.source_timeout);

	return gptp_clock_adjust_rate(precision_clk, 0);
}

static struct precision_time_domain gptp_clock_local_domain(uint32_t port)
{
	struct precision_pi_config config;

	(void)precision_pi_get_config(&gptp_clock.discipline, &config);
	config.local_domain.id = port;

	return config.local_domain;
}

static void gptp_clock_source_clear(void)
{
	struct precision_pi_config config;

	gptp_clock.active_clock = NULL;
	gptp_clock.active_port = 0U;
	gptp_clock.active_source_valid = false;
	memset(gptp_clock.active_gm_id, 0, sizeof(gptp_clock.active_gm_id));
	(void)precision_pi_set_local_domain(&gptp_clock.discipline, gptp_clock_local_domain(0U));
	precision_deadline_cancel(&gptp_clock.source_timeout);

	(void)precision_pi_get_config(&gptp_clock.discipline, &config);
	precision_time_mapping_init(&gptp_clock.mapping, config.source_domain, config.local_domain);
}

int gptp_clock_source_reset(void)
{
	struct precision_ptp_clock_adapter adapter;
	const struct precision_clock *precision_clk;
	struct precision_pi_config config;
	int ret;

	if (!gptp_clock.active_source_valid || gptp_clock.active_clock == NULL) {
		precision_pi_reset(&gptp_clock.discipline);
		precision_time_mapping_invalidate(&gptp_clock.mapping);
		gptp_clock_source_clear();
		return 0;
	}

	(void)precision_pi_get_config(&gptp_clock.discipline, &config);
	ret = precision_ptp_clock_init(&adapter, gptp_clock.active_clock, config.local_domain);
	if (ret < 0) {
		return ret;
	}

	precision_clk = precision_ptp_clock_get(&adapter);
	ret = gptp_clock_servo_reset(precision_clk);
	if (ret < 0) {
		return ret;
	}

	gptp_clock_source_clear();

	return 0;
}

bool gptp_clock_source_matches(const struct device *clk_dev, int port, const uint8_t *gm_id)
{
	return clk_dev != NULL && gm_id != NULL && gptp_clock.active_source_valid &&
	       gptp_clock.active_clock == clk_dev && gptp_clock.active_port == (uint16_t)port &&
	       memcmp(gptp_clock.active_gm_id, gm_id, GPTP_CLOCK_ID_LEN) == 0;
}

int gptp_clock_source_activate(const struct device *clk_dev, int port, const uint8_t *gm_id)
{
	struct precision_pi_config config;
	int ret;

	if (clk_dev == NULL || gm_id == NULL || port <= 0) {
		return -EINVAL;
	}

	if (gptp_clock_source_matches(clk_dev, port, gm_id)) {
		return 0;
	}

	ret = gptp_clock_source_reset();
	if (ret < 0) {
		return ret;
	}

	ret = precision_pi_set_local_domain(&gptp_clock.discipline,
					    gptp_clock_local_domain((uint32_t)port));
	if (ret < 0) {
		return ret;
	}

	(void)precision_pi_get_config(&gptp_clock.discipline, &config);
	precision_time_mapping_init(&gptp_clock.mapping, config.source_domain, config.local_domain);
	gptp_clock.active_clock = clk_dev;
	gptp_clock.active_port = (uint16_t)port;
	memcpy(gptp_clock.active_gm_id, gm_id, sizeof(gptp_clock.active_gm_id));
	gptp_clock.active_source_valid = true;

	return 0;
}

void gptp_clock_source_timeout_update(uint64_t timeout_ns)
{
	precision_time_t timeout = timeout_ns > (uint64_t)PRECISION_TIME_MAX
					   ? PRECISION_TIME_MAX
					   : (precision_time_t)timeout_ns;
	struct precision_pi_status status;

	(void)precision_pi_set_source_timeout(&gptp_clock.discipline, timeout, timeout);

	if (precision_pi_get_status(&gptp_clock.discipline, &status) == 0 &&
	    status.has_observation) {
		precision_deadline_schedule(&gptp_clock.source_timeout, timeout);
	} else {
		precision_deadline_cancel(&gptp_clock.source_timeout);
	}
}

bool gptp_clock_source_timeout_due(void)
{
	return precision_deadline_due(&gptp_clock.source_timeout);
}

void gptp_clock_source_timeout_reschedule(precision_time_t now_local_ns)
{
	precision_time_t remaining_ns;

	if (precision_pi_time_to_expiry(&gptp_clock.discipline, now_local_ns, &remaining_ns) < 0) {
		precision_deadline_cancel(&gptp_clock.source_timeout);
		return;
	}

	precision_deadline_schedule(&gptp_clock.source_timeout, remaining_ns);
}
