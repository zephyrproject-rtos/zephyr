/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Cycle-counter instrumentation and the worker thread that runs faulted
 * transfers.
 *
 * Timing uses k_cycle_get_32() and sys_clock_hw_cycles_per_sec(), which are
 * available on every platform and need no peripheral.
 *
 * A driver that never returns cannot be detected from the thread that called
 * it, because that thread is the one stuck.  Transfers therefore run on a
 * worker thread, and the test thread waits for each one for at most the test
 * wait limit, CONFIG_I2C_FAULT_TEST_TIMEOUT_MS.
 */

#ifndef I2C_TIMING_H_
#define I2C_TIMING_H_

#include <stdbool.h>
#include <stdint.h>

/** A measured interval, in cycle-counter ticks. */
struct i2c_timing_span {
	uint32_t start;
	uint32_t end;
};

/** Result of running a job on the worker thread. */
struct i2c_fault_outcome {
	/** Value the job returned, meaningless when timed_out is set. */
	int ret;
	/** Microseconds from job start to return, or the wait limit if it timed out. */
	uint32_t elapsed_us;
	/** Cycle counter sampled immediately before the job ran. */
	uint32_t start_cycles;
	/** Cycle counter sampled immediately after the job returned. */
	uint32_t end_cycles;
	/** Set when the job did not return within the wait limit. */
	bool timed_out;
};

/** Signature of a job run on the worker thread. */
typedef int (*i2c_fault_job_fn)(void *arg);

/** Sample the cycle counter at the start of an interval. */
void i2c_timing_begin(struct i2c_timing_span *span);

/** Sample the cycle counter at the end of an interval. */
void i2c_timing_end(struct i2c_timing_span *span);

/** Length of a measured interval in microseconds. */
uint32_t i2c_timing_us(const struct i2c_timing_span *span);

/**
 * Start the worker thread that runs jobs.
 *
 * Called once from the suite setup.  Safe to call again; later calls do
 * nothing.
 */
void i2c_timing_harness_start(void);

/**
 * Run @p fn on the worker thread and give it up to the wait limit to return.
 *
 * A job still running at the wait limit is left running.  Aborting the worker
 * in the middle of a driver call would leave the controller in an unknown
 * state.
 *
 * @param[in]  fn  job to run.
 * @param[in]  arg passed to @p fn unchanged.
 * @param[out] out result and elapsed time.
 *
 * @retval 0          The job returned within the wait limit.
 * @retval -ETIMEDOUT The job did not return, or an earlier job is still stuck.
 */
int i2c_timing_run_bounded(i2c_fault_job_fn fn, void *arg, struct i2c_fault_outcome *out);

/**
 * Hand @p fn to the worker thread and return without waiting.
 *
 * Must be paired with i2c_timing_collect().  i2c_timing_run_bounded() does
 * both.
 *
 * @param[in] fn  job to run.
 * @param[in] arg passed to @p fn unchanged.
 *
 * @retval 0          The job was handed to the worker.
 * @retval -ETIMEDOUT An earlier job is still stuck.
 */
int i2c_timing_submit(i2c_fault_job_fn fn, void *arg);

/**
 * Give the job handed over by i2c_timing_submit() up to the wait limit to
 * return.
 *
 * @param[out] out result and elapsed time.
 *
 * @retval 0          The job returned within the wait limit.
 * @retval -ETIMEDOUT The job did not return, or an earlier job is still stuck.
 */
int i2c_timing_collect(struct i2c_fault_outcome *out);

/**
 * Give a stuck job one more wait limit to return.
 *
 * Called by the suite before each test while a job is stuck.  If the job
 * returns within the extra wait, its result is discarded and the worker can
 * take new jobs again.
 *
 * @retval true  No job was stuck, or the stuck job has now returned.
 * @retval false The stuck job still has not returned.
 */
bool i2c_timing_harness_recover(void);

/**
 * Report whether a job did not return within the wait limit and still has
 * not returned.
 *
 * While that holds, the worker has not returned from its call, so
 * i2c_timing_submit() refuses new jobs.  The suite waits one more wait limit
 * through i2c_timing_harness_recover() and skips the tests that follow if the
 * job still has not returned.
 *
 * @retval true  A job did not return within the wait limit and still has not.
 * @retval false No job is stuck.
 */
bool i2c_timing_harness_stuck(void);

/**
 * Run @p fn on the worker thread and report it through
 * I2C_FAULT_ASSERT_RETURNED() if it does not return within the wait limit.
 *
 * @param[in]  fn        job to run.
 * @param[in]  arg       passed to @p fn unchanged.
 * @param[in]  condition condition identifier used in the failure message.
 * @param[out] out       result and elapsed time.
 */
void i2c_timing_run_or_fail(i2c_fault_job_fn fn, void *arg, const char *condition,
			    struct i2c_fault_outcome *out);

#endif /* I2C_TIMING_H_ */
