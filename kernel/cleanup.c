/*
 * Copyright (c) 2024, Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

struct k_thread_cleanup {
	k_thread_cleanup_handler_t routine;
	void *arg;
	sys_snode_t node;
};
BUILD_ASSERT(3 * sizeof(void *) == sizeof(struct k_thread_cleanup));

void z_impl_k_thread_cleanup_push(void **cleanup, k_thread_cleanup_handler_t routine, void *arg)
{
	struct k_thread_cleanup *const c = (struct k_thread_cleanup *)cleanup;

	*c = (struct k_thread_cleanup){
		.routine = routine,
		.arg = arg,
	};

	sys_slist_prepend(&_current->base.cleanup, &c->node);
}

#ifdef CONFIG_USERSPACE
void z_vrfy_k_thread_cleanup_push(void **cleanup, k_thread_cleanup_handler_t routine, void *arg)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(cleanup, 3 * sizeof(void *)));

	z_impl_k_thread_cleanup_push(cleanup, routine, arg);
}
#include <zephyr/syscalls/k_thread_cleanup_push_mrsh.c>
#endif

bool z_impl_k_thread_cleanup_pop_entry(k_thread_cleanup_handler_t *routine, void **arg)
{
	sys_snode_t *const node = sys_slist_get(&_current->base.cleanup);

	if (node == NULL) {
		return false;
	}

	struct k_thread_cleanup *const c = CONTAINER_OF(node, struct k_thread_cleanup, node);

	*routine = c->routine;
	*arg = c->arg;

	return true;
}

#ifdef CONFIG_USERSPACE
bool z_vrfy_k_thread_cleanup_pop_entry(k_thread_cleanup_handler_t *routine, void **arg)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(routine, sizeof(*routine)));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(arg, sizeof(*arg)));

	/* the entry lives in the caller's memory: never follow a link it has corrupted */
	sys_snode_t *const node = sys_slist_peek_head(&_current->base.cleanup);

	if (node != NULL) {
		struct k_thread_cleanup *const c = CONTAINER_OF(node, struct k_thread_cleanup, node);

		K_OOPS(K_SYSCALL_MEMORY_WRITE(c, sizeof(*c)));
	}

	return z_impl_k_thread_cleanup_pop_entry(routine, arg);
}
#include <zephyr/syscalls/k_thread_cleanup_pop_entry_mrsh.c>
#endif
