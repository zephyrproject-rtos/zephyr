/*
 * Copyright 2025 - 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * AMP mbox test (remote HiFi4 DSP side).
 *
 * On boot the DSP emits an "alive" beacon over the MU/mbox (via the shared
 * testipc request/response layer) so the ARM core can confirm it started. It
 * then services echo requests from the ARM core, closing the loop for the mbox
 * IPC usecase. Each echo also proves to the ARM side that the DSP is still
 * running.
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "testipc.h"

int main(void)
{
	int ret;
	uint32_t msg;

	printk("[DSP] Hello World! %s\n", CONFIG_BOARD_TARGET);

	ret = testipc_init();
	if (ret < 0) {
		printk("[DSP] IPC init fail: %d\n", ret);
		return ret;
	}

	/* Announce that we booted and are running. */
	ret = testipc_send(testipc_msg_make(AMP_OP_ALIVE, 0));
	if (ret < 0) {
		testipc_report_error(ret);
		return ret;
	}

	/* Service requests from the ARM core. */
	while (true) {
		ret = testipc_recv(&msg);
		if (ret < 0) {
			testipc_report_error(ret);
			return ret;
		}

		switch (testipc_msg_get_op(msg)) {
		case AMP_OP_ECHO_REQ:
			ret = testipc_send(testipc_msg_make(AMP_OP_ECHO_RESP,
							    testipc_msg_get_payload(msg)));
			if (ret < 0) {
				testipc_report_error(ret);
				return ret;
			}
			break;
		default:
			/* Ignore unexpected opcodes. */
			break;
		}
	}

	return 0;
}
