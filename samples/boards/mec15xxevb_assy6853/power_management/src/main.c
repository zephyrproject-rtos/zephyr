/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL LOG_LEVEL_INF
#include <logging/log.h>
LOG_MODULE_DECLARE(mec15_brd_test);
#include "power_mgmt.h"

#define MAX_CYCLES 5ul

void main(void)
{
	test_pwr_mgmt_singlethread(false, MAX_CYCLES);

	test_pwr_mgmt_singlethread(true, MAX_CYCLES);

	test_pwr_mgmt_multithread(false, MAX_CYCLES);

	test_pwr_mgmt_multithread(true, MAX_CYCLES);
}
