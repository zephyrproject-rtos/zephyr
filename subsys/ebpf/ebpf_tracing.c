/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/ebpf/ebpf_tracepoint.h>
#include <zephyr/ebpf/ebpf_prog.h>
#include "ebpf_vm.h"

LOG_MODULE_DECLARE(ebpf_vm, CONFIG_EBPF_LOG_LEVEL);

/**
 * Hold the program currently attached to each tracepoint.
 */
static atomic_ptr_t ebpf_tp_table[EBPF_TP_MAX];

static struct ebpf_prog *ebpf_tracepoint_get_active(enum ebpf_tracepoint_id tp_id)
{
	struct ebpf_prog *prog = NULL;

	if (tp_id >= EBPF_TP_MAX) {
		return NULL;
	}

	prog = (struct ebpf_prog *)atomic_ptr_get(&ebpf_tp_table[tp_id]);

	if (prog == NULL || prog->state != EBPF_PROG_STATE_ENABLED) {
		prog = NULL;
	}

	return prog;
}

bool ebpf_tracepoint_is_active(enum ebpf_tracepoint_id tp_id)
{
	return ebpf_tracepoint_get_active(tp_id) != NULL;
}

/**
 * Dispatch the enabled program bound to a tracepoint, if any.
 */
void ebpf_tracepoint_dispatch(enum ebpf_tracepoint_id tp_id, void *ctx, uint32_t ctx_size)
{
	struct ebpf_prog *prog = ebpf_tracepoint_get_active(tp_id);

	if (prog == NULL) {
		return;
	}

#ifdef CONFIG_EBPF_STATS
	uint32_t start_cycles = k_cycle_get_32();
#endif

	int64_t ret = ebpf_vm_exec(prog, ctx, ctx_size);

	ARG_UNUSED(ret);

#ifdef CONFIG_EBPF_STATS
	uint32_t elapsed = k_cycle_get_32() - start_cycles;

	prog->run_count++;
	prog->run_time_ns += k_cyc_to_ns_floor64(elapsed);
#endif
}

/**
 * Attach one program to one tracepoint slot in the dispatch table.
 */
int ebpf_tracing_attach(struct ebpf_prog *prog, int32_t tp_id)
{
	struct ebpf_prog *current;

	if (tp_id >= 0 && tp_id < EBPF_TP_MAX) {
		current = (struct ebpf_prog *)atomic_ptr_get(&ebpf_tp_table[tp_id]);
		if (current != NULL && current != prog) {
			LOG_ERR("TP %d already has program '%s' attached",
				tp_id, current->name);

			return -EBUSY;
		}

		if (current == prog || atomic_ptr_cas(&ebpf_tp_table[tp_id], NULL, prog)) {
			return 0;
		}

		current = (struct ebpf_prog *)atomic_ptr_get(&ebpf_tp_table[tp_id]);
		LOG_ERR("TP %d already has program '%s' attached",
			tp_id, current != NULL ? current->name : "<unknown>");

		return -EBUSY;
	}

	return -EINVAL;
}

void ebpf_tracing_detach(struct ebpf_prog *prog)
{
	if (prog->attach_point >= 0 && prog->attach_point < EBPF_TP_MAX) {
		(void)atomic_ptr_cas(&ebpf_tp_table[prog->attach_point], prog, NULL);
	}
}

/**
 * Bridge Zephyr ISR enter events into the eBPF tracepoint dispatcher.
 */
void sys_trace_isr_enter(void)
{
	if (ebpf_tracepoint_is_active(EBPF_TP_ISR_ENTER)) {
		struct ebpf_ctx_isr ctx = { .irq_num = 0 };

		ebpf_tracepoint_dispatch(EBPF_TP_ISR_ENTER, &ctx, sizeof(ctx));
	}
}

void sys_trace_isr_exit(void)
{
	if (ebpf_tracepoint_is_active(EBPF_TP_ISR_EXIT)) {
		struct ebpf_ctx_isr ctx = { .irq_num = 0 };

		ebpf_tracepoint_dispatch(EBPF_TP_ISR_EXIT, &ctx, sizeof(ctx));
	}
}

void sys_trace_isr_exit_to_scheduler(void)
{
	sys_trace_isr_exit();
}

void sys_trace_idle(void)
{
	if (ebpf_tracepoint_is_active(EBPF_TP_IDLE_ENTER)) {
		struct ebpf_ctx_thread ctx = { 0 };

		ebpf_tracepoint_dispatch(EBPF_TP_IDLE_ENTER, &ctx, sizeof(ctx));
	}
}

void sys_trace_idle_exit(void)
{
	if (ebpf_tracepoint_is_active(EBPF_TP_IDLE_EXIT)) {
		struct ebpf_ctx_thread ctx = { 0 };

		ebpf_tracepoint_dispatch(EBPF_TP_IDLE_EXIT, &ctx, sizeof(ctx));
	}
}
