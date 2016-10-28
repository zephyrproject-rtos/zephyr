/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @addtogroup t_driver_dma
 * @{
 * @defgroup t_dma_mem_to_mem test_dma_mem_to_mem
 * @}
 */

#include <zephyr.h>
#include <ztest.h>

extern void test_dma_m2m_chan0_burst8(void);
extern void test_dma_m2m_chan1_burst8(void);
extern void test_dma_m2m_chan0_burst16(void);
extern void test_dma_m2m_chan1_burst16(void);

#ifdef CONFIG_CONSOLE_HANDLER_SHELL
TC_CMD_DEFINE(test_dma_m2m_chan0_burst8)
TC_CMD_DEFINE(test_dma_m2m_chan1_burst8)
TC_CMD_DEFINE(test_dma_m2m_chan0_burst16)
TC_CMD_DEFINE(test_dma_m2m_chan1_burst16)
#endif

void test_main(void)
{
#ifdef CONFIG_CONSOLE_HANDLER_SHELL
	/* initialize shell commands */
	static const struct shell_cmd commands[] = {
		TC_CMD_ITEM(test_dma_m2m_chan0_burst8),
		TC_CMD_ITEM(test_dma_m2m_chan1_burst8),
		TC_CMD_ITEM(test_dma_m2m_chan0_burst16),
		TC_CMD_ITEM(test_dma_m2m_chan1_burst16),
		{ NULL, NULL }
	};
	SHELL_REGISTER("runtest", commands);
#else
	ztest_test_suite(dma_m2m_test,
			 ztest_unit_test(test_dma_m2m_chan0_burst8),
			 ztest_unit_test(test_dma_m2m_chan1_burst8),
			 ztest_unit_test(test_dma_m2m_chan0_burst16),
			 ztest_unit_test(test_dma_m2m_chan1_burst16));
	ztest_run_test_suite(dma_m2m_test);
#endif
}
