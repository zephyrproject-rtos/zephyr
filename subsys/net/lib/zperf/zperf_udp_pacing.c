/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Rate control for the zperf UDP senders */

#include <zephyr/kernel.h>

#include "zperf_udp_pacing.h"

#ifdef ZPERF_UDP_UPLOAD_CLOCK_COMPENSATE
/**
 * Add compensate to packet delay when time clock accuracy is lower than 1kHz.
 * After given period, compare actual sent packets with expected sent packets,
 * and get summary compensate ticks.
 * Then try to compensate in this loop.
 * If delay is not enough as it cannot be less than 0, pile up compensate ticks.
 */
static int cal_compensate_delay(struct compensate_ctx *ctx, int64_t loop_time, int delay)
{
	int64_t delta_time;
	int expected_pkts;
	int compensate_pkts;
	int compensate_ticks;
	int compensate_delay = delay;

	if (ctx->period == 0) {
		return delay;
	}

	if (ctx->period_start == -1) {
		ctx->period_start = loop_time;
		ctx->actual_pkts = 0;
	}

	ctx->actual_pkts++;
	delta_time = loop_time - ctx->period_start;

	if (delta_time < ctx->period) {
		return delay;
	}

	/* calculate compensate ticks and maintain it during whole traffic */
	expected_pkts = delta_time * USECS_PER_TICK / ctx->packet_duration_us;
	compensate_pkts = ctx->actual_pkts - expected_pkts;
	compensate_ticks = compensate_pkts * ctx->packet_duration_us / USECS_PER_TICK;
	ctx->compensate += compensate_ticks;

	if (ctx->compensate >= 0 || (ctx->compensate + delay) > 0) {
		compensate_delay += ctx->compensate;
		ctx->compensate = 0;
	} else {
		compensate_delay = 0;
		ctx->compensate += delay;
	}

	/* restart statistic period */
	ctx->period_start = -1;

	/* rate is higher than capability, no need to compensate */
	if (ctx->compensate < -1000) {
		ctx->period = 0;
	}

	return compensate_delay;
}
#endif

void zperf_udp_pacer_init(struct zperf_udp_pacer *pacer, uint32_t packet_duration_us,
			  int64_t start_time)
{
	pacer->packet_duration = k_us_to_ticks_ceil32(packet_duration_us);
	pacer->delay = pacer->packet_duration;
	pacer->last_loop_time = start_time;

#ifdef ZPERF_UDP_UPLOAD_CLOCK_COMPENSATE
	/* Compensate period, by default 10 ticks. An unpaced stream (zero
	 * packet duration) has nothing to compensate and must not divide
	 * by the duration.
	 */
	pacer->ctx = (struct compensate_ctx){
		.period = (packet_duration_us != 0U) ? 10 : 0,
		.period_start = -1,
		.packet_duration_us = packet_duration_us,
	};
#endif
}

int zperf_udp_pacer_next(struct zperf_udp_pacer *pacer, int64_t loop_time, int32_t *adjust)
{
	int compensate_delay;

	/* Algorithm to maintain a given baud rate */
	if (pacer->last_loop_time != loop_time) {
		*adjust = pacer->packet_duration;
		*adjust -= (int32_t)(loop_time - pacer->last_loop_time);
	} else {
		/* It's the first iteration so no need for adjustment
		 */
		*adjust = 0;
	}

	if ((*adjust >= 0) || (-*adjust < pacer->delay)) {
		pacer->delay += *adjust;
	} else {
		pacer->delay = 0U; /* delay should never be negative */
	}

	/* add clock compensate to packet delay when clock accuracy is lower than 1KHz */
#ifdef ZPERF_UDP_UPLOAD_CLOCK_COMPENSATE
	compensate_delay = cal_compensate_delay(&pacer->ctx, loop_time, (int)pacer->delay);
#else
	compensate_delay = pacer->delay;
#endif

	pacer->last_loop_time = loop_time;

	return compensate_delay;
}

void zperf_udp_pacer_wait(int delay)
{
#if defined(CONFIG_ARCH_POSIX)
	ARG_UNUSED(delay);

	k_busy_wait(USEC_PER_MSEC);
#else
	if (delay > 0) {
		k_sleep(K_TICKS(delay));
	}
#endif
}
