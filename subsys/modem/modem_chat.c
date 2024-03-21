/*
 * Copyright (c) 2022 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_chat, CONFIG_MODEM_MODULES_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <string.h>

#include <zephyr/modem/chat.h>

#define MODEM_CHAT_MATCHES_INDEX_RESPONSE (0)
#define MODEM_CHAT_MATCHES_INDEX_ABORT	  (1)
#define MODEM_CHAT_MATCHES_INDEX_UNSOL	  (2)

#define MODEM_CHAT_SCRIPT_STATE_RUNNING_BIT (0)

#if defined(CONFIG_LOG) && (CONFIG_MODEM_MODULES_LOG_LEVEL == LOG_LEVEL_DBG)

static char log_buffer[CONFIG_MODEM_CHAT_LOG_BUFFER];

static void modem_chat_log_received_command(struct modem_chat *chat)
{
	uint16_t log_buffer_pos = 0;
	uint16_t argv_len;

	for (uint16_t i = 0; i < chat->argc; i++) {
		argv_len = (uint16_t)strlen(chat->argv[i]);

		/* Validate argument fits in log buffer including termination */
		if (sizeof(log_buffer) < (log_buffer_pos + argv_len + 1)) {
			LOG_WRN("log buffer overrun");
			break;
		}

		/* Copy argument and append space */
		memcpy(&log_buffer[log_buffer_pos], chat->argv[i], argv_len);
		log_buffer_pos += argv_len;
		log_buffer[log_buffer_pos] = ' ';
		log_buffer_pos++;
	}

	/* Terminate line after last argument, overwriting trailing space */
	log_buffer_pos = log_buffer_pos == 0 ? log_buffer_pos : log_buffer_pos - 1;
	log_buffer[log_buffer_pos] = '\0';

	LOG_DBG("%s", log_buffer);
}

#else

static void modem_chat_log_received_command(struct modem_chat *chat)
{
}

#endif

static void modem_chat_script_stop(struct modem_chat *chat, enum modem_chat_script_result result)
{
	if ((chat == NULL) || (chat->script == NULL)) {
		return;
	}

	/* Handle result */
	if (result == MODEM_CHAT_SCRIPT_RESULT_SUCCESS) {
		LOG_DBG("%s: complete", chat->script->name);
	} else if (result == MODEM_CHAT_SCRIPT_RESULT_ABORT) {
		LOG_WRN("%s: aborted", chat->script->name);
	} else {
		LOG_WRN("%s: timed out", chat->script->name);
	}

	/* Call back with result */
	if (chat->script->callback != NULL) {
		chat->script->callback(chat, result, chat->user_data);
	}

	/* Clear parse_match in case it is stored in the script being stopped */
	if ((chat->parse_match != NULL) &&
	    ((chat->parse_match_type == MODEM_CHAT_MATCHES_INDEX_ABORT) ||
	     (chat->parse_match_type == MODEM_CHAT_MATCHES_INDEX_RESPONSE))) {
		chat->parse_match = NULL;
		chat->parse_match_len = 0;
	}

	/* Clear reference to script */
	chat->script = NULL;

	/* Clear response and abort commands */
	chat->matches[MODEM_CHAT_MATCHES_INDEX_ABORT] = NULL;
	chat->matches_size[MODEM_CHAT_MATCHES_INDEX_ABORT] = 0;
	chat->matches[MODEM_CHAT_MATCHES_INDEX_RESPONSE] = NULL;
	chat->matches_size[MODEM_CHAT_MATCHES_INDEX_RESPONSE] = 0;

	/* Cancel work */
	k_work_cancel_delayable(&chat->script_timeout_work);
	k_work_cancel(&chat->script_send_work);
	k_work_cancel_delayable(&chat->script_send_timeout_work);

	/* Clear script running state */
	atomic_clear_bit(&chat->script_state, MODEM_CHAT_SCRIPT_STATE_RUNNING_BIT);

	/* Store result of script for script stoppted indication */
	chat->script_result = result;

	/* Indicate script stopped */
	k_sem_give(&chat->script_stopped_sem);
}

static void modem_chat_set_script_send_state(struct modem_chat *chat,
					     enum modem_chat_script_send_state state)
{
	chat->script_send_pos = 0;
	chat->script_send_state = state;
}

static void modem_chat_script_send(struct modem_chat *chat)
{
	modem_chat_set_script_send_state(chat, MODEM_CHAT_SCRIPT_SEND_STATE_REQUEST);
	k_work_submit(&chat->script_send_work);
}

static void modem_chat_script_set_response_matches(struct modem_chat *chat)
{
	const struct modem_chat_script_chat *script_chat =
		&chat->script->script_chats[chat->script_chat_it];

	chat->matches[MODEM_CHAT_MATCHES_INDEX_RESPONSE] = script_chat->response_matches;
	chat->matches_size[MODEM_CHAT_MATCHES_INDEX_RESPONSE] = script_chat->response_matches_size;
}

static void modem_chat_script_clear_response_matches(struct modem_chat *chat)
{
	chat->matches[MODEM_CHAT_MATCHES_INDEX_RESPONSE] = NULL;
	chat->matches_size[MODEM_CHAT_MATCHES_INDEX_RESPONSE] = 0;
}

static void modem_chat_script_next(struct modem_chat *chat, bool initial)
{
	const struct modem_chat_script_chat *script_chat;

	/* Advance iterator if not initial */
	if (initial == true) {
		/* Reset iterator */
		chat->script_chat_it = 0;
	} else {
		/* Advance iterator */
		chat->script_chat_it++;
	}

	/* Check if end of script reached */
	if (chat->script_chat_it == chat->script->script_chats_size) {
		modem_chat_script_stop(chat, MODEM_CHAT_SCRIPT_RESULT_SUCCESS);

		return;
	}

	LOG_DBG("%s: step: %u", chat->script->name, chat->script_chat_it);

	script_chat = &chat->script->script_chats[chat->script_chat_it];

	/* Check if request must be sent */
	if (script_chat->request_size > 0) {
		LOG_DBG("sending: %.*s", script_chat->request_size, script_chat->request);
		modem_chat_script_clear_response_matches(chat);
		modem_chat_script_send(chat);
	} else {
		modem_chat_script_set_response_matches(chat);
	}
}

static void modem_chat_script_start(struct modem_chat *chat, const struct modem_chat_script *script)
{
	/* Save script */
	chat->script = script;

	/* Set abort matches */
	chat->matches[MODEM_CHAT_MATCHES_INDEX_ABORT] = script->abort_matches;
	chat->matches_size[MODEM_CHAT_MATCHES_INDEX_ABORT] = script->abort_matches_size;

	LOG_DBG("running script: %s", chat->script->name);

	/* Set first script command */
	modem_chat_script_next(chat, true);

	/* Start timeout work if script started */
	if (chat->script != NULL) {
		k_work_schedule(&chat->script_timeout_work, K_SECONDS(chat->script->timeout));
	}
}

static void modem_chat_script_run_handler(struct k_work *item)
{
	struct modem_chat *chat = CONTAINER_OF(item, struct modem_chat, script_run_work);

	/* Start script */
	modem_chat_script_start(chat, chat->pending_script);
}

static void modem_chat_script_timeout_handler(struct k_work *item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(item);
	struct modem_chat *chat = CONTAINER_OF(dwork, struct modem_chat, script_timeout_work);

	/* Abort script */
	modem_chat_script_stop(chat, MODEM_CHAT_SCRIPT_RESULT_TIMEOUT);
}

static void modem_chat_script_abort_handler(struct k_work *item)
{
	struct modem_chat *chat = CONTAINER_OF(item, struct modem_chat, script_abort_work);

	/* Validate script is currently running */
	if (chat->script == NULL) {
		return;
	}

	/* Abort script */
	modem_chat_script_stop(chat, MODEM_CHAT_SCRIPT_RESULT_ABORT);
}

static bool modem_chat_script_chat_is_no_response(struct modem_chat *chat)
{
	const struct modem_chat_script_chat *script_chat =
		&chat->script->script_chats[chat->script_chat_it];

	return (script_chat->response_matches_size == 0) ? true : false;
}

static uint16_t modem_chat_script_chat_get_send_timeout(struct modem_chat *chat)
{
	const struct modem_chat_script_chat *script_chat =
		&chat->script->script_chats[chat->script_chat_it];

	return script_chat->timeout;
}

/* Returns true when request part has been sent */
static bool modem_chat_send_script_request_part(struct modem_chat *chat)
{
	const struct modem_chat_script_chat *script_chat =
		&chat->script->script_chats[chat->script_chat_it];

	uint8_t *request_part;
	uint16_t request_size;
	uint16_t request_part_size;
	int ret;

	switch (chat->script_send_state) {
	case MODEM_CHAT_SCRIPT_SEND_STATE_REQUEST:
		request_part = (uint8_t *)(&script_chat->request[chat->script_send_pos]);
		request_size = script_chat->request_size;
		break;

	case MODEM_CHAT_SCRIPT_SEND_STATE_DELIMITER:
		request_part = (uint8_t *)(&chat->delimiter[chat->script_send_pos]);
		request_size = chat->delimiter_size;
		break;

	default:
		return false;
	}

	request_part_size = request_size - chat->script_send_pos;
	ret = modem_pipe_transmit(chat->pipe, request_part, request_part_size);
	if (ret < 1) {
		return false;
	}

	chat->script_send_pos += (uint16_t)ret;

	/* Return true if all data was sent */
	return request_size <= chat->script_send_pos;
}

static void modem_chat_script_send_handler(struct k_work *item)
{
	struct modem_chat *chat = CONTAINER_OF(item, struct modem_chat, script_send_work);
	uint16_t timeout;

	if (chat->script == NULL) {
		return;
	}

	switch (chat->script_send_state) {
	case MODEM_CHAT_SCRIPT_SEND_STATE_IDLE:
		return;

	case MODEM_CHAT_SCRIPT_SEND_STATE_REQUEST:
		if (!modem_chat_send_script_request_part(chat)) {
			return;
		}

		modem_chat_set_script_send_state(chat, MODEM_CHAT_SCRIPT_SEND_STATE_DELIMITER);
		__fallthrough;

	case MODEM_CHAT_SCRIPT_SEND_STATE_DELIMITER:
		if (!modem_chat_send_script_request_part(chat)) {
			return;
		}

		modem_chat_set_script_send_state(chat, MODEM_CHAT_SCRIPT_SEND_STATE_IDLE);
		break;
	}

	if (modem_chat_script_chat_is_no_response(chat)) {
		timeout = modem_chat_script_chat_get_send_timeout(chat);
		if (timeout == 0) {
			modem_chat_script_next(chat, false);
		} else {
			k_work_schedule(&chat->script_send_timeout_work, K_MSEC(timeout));
		}
	} else {
		modem_chat_script_set_response_matches(chat);
	}
}

static void modem_chat_script_send_timeout_handler(struct k_work *item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(item);
	struct modem_chat *chat = CONTAINER_OF(dwork, struct modem_chat, script_send_timeout_work);

	/* Validate script is currently running */
	if (chat->script == NULL) {
		return;
	}

	modem_chat_script_next(chat, false);
}

static void modem_chat_parse_reset(struct modem_chat *chat)
{
	/* Reset parameters used for parsing */
	chat->receive_buf_len = 0;
	chat->delimiter_match_len = 0;
	chat->argc = 0;
	chat->parse_match = NULL;
}

/* Exact match is stored at end of receive buffer */
static void modem_chat_parse_save_match(struct modem_chat *chat)
{
	uint8_t *argv;

	/* Store length of match including NULL to avoid overwriting it if buffer overruns */
	chat->parse_match_len = chat->receive_buf_len + 1;

	/* Copy match to end of receive buffer */
	argv = &chat->receive_buf[chat->receive_buf_size - chat->parse_match_len];

	/* Copy match to end of receive buffer (excluding NULL) */
	memcpy(argv, &chat->receive_buf[0], chat->parse_match_len - 1);

	/* Save match */
	chat->argv[chat->argc] = argv;

	/* Terminate match */
	chat->receive_buf[chat->receive_buf_size - 1] = '\0';

	/* Increment argument count */
	chat->argc++;
}

static bool modem_chat_match_matches_received(struct modem_chat *chat,
					      const struct modem_chat_match *match)
{
	for (uint16_t i = 0; i < match->match_size; i++) {
		if ((match->match[i] == chat->receive_buf[i]) ||
		    (match->wildcards == true && match->match[i] == '?')) {
			continue;
		}

		return false;
	}

	return true;
}

static bool modem_chat_parse_find_match(struct modem_chat *chat)
{
	/* Find in all matches types */
	for (uint16_t i = 0; i < ARRAY_SIZE(chat->matches); i++) {
		/* Find in all matches of matches type */
		for (uint16_t u = 0; u < chat->matches_size[i]; u++) {
			/* Validate match size matches received data length */
			if (chat->matches[i][u].match_size != chat->receive_buf_len) {
				continue;
			}

			/* Validate match */
			if (modem_chat_match_matches_received(chat, &chat->matches[i][u]) ==
			    false) {
				continue;
			}

			/* Complete match found */
			chat->parse_match = &chat->matches[i][u];
			chat->parse_match_type = i;
			return true;
		}
	}

	return false;
}

static bool modem_chat_parse_is_separator(struct modem_chat *chat)
{
	for (uint16_t i = 0; i < chat->parse_match->separators_size; i++) {
		if ((chat->parse_match->separators[i]) ==
		    (chat->receive_buf[chat->receive_buf_len - 1])) {
			return true;
		}
	}

	return false;
}

static bool modem_chat_parse_end_del_start(struct modem_chat *chat)
{
	for (uint8_t i = 0; i < chat->delimiter_size; i++) {
		if (chat->receive_buf[chat->receive_buf_len - 1] == chat->delimiter[i]) {
			return true;
		}
	}

	return false;
}

static bool modem_chat_parse_end_del_complete(struct modem_chat *chat)
{
	/* Validate length of end delimiter */
	if (chat->receive_buf_len < chat->delimiter_size) {
		return false;
	}

	/* Compare end delimiter with receive buffer content */
	return (memcmp(&chat->receive_buf[chat->receive_buf_len - chat->delimiter_size],
		       chat->delimiter, chat->delimiter_size) == 0)
		       ? true
		       : false;
}

static void modem_chat_on_command_received_unsol(struct modem_chat *chat)
{
	/* Callback */
	if (chat->parse_match->callback != NULL) {
		chat->parse_match->callback(chat, (char **)chat->argv, chat->argc, chat->user_data);
	}
}

static void modem_chat_on_command_received_abort(struct modem_chat *chat)
{
	/* Callback */
	if (chat->parse_match->callback != NULL) {
		chat->parse_match->callback(chat, (char **)chat->argv, chat->argc, chat->user_data);
	}

	/* Abort script */
	modem_chat_script_stop(chat, MODEM_CHAT_SCRIPT_RESULT_ABORT);
}

static void modem_chat_on_command_received_resp(struct modem_chat *chat)
{
	/* Callback */
	if (chat->parse_match->callback != NULL) {
		chat->parse_match->callback(chat, (char **)chat->argv, chat->argc, chat->user_data);
	}

	/* Validate response command is not partial */
	if (chat->parse_match->partial) {
		return;
	}

	/* Advance script */
	modem_chat_script_next(chat, false);
}

static bool modem_chat_parse_find_catch_all_match(struct modem_chat *chat)
{
	/* Find in all matches types */
	for (uint16_t i = 0; i < ARRAY_SIZE(chat->matches); i++) {
		/* Find in all matches of matches type */
		for (uint16_t u = 0; u < chat->matches_size[i]; u++) {
			/* Validate match config is matching previous bytes */
			if (chat->matches[i][u].match_size == 0) {
				chat->parse_match = &chat->matches[i][u];
				chat->parse_match_type = i;
				return true;
			}
		}
	}

	return false;
}

static void modem_chat_on_command_received(struct modem_chat *chat)
{
	modem_chat_log_received_command(chat);

	switch (chat->parse_match_type) {
	case MODEM_CHAT_MATCHES_INDEX_UNSOL:
		modem_chat_on_command_received_unsol(chat);
		break;

	case MODEM_CHAT_MATCHES_INDEX_ABORT:
		modem_chat_on_command_received_abort(chat);
		break;

	case MODEM_CHAT_MATCHES_INDEX_RESPONSE:
		modem_chat_on_command_received_resp(chat);
		break;
	}
}

static void modem_chat_on_unknown_command_received(struct modem_chat *chat)
{
	/* Terminate received command */
	chat->receive_buf[chat->receive_buf_len - chat->delimiter_size] = '\0';

	/* Try to find catch all match */
	if (modem_chat_parse_find_catch_all_match(chat) == false) {
		LOG_DBG("%s", chat->receive_buf);
		return;
	}

	/* Parse command */
	chat->argv[0] = "";
	chat->argv[1] = chat->receive_buf;
	chat->argc = 2;

	modem_chat_on_command_received(chat);
}

static void modem_chat_process_byte(struct modem_chat *chat, uint8_t byte)
{
	/* Validate receive buffer not overrun */
	if (chat->receive_buf_size == chat->receive_buf_len) {
		LOG_WRN("receive buffer overrun");
		modem_chat_parse_reset(chat);
		return;
	}

	/* Validate argv buffer not overrun */
	if (chat->argc == chat->argv_size) {
		LOG_WRN("argv buffer overrun");
		modem_chat_parse_reset(chat);
		return;
	}

	/* Copy byte to receive buffer */
	chat->receive_buf[chat->receive_buf_len] = byte;
	chat->receive_buf_len++;

	/* Validate end delimiter not complete */
	if (modem_chat_parse_end_del_complete(chat) == true) {
		/* Filter out empty lines */
		if (chat->receive_buf_len == chat->delimiter_size) {
			/* Reset parser */
			modem_chat_parse_reset(chat);
			return;
		}

		/* Check if match exists */
		if (chat->parse_match == NULL) {
			/* Handle unknown command */
			modem_chat_on_unknown_command_received(chat);

			/* Reset parser */
			modem_chat_parse_reset(chat);
			return;
		}

		/* Check if trailing argument exists */
		if (chat->parse_arg_len > 0) {
			chat->argv[chat->argc] =
				&chat->receive_buf[chat->receive_buf_len - chat->delimiter_size -
						   chat->parse_arg_len];
			chat->receive_buf[chat->receive_buf_len - chat->delimiter_size] = '\0';
			chat->argc++;
		}

		/* Handle received command */
		modem_chat_on_command_received(chat);

		/* Reset parser */
		modem_chat_parse_reset(chat);
		return;
	}

	/* Validate end delimiter not started */
	if (modem_chat_parse_end_del_start(chat) == true) {
		return;
	}

	/* Find matching command if missing */
	if (chat->parse_match == NULL) {
		/* Find matching command */
		if (modem_chat_parse_find_match(chat) == false) {
			return;
		}

		/* Save match */
		modem_chat_parse_save_match(chat);

		/* Prepare argument parser */
		chat->parse_arg_len = 0;
		return;
	}

	/* Check if separator reached */
	if (modem_chat_parse_is_separator(chat) == true) {
		/* Check if argument is empty */
		if (chat->parse_arg_len == 0) {
			/* Save empty argument */
			chat->argv[chat->argc] = "";
		} else {
			/* Save pointer to start of argument */
			chat->argv[chat->argc] =
				&chat->receive_buf[chat->receive_buf_len - chat->parse_arg_len - 1];

			/* Replace separator with string terminator */
			chat->receive_buf[chat->receive_buf_len - 1] = '\0';
		}

		/* Increment argument count */
		chat->argc++;

		/* Reset parse argument length */
		chat->parse_arg_len = 0;
		return;
	}

	/* Increment argument length */
	chat->parse_arg_len++;
}

static bool modem_chat_discard_byte(struct modem_chat *chat, uint8_t byte)
{
	for (uint8_t i = 0; i < chat->filter_size; i++) {
		if (byte == chat->filter[i]) {
			return true;
		}
	}

	return false;
}

/* Process chunk of received bytes */
static void modem_chat_process_bytes(struct modem_chat *chat)
{
	for (uint16_t i = 0; i < chat->work_buf_len; i++) {
		if (modem_chat_discard_byte(chat, chat->work_buf[i])) {
			continue;
		}

		modem_chat_process_byte(chat, chat->work_buf[i]);
	}
}

static void modem_chat_process_handler(struct k_work *item)
{
	struct modem_chat *chat = CONTAINER_OF(item, struct modem_chat, receive_work);
	int ret;

	/* Fill work buffer */
	ret = modem_pipe_receive(chat->pipe, chat->work_buf, sizeof(chat->work_buf));
	if (ret < 1) {
		return;
	}

	/* Save received data length */
	chat->work_buf_len = (size_t)ret;

	/* Process data */
	modem_chat_process_bytes(chat);
	k_work_submit(&chat->receive_work);
}

static void modem_chat_pipe_callback(struct modem_pipe *pipe, enum modem_pipe_event event,
				     void *user_data)
{
	struct modem_chat *chat = (struct modem_chat *)user_data;

	switch (event) {
	case MODEM_PIPE_EVENT_RECEIVE_READY:
		k_work_submit(&chat->receive_work);
		break;

	case MODEM_PIPE_EVENT_TRANSMIT_IDLE:
		k_work_submit(&chat->script_send_work);
		break;

	default:
		break;
	}
}

int modem_chat_init(struct modem_chat *chat, const struct modem_chat_config *config)
{
	__ASSERT_NO_MSG(chat != NULL);
	__ASSERT_NO_MSG(config != NULL);
	__ASSERT_NO_MSG(config->receive_buf != NULL);
	__ASSERT_NO_MSG(config->receive_buf_size > 0);
	__ASSERT_NO_MSG(config->argv != NULL);
	__ASSERT_NO_MSG(config->argv_size > 0);
	__ASSERT_NO_MSG(config->delimiter != NULL);
	__ASSERT_NO_MSG(config->delimiter_size > 0);
	__ASSERT_NO_MSG(!((config->filter == NULL) && (config->filter > 0)));
	__ASSERT_NO_MSG(!((config->unsol_matches == NULL) && (config->unsol_matches_size > 0)));

	memset(chat, 0x00, sizeof(*chat));
	chat->pipe = NULL;
	chat->user_data = config->user_data;
	chat->receive_buf = config->receive_buf;
	chat->receive_buf_size = config->receive_buf_size;
	chat->argv = config->argv;
	chat->argv_size = config->argv_size;
	chat->delimiter = config->delimiter;
	chat->delimiter_size = config->delimiter_size;
	chat->filter = config->filter;
	chat->filter_size = config->filter_size;
	chat->matches[MODEM_CHAT_MATCHES_INDEX_UNSOL] = config->unsol_matches;
	chat->matches_size[MODEM_CHAT_MATCHES_INDEX_UNSOL] = config->unsol_matches_size;
	atomic_set(&chat->script_state, 0);
	k_sem_init(&chat->script_stopped_sem, 0, 1);
	k_work_init(&chat->receive_work, modem_chat_process_handler);
	k_work_init(&chat->script_run_work, modem_chat_script_run_handler);
	k_work_init_delayable(&chat->script_timeout_work, modem_chat_script_timeout_handler);
	k_work_init(&chat->script_abort_work, modem_chat_script_abort_handler);
	k_work_init(&chat->script_send_work, modem_chat_script_send_handler);
	k_work_init_delayable(&chat->script_send_timeout_work,
			      modem_chat_script_send_timeout_handler);

	return 0;
}

int modem_chat_attach(struct modem_chat *chat, struct modem_pipe *pipe)
{
	chat->pipe = pipe;
	modem_chat_parse_reset(chat);
	modem_pipe_attach(chat->pipe, modem_chat_pipe_callback, chat);
	return 0;
}

int modem_chat_run_script_async(struct modem_chat *chat, const struct modem_chat_script *script)
{
	bool script_is_running;

	if (chat->pipe == NULL) {
		return -EPERM;
	}

	/* Validate script */
	if ((script->script_chats == NULL) || (script->script_chats_size == 0) ||
	    ((script->abort_matches != NULL) && (script->abort_matches_size == 0))) {
		return -EINVAL;
	}

	/* Validate script commands */
	for (uint16_t i = 0; i < script->script_chats_size; i++) {
		if ((script->script_chats[i].request_size == 0) &&
		    (script->script_chats[i].response_matches_size == 0)) {
			return -EINVAL;
		}
	}

	script_is_running =
		atomic_test_and_set_bit(&chat->script_state, MODEM_CHAT_SCRIPT_STATE_RUNNING_BIT);

	if (script_is_running == true) {
		return -EBUSY;
	}

	chat->pending_script = script;
	k_work_submit(&chat->script_run_work);
	return 0;
}

int modem_chat_run_script(struct modem_chat *chat, const struct modem_chat_script *script)
{
	int ret;

	k_sem_reset(&chat->script_stopped_sem);

	ret = modem_chat_run_script_async(chat, script);
	if (ret < 0) {
		return ret;
	}

	ret = k_sem_take(&chat->script_stopped_sem, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	return chat->script_result == MODEM_CHAT_SCRIPT_RESULT_SUCCESS ? 0 : -EAGAIN;
}

void modem_chat_script_abort(struct modem_chat *chat)
{
	k_work_submit(&chat->script_abort_work);
}

void modem_chat_release(struct modem_chat *chat)
{
	struct k_work_sync sync;

	if (chat->pipe) {
		modem_pipe_release(chat->pipe);
	}

	k_work_cancel_sync(&chat->script_run_work, &sync);
	k_work_cancel_sync(&chat->script_abort_work, &sync);
	k_work_cancel_sync(&chat->receive_work, &sync);
	k_work_cancel_sync(&chat->script_send_work, &sync);

	chat->pipe = NULL;
	chat->receive_buf_len = 0;
	chat->work_buf_len = 0;
	chat->argc = 0;
	chat->script = NULL;
	chat->script_chat_it = 0;
	atomic_set(&chat->script_state, 0);
	chat->script_result = MODEM_CHAT_SCRIPT_RESULT_ABORT;
	k_sem_reset(&chat->script_stopped_sem);
	chat->script_send_state = MODEM_CHAT_SCRIPT_SEND_STATE_IDLE;
	chat->script_send_pos = 0;
	chat->parse_match = NULL;
	chat->parse_match_len = 0;
	chat->parse_arg_len = 0;
	chat->matches[MODEM_CHAT_MATCHES_INDEX_ABORT] = NULL;
	chat->matches_size[MODEM_CHAT_MATCHES_INDEX_ABORT] = 0;
	chat->matches[MODEM_CHAT_MATCHES_INDEX_RESPONSE] = NULL;
	chat->matches_size[MODEM_CHAT_MATCHES_INDEX_RESPONSE] = 0;
}
