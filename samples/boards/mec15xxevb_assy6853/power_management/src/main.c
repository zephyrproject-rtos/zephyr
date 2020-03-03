/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL LOG_LEVEL_INF
#include <logging/log.h>
LOG_MODULE_DECLARE(mec15_brd_test);
#include "power_mgmt.h"


void main(void)
{
	test_pwr_mgmt_singlethread(false, 10);

	test_pwr_mgmt_singlethread(true, 10);

	test_pwr_mgmt_multithread(false, 10);

	test_pwr_mgmt_multithread(true, 10);
}
