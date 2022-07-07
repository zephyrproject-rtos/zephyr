/*
 * Copyright (c) 2021 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <ztest.h>

extern void test_newlib_thread_safety_locks(void);
extern void test_newlib_thread_safety_stress(void);

void test_main(void)
{
#ifdef CONFIG_USERSPACE
	k_thread_system_pool_assign(k_current_get());
#endif /* CONFIG_USERSPACE */

	/* Invoke testsuites */
#ifdef CONFIG_NEWLIB_THREAD_SAFETY_TEST_LOCKS
	test_newlib_thread_safety_locks();
#endif /* CONFIG_NEWLIB_THREAD_SAFETY_TEST_LOCKS */
#ifdef CONFIG_NEWLIB_THREAD_SAFETY_TEST_STRESS
	test_newlib_thread_safety_stress();
#endif /* CONFIG_NEWLIB_THREAD_SAFETY_TEST_STRESS */
}
