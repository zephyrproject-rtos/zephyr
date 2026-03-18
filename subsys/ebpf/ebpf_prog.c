/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/iterable_sections.h>
#include <string.h>

#include <zephyr/ebpf/ebpf_prog.h>
#include <zephyr/ebpf/ebpf_tracepoint.h>

LOG_MODULE_DECLARE(ebpf_vm, CONFIG_EBPF_LOG_LEVEL);

int ebpf_verify(const struct ebpf_prog *prog);
int ebpf_tracing_attach(struct ebpf_prog *prog, int32_t tp_id);
void ebpf_tracing_detach(struct ebpf_prog *prog);

/**
 * Check whether a program type is allowed to attach to a tracepoint.
 */
static bool ebpf_prog_matches_tracepoint(enum ebpf_prog_type type, int32_t tp_id)
{
	switch (type) {
	case EBPF_PROG_TYPE_TRACEPOINT:
		return true;
	case EBPF_PROG_TYPE_SCHED:
		return tp_id >= EBPF_TP_THREAD_SWITCHED_IN && tp_id <= EBPF_TP_IDLE_EXIT;
	case EBPF_PROG_TYPE_SYSCALL:
		return tp_id == EBPF_TP_SYSCALL_ENTER || tp_id == EBPF_TP_SYSCALL_EXIT;
	case EBPF_PROG_TYPE_ISR:
		return tp_id == EBPF_TP_ISR_ENTER || tp_id == EBPF_TP_ISR_EXIT;
	default:
		return false;
	}
}

int ebpf_prog_attach(struct ebpf_prog *prog, int32_t tp_id)
{
	if (prog == NULL) {
		return -EINVAL;
	}

	if (tp_id < 0 || tp_id >= EBPF_TP_MAX) {
		LOG_ERR("Program '%s' invalid TP %d", prog->name, tp_id);
		return -EINVAL;
	}

	if (prog->attach_point >= 0) {
		LOG_WRN("Program '%s' already attached to TP %d",
			prog->name, prog->attach_point);
		return -EALREADY;
	}

	if (!ebpf_prog_matches_tracepoint(prog->type, tp_id)) {
		LOG_ERR("Program '%s' type %d incompatible with TP %d",
			prog->name, prog->type, tp_id);
		return -EINVAL;
	}

#ifdef CONFIG_EBPF_TRACING
	/* Attach to the tracepoint (this will call back into the program when the TP fires). */
	int ret = ebpf_tracing_attach(prog, tp_id);

	if (ret != 0) {
		return ret;
	}
#endif

	prog->attach_point = tp_id;

	LOG_DBG("Program '%s' attached to TP %d", prog->name, tp_id);

	return 0;
}

int ebpf_prog_detach(struct ebpf_prog *prog)
{
	if (prog == NULL) {
		return -EINVAL;
	}

	if (prog->state == EBPF_PROG_STATE_ENABLED) {
		ebpf_prog_disable(prog);
	}

#ifdef CONFIG_EBPF_TRACING
	ebpf_tracing_detach(prog);
#endif

	prog->attach_point = -1;

	LOG_DBG("Program '%s' detached", prog->name);

	return 0;
}

int ebpf_prog_enable(struct ebpf_prog *prog)
{
	if (prog == NULL) {
		return -EINVAL;
	}

	if (prog->attach_point < 0) {
		LOG_ERR("Program '%s' not attached", prog->name);
		return -ENOENT;
	}

	/* Run verifier if not already verified */
	if (prog->state == EBPF_PROG_STATE_DISABLED) {
		int ret = ebpf_verify(prog);

		if (ret != 0) {
			LOG_ERR("Verification failed for '%s': %d", prog->name, ret);
			return ret;
		}
		prog->state = EBPF_PROG_STATE_VERIFIED;
	}

	prog->state = EBPF_PROG_STATE_ENABLED;
	LOG_DBG("Program '%s' enabled", prog->name);

	return 0;
}

int ebpf_prog_disable(struct ebpf_prog *prog)
{
	if (prog == NULL) {
		return -EINVAL;
	}

	prog->state = EBPF_PROG_STATE_DISABLED;
	LOG_DBG("Program '%s' disabled", prog->name);

	return 0;
}

struct ebpf_prog *ebpf_prog_find(const char *name)
{
	STRUCT_SECTION_FOREACH(ebpf_prog, prog) {
		if (strcmp(prog->name, name) == 0) {
			return prog;
		}
	}

	return NULL;
}
