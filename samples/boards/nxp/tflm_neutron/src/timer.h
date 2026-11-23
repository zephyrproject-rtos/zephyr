/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 *
 * Small wall-clock timer around a region of code (e.g. TFLM Invoke()), built on
 * Zephyr's timing subsystem (DWT cycle counter at the CPU core clock).
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>
#include <zephyr/timing/timing.h>

#ifdef __cplusplus
extern "C" {
#endif

/** A measured interval: elapsed CPU cycles and the derived microseconds. */
struct timer_result {
	uint64_t cycles;
	uint64_t us;
};

/** One-shot timer: start/stop bracket a region, then read the result. */
struct timer {
	timing_t start;
	timing_t end;
};

/**
 * Start timing. Initializes and starts the timing subsystem and takes the
 * start timestamp.
 */
void timer_start(struct timer *t);

/**
 * Stop timing and compute the elapsed interval. Takes the end timestamp, stops
 * the timing subsystem and fills @p res with the elapsed cycles and the
 * microseconds derived from the CPU core clock.
 */
void timer_stop(struct timer *t, struct timer_result *res);

/** CPU core clock in Hz, as used to convert cycles to microseconds. */
uint64_t timer_cpu_hz(void);

/**
 * Print a measured interval as "<label>: <us> us (<cycles> cpu cycles @ <MHz>)".
 * @p label is a caller-supplied prefix (e.g. "Invoke time").
 */
void timer_report(const char *label, const struct timer_result *res);

#ifdef __cplusplus
}
#endif

#endif /* TIMER_H_ */
