/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log_backend_adsp_hda.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/ztest.h>
#include <intel_adsp_ipc.h>
#include "tests.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hda_test, LOG_LEVEL_DBG);

/* Define a prime length message string (13 bytes long when including the \0 terminator)
 *
 * This helps ensure most if not all messages are not going to land on a 128 byte boundary
 * which is important to test the padding and wrapping feature
 */
#ifdef CONFIG_LOG_PRINTK
#define FMT_STR "TEST:%06d\n"
#else
#define FMT_STR "TEST:%07d"
#endif

#define FMT_STR_LEN 13

/* Now define the number of iterations such that we ensure a large number of wraps
 * on the HDA ring.
 */
#define TEST_ITERATIONS ((CONFIG_LOG_BACKEND_ADSP_HDA_SIZE/FMT_STR_LEN)*200)

ZTEST(intel_adsp_hda_log, test_hda_logger)
{
	TC_PRINT("Testing hda log backend, log buffer size %u, iterations %u\n",
		CONFIG_LOG_BACKEND_ADSP_HDA_SIZE, TEST_ITERATIONS);

	/* Wait a moment so the output isn't mangled with the above printk */
	k_msleep(100);

	for (int i = 0; i < TEST_ITERATIONS; i++) {
		if (IS_ENABLED(CONFIG_LOG_PRINTK)) {
			printk(FMT_STR, i);
		} else {
			LOG_INF(FMT_STR, i);
		}
	}

	/* Wait for the flush to happen */
	k_msleep(CONFIG_LOG_BACKEND_ADSP_HDA_FLUSH_TIME + 10);

	/* Test the flush timer works by writing a short string */
	LOG_INF("Timeout flush working if shown");

	/* Wait again for the flush to happen */
	k_msleep(CONFIG_LOG_BACKEND_ADSP_HDA_FLUSH_TIME + 10);
}

ZTEST_SUITE(intel_adsp_hda_log, NULL, NULL, NULL, NULL, NULL);
