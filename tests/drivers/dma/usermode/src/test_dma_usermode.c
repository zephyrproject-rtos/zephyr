/*
 * Copyright (c) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Verify usermode APIs for DMA
 */

#include "zephyr/fatal_types.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/ztest.h>

ZTEST_BMEM static volatile bool expect_fault;
ZTEST_BMEM static volatile unsigned int expected_reason;
ZTEST_BMEM static volatile bool faulted;

static void clear_fault(void)
{
	expect_fault = false;
	compiler_barrier();
}

static void set_fault(unsigned int reason)
{
	faulted = false;
	expect_fault = true;
	expected_reason = reason;
	compiler_barrier();
}

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	printk("Caught system error -- reason %d\n", reason);
	faulted = true;

	if (expect_fault) {
		if (expected_reason == reason) {
			printk("System error was expected\n");
			clear_fault();
		} else {
			printk("Wrong fault reason, expecting %d\n",
			       expected_reason);
			TC_END_REPORT(TC_FAIL);
			k_fatal_halt(reason);
		}
	} else {
		printk("Unexpected fault during test\n");
		TC_END_REPORT(TC_FAIL);
		k_fatal_halt(reason);
	}
}

const struct device *dma = DEVICE_DT_GET(DT_NODELABEL(dma));

ZTEST_USER(dma_usermode, test_invalid_chan_filter)
{
	zassert_true(k_is_user_context());

	set_fault(K_ERR_CPU_EXCEPTION);
	dma_chan_filter(dma, 0, NULL);

	TC_END_REPORT(TC_FAIL);
}


ZTEST_USER(dma_usermode, test_invalid_config)
{
	zassert_true(k_is_user_context());

	struct dma_config cfg;

	set_fault(K_ERR_CPU_EXCEPTION);
	dma_config(dma, 0, &cfg);

	TC_END_REPORT(TC_FAIL);
}

ZTEST_USER(dma_usermode, test_invalid_start)
{
	zassert_true(k_is_user_context());

	set_fault(K_ERR_CPU_EXCEPTION);
	dma_start(dma, 0);

	TC_END_REPORT(TC_FAIL);
}

ZTEST_USER(dma_usermode, test_invalid_stop)
{
	zassert_true(k_is_user_context());

	set_fault(K_ERR_CPU_EXCEPTION);
	dma_stop(dma, 0);

	TC_END_REPORT(TC_FAIL);
}

ZTEST_USER(dma_usermode, test_invalid_suspend)
{
	zassert_true(k_is_user_context());

	set_fault(K_ERR_CPU_EXCEPTION);
	dma_suspend(dma, 0);

	TC_END_REPORT(TC_FAIL);
}

ZTEST_USER(dma_usermode, test_invalid_resume)
{
	zassert_true(k_is_user_context());

	set_fault(K_ERR_CPU_EXCEPTION);
	dma_resume(dma, 0);

	TC_END_REPORT(TC_FAIL);
}

ZTEST_USER(dma_usermode, test_invalid_reload)
{
	zassert_true(k_is_user_context());

	set_fault(K_ERR_CPU_EXCEPTION);
	dma_reload(dma, 0, 0, 1, 1);

	TC_END_REPORT(TC_FAIL);
}

ZTEST_USER(dma_usermode, test_invalid_get_status)
{
	struct dma_status status;

	zassert_true(k_is_user_context());

	set_fault(K_ERR_CPU_EXCEPTION);
	dma_get_status(dma, 0, &status);

	TC_END_REPORT(TC_FAIL);
}

void *userspace_setup(void)
{
	return NULL;
}


ZTEST_SUITE(dma_usermode, NULL, userspace_setup, NULL, NULL, NULL);
