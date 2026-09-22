/*
 * Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

#define __customramfunc                                                                            \
	__attribute__((noinline)) __attribute__((long_call, section(".customramfunc")))

/**
 * @brief Simple func to verify PXN via DT config since code with
 *        __customramfunc falls in an MPU region with PXN set
 *
 */
__customramfunc bool custom_ram_func(void)
{
	compiler_barrier();
	return true;
}

/**
 * @brief This is a simple solution to execute a code with and without PXN set
 *        because code that falls in `__ramfunc` section has PXN attr set if
 *        built with CONFIG_USERSPACE and unset otherwise.
 */
__ramfunc bool ram_function(void)
{
	compiler_barrier();
	return true;
}

#ifdef CONFIG_USERSPACE
/**
 * @brief Verify that MPU region with PXN attribute set can be executed from
 *        unprivileged mode.
 */
ZTEST_USER(arm_mpu_pxn, test_arm_mpu_pxn_static_config_user)
{
	volatile bool ramfunc_called = false;
	/*
	 * With CONFIG_USERSPACE
	 *  - this func is called in unprivileged mode
	 *  - and __ramfunc falls in an MPU region which has PXN attribute set
	 *
	 * Calling ram_function shouldn't result in an exception.
	 * This confirms that ram_function though part of a region with PXN attribute set,
	 * can be called from an unprivileged code.
	 */
	ramfunc_called = ram_function();
	zassert_true(true == ramfunc_called,
		     "Executing code in __ramfunc failed in unprivileged mode.");
}
#endif

/**
 * @brief Verify that region marked with PXN attribute via DT can be executed
 *        from unprivileged mode but cannot be executed from privileged mode.
 */
ZTEST_USER(arm_mpu_pxn, test_arm_mpu_pxn_dt)
{
	volatile bool ramfunc_called = false;
#ifndef CONFIG_USERSPACE
	/*
	 * It is expected that calling ram_function() should result in an exception
	 * because with CONFIG_USERSPACE, ram_function falls in region with
	 * PXN attribute set.
	 */
	ztest_set_fault_valid(true);
#endif

	ramfunc_called = custom_ram_func();

#ifdef CONFIG_USERSPACE
	zassert_true(1 == ramfunc_called,
		     "Executing code in __customramfunc failed in unprivileged mode.");
#else
	/*
	 * If calling ram_function didn't result in an MPU fault then,
	 * PXN isn't working as expected so, fail the test.
	 */
	ztest_test_fail();
#endif
}

/**
 * @brief This func is always called in privileged mode so, verify that:
 *        - region marked with PXN attribute
 *          cannot be executed from privileged mode
 *        - and same region when marked without PXN attribute
 *          can be executed from privileged mode
 */
ZTEST(arm_mpu_pxn, test_arm_mpu_pxn_static_config)
{
	volatile bool ramfunc_called = false;
#ifdef CONFIG_USERSPACE
	/*
	 * It is expected that calling ram_function() should result in an exception
	 * because with CONFIG_USERSPACE, ram_function falls in region with
	 * PXN attribute set.
	 */
	ztest_set_fault_valid(true);
#endif
	ramfunc_called = ram_function();

#ifdef CONFIG_USERSPACE
	/*
	 * If calling ram_function didn't result in an MPU fault then,
	 * PXN isn't working as expected so, fail the test.
	 */
	ztest_test_fail();
#else
	zassert_true(1 == ramfunc_called, "Executing code in __ramfunc failed in privileged mode.");
#endif
}

ZTEST_SUITE(arm_mpu_pxn, NULL, NULL, NULL, NULL, NULL);
