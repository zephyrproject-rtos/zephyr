/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_driver_dac
 * @{
 * @defgroup t_dac_loopback test_dac_loopback
 * @}
 */

#include <zephyr.h>
#include <ztest.h>

extern void test_dac_loopback(void);

void test_main(void)
{
	ztest_test_suite(dac_loopback_test,
			 ztest_unit_test(test_dac_loopback));
	ztest_run_test_suite(dac_loopback_test);
}
