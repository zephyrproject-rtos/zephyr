/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_ASYNC_CLIENT_H_
#define ZEPHYR_INCLUDE_SYS_ASYNC_CLIENT_H_

#include <kernel.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ASYNC_CLIENT_USER_FLAG_OFFSET 8
#define ASYNC_CLIENT_NOTIFY_METHOD_MASK 0x3

/** @internal
 *
 * Flag fields used to specify asynchronous operation behavior.
 */
enum async_client_flags {
	/* Known-invalid field, used in validation */
	ASYNC_CLIENT_NOTIFY_INVALID     = 0,

	/*
	 * Indicates that no notification will be provided.
	 *
	 * Callers must check for completions using
	 * qop_mngr_fetch_result().
	 *
	 * See qop_mngr_op_init_spinwait().
	 */
	ASYNC_CLIENT_NOTIFY_SPINWAIT    = 1,

	/*
	 * Select notification through @ref k_poll signal
	 *
	 * See onoff_client_init_signal().
	 */
	ASYNC_CLIENT_NOTIFY_SIGNAL      = 2,

	/**
	 * Select notification through a user-provided callback.
	 *
	 * See onoff_client_init_callback().
	 */
	ASYNC_CLIENT_NOTIFY_CALLBACK    = 3,

	ASYNC_CLIENT_NOTIFIED    = BIT(ASYNC_CLIENT_USER_FLAG_OFFSET - 1),
};

struct async_client;

typedef void (*async_client_callback)(int res, void *user_data);

struct async_client {
	union async {
		/* Pointer to signal used to notify client.
		 *
		 * The signal value corresponds to the res parameter
		 * of qop_op_callback.
		 */
		IF_ENABLED(CONFIG_POLL,(struct k_poll_signal *signal;))

		/* Handler and argument for callback notification. */
		struct callback {
			async_client_callback handler;
			void *user_data;
		} callback;

		/* Used when spin wait */
	} async;

	int result;
	u32_t flags;
};

void async_client_notify(void *srv, void *cli,
			struct async_client *async_cli, int res);

static inline int async_client_fetch_result(const struct async_client *cli,
					    int *result)
{
	__ASSERT_NO_MSG(cli != NULL);
	__ASSERT_NO_MSG(result != NULL);

	int rv = -EAGAIN;

	if (cli->flags & ASYNC_CLIENT_NOTIFIED) {
		rv = 0;
		*result = cli->result;
	}
	return rv;
}

static inline void async_client_init_spinwait(struct async_client *cli)
{
	__ASSERT_NO_MSG(cli != NULL);

	*cli = (struct async_client){
		.flags = ASYNC_CLIENT_NOTIFY_SPINWAIT,
	};
}

static inline void async_client_init_signal(struct async_client *cli,
					    struct k_poll_signal *sigp)
{
	__ASSERT_NO_MSG(cli != NULL);
	__ASSERT_NO_MSG(sigp != NULL);

	*cli = (struct async_client){
		IF_ENABLED(CONFIG_POLL, (
			.async = { .signal = sigp },
			.flags = ASYNC_CLIENT_NOTIFY_SIGNAL,
		))
	};
}

static inline void async_client_init_callback(struct async_client *cli,
					      async_client_callback handler,
					      void *user_data)
{
	__ASSERT_NO_MSG(cli != NULL);
	__ASSERT_NO_MSG(handler != NULL);

	*cli = (struct async_client){
		.async = {
			.callback = {
				.handler = handler,
				.user_data = user_data,
			},
		},
		.flags = ASYNC_CLIENT_NOTIFY_CALLBACK,
	};
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_ASYNC_CLIENT_H_ */
