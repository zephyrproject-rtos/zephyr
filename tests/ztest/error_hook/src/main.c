/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq_offload.h>
#include <zephyr/internal/syscall_handler.h>

#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define THREAD_TEST_PRIORITY 5

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static struct k_thread tdata;

static ZTEST_BMEM int case_type;

/* A semaphore using inside irq_offload */
extern struct k_sem offload_sem;

/* test case type */
enum {
	ZTEST_CATCH_FATAL_ACCESS,
	ZTEST_CATCH_FATAL_ILLEAGAL_INSTRUCTION,
	ZTEST_CATCH_FATAL_DIVIDE_ZERO,
	ZTEST_CATCH_FATAL_K_PANIC,
	ZTEST_CATCH_FATAL_K_OOPS,
	ZTEST_CATCH_FATAL_IN_ISR,
	ZTEST_CATCH_ASSERT_FAIL,
	ZTEST_CATCH_ASSERT_IN_ISR,
	ZTEST_CATCH_USER_FATAL_K_OOPS,
	ZTEST_ERROR_MAX
} error_case_type;


static void trigger_assert_fail(void *a)
{
	/* trigger an assert fail condition */
	__ASSERT(a != NULL, "parameter a should not be NULL!");
}

/*
 * Do not optimize to prevent GCC from generating invalid
 * opcode exception instruction instead of real instruction.
 */
__no_optimization static void trigger_fault_illegal_instruction(void)
{
	void *a = NULL;

	/* execute an illegal instruction */
	((void(*)(void))&a)();
}

/*
 * Do not optimize to prevent GCC from generating invalid
 * opcode exception instruction instead of real instruction.
 */
__no_optimization static void trigger_fault_access(void)
{
#if defined(CONFIG_SOC_ARC_IOT) || defined(CONFIG_SOC_FAMILY_NSIM_ARC_CLASSIC) || \
	defined(CONFIG_SOC_FAMILY_NSIM_ARC_V) || defined(CONFIG_SOC_EMSK)
	/* For iotdk, em_starterkit and ARC/nSIM, nSIM simulates full address space of
	 * memory, iotdk has eflash at 0x0 address, em_starterkit has ICCM at 0x0 address,
	 * access to 0x0 address doesn't generate any exception. So we access to 0XFFFFFFFF
	 * address instead to trigger exception. See issue #31419.
	 */
	void *a = (void *)0xFFFFFFFF;
#elif defined(CONFIG_CPU_CORTEX_M) || defined(CONFIG_CPU_AARCH32_CORTEX_R) || \
	defined(CONFIG_CPU_AARCH64_CORTEX_R)
	/* As this test case only runs when User Mode is enabled,
	 * accessing _current always triggers a memory access fault,
	 * and is guaranteed not to trigger SecureFault exceptions.
	 */
	void *a = (void *)_current;
#else
	/* For most arch which support userspace, dereferencing NULL
	 * pointer will be caught by exception.
	 *
	 * Note: this is not applicable for ARM Cortex-M:
	 * In Cortex-M, nPRIV read access to address 0x0 is generally allowed,
	 * provided that it is "mapped" e.g. when CONFIG_FLASH_BASE_ADDRESS is
	 * 0x0. So, de-referencing NULL pointer is not guaranteed to trigger an
	 * exception.
	 */
	void *a = (void *)NULL;
#endif
	/* access an illegal address */
	volatile int b = *((int *)a);

	printk("b is %d\n", b);
}


/*
 * Do not optimize the divide instruction. GCC will generate invalid
 * opcode exception instruction instead of real divide instruction.
 */
__no_optimization static void trigger_fault_divide_zero(void)
{
	int a = 1;
	int b = 0;

	/* divide by zero */
	a = a / b;
	printk("a is %d\n", a);

/*
 * While no optimization is enabled, some QEMU such as QEMU cortex a53
 * series, QEMU mps2 and mps3 series and QEMU ARC series boards will not
 * trigger an exception for divide zero. They might need to enable the divide
 * zero exception. We only skip the QEMU board here, this means this
 * test will still apply on the physical board.
 * For the Cortex-M0, M0+, M23 (CONFIG_ARMV6_M_ARMV8_M_BASELINE)
 * which does not include a divide instruction, the test is skipped,
 * and there will be no hardware exception for that.
 * For ARMv8-R, divide by zero trapping is not supported in hardware.
 */
#if (defined(CONFIG_SOC_SERIES_MPS2) && defined(CONFIG_QEMU_TARGET)) || \
	(defined(CONFIG_SOC_SERIES_MPS3) && defined(CONFIG_QEMU_TARGET)) || \
	defined(CONFIG_BOARD_QEMU_CORTEX_A53) || defined(CONFIG_SOC_QEMU_ARC) || \
	defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) || \
	defined(CONFIG_BOARD_QEMU_CORTEX_R5) || \
	defined(CONFIG_ARMV8_R) || defined(CONFIG_AARCH32_ARMV8_R) || \
	defined(CONFIG_BOARD_FVP_BASE_REVC_2XAEMV8A) || \
	defined(CONFIG_SOC_NSIM_EM11D)
	ztest_test_skip();
#endif
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
	/* Semaphore used inside irq_offload needs to be
	 * released after an assert or a fault has happened.
	 */
	k_sem_give(&offload_sem);
}

/* This is the fatal error hook that allows you to do actions after
 * the fatal error has occurred. This is optional; you can choose
 * to define the hook yourself. If not, the program will use the
 * default one.
 */
void ztest_post_fatal_error_hook(unsigned int reason,
		const struct arch_esf *pEsf)
{
	switch (case_type) {
	case ZTEST_CATCH_FATAL_ACCESS:
	case ZTEST_CATCH_FATAL_ILLEAGAL_INSTRUCTION:
	case ZTEST_CATCH_FATAL_DIVIDE_ZERO:
	case ZTEST_CATCH_FATAL_K_PANIC:
	case ZTEST_CATCH_FATAL_K_OOPS:
	case ZTEST_CATCH_USER_FATAL_K_OOPS:
		zassert_true(true);
		break;

	/* Unfortunately, the case of trigger a fatal error
	 * inside ISR context still cannot be dealt with,
	 * So please don't use it this way.
	 */
	case ZTEST_CATCH_FATAL_IN_ISR:
		zassert_true(false);
		break;
	default:
		zassert_true(false);
		break;
	}
}

/* This is the assert fail post hook that allows you to do actions after
 * the assert fail happened. This is optional, you can choose to define
 * the hook yourself. If not, the program will use the default one.
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
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int sub_type = *(int *)p1;

	printk("case type is %d\n", case_type);

	ztest_set_fault_valid(false);

	switch (sub_type) {
	case ZTEST_CATCH_FATAL_ACCESS:
		ztest_set_fault_valid(true);
		trigger_fault_access();
		break;
	case ZTEST_CATCH_FATAL_ILLEAGAL_INSTRUCTION:
		ztest_set_fault_valid(true);
		trigger_fault_illegal_instruction();
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

	if (k_is_user_context()) {
		perm = perm | K_USER;
	}

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			tThread_entry,
			(void *)&case_type, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			perm, K_NO_WAIT);

	ret = k_thread_join(tid, K_FOREVER);

	return ret;
}

/**
 * @brief Test if a fatal error can be caught
 *
 * @details Valid a fatal error we triggered in thread context works.
 * If the fatal error happened and the program enter assert_post_handler,
 * that means fatal error triggered as expected.
 */
ZTEST_USER(error_hook_tests, test_catch_fatal_error)
{
#if defined(CONFIG_USERSPACE)
	run_trigger_thread(ZTEST_CATCH_FATAL_ACCESS);
	run_trigger_thread(ZTEST_CATCH_FATAL_ILLEAGAL_INSTRUCTION);
#if !defined(CONFIG_RISCV)
	/*
	 * Because RISC-V Arch doesn't trigger exception for division-by-zero,
	 * this test couldn't support RISC-V.
	 * (RISC-V ISA Manual v2.2, Ch6.2 Division Operation)
	 */
	run_trigger_thread(ZTEST_CATCH_FATAL_DIVIDE_ZERO);
#endif
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
ZTEST_USER(error_hook_tests, test_catch_assert_fail)
{
	case_type = ZTEST_CATCH_ASSERT_FAIL;

	printk("1\n");
	ztest_set_assert_valid(false);

	printk("2\n");
	ztest_set_assert_valid(true);

	printk("3\n");
	trigger_assert_fail(NULL);

	printk("4\n");
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
ZTEST(error_hook_tests, test_catch_assert_in_isr)
{
	case_type = ZTEST_CATCH_ASSERT_IN_ISR;
	irq_offload(tIsr_assert, NULL);
}


#if defined(CONFIG_USERSPACE)
static void trigger_z_oops(void)
{
	/* Set up a dummy syscall frame, pointing to a valid area in memory. */
	_current->syscall_frame = _image_ram_start;

	K_OOPS(true);
}

/**
 * @brief Test if a z_oops can be catched
 *
 * @details Valid a z_oops we triggered in thread context works.
 * If the z_oops happened and the program enter our handler,
 * that means z_oops triggered as expected. This test only for
 * userspace.
 */
ZTEST(error_hook_tests, test_catch_z_oops)
{
	case_type = ZTEST_CATCH_USER_FATAL_K_OOPS;

	ztest_set_fault_valid(true);
	trigger_z_oops();
}
#endif


static void *error_hook_tests_setup(void)
{
#if defined(CONFIG_USERSPACE)
	k_thread_access_grant(k_current_get(), &tdata, &tstack);
#endif
	return NULL;
}
ZTEST_SUITE(error_hook_tests, NULL, error_hook_tests_setup, NULL, NULL, NULL);

static void *fail_assume_in_setup_setup(void)
{
	/* Fail the assume, will skip all the tests */
	zassume_true(false);
	return NULL;
}

ZTEST_SUITE(fail_assume_in_setup, NULL, fail_assume_in_setup_setup, NULL, NULL, NULL);

ZTEST_EXPECT_SKIP(fail_assume_in_setup, test_to_skip0);
ZTEST(fail_assume_in_setup, test_to_skip0)
{
	/* This test should never be run */
	ztest_test_fail();
}

ZTEST_EXPECT_SKIP(fail_assume_in_setup, test_to_skip1);
ZTEST(fail_assume_in_setup, test_to_skip1)
{
	/* This test should never be run */
	ztest_test_fail();
}

static void fail_assume_in_before_before(void *unused)
{
	ARG_UNUSED(unused);
	zassume_true(false);
}

ZTEST_SUITE(fail_assume_in_before, NULL, NULL, fail_assume_in_before_before, NULL, NULL);

ZTEST_EXPECT_SKIP(fail_assume_in_before, test_to_skip0);
ZTEST(fail_assume_in_before, test_to_skip0)
{
	/* This test should never be run */
	ztest_test_fail();
}

ZTEST_EXPECT_SKIP(fail_assume_in_before, test_to_skip1);
ZTEST(fail_assume_in_before, test_to_skip1)
{
	/* This test should never be run */
	ztest_test_fail();
}

ZTEST_SUITE(fail_assume_in_test, NULL, NULL, NULL, NULL, NULL);

ZTEST_EXPECT_SKIP(fail_assume_in_test, test_to_skip);
ZTEST(fail_assume_in_test, test_to_skip)
{
	zassume_true(false);
	ztest_test_fail();
}

void test_main(void)
{
	ztest_run_test_suites(NULL, false, 1, 1);
	/* Can't run ztest_verify_all_test_suites_ran() since some tests are
	 * skipped by design.
	 */
}
