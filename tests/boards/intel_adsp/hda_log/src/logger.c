/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log_backend_adsp_hda.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/ztest.h>
#include <cavs_ipc.h>
#include "tests.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hda_test, LOG_LEVEL_DBG);

ZTEST(intel_adsp_hda_log, test_hda_logger)
{
	TC_PRINT("Testing hda log backend\n");

	/* Wait a moment so the output isn't mangled */
	k_msleep(100);

	/* Ensure multiple wraps and many many logs are written */
	for (int i = 0; i < 4096; i++) {
		LOG_DBG("test hda log message %d", i);
	}

	/* Wait another moment so dma may flush */
	k_sleep(K_MSEC(100));
}

ZTEST_SUITE(intel_adsp_hda_log, NULL, NULL, NULL, NULL, NULL);
