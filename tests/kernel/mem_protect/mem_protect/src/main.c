/*
 * Parts derived from tests/kernel/fatal/src/main.c, which has the
 * following copyright and license:
 *
 * Copyright (c) 2017, 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <ztest.h>
#include <zephyr/kernel_structs.h>
#include <string.h>
#include <stdlib.h>
#include "mem_protect.h"

K_HEAP_DEFINE(test_mem_heap, TEST_HEAP_SIZE);

K_THREAD_STACK_DEFINE(test_stack, KOBJECT_STACK_SIZE);

void test_main(void)
{
	k_thread_priority_set(k_current_get(), -1);

	k_thread_heap_assign(k_current_get(), &test_mem_heap);

	ztest_test_suite(memory_protection_test_suite,
		/* inherit.c */
		ztest_unit_test(test_permission_inheritance),
		ztest_unit_test(test_inherit_resource_pool),

		/* mem_domain.c */
		ztest_unit_test(test_mem_domain_setup),
		ztest_unit_test(test_mem_domain_valid_access),
		ztest_unit_test(test_mem_domain_invalid_access),
		ztest_unit_test(test_mem_domain_no_writes_to_ro),
		ztest_unit_test(test_mem_domain_remove_add_partition),
		ztest_unit_test(test_mem_domain_api_supervisor_only),
		ztest_unit_test(test_mem_domain_boot_threads),
		ztest_unit_test(test_mem_domain_migration),
		ztest_unit_test(test_mem_part_overlap),
		ztest_unit_test(test_mem_domain_init_fail),
		ztest_unit_test(test_mem_domain_remove_part_fail),
		ztest_unit_test(test_mem_part_add_error_null),
		ztest_unit_test(test_mem_part_add_error_zerosize),
		ztest_unit_test(test_mem_part_error_wraparound),
		ztest_unit_test(test_mem_part_remove_error_zerosize),

		/* mem_partition.c */
		ztest_user_unit_test(test_mem_part_assign_bss_vars_zero),
		ztest_unit_test(test_mem_part_auto_determ_size),

		/* kobject.c */
		ztest_unit_test(test_kobject_access_grant),
		ztest_unit_test(test_kobject_access_grant_error),
		ztest_user_unit_test(test_kobject_access_grant_error_user_null),
		ztest_user_unit_test(test_kobject_access_grant_error_user),
		ztest_user_unit_test(test_kobject_access_all_grant_error),
		ztest_unit_test(test_syscall_invalid_kobject),
		ztest_unit_test(test_thread_without_kobject_permission),
		ztest_unit_test(test_kobject_revoke_access),
		ztest_unit_test(test_kobject_grant_access_kobj),
		ztest_unit_test(test_kobject_grant_access_kobj_invalid),
		ztest_unit_test(test_kobject_release_from_user),
		ztest_unit_test(test_kobject_invalid),
		ztest_unit_test(test_kobject_access_all_grant),
		ztest_unit_test(test_thread_has_residual_permissions),
		ztest_unit_test(test_kobject_access_grant_to_invalid_thread),
		ztest_user_unit_test(test_kobject_access_invalid_kobject),
		ztest_user_unit_test(test_access_kobject_without_init_access),
		ztest_unit_test(test_access_kobject_without_init_with_access),
		ztest_unit_test(test_kobject_reinitialize_thread_kobj),
		ztest_unit_test(test_create_new_thread_from_user),
		ztest_unit_test(test_new_user_thread_with_in_use_stack_obj),
		ztest_unit_test(test_create_new_thread_from_user_no_access_stack),
		ztest_unit_test(test_create_new_thread_from_user_invalid_stacksize),
		ztest_unit_test(test_create_new_thread_from_user_huge_stacksize),
		ztest_unit_test(test_create_new_supervisor_thread_from_user),
		ztest_unit_test(test_create_new_essential_thread_from_user),
		ztest_unit_test(test_create_new_higher_prio_thread_from_user),
		ztest_unit_test(test_create_new_invalid_prio_thread_from_user),
		ztest_unit_test(test_mark_thread_exit_uninitialized),
		ztest_unit_test(test_mem_part_assert_add_overmax),
		ztest_user_unit_test(test_kobject_init_error),
		ztest_unit_test(test_alloc_kobjects),
		ztest_unit_test(test_thread_alloc_out_of_idx),
		ztest_unit_test(test_kobj_create_out_of_memory),
		ztest_unit_test(test_kobject_perm_error),
		ztest_unit_test(test_kobject_free_error),
		ztest_unit_test(test_all_kobjects_str)
		);

	ztest_run_test_suite(memory_protection_test_suite);
}
