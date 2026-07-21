/*
 * Copyright (c) 2025 Embeint Holdings Pty Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/modem/at/user_pipe.h>
#include <zephyr/modem/pipelink.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/logging/log.h>

#define AT_UTIL_MODEM_NODE    DT_ALIAS(modem)
#define AT_UTIL_PIPELINK_NAME _CONCAT(user_pipe_, CONFIG_MODEM_AT_USER_PIPE_IDX)

#define AT_UTIL_STATE_OPENED_BIT       0
#define AT_UTIL_STATE_PIPE_CLAIMED_BIT 1

MODEM_PIPELINK_DT_DECLARE(AT_UTIL_MODEM_NODE, AT_UTIL_PIPELINK_NAME);

static struct modem_pipelink *at_util_pipelink =
	MODEM_PIPELINK_DT_GET(AT_UTIL_MODEM_NODE, AT_UTIL_PIPELINK_NAME);

static struct k_sem at_util_pipe_access;
static struct k_work at_util_open_pipe_work;
static struct modem_chat *at_util_chat;
static atomic_t at_util_state;

LOG_MODULE_REGISTER(modem_at_user_pipe, CONFIG_MODEM_LOG_LEVEL);

static void at_util_pipe_callback(struct modem_pipe *pipe, enum modem_pipe_event event,
				  void *user_data)
{
	ARG_UNUSED(user_data);

	switch (event) {
	case MODEM_PIPE_EVENT_OPENED:
		LOG_INF("pipe opened");
		atomic_set_bit(&at_util_state, AT_UTIL_STATE_OPENED_BIT);
		break;

	case MODEM_PIPE_EVENT_CLOSED:
		LOG_INF("pipe closed");
		atomic_clear_bit(&at_util_state, AT_UTIL_STATE_OPENED_BIT);
		modem_at_user_pipe_release();
		break;

	default:
		break;
	}
}

void at_util_pipelink_callback(struct modem_pipelink *link, enum modem_pipelink_event event,
			       void *user_data)
{
	ARG_UNUSED(user_data);

	switch (event) {
	case MODEM_PIPELINK_EVENT_CONNECTED:
		LOG_INF("pipe connected");
		k_work_submit(&at_util_open_pipe_work);
		break;

	case MODEM_PIPELINK_EVENT_DISCONNECTED:
		LOG_INF("pipe disconnected");
		/* Clear the opened state ASAP to prevent future claims.
		 * MODEM_PIPE_EVENT_CLOSED handles the actual close.
		 */
		atomic_clear_bit(&at_util_state, AT_UTIL_STATE_OPENED_BIT);
		break;

	default:
		break;
	}
}

static void at_util_open_pipe_handler(struct k_work *work)
{
	struct modem_pipe *pipe = modem_pipelink_get_pipe(at_util_pipelink);
	ARG_UNUSED(work);

	LOG_INF("opening pipe");

	modem_pipe_attach(pipe, at_util_pipe_callback, NULL);

	modem_pipe_open_async(pipe);
}

int modem_at_user_pipe_claim(struct modem_chat *chat, k_timeout_t timeout)
{
	struct modem_pipe *pipe = modem_pipelink_get_pipe(at_util_pipelink);
	uint8_t drain_buffer[16];
	int rc;

	if (!atomic_test_bit(&at_util_state, AT_UTIL_STATE_OPENED_BIT)) {
		/* No expectation the pipe will become available in any reasonable timeframe */
		return -EPERM;
	}

	/* Wait until the pipe is available */
	if (k_sem_take(&at_util_pipe_access, timeout) < 0) {
		return -EBUSY;
	}

	if (!atomic_test_bit(&at_util_state, AT_UTIL_STATE_OPENED_BIT)) {
		/* Pipe is available but underlying channel closed while waiting */
		k_sem_give(&at_util_pipe_access);
		return -EPERM;
	}

	/* Drain any pending bytes that the previous user may not have consumed */
	while (modem_pipe_receive(pipe, drain_buffer, sizeof(drain_buffer)) > 0) {
		;
	}

	__ASSERT_NO_MSG(at_util_chat == NULL);
	at_util_chat = chat;
	rc = modem_chat_attach(at_util_chat, pipe);
	__ASSERT_NO_MSG(rc == 0);
	atomic_set_bit(&at_util_state, AT_UTIL_STATE_PIPE_CLAIMED_BIT);

	LOG_DBG("chat attached");
	return 0;
}

void modem_at_user_pipe_release(void)
{
	if (atomic_test_and_clear_bit(&at_util_state, AT_UTIL_STATE_PIPE_CLAIMED_BIT)) {
		__ASSERT_NO_MSG(at_util_chat != NULL);
		modem_chat_release(at_util_chat);
		at_util_chat = NULL;
		LOG_DBG("chat released");
		k_sem_give(&at_util_pipe_access);
	}
}

int modem_at_user_pipe_init(void)
{
	/* Initialise workers and setup callbacks */
	k_sem_init(&at_util_pipe_access, 1, 1);
	k_work_init(&at_util_open_pipe_work, at_util_open_pipe_handler);
	modem_pipelink_attach(at_util_pipelink, at_util_pipelink_callback, NULL);
	return 0;
}

SYS_INIT(modem_at_user_pipe_init, APPLICATION, 0);
