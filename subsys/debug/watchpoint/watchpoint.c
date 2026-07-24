/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/debug/debugpoint_internal.h>
#include <zephyr/debug/watchpoint.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/arch_interface.h>
#include <stdint.h>

enum watchpoint_state {
	WATCHPOINT_STATE_DISARMED,
	WATCHPOINT_STATE_ARMING,
	WATCHPOINT_STATE_ARMED,
	WATCHPOINT_STATE_DISARMING,
};

static enum z_debugpoint_type watchpoint_type(uint32_t flags)
{
	if ((flags & K_WATCHPOINT_RW) == K_WATCHPOINT_RW) {
		return Z_DEBUGPOINT_WATCH_RW;
	}

	return (flags & K_WATCHPOINT_WRITE) != 0U ?
	       Z_DEBUGPOINT_WATCH_WRITE : Z_DEBUGPOINT_WATCH_READ;
}

static uint32_t watchpoint_flags(enum z_debugpoint_type type)
{
	if (type == Z_DEBUGPOINT_WATCH_WRITE) {
		return K_WATCHPOINT_WRITE;
	}
	if (type == Z_DEBUGPOINT_WATCH_RW) {
		return K_WATCHPOINT_RW;
	}

	return K_WATCHPOINT_READ;
}

static enum k_watchpoint_timing watchpoint_timing(enum z_debugpoint_timing timing)
{
	if (timing == Z_DEBUGPOINT_TIMING_BEFORE) {
		return K_WATCHPOINT_TIMING_BEFORE;
	}
	if (timing == Z_DEBUGPOINT_TIMING_AFTER) {
		return K_WATCHPOINT_TIMING_AFTER;
	}

	return K_WATCHPOINT_TIMING_UNKNOWN;
}

static void watchpoint_deactivate(void *arg)
{
	struct k_watchpoint *wp = arg;

	for (;;) {
		atomic_val_t state = atomic_get(&wp->_state);

		if (state != WATCHPOINT_STATE_ARMED &&
		    state != WATCHPOINT_STATE_ARMING) {
			return;
		}
		if (atomic_cas(&wp->_state, state, WATCHPOINT_STATE_DISARMED)) {
			return;
		}
	}
}

#if defined(CONFIG_WATCHPOINT_CALLSTACK)
struct watchpoint_stack_ctx {
	uintptr_t *buf;
	size_t depth;
	size_t max_depth;
};

static bool watchpoint_stack_cb(void *cookie, unsigned long addr)
{
	struct watchpoint_stack_ctx *ctx = cookie;
	uintptr_t pc = (uintptr_t)addr;

	if (ctx->depth > 0U && ctx->buf[ctx->depth - 1U] == pc) {
		return true;
	}
	if (ctx->depth >= ctx->max_depth) {
		return false;
	}

	ctx->buf[ctx->depth++] = pc;
	return true;
}

static size_t watchpoint_capture_stack(const struct z_debugpoint_event *event,
				       uintptr_t *buf, size_t max_depth)
{
	struct watchpoint_stack_ctx ctx = {
		.buf = buf,
		.max_depth = max_depth,
	};

	if (max_depth == 0U) {
		return 0U;
	}
	if (event->pc != NULL) {
		buf[ctx.depth++] = (uintptr_t)event->pc;
	}
	if (event->esf != NULL && ctx.depth < ctx.max_depth) {
		arch_stack_walk(watchpoint_stack_cb, &ctx, _current, event->esf);
	}

	return ctx.depth;
}
#endif

static void watchpoint_callback(const struct z_debugpoint_config *config,
				const struct z_debugpoint_event *event,
				void *arg)
{
	ARG_UNUSED(config);

	struct k_watchpoint *wp = arg;
#if defined(CONFIG_WATCHPOINT_CALLSTACK)
	uintptr_t callstack[CONFIG_WATCHPOINT_CALLSTACK_DEPTH];
	size_t callstack_depth = watchpoint_capture_stack(event, callstack,
							 ARRAY_SIZE(callstack));
#else
	const uintptr_t *callstack = NULL;
	size_t callstack_depth = 0U;
#endif
	struct k_watchpoint_event wp_event = {
		.pc = event->pc,
		.access_addr = event->access_addr,
		.access_addr_valid = event->access_addr_valid,
		.access_size = event->access_size,
		.flags = watchpoint_flags(event->type),
		.timing = watchpoint_timing(event->timing),
		.rearm_required = event->rearm_required,
		.callstack = callstack,
		.callstack_depth = callstack_depth,
	};

	wp->cb(wp, &wp_event, wp->arg);

	if (event->rearm_required) {
		watchpoint_deactivate(wp);
	}
}

static bool watchpoint_in_callback(void)
{
	return z_debugpoint_in_callback();
}

static bool invalid_call_context(void)
{
	return k_is_in_isr() || !arch_cpu_irqs_are_enabled() ||
	       watchpoint_in_callback();
}

static bool invalid_watchpoint(const struct k_watchpoint *wp)
{
	return wp == NULL || wp->size == 0U ||
	       UINTPTR_MAX - (uintptr_t)wp->addr < wp->size - 1U ||
	       (wp->flags & K_WATCHPOINT_RW) == 0U ||
	       (wp->flags & ~K_WATCHPOINT_RW) != 0U || wp->cb == NULL;
}

int k_watchpoint_add(struct k_watchpoint *wp)
{
	if (invalid_watchpoint(wp)) {
		return -EINVAL;
	}
	if (invalid_call_context()) {
		return -EWOULDBLOCK;
	}

	if (!atomic_cas(&wp->_state, WATCHPOINT_STATE_DISARMED,
			WATCHPOINT_STATE_ARMING)) {
		return -EBUSY;
	}

	struct z_debugpoint_config config = {
		.type = watchpoint_type(wp->flags),
		.addr = wp->addr,
		.size = wp->size,
		.callback = watchpoint_callback,
		.user_data = wp,
	};

	int ret = z_debugpoint_add(&config, &wp->_handle);

	if (ret == 0) {
		(void)atomic_cas(&wp->_state, WATCHPOINT_STATE_ARMING,
				 WATCHPOINT_STATE_ARMED);
		return 0;
	}

	(void)atomic_cas(&wp->_state, WATCHPOINT_STATE_ARMING,
			 WATCHPOINT_STATE_DISARMED);
	return ret;
}

int k_watchpoint_remove(struct k_watchpoint *wp)
{
	if (wp == NULL) {
		return -EINVAL;
	}
	if (invalid_call_context()) {
		return -EWOULDBLOCK;
	}

	atomic_val_t state;

	for (;;) {
		state = atomic_get(&wp->_state);
		if (state == WATCHPOINT_STATE_DISARMED) {
			if (atomic_cas(&wp->_state, WATCHPOINT_STATE_DISARMED,
				       WATCHPOINT_STATE_DISARMING)) {
				break;
			}
			continue;
		}
		if (state != WATCHPOINT_STATE_ARMED) {
			return -EBUSY;
		}
		if (atomic_cas(&wp->_state, state, WATCHPOINT_STATE_DISARMING)) {
			break;
		}
	}

	int ret = z_debugpoint_remove(wp->_handle);

	if (ret == 0) {
		wp->_handle = Z_DEBUGPOINT_HANDLE_INVALID;
		(void)atomic_cas(&wp->_state, WATCHPOINT_STATE_DISARMING,
				 WATCHPOINT_STATE_DISARMED);
	} else {
		(void)atomic_cas(&wp->_state, WATCHPOINT_STATE_DISARMING, state);
	}
	return ret;
}

bool k_watchpoint_is_active(const struct k_watchpoint *wp)
{
	if (wp == NULL) {
		return false;
	}

	atomic_val_t state = atomic_get(&wp->_state);

	/* Transitional states can still own hardware and always own the descriptor. */
	return state != WATCHPOINT_STATE_DISARMED;
}
