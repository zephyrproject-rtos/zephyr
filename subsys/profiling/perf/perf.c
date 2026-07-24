/*
 *  Copyright (c) 2023 KNS Group LLC (YADRO)
 *  Copyright (c) 2020 Yonatan Goldschmidt <yon.goldschmidt@gmail.com>
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t arch_perf_current_stack_trace(uintptr_t *buf, size_t size);

/*
 * The sampling timer only fires on one CPU. The remaining CPUs are sampled by
 * broadcasting an IPI to them, which needs architecture support. Without it
 * (uniprocessor builds, or SMP without IPI support) only the CPU the timer
 * fires on is sampled.
 */
#if defined(CONFIG_SMP) && defined(CONFIG_SCHED_IPI_SUPPORTED)
#define PERF_IPI_SAMPLING 1
#else
#define PERF_IPI_SAMPLING 0
#endif

/** Statistics gathered during a single recording. */
struct perf_stats {
	/** Sampling attempts, including the ones filtered out by thread */
	atomic_t attempts[CONFIG_MP_MAX_NUM_CPUS];
	/** Samples that made it into the buffer */
	atomic_t samples[CONFIG_MP_MAX_NUM_CPUS];
	atomic_t timer_fires;
	atomic_t ipi_calls;
	atomic_t ipi_handler_calls;
};

struct perf_data_t {
	struct k_timer timer;
	struct k_work_delayable dwork;

	/* Protects the sample buffer against concurrent samplers */
	struct k_spinlock lock;

	const struct shell *sh;

	size_t idx;
	uintptr_t buf[CONFIG_PROFILING_PERF_BUFFER_SIZE];
	bool buf_full;
	k_tid_t target_thread;

	struct perf_stats stats;

#if PERF_IPI_SAMPLING
	struct k_ipi_work ipi_work;
	bool sampling_active;
#endif
};

static void perf_tracer(struct k_timer *timer);
static void perf_dwork_handler(struct k_work *work);
static struct perf_data_t perf_data = {
	.timer = Z_TIMER_INITIALIZER(perf_data.timer, perf_tracer, NULL),
	.dwork = Z_WORK_DELAYABLE_INITIALIZER(perf_dwork_handler),
};

/* Sample the CPU this runs on. Called from the timer ISR and from the IPI. */
static void perf_do_sample(void)
{
	k_tid_t current_thread = k_current_get();
	unsigned int cpu_id = CPU_ID;
	size_t trace_length = 0;
	k_spinlock_key_t key;

	atomic_inc(&perf_data.stats.attempts[cpu_id]);

	/* If target_thread is set, only trace that specific thread */
	if (perf_data.target_thread != NULL && perf_data.target_thread != current_thread) {
		return;
	}

	atomic_inc(&perf_data.stats.samples[cpu_id]);

	key = k_spin_lock(&perf_data.lock);

	if (perf_data.buf_full) {
		k_spin_unlock(&perf_data.lock, key);
		return;
	}

	if (++perf_data.idx < CONFIG_PROFILING_PERF_BUFFER_SIZE) {
		trace_length = arch_perf_current_stack_trace(perf_data.buf + perf_data.idx,
							     CONFIG_PROFILING_PERF_BUFFER_SIZE -
								     perf_data.idx);
	}

	if (trace_length != 0) {
		perf_data.buf[perf_data.idx - 1] = trace_length;
		perf_data.idx += trace_length;
	} else {
		--perf_data.idx;
		perf_data.buf_full = true;
		k_work_reschedule(&perf_data.dwork, K_NO_WAIT);
	}

	k_spin_unlock(&perf_data.lock, key);
}

#if PERF_IPI_SAMPLING

/* IPI handler - called on every CPU targeted by perf_sample_other_cpus() */
static void perf_ipi_handler(struct k_ipi_work *work)
{
	ARG_UNUSED(work);

	atomic_inc(&perf_data.stats.ipi_handler_calls);

	/* Only sample if we're actively profiling */
	if (perf_data.sampling_active) {
		perf_do_sample();
	}
}

static void perf_sampling_start(void)
{
	k_ipi_work_init(&perf_data.ipi_work);
	perf_data.sampling_active = true;
}

static void perf_sampling_stop(void)
{
	/* Stop the IPI handler from sampling before the timer is stopped */
	perf_data.sampling_active = false;
}

static void perf_sample_other_cpus(void)
{
	if (!perf_data.sampling_active || arch_num_cpus() <= 1) {
		return;
	}

	/* The current CPU is filtered out by k_ipi_work_add() */
	if (k_ipi_work_add(&perf_data.ipi_work, BIT_MASK(arch_num_cpus()),
			   perf_ipi_handler) != 0) {
		/* Previous broadcast is still in flight, skip this period */
		return;
	}

	atomic_inc(&perf_data.stats.ipi_calls);
	k_ipi_work_signal();
}

#else /* !PERF_IPI_SAMPLING */

static inline void perf_sampling_start(void)
{
}

static inline void perf_sampling_stop(void)
{
}

static inline void perf_sample_other_cpus(void)
{
}

#endif /* PERF_IPI_SAMPLING */

static void perf_stats_reset(struct perf_stats *stats)
{
	memset(stats, 0, sizeof(*stats));
}

static void perf_stats_print(const struct shell *sh, struct perf_stats *stats)
{
	shell_print(sh, "\n=== Perf Statistics ===");
	shell_print(sh, "Timer fires: %ld", (long)atomic_get(&stats->timer_fires));

	if (PERF_IPI_SAMPLING) {
		shell_print(sh, "IPI calls: %ld", (long)atomic_get(&stats->ipi_calls));
		shell_print(sh, "IPI handler calls: %ld",
			    (long)atomic_get(&stats->ipi_handler_calls));
	}

	shell_print(sh, "");
	for (unsigned int i = 0; i < arch_num_cpus(); i++) {
		shell_print(sh, "CPU %u: Attempts=%ld, Samples=%ld", i,
			    (long)atomic_get(&stats->attempts[i]),
			    (long)atomic_get(&stats->samples[i]));
	}
	shell_print(sh, "======================\n");
}

static void perf_tracer(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	atomic_inc(&perf_data.stats.timer_fires);

	perf_do_sample();
	perf_sample_other_cpus();
}

static void perf_dwork_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct perf_data_t *perf_data_ptr = CONTAINER_OF(dwork, struct perf_data_t, dwork);

	perf_sampling_stop();
	k_timer_stop(&perf_data_ptr->timer);

	perf_stats_print(perf_data_ptr->sh, &perf_data_ptr->stats);

	if (perf_data_ptr->buf_full) {
		shell_error(perf_data_ptr->sh, "Perf buf overflow!");
	} else {
		shell_print(perf_data_ptr->sh, "Perf done!");
	}
}

static int cmd_perf_record(const struct shell *sh, size_t argc, char **argv)
{
	if (k_work_delayable_is_pending(&perf_data.dwork)) {
		shell_warn(sh, "Perf is running");
		return -EINPROGRESS;
	}

	if (perf_data.buf_full) {
		shell_warn(sh, "Perf buffer is full");
		return -ENOBUFS;
	}
	k_timeout_t duration = K_MSEC(strtoll(argv[1], NULL, 10));
	k_timeout_t period = K_NSEC(1000000000 / strtoll(argv[2], NULL, 10));
	k_tid_t target_thread = NULL;

	/* If third argument is provided, use it as thread id */
	if (argc >= 4) {
		target_thread = (k_tid_t)strtoul(argv[3], NULL, 16);
		shell_print(sh, "Tracing thread: %p", target_thread);
	}

	perf_data.sh = sh;
	perf_data.target_thread = target_thread;
	perf_stats_reset(&perf_data.stats);

	shell_print(sh, "Number of CPUs: %u", arch_num_cpus());

	/* Enable sampling before the timer may fire */
	perf_sampling_start();

	/* Start the timer that samples this CPU and IPIs the other ones */
	k_timer_user_data_set(&perf_data.timer, &perf_data);
	k_timer_start(&perf_data.timer, K_NO_WAIT, period);

	k_work_schedule(&perf_data.dwork, duration);

	shell_print(sh, "Enabled perf");

	return 0;
}

static int cmd_perf_clear(const struct shell *sh, size_t argc, char **argv)
{
	if (sh != NULL) {
		if (k_work_delayable_is_pending(&perf_data.dwork)) {
			shell_warn(sh, "Perf is running");
			return -EINPROGRESS;
		}
		shell_print(sh, "Perf buffer cleared");
	}

	perf_data.idx = 0;
	perf_data.buf_full = false;

	return 0;
}

static int cmd_perf_info(const struct shell *sh, size_t argc, char **argv)
{
	if (k_work_delayable_is_pending(&perf_data.dwork)) {
		shell_print(sh, "Perf is running");
	}

	shell_print(sh, "Perf buf: %zu/%d %s", perf_data.idx, CONFIG_PROFILING_PERF_BUFFER_SIZE,
		    perf_data.buf_full ? "(full)" : "");

	return 0;
}

static int cmd_perf_print(const struct shell *sh, size_t argc, char **argv)
{
	if (k_work_delayable_is_pending(&perf_data.dwork)) {
		shell_warn(sh, "Perf is running");
		return -EINPROGRESS;
	}

	shell_print(sh, "Perf buf length %zu", perf_data.idx);
	for (size_t i = 0; i < perf_data.idx; i++) {
		shell_print(sh, "%016lx", perf_data.buf[i]);
	}

	cmd_perf_clear(NULL, 0, NULL);

	return 0;
}

#define CMD_HELP_RECORD                                                                            \
	"Start recording for <duration> ms on <frequency> Hz\n"                                    \
	"Usage: record <duration> <frequency> [thread_id]\n"                                       \
	"  thread_id: optional thread ID in hex format to trace specific thread"

SHELL_STATIC_SUBCMD_SET_CREATE(
	m_sub_perf, SHELL_CMD_ARG(record, NULL, CMD_HELP_RECORD, cmd_perf_record, 3, 1),
	SHELL_CMD_ARG(printbuf, NULL, "Print the perf buffer", cmd_perf_print, 0, 0),
	SHELL_CMD_ARG(clear, NULL, "Clear the perf buffer", cmd_perf_clear, 0, 0),
	SHELL_CMD_ARG(info, NULL, "Print the perf info", cmd_perf_info, 0, 0),
	SHELL_SUBCMD_SET_END);
SHELL_CMD_ARG_REGISTER(perf, &m_sub_perf, "Lightweight profiler", NULL, 0, 0);
