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
	k_timeout_t duration;

	bool is_running;

	const struct shell *sh;

	struct k_event event;
	struct k_thread event_handler_thread;

	size_t idx;
	uintptr_t buf[CONFIG_PROFILING_PERF_BUFFER_SIZE];
};

#define PERF_EVENT_TRACING_BUF_OVERFLOW (1 << 0)

#define PERF_THREAD_STACK_SIZE  2048
#define PERF_THREAD_PRIORITY	7

static struct perf_data_t perf_data = {
	.idx = 0,
	.is_running = false
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
		k_timer_stop(timer);
		k_event_set(&perf_data_ptr->event, PERF_EVENT_TRACING_BUF_OVERFLOW);
	}
}

static void perf_events_handler(void *perf_data_param, void *, void *)
{
	struct perf_data_t *perf_data_ptr = (struct perf_data_t *) perf_data_param;
	uint32_t perf_events = k_event_wait(&perf_data.event, PERF_EVENT_TRACING_BUF_OVERFLOW,
			true, perf_data_ptr->duration);

	if (perf_events) {
		shell_print(perf_data_ptr->sh, "Perf buf overflow!");
	} else {
		k_timer_stop(&perf_data_ptr->timer);
		shell_print(perf_data_ptr->sh, "Perf done!");
	}

	perf_data.is_running = false;
}

K_THREAD_STACK_DEFINE(perf_event_handler_thread_stack_area, PERF_THREAD_STACK_SIZE);

static int perf_init(void)
{
	k_timer_init(&perf_data.timer, perf_tracer, NULL);
	k_event_init(&perf_data.event);

	return 0;
}

int cmd_perf_record(const struct shell *sh, size_t argc, char **argv)
{
	if (perf_data.is_running) {
		shell_print(sh, "Perf is already running");
		return -EINPROGRESS;
	}

	perf_data.is_running = true;

	perf_data.duration = K_MSEC(strtoll(argv[1], NULL, 10));
	k_timeout_t period = K_NSEC(1000000000 / strtoll(argv[2], NULL, 10));

	perf_data.sh = sh;

	k_event_clear(&perf_data.event, 0xffffffff);

	k_timer_user_data_set(&perf_data.timer, &perf_data);
	k_timer_start(&perf_data.timer, K_NO_WAIT, period);

	k_thread_create(&perf_data.event_handler_thread,
					perf_event_handler_thread_stack_area,
					K_THREAD_STACK_SIZEOF(perf_event_handler_thread_stack_area),
					perf_events_handler,
					&perf_data, NULL, NULL,
					PERF_THREAD_PRIORITY, 0, K_NO_WAIT);

	shell_print(sh, "Enabled perf");

	return 0;
}

int cmd_perf_print(const struct shell *sh, size_t argc, char **argv)
{
	if (perf_data.is_running) {
		shell_print(sh, "Perf is still running. Wait for ending");
		return -EINPROGRESS;
	}

	shell_print(sh, "Perf buf length %zu", perf_data.idx);
	for (size_t i = 0; i < perf_data.idx; i++) {
		shell_print(sh, "%016lx", perf_data.buf[i]);
	}

	perf_data.idx = 0;

	return 0;
}

static int cmd_perf(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "perfy");
	return 0;
}

#define CMD_HELP_RECORD										\
	"Start recording for <duration> ms on <frequency> Hz\n"	\
	"Usage: record <duration> <frequency>\n"

SHELL_STATIC_SUBCMD_SET_CREATE(m_sub_perf,
	SHELL_CMD_ARG(record, NULL, CMD_HELP_RECORD, cmd_perf_record, 3, 0),
	SHELL_CMD_ARG(printbuf, NULL, "Print the perf buffer", cmd_perf_print, 0, 0),
	SHELL_SUBCMD_SET_END
);
SHELL_CMD_ARG_REGISTER(perf, &m_sub_perf, "Perf!", cmd_perf, 0, 0);

SYS_INIT(perf_init, APPLICATION, 0);
