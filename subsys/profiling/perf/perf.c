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

	const struct shell *sh;

	struct k_work_delayable dwork;

	size_t idx;
	uintptr_t buf[CONFIG_PROFILING_PERF_BUFFER_SIZE];
	bool buf_full;
};

static void perf_tracer(struct k_timer *timer);
static void perf_dwork_handler(struct k_work *work);
static struct perf_data_t perf_data = {
	.timer = Z_TIMER_INITIALIZER(perf_data.timer, perf_tracer, NULL),
	.dwork = Z_WORK_DELAYABLE_INITIALIZER(perf_dwork_handler),
};

static void perf_tracer(struct k_timer *timer)
{
	struct perf_data_t *perf_data_ptr =
		(struct perf_data_t *)k_timer_user_data_get(timer);

	size_t trace_length = 0;

	if (++perf_data_ptr->idx < CONFIG_PROFILING_PERF_BUFFER_SIZE) {
		trace_length = arch_perf_current_stack_trace(
					perf_data_ptr->buf + perf_data_ptr->idx,
					CONFIG_PROFILING_PERF_BUFFER_SIZE - perf_data_ptr->idx);
	}

	if (trace_length != 0) {
		perf_data_ptr->buf[perf_data_ptr->idx - 1] = trace_length;
		perf_data_ptr->idx += trace_length;
	} else {
		--perf_data_ptr->idx;
		perf_data_ptr->buf_full = true;
		k_work_reschedule(&perf_data_ptr->dwork, K_NO_WAIT);
	}
}

static void perf_dwork_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct perf_data_t *perf_data_ptr = CONTAINER_OF(dwork, struct perf_data_t, dwork);

	k_timer_stop(&perf_data_ptr->timer);
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

	perf_data.sh = sh;

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
	"Usage: record <duration> <frequency>"

SHELL_STATIC_SUBCMD_SET_CREATE(m_sub_perf,
	SHELL_CMD_ARG(record, NULL, CMD_HELP_RECORD, cmd_perf_record, 3, 0),
	SHELL_CMD_ARG(printbuf, NULL, "Print the perf buffer", cmd_perf_print, 0, 0),
	SHELL_CMD_ARG(clear, NULL, "Clear the perf buffer", cmd_perf_clear, 0, 0),
	SHELL_CMD_ARG(info, NULL, "Print the perf info", cmd_perf_info, 0, 0),
	SHELL_SUBCMD_SET_END
);
SHELL_CMD_ARG_REGISTER(perf, &m_sub_perf, "Lightweight profiler", NULL, 0, 0);
