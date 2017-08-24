/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addgroup t_irq_offload
 * @{
 * @defgroup t_thread_contex test_main
 * @brief TestPurpose: verify thread context
 * @details Check whether offloaded running function is in
 *          interrupt context, on the IRQ stack or not.
 * - API coverage
 *   - irq_offload
 * @}
 */

#include <ztest.h>
extern void test_irq_offload(void);

/**test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_irq_offload_fn,
			ztest_unit_test(test_irq_offload));
	ztest_run_test_suite(test_irq_offload_fn);

}

