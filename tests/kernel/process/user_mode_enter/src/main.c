/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The boot thread enters user mode, allocates a kernel object and creates a
 * user thread that uses it. The child must end up in the same process as its
 * creator, and objects granted to the boot thread before it entered user mode
 * must still be usable afterwards.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/kobject.h>

#define STACKSZ 1024

K_THREAD_STACK_DEFINE(child_stack, STACKSZ);
static struct k_thread child;
K_SEM_DEFINE(granted_before_enter, 0, 1);

static void child_fn(void *p1, void *p2, void *p3)
{
	struct k_mutex *mu = p1;

	/* the object was allocated by the creator after it entered user mode */
	if (k_mutex_lock(mu, K_NO_WAIT) != 0) {
		printk("user_mode_enter: child cannot lock the creator's mutex\n");
		return;
	}
	k_mutex_unlock(mu);
	printk("user_mode_enter: child ok\n");
}

static void user_entry(void *p1, void *p2, void *p3)
{
	struct k_mutex *mu;

	/* granted to the boot thread before it dropped to user mode */
	k_sem_give(&granted_before_enter);

	mu = k_object_alloc(K_OBJ_MUTEX);
	if (mu == NULL) {
		printk("user_mode_enter: k_object_alloc failed\n");
		return;
	}
	k_mutex_init(mu);

	k_thread_create(&child, child_stack, STACKSZ, child_fn, mu, NULL, NULL,
			k_thread_priority_get(k_current_get()), K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	k_thread_join(&child, K_FOREVER);
}

int main(void)
{
	k_thread_access_grant(k_current_get(), &child, &child_stack, &granted_before_enter);
	k_thread_user_mode_enter(user_entry, NULL, NULL, NULL);

	return 0;
}
