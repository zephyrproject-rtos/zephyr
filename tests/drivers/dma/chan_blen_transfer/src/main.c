/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/zephyr.h>
#include <ztest.h>

extern void test_dma_m2m_chan0_burst8(void);
extern void test_dma_m2m_chan1_burst8(void);
extern void test_dma_m2m_chan0_burst16(void);
extern void test_dma_m2m_chan1_burst16(void);

#ifdef CONFIG_SHELL
TC_CMD_DEFINE(test_dma_m2m_chan0_burst8)
TC_CMD_DEFINE(test_dma_m2m_chan1_burst8)
TC_CMD_DEFINE(test_dma_m2m_chan0_burst16)
TC_CMD_DEFINE(test_dma_m2m_chan1_burst16)

SHELL_CMD_REGISTER(test_dma_m2m_chan0_burst8, NULL, NULL,
			TC_CMD_ITEM(test_dma_m2m_chan0_burst8));
SHELL_CMD_REGISTER(test_dma_m2m_chan1_burst8, NULL, NULL,
			TC_CMD_ITEM(test_dma_m2m_chan1_burst8));
SHELL_CMD_REGISTER(test_dma_m2m_chan0_burst16, NULL, NULL,
			TC_CMD_ITEM(test_dma_m2m_chan0_burst16));
SHELL_CMD_REGISTER(test_dma_m2m_chan1_burst16, NULL, NULL,
			TC_CMD_ITEM(test_dma_m2m_chan1_burst16));
#endif

void test_main(void)
{
#ifndef CONFIG_SHELL
	ztest_test_suite(dma_m2m_test,
			 ztest_unit_test(test_dma_m2m_chan0_burst8),
			 ztest_unit_test(test_dma_m2m_chan1_burst8),
			 ztest_unit_test(test_dma_m2m_chan0_burst16),
			 ztest_unit_test(test_dma_m2m_chan1_burst16));
	ztest_run_test_suite(dma_m2m_test);
#endif
}
