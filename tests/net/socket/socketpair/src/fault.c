/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_main.h"

#include <zephyr/kernel.h>
#include <zephyr/ztest_error_hook.h>

#if defined(CONFIG_USERSPACE) && defined(CONFIG_ZTEST_FATAL_HOOK)
static K_THREAD_STACK_DEFINE(fault_child_stack, 2048);
static struct k_thread fault_child_thread;

static void ungranted_recv_entry(void *p1, void *p2, void *p3)
{
	char c;

	/* the fault must be armed by the faulting thread itself */
	ztest_set_fault_valid(true);
	(void)zsock_recv(POINTER_TO_INT(p1), &c, 1, 0);
	zassert_unreachable("ungranted socket fd access did not fault");
}

ZTEST_F(net_socketpair, test_zsock_recv_fault)
{
	/* no K_INHERIT_PERMS: the child holds no grant on the fd slot */
	k_thread_create(&fault_child_thread, fault_child_stack,
			K_THREAD_STACK_SIZEOF(fault_child_stack), ungranted_recv_entry,
			INT_TO_POINTER(fixture->sv[0]), NULL, NULL,
			k_thread_priority_get(k_current_get()), K_USER, K_NO_WAIT);
	zassert_ok(k_thread_join(&fault_child_thread, K_FOREVER));
}
#endif
