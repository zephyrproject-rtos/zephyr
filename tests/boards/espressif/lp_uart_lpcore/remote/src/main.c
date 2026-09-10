/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * LP-core side of the LP UART loopback test.
 *
 * Runs the LP-UART in hardware internal loopback (TX tied to RX inside the
 * peripheral, no pins), echoes a test string through it, then reports the
 * pass/fail verdict to the HP core over the mailbox.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/mbox.h>
#include <string.h>

#define LP_UART_NODE DT_NODELABEL(lp_uart)

#define TEST_MSG       "lp core loopback"
#define LP_RESULT_PASS 0xA5A5A5A5U
#define LP_RESULT_FAIL 0xDEAD0000U

static const struct device *const lp_uart = DEVICE_DT_GET(LP_UART_NODE);

static bool run_loopback(void)
{
	unsigned char c;

	if (!device_is_ready(lp_uart)) {
		return false;
	}

	/* Drop any stale bytes */
	while (uart_poll_in(lp_uart, &c) == 0) {
	}

	for (int i = 0; i < (int)strlen(TEST_MSG); i++) {
		uart_poll_out(lp_uart, TEST_MSG[i]);

		int got = -1;

		for (int retry = 0; retry < 1000; retry++) {
			if (uart_poll_in(lp_uart, &c) == 0) {
				got = c;
				break;
			}
			k_busy_wait(100);
		}
		if (got != TEST_MSG[i]) {
			return false;
		}
	}

	return true;
}

int main(void)
{
	const struct mbox_dt_spec tx_channel = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), tx);
	struct mbox_msg msg = {0};
	uint32_t result;

	result = run_loopback() ? LP_RESULT_PASS : LP_RESULT_FAIL;

	msg.data = &result;
	msg.size = sizeof(result);

	while (1) {
		mbox_send_dt(&tx_channel, &msg);
		k_sleep(K_MSEC(100));
	}

	return 0;
}
