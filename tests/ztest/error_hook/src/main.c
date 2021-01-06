/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <irq_offload.h>
#include <syscall_handler.h>
#include <ztest_error_hook.h>

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
#define THREAD_TEST_PRIORITY 5

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static struct k_thread tdata;

static ZTEST_BMEM int case_type;

/* A semaphore using inside irq_offload */
extern struct k_sem offload_sem;

/* test case type */
enum {
	ZTEST_CATCH_FATAL_ACCESS_NULL,
	ZTEST_CATCH_FATAL_ILLEAGAL_INSTRUCTION,
	ZTEST_CATCH_FATAL_DIVIDE_ZERO,
	ZTEST_CATCH_FATAL_K_PANIC,
	ZTEST_CATCH_FATAL_K_OOPS,
	ZTEST_CATCH_FATAL_IN_ISR,
	ZTEST_CATCH_ASSERT_FAIL,
	ZTEST_CATCH_ASSERT_IN_ISR,
	ZTEST_CATCH_USER_FATAL_Z_OOPS,
	ZTEST_ERROR_MAX
} error_case_type;


static void trigger_assert_fail(void *a)
{
	/* trigger an assert fail condition */
	__ASSERT(a != NULL, "parameter a should not be NULL!");
}

static void trigger_fault_illeagl_instuction(void)
{
	void *a = NULL;

	/* execute an illeagal instructions */
	((void(*)(void))&a)();
}

static void trigger_fault_access_null(void)
{
	void *a = NULL;

	/* access a null of address */
	int b = *((int *)a);

	printk("b is %d\n", b);
}

static void trigger_fault_divide_zero(void)
{
	int a = 1;
	int b = 0;

	/* divde zero */
	a = a / b;
	printk("a is %d\n", a);
}

static void trigger_fault_oops(void)
{
	k_oops();
}

static void trigger_fault_panic(void)
{
	k_panic();
}

static void release_offload_sem(void)
{
	/* Semaphore used inside irq_offload need to be
	 * released after assert or fault happened.
	 */
	k_sem_give(&offload_sem);
}

/* This is the fatal error hook that allow you to do actions after
 * fatal error happened. This is optional, you can choose to define
 * this yourself. If not, it will use the default one.
 */
void ztest_post_fatal_error_hook(unsigned int reason,
		const z_arch_esf_t *pEsf)
{
	switch (case_type) {
	case ZTEST_CATCH_FATAL_ACCESS_NULL:
	case ZTEST_CATCH_FATAL_ILLEAGAL_INSTRUCTION:
	case ZTEST_CATCH_FATAL_DIVIDE_ZERO:
	case ZTEST_CATCH_FATAL_K_PANIC:
	case ZTEST_CATCH_FATAL_K_OOPS:
	case ZTEST_CATCH_USER_FATAL_Z_OOPS:
		zassert_true(true, NULL);
		break;

	/* Unfortunately, the case of trigger a fatal error
	 * inside ISR context still cannot be dealed with,
	 * So please don't use it this way.
	 */
	case ZTEST_CATCH_FATAL_IN_ISR:
		zassert_true(false, NULL);
		break;
	default:
		zassert_true(false, NULL);
		break;
	}
}

/* This is the assert fail post hook that allow you to do actions after
 * assert fail happened. This is optional, you can choose to define
 * this yourself. If not, it will use the default one.
 */
void ztest_post_assert_fail_hook(void)
{
	switch (case_type) {
	case ZTEST_CATCH_ASSERT_FAIL:
		ztest_test_pass();
		break;
	case ZTEST_CATCH_ASSERT_IN_ISR:
		release_offload_sem();
		ztest_test_pass();
		break;

	default:
		ztest_test_fail();
		break;
	}
}

static void tThread_entry(void *p1, void *p2, void *p3)
{
	int sub_type = *(int *)p1;

	printk("case type is %d\n", case_type);

	ztest_set_fault_valid(false);

	switch (sub_type) {
	case ZTEST_CATCH_FATAL_ACCESS_NULL:
		ztest_set_fault_valid(true);
		trigger_fault_access_null();
		break;
	case ZTEST_CATCH_FATAL_ILLEAGAL_INSTRUCTION:
		ztest_set_fault_valid(true);
		trigger_fault_illeagl_instuction();
		break;
	case ZTEST_CATCH_FATAL_DIVIDE_ZERO:
		ztest_set_fault_valid(true);
		trigger_fault_divide_zero();
		break;
	case ZTEST_CATCH_FATAL_K_PANIC:
		ztest_set_fault_valid(true);
		trigger_fault_panic();
		break;
	case ZTEST_CATCH_FATAL_K_OOPS:
		ztest_set_fault_valid(true);
		trigger_fault_oops();
		break;

	default:
		break;
	}

	/* should not reach here */
	ztest_test_fail();
}

static int run_trigger_thread(int i)
{
	int ret;
	uint32_t perm = K_INHERIT_PERMS;

	case_type = i;

	if (_is_user_context()) {
		perm = perm | K_USER;
	}

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)tThread_entry,
			(void *)&case_type, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			perm, K_NO_WAIT);

	ret = k_thread_join(tid, K_FOREVER);

	return ret;
}

/**
 * @brief Test if a fatal error can be catched
 *
 * @details Valid a fatal error we triggered in thread context works.
 * If the fatal error happened and the program enter assert_post_handler,
 * that means fatal error triggered as expected.
 */
void test_catch_fatal_error(void)
{
#if defined(CONFIG_ARCH_HAS_USERSPACE)
	run_trigger_thread(ZTEST_CATCH_FATAL_ACCESS_NULL);
	run_trigger_thread(ZTEST_CATCH_FATAL_ILLEAGAL_INSTRUCTION);
	run_trigger_thread(ZTEST_CATCH_FATAL_DIVIDE_ZERO);
#endif
	run_trigger_thread(ZTEST_CATCH_FATAL_K_PANIC);
	run_trigger_thread(ZTEST_CATCH_FATAL_K_OOPS);
}

/**
 * @brief Test if catching an assert works
 *
 * @details Valid the assert in thread context works or not. If the assert
 * fail happened and the program enter assert_post_handler, that means
 * assert works as expected.
 */
void test_catch_assert_fail(void)
{
	case_type = ZTEST_CATCH_ASSERT_FAIL;

	ztest_set_assert_valid(false);

	ztest_set_assert_valid(true);
	trigger_assert_fail(NULL);

	ztest_test_fail();
}

/* a handler using by irq_offload  */
static void tIsr_assert(const void *p)
{
	ztest_set_assert_valid(true);
	trigger_assert_fail(NULL);
}

/**
 * @brief Test if an assert fail works in ISR context
 *
 * @details Valid the assert in ISR context works or not. If the assert
 * fail happened and the program enter assert_post_handler, that means
 * assert works as expected.
 */
void test_catch_assert_in_isr(void)
{
	case_type = ZTEST_CATCH_ASSERT_IN_ISR;
	irq_offload(tIsr_assert, NULL);
}


#if defined(CONFIG_USERSPACE)
static void trigger_z_oops(void *a)
{
	Z_OOPS(*((bool *)a));
}

/**
 * @brief Test if a z_oops can be catched
 *
 * @details Valid a z_oops we triggered in thread context works.
 * If the z_oops happened and the program enter our handler,
 * that means z_oops triggered as expected. This test only for
 * userspace.
 */
void test_catch_z_oops(void)
{
	case_type = ZTEST_CATCH_USER_FATAL_Z_OOPS;

	ztest_set_fault_valid(true);
	trigger_z_oops((void *)false);
}
#endif


void test_main(void)
{

#if defined(CONFIG_USERSPACE)
	k_thread_access_grant(k_current_get(), &tdata, &tstack);

	ztest_test_suite(error_hook_tests,
			 ztest_user_unit_test(test_catch_assert_fail),
			 ztest_user_unit_test(test_catch_fatal_error),
			 ztest_unit_test(test_catch_assert_in_isr),
			 ztest_user_unit_test(test_catch_z_oops)
			 );
	ztest_run_test_suite(error_hook_tests);
#else
	ztest_test_suite(error_hook_tests,
			 ztest_unit_test(test_catch_fatal_error),
			 ztest_unit_test(test_catch_assert_fail),
			 ztest_unit_test(test_catch_assert_in_isr)
			 );
	ztest_run_test_suite(error_hook_tests);
#endif
}
