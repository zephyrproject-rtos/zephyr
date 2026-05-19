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

size_t arch_perf_current_stack_trace(uintptr_t *buf, size_t size);

struct perf_data_t {
	struct k_timer timer;
#ifdef CONFIG_SMP
	struct k_spinlock lock;
	struct k_ipi_work ipi_work;
#endif

	const struct shell *sh;

	struct k_work_delayable dwork;

	size_t idx;
	uintptr_t buf[CONFIG_PROFILING_PERF_BUFFER_SIZE];
	bool buf_full;
	k_tid_t target_thread;
#ifdef CONFIG_SMP
	bool sampling_active;
	/* Statistics: per-CPU counters */
	atomic_t sample_count[CONFIG_MP_MAX_NUM_CPUS];
	/* Track total attempts including filtered samples */
	atomic_t sample_attempts[CONFIG_MP_MAX_NUM_CPUS];
	/* Track timer fires and IPI calls */
	atomic_t timer_fires;
	atomic_t ipi_calls;
	atomic_t ipi_handler_calls;
#endif
};

static void perf_tracer(struct k_timer *timer);
static void perf_dwork_handler(struct k_work *work);
static struct perf_data_t perf_data = {
	.timer = Z_TIMER_INITIALIZER(perf_data.timer, perf_tracer, NULL),
	.dwork = Z_WORK_DELAYABLE_INITIALIZER(perf_dwork_handler),
};

/* Common sampling function called from timer on each CPU */
static void perf_do_sample(void)
{
	size_t trace_length = 0;
	k_tid_t current_thread;
#ifdef CONFIG_SMP
	unsigned int cpu_id = arch_curr_cpu()->id;
	k_spinlock_key_t key;
#endif

	current_thread = k_current_get();

#ifdef CONFIG_SMP
	/* Track every attempt to sample */
	atomic_inc(&perf_data.sample_attempts[cpu_id]);
#endif

	/* If target_thread is set, only trace that specific thread */
	if (perf_data.target_thread != NULL && perf_data.target_thread != current_thread) {
		return;
	}

#ifdef CONFIG_SMP
	/* Count actual sample taken */
	atomic_inc(&perf_data.sample_count[cpu_id]);
	/* Lock to protect shared buffer access across cores */
	key = k_spin_lock(&perf_data.lock);
#endif

	if (perf_data.buf_full) {
#ifdef CONFIG_SMP
		k_spin_unlock(&perf_data.lock, key);
#endif
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

#ifdef CONFIG_SMP
	k_spin_unlock(&perf_data.lock, key);
#endif
}
#ifdef CONFIG_SMP
/* IPI handler - called on all CPUs when IPI is sent */
void perf_sched_ipi_handler(struct k_ipi_work *item)
{
	atomic_inc(&perf_data.ipi_handler_calls);
	/* Only sample if we're actively profiling */
	if (perf_data.sampling_active) {
		perf_do_sample();
	}
}
#endif

static void perf_tracer(struct k_timer *timer)
{
	ARG_UNUSED(timer);

#ifdef CONFIG_SMP
	atomic_inc(&perf_data.timer_fires);
#endif

	/* Sample on current CPU */
	perf_do_sample();

#ifdef CONFIG_SMP
	/* Send IPI to all other CPUs to trigger sampling there */
	if (perf_data.sampling_active && arch_num_cpus() > 1) {
		atomic_inc(&perf_data.ipi_calls);
		k_ipi_work_add(&perf_data.ipi_work, 0x00ff, perf_sched_ipi_handler);
		k_ipi_work_signal();
	}
#endif
}

static void perf_dwork_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct perf_data_t *perf_data_ptr = CONTAINER_OF(dwork, struct perf_data_t, dwork);

#ifdef CONFIG_SMP
	/* Disable sampling first to prevent IPI handler from sampling */
	perf_data_ptr->sampling_active = false;
#endif

	/* Stop the timer */
	k_timer_stop(&perf_data_ptr->timer);
#ifdef CONFIG_SMP
	/* Print statistics */
	shell_print(perf_data_ptr->sh, "\n=== Perf Statistics ===");
	shell_print(perf_data_ptr->sh, "Timer fires: %ld",
		    (long)atomic_get(&perf_data_ptr->timer_fires));
	shell_print(perf_data_ptr->sh, "IPI calls: %ld",
		    (long)atomic_get(&perf_data_ptr->ipi_calls));
	shell_print(perf_data_ptr->sh, "IPI handler calls: %ld",
		    (long)atomic_get(&perf_data_ptr->ipi_handler_calls));
	shell_print(perf_data_ptr->sh, "");
	for (unsigned int i = 0; i < arch_num_cpus(); i++) {
		shell_print(perf_data_ptr->sh, "CPU %u: Attempts=%ld, Samples=%ld", i,
			    (long)atomic_get(&perf_data_ptr->sample_attempts[i]),
			    (long)atomic_get(&perf_data_ptr->sample_count[i]));
	}
	shell_print(perf_data_ptr->sh, "======================\n");
#endif

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

#ifdef CONFIG_SMP
	/* Clear statistics and enable sampling */
	unsigned int num_cpus = arch_num_cpus();

	for (unsigned int i = 0; i < num_cpus; i++) {
		atomic_set(&perf_data.sample_count[i], 0);
		atomic_set(&perf_data.sample_attempts[i], 0);
	}
	atomic_set(&perf_data.timer_fires, 0);
	atomic_set(&perf_data.ipi_calls, 0);
	atomic_set(&perf_data.ipi_handler_calls, 0);

	/* Print CPU info */
	shell_print(sh, "Number of CPUs: %u", num_cpus);
	shell_print(sh, "Starting SMP profiling with IPI...");

	/* Enable sampling flag before starting timer */
	perf_data.sampling_active = true;
	k_ipi_work_init(&perf_data.ipi_work);
#endif

	/* Start single timer that will trigger IPI to other CPUs */
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
