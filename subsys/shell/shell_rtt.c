/*
 * Copyright (c) 2018 Makaio GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell_rtt.h>
#include <init.h>
#include <SEGGER_RTT.h>
#include <logging/log.h>

BUILD_ASSERT(!(IS_ENABLED(CONFIG_LOG_BACKEND_RTT) &&
	       COND_CODE_0(CONFIG_LOG_BACKEND_RTT_BUFFER, (1), (0))),
	     "Conflicting log RTT backend enabled on the same channel");

SHELL_RTT_DEFINE(shell_transport_rtt);
SHELL_DEFINE(shell_rtt, CONFIG_SHELL_PROMPT_RTT, &shell_transport_rtt,
	     CONFIG_SHELL_BACKEND_RTT_LOG_MESSAGE_QUEUE_SIZE,
	     CONFIG_SHELL_BACKEND_RTT_LOG_MESSAGE_QUEUE_TIMEOUT,
	     SHELL_FLAG_OLF_CRLF);

LOG_MODULE_REGISTER(shell_rtt, CONFIG_SHELL_RTT_LOG_LEVEL);

static bool rtt_blocking;

static void timer_handler(struct k_timer *timer)
{
	const struct shell_rtt *sh_rtt = k_timer_user_data_get(timer);

	if (SEGGER_RTT_HasData(0)) {
		sh_rtt->handler(SHELL_TRANSPORT_EVT_RX_RDY, sh_rtt->context);
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

	k_timer_init(&sh_rtt->timer, timer_handler, NULL);
	k_timer_user_data_set(&sh_rtt->timer, (void *)sh_rtt);
	k_timer_start(&sh_rtt->timer, K_MSEC(CONFIG_SHELL_RTT_RX_POLL_PERIOD),
		      K_MSEC(CONFIG_SHELL_RTT_RX_POLL_PERIOD));

	return 0;
}

static int uninit(const struct shell_transport *transport)
{
	struct shell_rtt *sh_rtt = (struct shell_rtt *)transport->ctx;

	k_timer_stop(&sh_rtt->timer);

	return 0;
}

static int enable(const struct shell_transport *transport, bool blocking)
{
	struct shell_rtt *sh_rtt = (struct shell_rtt *)transport->ctx;

	if (blocking) {
		rtt_blocking = true;
		k_timer_stop(&sh_rtt->timer);
	}

	return 0;
}

static int write(const struct shell_transport *transport,
		 const void *data, size_t length, size_t *cnt)
{
	struct shell_rtt *sh_rtt = (struct shell_rtt *)transport->ctx;
	const uint8_t *data8 = (const uint8_t *)data;

	if (rtt_blocking) {
		*cnt = SEGGER_RTT_WriteNoLock(0, data8, length);
		while (SEGGER_RTT_HasDataUp(0)) {
			/* empty */
		}
	} else {
		*cnt = SEGGER_RTT_Write(0, data8, length);
	}

	sh_rtt->handler(SHELL_TRANSPORT_EVT_TX_RDY, sh_rtt->context);

	return 0;
}

static int read(const struct shell_transport *transport,
		void *data, size_t length, size_t *cnt)
{
	*cnt = SEGGER_RTT_Read(0, data, length);

	return 0;
}

const struct shell_transport_api shell_rtt_transport_api = {
	.init = init,
	.uninit = uninit,
	.enable = enable,
	.write = write,
	.read = read
};

static int enable_shell_rtt(const struct device *arg)
{
	ARG_UNUSED(arg);
	bool log_backend = CONFIG_SHELL_RTT_INIT_LOG_LEVEL > 0;
	uint32_t level = (CONFIG_SHELL_RTT_INIT_LOG_LEVEL > LOG_LEVEL_DBG) ?
		      CONFIG_LOG_MAX_LEVEL : CONFIG_SHELL_RTT_INIT_LOG_LEVEL;

	shell_init(&shell_rtt, NULL, true, log_backend, level);

	return 0;
}

/* Function is used for testing purposes */
const struct shell *shell_backend_rtt_get_ptr(void)
{
	return &shell_rtt;
}
SYS_INIT(enable_shell_rtt, POST_KERNEL, 0);
