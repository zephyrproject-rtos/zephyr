/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include <zephyr/precision_timing/precision_clock.h>
#include <zephyr/precision_timing/precision_pi.h>
#include <zephyr/precision_timing/precision_time.h>

struct software_clock {
	precision_time_t time;
	int64_t scaled_ppm;
};

static int software_clock_read(const struct precision_clock *precision_clk,
			       precision_time_t *time_ns)
{
	struct software_clock *software_clk = precision_clk->data;

	*time_ns = software_clk->time;
	return 0;
}

static int software_clock_set(const struct precision_clock *precision_clk,
			      precision_time_t time_ns)
{
	struct software_clock *software_clk = precision_clk->data;

	software_clk->time = time_ns;
	return 0;
}

static int software_clock_adjust_phase(const struct precision_clock *precision_clk,
				       precision_time_t phase_ns)
{
	struct software_clock *software_clk = precision_clk->data;

	return precision_time_add(software_clk->time, phase_ns, &software_clk->time);
}

static int software_clock_adjust_rate(const struct precision_clock *precision_clk,
				      int64_t scaled_ppm)
{
	struct software_clock *software_clk = precision_clk->data;

	software_clk->scaled_ppm = scaled_ppm;
	return 0;
}

static const struct precision_clock_api software_clock_api = {
	.read = software_clock_read,
	.set = software_clock_set,
	.adjust_phase = software_clock_adjust_phase,
	.adjust_rate = software_clock_adjust_rate,
};

int main(void)
{
	struct software_clock clock_data = {0};
	struct precision_clock precision_clk = {
		.api = &software_clock_api,
		.data = &clock_data,
	};
	struct precision_pi pi;
	precision_time_t time_ns;
	double rate_ppm;
	int64_t scaled_ppm;
	int ret;

	precision_pi_init(&pi, 0.5, 0.15);

	ret = precision_clock_set(&precision_clk, INT64_C(1000000000));
	if (ret < 0) {
		return ret;
	}

	ret = precision_clock_adjust_phase(&precision_clk, 250000);
	if (ret < 0) {
		return ret;
	}

	rate_ppm = precision_pi_update(&pi, 25.0);
	scaled_ppm = (int64_t)(rate_ppm * PRECISION_CLOCK_SCALED_PPM_ONE);
	ret = precision_clock_adjust_rate(&precision_clk, scaled_ppm);
	if (ret < 0) {
		return ret;
	}

	ret = precision_clock_read(&precision_clk, &time_ns);
	if (ret < 0) {
		return ret;
	}

	printf("Precision clock: time %" PRId64 " ns, rate %.2f ppm\n", time_ns,
	       (double)clock_data.scaled_ppm / PRECISION_CLOCK_SCALED_PPM_ONE);

	return 0;
}
