/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * CONFIG_MEM_WRITE_FAULT_HOOK: a write to a present, write-protected page is
 * offered to z_mem_write_fault() before it becomes a fatal error. The test is
 * the hook's provider: it makes the page writable, in the kernel's view with
 * k_mem_update_flags() and in the faulting thread's memory domain with
 * arch_mem_domain_page_attr_set(), and lets the write retry.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

#include <kernel_arch_interface.h>
#include <kernel_internal.h>

#define PAGE CONFIG_MMU_PAGE_SIZE

/*
 * The page the hook resolves faults on, and how often it did. Volatile: the
 * stores that arm the hook must reach memory before the faulting write.
 */
static ZTEST_BMEM uint8_t *volatile tracked;
static ZTEST_BMEM volatile int resolved;
#ifdef CONFIG_USERSPACE
static ZTEST_BMEM volatile bool tracked_user;
static struct k_mem_partition tracked_part;
#endif

bool z_mem_write_fault(void *addr)
{
	uint8_t *page = tracked;

	if ((page == NULL) || ((uint8_t *)addr < page) || ((uint8_t *)addr >= page + PAGE)) {
		return false;
	}

	resolved++;
	(void)k_mem_update_flags(page, PAGE, K_MEM_PERM_RW);
#ifdef CONFIG_USERSPACE
	if (tracked_user) {
		(void)arch_mem_domain_page_attr_set(&k_mem_domain_default, page, PAGE,
						    K_MEM_PARTITION_P_RW_U_RW);
	}
#endif

	return true;
}

static void *map_protected(void)
{
	uint8_t *page = k_mem_map(PAGE, K_MEM_PERM_RW | K_MEM_MAP_LOCK);

	zassert_not_null(page, "k_mem_map() failed");
	page[0] = 'A';
	zassert_ok(k_mem_update_flags(page, PAGE, 0));

	return page;
}

/* a kernel write to the protected page is resolved by the hook and completes */
ZTEST(mem_write_fault, test_z_mem_write_fault)
{
	/* volatile: the write must reach the page before the hook count is read */
	volatile uint8_t *page = map_protected();

	resolved = 0;
	tracked = (uint8_t *)page;

	page[0] = 'B';
	zassert_equal(page[0], 'B', "the write did not complete");
	zassert_equal(resolved, 1, "the hook ran %d times", resolved);

	/* writable now: no further fault */
	page[1] = 'C';
	zassert_equal(resolved, 1, "the hook ran again");

	tracked = NULL;
	k_mem_unmap((void *)page, PAGE);
}

/* a write the hook does not resolve stays fatal */
ZTEST(mem_write_fault, test_z_mem_write_fault_unresolved)
{
	volatile uint8_t *page = map_protected();

	tracked = NULL;

	ztest_set_fault_valid(true);
	page[0] = 'B';
	ztest_test_fail();
}

#ifdef CONFIG_USERSPACE
static void user_write(void *p1, void *p2, void *p3)
{
	volatile uint8_t *page = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	page[0] = 'U';
	zassert_equal(page[0], 'U', "the user write did not complete");
	zassert_equal(resolved, 1, "the hook ran %d times", resolved);
	page[PAGE - 1] = 'V';
	zassert_equal(resolved, 1, "the hook ran again");
}

static K_THREAD_STACK_DEFINE(user_stack, 2048);
static struct k_thread user_thread;

/* a user write through a read-only partition is resolved for the domain too */
ZTEST(mem_write_fault, test_arch_mem_domain_page_attr_set)
{
	uint8_t *page = map_protected();

	tracked_part = (struct k_mem_partition){
		.start = (uintptr_t)page,
		.size = PAGE,
		.attr = K_MEM_PARTITION_P_RO_U_RO,
	};
	zassert_ok(k_mem_domain_add_partition(&k_mem_domain_default, &tracked_part));

	resolved = 0;
	tracked = page;
	tracked_user = true;

	k_thread_create(&user_thread, user_stack, K_THREAD_STACK_SIZEOF(user_stack), user_write,
			page, NULL, NULL, K_PRIO_PREEMPT(0), K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	zassert_ok(k_thread_join(&user_thread, K_SECONDS(5)));

	tracked_user = false;
	tracked = NULL;
	zassert_ok(k_mem_domain_remove_partition(&k_mem_domain_default, &tracked_part));
	k_mem_unmap(page, PAGE);
}
#endif /* CONFIG_USERSPACE */

ZTEST_SUITE(mem_write_fault, NULL, NULL, NULL, NULL, NULL);
