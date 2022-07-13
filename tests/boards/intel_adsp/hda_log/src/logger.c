/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log_backend_adsp_hda.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <ztest.h>
#include <cavs_ipc.h>
#include "tests.h"

#define CHANNEL 6
#define HOST_BUF_SIZE 512

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hda_test, LOG_LEVEL_DBG);

#define IPC_TIMEOUT K_MSEC(1500)

void hda_log_hook(uint32_t written)
{
	/* We *must* send this, but we may be in a timer ISR, so we are
	 * forced into a retry loop without timeouts and such.
	 */
	bool done = false;

	/*  Now send the next one */
	do {
		done = cavs_ipc_send_message(CAVS_HOST_DEV, IPCCMD_HDA_PRINT,
					     (written << 8) | CHANNEL);
	} while (!done);


	/* Previous message may not be done yet, wait for that */
	do {
		done = cavs_ipc_is_complete(CAVS_HOST_DEV);
	} while (!done);


}


void test_hda_logger(void)
{
	const struct log_backend *hda_log_backend = log_backend_get(0);

	zassert_not_null(hda_log_backend, "Expected hda log backend");

	hda_ipc_msg(CAVS_HOST_DEV, IPCCMD_HDA_RESET, CHANNEL, IPC_TIMEOUT);
	hda_ipc_msg(CAVS_HOST_DEV, IPCCMD_HDA_CONFIG,
		    CHANNEL | (HOST_BUF_SIZE << 8), IPC_TIMEOUT);
	adsp_hda_log_init(hda_log_hook, CHANNEL);
	hda_ipc_msg(CAVS_HOST_DEV, IPCCMD_HDA_START, CHANNEL, IPC_TIMEOUT);
	hda_ipc_msg(CAVS_HOST_DEV, IPCCMD_HDA_PRINT,
		    ((HOST_BUF_SIZE*2) << 8) | CHANNEL, IPC_TIMEOUT);

	printk("Testing log backend\n");

	for (int i = 0; i < 512; i++) {
		LOG_DBG("test hda log message %d", i);
	}

	printk("Sleeping to let the log flush\n");

	k_sleep(K_MSEC(500));

	printk("Done\n");
}
