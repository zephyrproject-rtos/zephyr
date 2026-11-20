/*
 * Copyright (c) 2017 Wind River Systems, Inc.
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * System call verifiers for kernel/poll.c, compiled only with CONFIG_USERSPACE.
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <zephyr/internal/syscall_handler.h>
#include <string.h>

static inline int z_vrfy_k_poll(struct k_poll_event *events,
				int num_events, k_timeout_t timeout)
{
	int ret;
	struct k_poll_event *events_copy = NULL;
	uint32_t bounds;

	/* Validate the events buffer and make a copy of it in an
	 * allocated kernel-side buffer.
	 */
	if (K_SYSCALL_VERIFY(num_events >= 0)) {
		ret = -EINVAL;
		goto out;
	}
	if (K_SYSCALL_VERIFY_MSG(!u32_mul_overflow(num_events,
						   sizeof(struct k_poll_event),
						   &bounds),
				 "num_events too large")) {
		ret = -EINVAL;
		goto out;
	}
	events_copy = z_thread_malloc(bounds);
	if (!events_copy) {
		ret = -ENOMEM;
		goto out;
	}

	if (K_SYSCALL_MEMORY_WRITE(events, bounds)) {
		goto oops_free;
	}

	(void)memcpy(events_copy, events, bounds);

	/* Validate what's inside events_copy */
	for (int i = 0; i < num_events; i++) {
		struct k_poll_event *e = &events_copy[i];

		if (K_SYSCALL_VERIFY(e->mode == K_POLL_MODE_NOTIFY_ONLY)) {
			ret = -EINVAL;
			goto out_free;
		}

		switch (e->type) {
		case K_POLL_TYPE_IGNORE:
			break;
		case K_POLL_TYPE_SIGNAL:
			if (K_SYSCALL_OBJ(e->signal, K_OBJ_POLL_SIGNAL)) {
				goto oops_free;
			}
			break;
		case K_POLL_TYPE_SEM_AVAILABLE:
			if (K_SYSCALL_OBJ(e->sem, K_OBJ_SEM)) {
				goto oops_free;
			}
			break;
		case K_POLL_TYPE_DATA_AVAILABLE:
			if (K_SYSCALL_OBJ(e->queue, K_OBJ_QUEUE)) {
				goto oops_free;
			}
			break;
		case K_POLL_TYPE_MSGQ_DATA_AVAILABLE:
			if (K_SYSCALL_OBJ(e->msgq, K_OBJ_MSGQ)) {
				goto oops_free;
			}
			break;
		case K_POLL_TYPE_PIPE_DATA_AVAILABLE:
			if (K_SYSCALL_OBJ(e->pipe, K_OBJ_PIPE)) {
				goto oops_free;
			}
			break;
		default:
			ret = -EINVAL;
			goto out_free;
		}
	}

	ret = k_poll(events_copy, num_events, timeout);
	(void)memcpy((void *)events, events_copy, bounds);
out_free:
	k_free(events_copy);
out:
	return ret;
oops_free:
	k_free(events_copy);
	K_OOPS(1);
}
#include <zephyr/syscalls/k_poll_mrsh.c>

static inline void z_vrfy_k_poll_signal_init(struct k_poll_signal *sig)
{
	K_OOPS(K_SYSCALL_OBJ_INIT(sig, K_OBJ_POLL_SIGNAL));
	z_impl_k_poll_signal_init(sig);
}
#include <zephyr/syscalls/k_poll_signal_init_mrsh.c>

void z_vrfy_k_poll_signal_check(struct k_poll_signal *sig,
			       unsigned int *signaled, int *result)
{
	K_OOPS(K_SYSCALL_OBJ(sig, K_OBJ_POLL_SIGNAL));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(signaled, sizeof(unsigned int)));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(result, sizeof(int)));
	z_impl_k_poll_signal_check(sig, signaled, result);
}
#include <zephyr/syscalls/k_poll_signal_check_mrsh.c>

static inline int z_vrfy_k_poll_signal_raise(struct k_poll_signal *sig,
					     int result)
{
	K_OOPS(K_SYSCALL_OBJ(sig, K_OBJ_POLL_SIGNAL));
	return z_impl_k_poll_signal_raise(sig, result);
}
#include <zephyr/syscalls/k_poll_signal_raise_mrsh.c>

static inline void z_vrfy_k_poll_signal_reset(struct k_poll_signal *sig)
{
	K_OOPS(K_SYSCALL_OBJ(sig, K_OBJ_POLL_SIGNAL));
	z_impl_k_poll_signal_reset(sig);
}
#include <zephyr/syscalls/k_poll_signal_reset_mrsh.c>
