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

#define AT_UTIL_STATE_ATTACHED_BIT       0
#define AT_UTIL_STATE_SCRIPT_RUNNING_BIT 1

MODEM_PIPELINK_DT_DECLARE(AT_UTIL_MODEM_NODE, AT_UTIL_PIPELINK_NAME);

static struct modem_pipelink *at_util_pipelink =
	MODEM_PIPELINK_DT_GET(AT_UTIL_MODEM_NODE, AT_UTIL_PIPELINK_NAME);

static struct k_work at_util_open_pipe_work;
static struct k_work at_util_attach_chat_work;
static struct k_work at_util_release_chat_work;
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
		k_work_submit(&at_util_attach_chat_work);
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
		k_work_submit(&at_util_release_chat_work);
		break;

	default:
		break;
	}
}

static void at_util_open_pipe_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	LOG_INF("opening pipe");

	modem_pipe_attach(modem_pipelink_get_pipe(at_util_pipelink), at_util_pipe_callback, NULL);

	modem_pipe_open_async(modem_pipelink_get_pipe(at_util_pipelink));
}

static void at_util_attach_chat_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	modem_chat_attach(at_util_chat, modem_pipelink_get_pipe(at_util_pipelink));
	atomic_set_bit(&at_util_state, AT_UTIL_STATE_ATTACHED_BIT);
	LOG_INF("chat attached");
}

static void at_util_release_chat_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	modem_chat_release(at_util_chat);
	atomic_clear_bit(&at_util_state, AT_UTIL_STATE_ATTACHED_BIT);
	LOG_INF("chat released");
}

void modem_at_user_pipe_init(struct modem_chat *chat)
{
	at_util_chat = chat;
	/* Initialise workers and setup callbacks */
	k_work_init(&at_util_open_pipe_work, at_util_open_pipe_handler);
	k_work_init(&at_util_attach_chat_work, at_util_attach_chat_handler);
	k_work_init(&at_util_release_chat_work, at_util_release_chat_handler);
	modem_pipelink_attach(at_util_pipelink, at_util_pipelink_callback, NULL);
}

int modem_at_user_pipe_claim(void)
{
	if (!atomic_test_bit(&at_util_state, AT_UTIL_STATE_ATTACHED_BIT)) {
		return -EPERM;
	}

	if (atomic_test_and_set_bit(&at_util_state, AT_UTIL_STATE_SCRIPT_RUNNING_BIT)) {
		return -EBUSY;
	}

	return 0;
}

void modem_at_user_pipe_release(void)
{
	atomic_clear_bit(&at_util_state, AT_UTIL_STATE_SCRIPT_RUNNING_BIT);
}
