/*
 * Copyright (c) 2018 Makaio GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell_rtt.h>
#include <init.h>
#include <rtt/SEGGER_RTT.h>
#include <logging/sys_log.h>

SHELL_RTT_DEFINE(shell_transport_rtt);
SHELL_DEFINE(shell_rtt, "rtt:~$ ", &shell_transport_rtt, 10,
	     SHELL_FLAG_OLF_CRLF);

static struct k_thread rtt_rx_thread;
static K_THREAD_STACK_DEFINE(rtt_rx_stack, 1024);

static void shell_rtt_rx_process(struct shell_rtt *sh_rtt)
{
	u32_t count;

	while (1) {
		count = SEGGER_RTT_Read(0, sh_rtt->rx, sizeof(sh_rtt->rx));

		if (count > 0) {
			sh_rtt->rx_cnt = count;
			sh_rtt->handler(SHELL_TRANSPORT_EVT_RX_RDY,
					sh_rtt->context);
		}

		k_sleep(K_MSEC(10));
	}
}


static int init(const struct shell_transport *transport,
		const void *config,
		shell_transport_handler_t evt_handler,
		void *context)
{
	struct shell_rtt *sh_rtt = (struct shell_rtt *)transport->ctx;

	sh_rtt->handler = evt_handler;
	sh_rtt->context = context;

	k_thread_create(&rtt_rx_thread, rtt_rx_stack,
			K_THREAD_STACK_SIZEOF(rtt_rx_stack),
			(k_thread_entry_t)shell_rtt_rx_process,
			sh_rtt, NULL, NULL, K_PRIO_COOP(8),
			0, K_NO_WAIT);

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
	const u8_t *data8 = (const u8_t *)data;

	*cnt = SEGGER_RTT_Write(0, data8, length);

	sh_rtt->handler(SHELL_TRANSPORT_EVT_TX_RDY, sh_rtt->context);
	return 0;
}

static int read(const struct shell_transport *transport,
		void *data, size_t length, size_t *cnt)
{
	struct shell_rtt *sh_rtt = (struct shell_rtt *)transport->ctx;

	if (sh_rtt->rx_cnt) {
		memcpy(data, sh_rtt->rx, sh_rtt->rx_cnt);
		*cnt = sh_rtt->rx_cnt;
		sh_rtt->rx_cnt = 0;
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

static int enable_shell_rtt(struct device *arg)
{
	ARG_UNUSED(arg);
	bool log_backend = CONFIG_SHELL_BACKEND_RTT_LOG_LEVEL > 0;
	u32_t level = (CONFIG_SHELL_BACKEND_RTT_LOG_LEVEL > LOG_LEVEL_DBG) ?
		      CONFIG_LOG_MAX_LEVEL : CONFIG_SHELL_BACKEND_RTT_LOG_LEVEL;

	shell_init(&shell_rtt, NULL, true, log_backend, level);

	return 0;
}

/* Function is used for testing purposes */
const struct shell *shell_backend_rtt_get_ptr(void)
{
	return &shell_rtt;
}
SYS_INIT(enable_shell_rtt, POST_KERNEL, 0);
