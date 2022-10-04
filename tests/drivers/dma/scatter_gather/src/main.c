/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/zephyr.h>
#include <ztest.h>

extern void test_dma_m2m_sg(void);

void test_main(void)
{
	ztest_test_suite(dma_m2m_sg_test,
			 ztest_unit_test(test_dma_m2m_sg));
	ztest_run_test_suite(dma_m2m_sg_test);
}
