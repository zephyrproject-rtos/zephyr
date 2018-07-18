/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell_rtt.h>
#include <rtt/SEGGER_RTT.h>

static void timer_handler(struct k_timer *timer)
{
	struct shell_rtt *sh_rtt =
			CONTAINER_OF(timer, struct shell_rtt, timer);
	int key = SEGGER_RTT_GetKey();


	if (key >= 0) {
		sh_rtt->rx_cnt = 1;
		sh_rtt->rx[0] = (u8_t)key;
		sh_rtt->handler(SHELL_TRANSPORT_EVT_RX_RDY, sh_rtt->context);
	}
}


static int init(const struct shell_transport *transport,
		void const *config,
		shell_transport_handler_t evt_handler,
		void *context)
{
	struct shell_rtt *sh_rtt = (struct shell_rtt *)transport->ctx;

	sh_rtt->handler = evt_handler;
	sh_rtt->context = context;

	k_timer_init(&sh_rtt->timer, timer_handler, NULL);

	k_timer_start(&sh_rtt->timer, 20, 20);

	return 0;
}

static int uninit(const struct shell_transport *transport)
{
	return 0;
}

static int enable(const struct shell_transport *transport, bool blocking)
{
	return 0;
}

static int write(const struct shell_transport *transport,
		 const void *data, size_t length, size_t *cnt)
{
	struct shell_rtt *sh_rtt = (struct shell_rtt *)transport->ctx;

	SEGGER_RTT_WriteNoLock(0, data, length);
	*cnt = length;
	sh_rtt->handler(SHELL_TRANSPORT_EVT_TX_RDY, sh_rtt->context);

	return 0;
}

static int read(const struct shell_transport *transport,
		void *data, size_t length, size_t *cnt)
{
	struct shell_rtt *sh_rtt = (struct shell_rtt *)transport->ctx;

	if (sh_rtt->rx_cnt) {
		memcpy(data, sh_rtt->rx, 1);
		sh_rtt->rx_cnt = 0;
		*cnt = 1;
	} else {
		*cnt = 0;
	}

	return 0;
}

const struct shell_transport_api shell_rtt_transport_api = {
	.init = init,
	.uninit = uninit,
	.enable = enable,
	.write = write,
	.read = read
};
