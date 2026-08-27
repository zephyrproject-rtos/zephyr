/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/cpu_load.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_backend.h>

#include <zephyr/drivers/counter.h>

/* Tolerance of the load assertions, in per mille.
 *
 * Generous enough to absorb the tick granularity of slow targets (k_msleep()
 * rounds up to a tick, so the busy/idle split is not exactly even) while still
 * far tighter than any real failure: a backend that fails to account idle time
 * reports 1000, and one that over-accounts it reports 0.
 */
#define DELTA 75

/* CPU whose load is measured. On SMP the worker below is pinned to it so that
 * the busy/idle sequence is attributed to a known CPU.
 */
#define TEST_CPU 0

#define WORKER_STACK_SIZE 2048

K_THREAD_STACK_DEFINE(worker_stack, WORKER_STACK_SIZE);
static struct k_thread worker;

static int load_busy;
static int load_half;
static int load_idle;

/* Runs the busy/idle sequence on TEST_CPU and records the measured load.
 *
 * While this thread busy-waits, TEST_CPU is fully loaded; while it sleeps,
 * TEST_CPU is idle, as the test thread is blocked joining it and nothing else
 * is runnable there.
 */
static void load_seq(void *p1, void *p2, void *p3)
{
	uint32_t t_ms = 100;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* Reset the measurement, then keep the CPU busy for the whole window. */
	(void)cpu_load_get_cpu(TEST_CPU, true);
	k_busy_wait(t_ms * USEC_PER_MSEC);
	load_busy = cpu_load_get_cpu(TEST_CPU, false);

	/* Idle for the same amount of time without resetting, so that the
	 * window covers one busy and one idle period.
	 */
	k_msleep(t_ms);
	load_half = cpu_load_get_cpu(TEST_CPU, false);

	/* Reset and stay idle for the whole window. */
	(void)cpu_load_get_cpu(TEST_CPU, true);
	k_msleep(t_ms);
	load_idle = cpu_load_get_cpu(TEST_CPU, false);
}

static void run_load_seq(void)
{
	k_tid_t tid;

	if (IS_ENABLED(CONFIG_SMP) && !IS_ENABLED(CONFIG_SCHED_CPU_MASK)) {
		/* Without pinning the worker may migrate between CPUs, which
		 * makes attributing the busy/idle sequence to a single CPU
		 * meaningless.
		 */
		ztest_test_skip();
	}

	tid = k_thread_create(&worker, worker_stack, WORKER_STACK_SIZE, load_seq,
			      NULL, NULL, NULL, K_PRIO_PREEMPT(1), 0, K_FOREVER);

#if defined(CONFIG_SMP) && defined(CONFIG_SCHED_CPU_MASK)
	/* A running thread cannot be pinned, hence the K_FOREVER delay above:
	 * pin the worker before starting it.
	 */
	zassert_ok(k_thread_cpu_pin(tid, TEST_CPU));
#endif

	k_thread_start(tid);
	zassert_ok(k_thread_join(tid, K_FOREVER));
}

ZTEST(cpu_load, test_load)
{
	if (CONFIG_CPU_LOAD_LOG_PERIODICALLY > 0) {
		cpu_load_log_control(false);
	}

	run_load_seq();

	/* Results are in per mille. */
	zassert_within(load_busy, 1000, DELTA, "busy window: %d", load_busy);
	zassert_within(load_half, 500, DELTA, "half-busy window: %d", load_half);
	zassert_within(load_idle, 0, DELTA, "idle window: %d", load_idle);
}

/* The per-CPU getter must report every CPU and reject out-of-range ids. */
ZTEST(cpu_load, test_get_cpu)
{
	for (unsigned int cpu = 0; cpu < CONFIG_MP_MAX_NUM_CPUS; cpu++) {
		int load = cpu_load_get_cpu(cpu, true);

		zassert_true(load >= 0, "CPU%u: unexpected error %d", cpu, load);
		zassert_true(load <= 1000, "CPU%u: load %d out of range", cpu, load);
	}

	zassert_equal(cpu_load_get_cpu(CONFIG_MP_MAX_NUM_CPUS, false), -EINVAL);
}

/* The current-CPU getter and the per mille to percent helper. */
ZTEST(cpu_load, test_get_current_cpu)
{
	int load = cpu_load_get(true);

	zassert_true(load >= 0, "unexpected error %d", load);
	zassert_true(load <= 1000, "load %d out of range", load);

	zassert_equal(CPU_LOAD_PERMILLE_TO_PERCENT(1000), 100);
	zassert_equal(CPU_LOAD_PERMILLE_TO_PERCENT(505), 50);
	zassert_equal(CPU_LOAD_PERMILLE_TO_PERCENT(0), 0);
}

#if CONFIG_CPU_LOAD_LOG_PERIODICALLY > 0
/* The periodic report emits one line per CPU per period. */
#define LOGS_PER_PERIOD CONFIG_MP_MAX_NUM_CPUS
#define EXPECTED_LOGS   (3 * LOGS_PER_PERIOD)
#define LOGS_TOLERANCE  LOGS_PER_PERIOD

static int cpu_load_src_id;
static atomic_t log_cnt;

static void process(const struct log_backend *const backend,
		union log_msg_generic *msg)
{
	ARG_UNUSED(backend);
	const void *source = msg->log.hdr.source;
	int source_id = log_const_source_id((const struct log_source_const_data *)source);

	if (source_id == cpu_load_src_id) {
		atomic_inc(&log_cnt);
	}
}

static void init(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);
}

const struct log_backend_api mock_log_backend_api = {
	.process = process,
	.init = init
};

LOG_BACKEND_DEFINE(dummy, mock_log_backend_api, false, NULL);

ZTEST(cpu_load, test_periodic_report)
{
	log_backend_enable(&dummy, NULL, LOG_LEVEL_INF);
	cpu_load_log_control(true);

	cpu_load_src_id = log_source_id_get(STRINGIFY(cpu_load));
	atomic_set(&log_cnt, 0);
	k_msleep(3 * CONFIG_CPU_LOAD_LOG_PERIODICALLY);
	log_flush();
	zassert_within(log_cnt, EXPECTED_LOGS, LOGS_TOLERANCE);

	cpu_load_log_control(false);
	k_msleep(1);
	atomic_set(&log_cnt, 0);
	k_msleep(3 * CONFIG_CPU_LOAD_LOG_PERIODICALLY);
	log_flush();
	zassert_equal(log_cnt, 0);

	cpu_load_log_control(true);
	k_msleep(3 * CONFIG_CPU_LOAD_LOG_PERIODICALLY);
	log_flush();
	zassert_within(log_cnt, EXPECTED_LOGS, LOGS_TOLERANCE);

	cpu_load_log_control(false);
	log_backend_disable(&dummy);
}

static uint32_t num_low_callbacks;
static uint8_t last_low_percent;

/* Records rather than asserts: this runs from the periodic timer, i.e. in
 * interrupt context, where a failing assertion would be reported out of band.
 */
void low_load_cb(uint8_t percent)
{
	last_low_percent = percent;
	num_low_callbacks++;
}

static uint32_t num_load_callbacks;
static uint8_t last_cpu_load_percent;

void high_load_cb(uint8_t percent)
{
	last_cpu_load_percent = percent;
	num_load_callbacks++;
}

ZTEST(cpu_load, test_callback_load_low)
{
	int ret;

	num_low_callbacks = 0;
	last_low_percent = 0;
	cpu_load_log_control(true);

	ret = cpu_load_cb_reg(low_load_cb, 99);
	zassert_equal(ret, 0);

	k_msleep(CONFIG_CPU_LOAD_LOG_PERIODICALLY * 4);

	ret = cpu_load_cb_reg(NULL, 99);
	zassert_equal(ret, 0);
	cpu_load_log_control(false);

	zassert_equal(num_low_callbacks, 0,
		      "idle system triggered %u callback(s), highest load %u%%",
		      num_low_callbacks, last_low_percent);
}

ZTEST(cpu_load, test_callback_load_high)
{
	int ret;

	cpu_load_log_control(true);

	ret = cpu_load_cb_reg(high_load_cb, 99);
	zassert_equal(ret, 0);

	k_busy_wait(CONFIG_CPU_LOAD_LOG_PERIODICALLY * 4 * 1000);
	zassert_between_inclusive(last_cpu_load_percent, 99, 100, "percent: %u",
				  last_cpu_load_percent);
	zassert_between_inclusive(num_load_callbacks, 2, 7, "callbacks: %u",
				  num_load_callbacks);

	/* Reset the callback */
	ret = cpu_load_cb_reg(NULL, 99);
	zassert_equal(ret, 0);

	num_load_callbacks = 0;
	k_busy_wait(CONFIG_CPU_LOAD_LOG_PERIODICALLY * 4 * 1000);
	zassert_equal(num_load_callbacks, 0, "callbacks: %u", num_load_callbacks);
	cpu_load_log_control(false);
}

#endif /* CONFIG_CPU_LOAD_LOG_PERIODICALLY > 0 */

/* Leave no periodic reporting or callback armed behind: a stale callback would
 * fire during an unrelated test.
 */
static void cpu_load_after(void *fixture)
{
	ARG_UNUSED(fixture);

	cpu_load_log_control(false);
	(void)cpu_load_cb_reg(NULL, 0);
}

ZTEST_SUITE(cpu_load, NULL, NULL, NULL, cpu_load_after, NULL);
