/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_NET_L2_ETHERNET_GPTP_GPTP_CLOCK_H_
#define ZEPHYR_SUBSYS_NET_L2_ETHERNET_GPTP_GPTP_CLOCK_H_

#include <zephyr/device.h>
#include <zephyr/net/gptp.h>
#include <zephyr/precision_timing/precision_timing.h>

struct gptp_domain;

/** gPTP clock discipline data. */
struct gptp_clock_data {
	/** gPTP domain pointer. */
	struct gptp_domain *domain;
	/** Shared precision timing discipline instance. */
	struct precision_pi_discipline discipline;
	/** Shared mapping between gPTP source time and local PHC time. */
	struct precision_time_mapping mapping;
	/** PTP clock device currently controlled by the discipline. */
	const struct device *active_clock;
	/** Identity of the grandmaster currently feeding the discipline. */
	uint8_t active_gm_id[GPTP_CLOCK_ID_LEN];
	/** Deadline for the next PHC source-timeout check. */
	struct precision_deadline source_timeout;
	/** gPTP port associated with the active clock. */
	uint16_t active_port;
	/** Whether the active clock, port, and grandmaster identity are valid. */
	bool active_source_valid;
};

extern struct gptp_clock_data gptp_clock;

void gptp_clock_servo_fault(void);
int gptp_clock_servo_reset(const struct precision_clock *precision_clk);
int gptp_clock_set(const struct precision_clock *precision_clk,
		   const struct precision_time_point *target);
int gptp_clock_adjust_rate(const struct precision_clock *precision_clk, int32_t rate_ppb);
int gptp_clock_source_reset(void);
bool gptp_clock_source_matches(const struct device *clk_dev, int port, const uint8_t *gm_id);
int gptp_clock_source_activate(const struct device *clk_dev, int port, const uint8_t *gm_id);
void gptp_clock_source_timeout_update(uint64_t timeout_ns);
bool gptp_clock_source_timeout_due(void);
void gptp_clock_source_timeout_reschedule(precision_time_t now_local_ns);

#endif /* ZEPHYR_SUBSYS_NET_L2_ETHERNET_GPTP_GPTP_CLOCK_H_ */
