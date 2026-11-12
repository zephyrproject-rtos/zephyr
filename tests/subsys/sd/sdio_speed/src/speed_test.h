/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SDIO_SPEED_TEST_H_
#define SDIO_SPEED_TEST_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/sd_spec.h>

struct speed_case {
	uint32_t caps;
	enum sd_voltage voltage;
	uint32_t host_max;
	uint8_t initial_reg;
	uint8_t rejected_reg;
	uint8_t accepted_reg;
	uint8_t want_write_data;
	int32_t accept_at;
	int32_t read_error;
	int32_t write_error;
	int32_t write_error_at;
	int32_t host_error;
	int32_t want_ret;
	int32_t want_writes;
	uint32_t want_sets;
	uint32_t want_clock;
	enum sdhc_timing_mode want_timing;
};

/* Initial attempt plus N retries; persistent rejection leaves the host unchanged. */
#define REJECTED(caps_, volts_, write_, readback_, timing_) \
	.caps = caps_, .voltage = volts_, .host_max = 208000000U, \
	.initial_reg = 0x81U, .rejected_reg = readback_, .want_write_data = write_, \
	.want_writes = -1, .want_clock = 25000000U, .want_timing = timing_

#define ACCEPTED(caps_, volts_, write_, clock_, timing_) \
	.caps = caps_, .voltage = volts_, .host_max = 208000000U, \
	.initial_reg = 0x81U, .rejected_reg = 0x81U, .accepted_reg = write_, \
	.want_write_data = write_, .accept_at = 1, .want_writes = 1, .want_sets = 1U, \
	.want_clock = clock_, .want_timing = timing_

void sdio_speed_run_case(const struct speed_case *test);

struct init_case {
	uint8_t uhs_reg;
	uint8_t speed_write;
	bool accept;
	uint32_t want_clock;
	enum sdhc_timing_mode want_timing;
	uint32_t want_tunes;
	int32_t tuning_error;
	int32_t want_ret;
};

void sdio_speed_run_init_case(const struct init_case *test);

#endif /* SDIO_SPEED_TEST_H_ */
