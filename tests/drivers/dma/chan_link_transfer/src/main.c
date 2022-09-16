/*
 * Copyright (c) 2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <ztest.h>

extern void test_dma_m2m_chan0_1_major_link(void);
extern void test_dma_m2m_chan0_1_minor_link(void);
extern void test_dma_m2m_chan0_1_minor_major_link(void);

#ifdef CONFIG_SHELL
TC_CMD_DEFINE(test_dma_m2m_chan0_1_major_link)
TC_CMD_DEFINE(test_dma_m2m_chan0_1_minor_link)
TC_CMD_DEFINE(test_dma_m2m_chan0_1_minor_major_link)

SHELL_CMD_REGISTER(test_dma_m2m_chan0_1_major_link, NULL, NULL,
		   TC_CMD_ITEM(test_dma_m2m_chan0_1_major_link));
SHELL_CMD_REGISTER(test_dma_m2m_chan0_1_minor_link, NULL, NULL,
		   TC_CMD_ITEM(test_dma_m2m_chan0_1_minor_link));
SHELL_CMD_REGISTER(test_dma_m2m_chan0_1_minor_major_link, NULL, NULL,
		   TC_CMD_ITEM(test_dma_m2m_chan0_1_minor_major_link));

#endif

void test_main(void)
{
#ifndef CONFIG_SHELL
	ztest_test_suite(
		dma_m2m_link_test,
		ztest_unit_test(test_dma_m2m_chan0_1_major_link),
		ztest_unit_test(test_dma_m2m_chan0_1_minor_link),
		ztest_unit_test(test_dma_m2m_chan0_1_minor_major_link));
	ztest_run_test_suite(dma_m2m_link_test);
#endif
}
