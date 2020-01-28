/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/async_client.h>

void async_client_notify(void *srv, void *cli,
			 struct async_client *async_cli, int res)
{
	unsigned int flags = async_cli->flags;

	/* Store the result, and notify if requested. */
	async_cli->flags |= ASYNC_CLIENT_NOTIFIED;
	async_cli->result = res;
	switch (flags & ASYNC_CLIENT_NOTIFY_METHOD_MASK) {
	case ASYNC_CLIENT_NOTIFY_SPINWAIT:
		break;
	case ASYNC_CLIENT_NOTIFY_CALLBACK:
		async_cli->async.callback.handler(res,
					async_cli->async.callback.user_data);
		break;
	case ASYNC_CLIENT_NOTIFY_SIGNAL:
		IF_ENABLED(CONFIG_POLL,
			(k_poll_signal_raise(async_cli->async.signal, res);)
		)
		break;
	default:
		__ASSERT_NO_MSG(false);
	}
}
