/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* Rate control for the zperf UDP senders */

#ifndef ZPERF_UDP_PACING_H_
#define ZPERF_UDP_PACING_H_

#include <zephyr/kernel.h>

#define USECS_PER_TICK (Z_HZ_us / Z_HZ_ticks)
#if (USECS_PER_TICK >= 1000)
#define ZPERF_UDP_UPLOAD_CLOCK_COMPENSATE
#endif

#ifdef ZPERF_UDP_UPLOAD_CLOCK_COMPENSATE
struct compensate_ctx {
	int period;
	int64_t period_start;
	int packet_duration_us;
	int actual_pkts;
	int compensate;
};
#endif

struct zperf_udp_pacer {
	/* Time between two packets at the requested rate, in ticks */
	uint32_t packet_duration;
	/* Delay to wait after the current packet, in ticks */
	uint32_t delay;
	/* When the previous packet was sent */
	int64_t last_loop_time;
#ifdef ZPERF_UDP_UPLOAD_CLOCK_COMPENSATE
	struct compensate_ctx ctx;
#endif
};

/**
 * Prepare to send packets of the given duration, starting at @p start_time
 * (in ticks).
 */
void zperf_udp_pacer_init(struct zperf_udp_pacer *pacer, uint32_t packet_duration_us,
			  int64_t start_time);

/**
 * Account for a packet about to be sent at @p loop_time (in ticks) and
 * return how many ticks to wait after sending it. The drift correction
 * applied is stored in @p adjust, for logging.
 */
int zperf_udp_pacer_next(struct zperf_udp_pacer *pacer, int64_t loop_time, int32_t *adjust);

/** Wait the delay zperf_udp_pacer_next() returned for the packet just sent. */
void zperf_udp_pacer_wait(int delay);

#endif /* ZPERF_UDP_PACING_H_ */
