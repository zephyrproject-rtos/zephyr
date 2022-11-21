/** @file
 * @brief Modem command handler for modem context driver
 *
 * Text-based command handler implementation for modem context driver.
 */

/*
 * Copyright (c) 2019-2020 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_cmd_handler, CONFIG_MODEM_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <stddef.h>
#include <zephyr/net/buf.h>

#include "modem_context.h"
#include "modem_cmd_handler.h"


/*
 * Parsing Functions
 */

static bool is_crlf(uint8_t c)
{
	if (c == '\n' || c == '\r') {
		return true;
	} else {
		return false;
	}
}

static void skipcrlf(struct modem_cmd_handler_data *data)
{
	while (data->rx_buf && data->rx_buf->len &&
			is_crlf(*data->rx_buf->data)) {
		net_buf_pull_u8(data->rx_buf);
		if (!data->rx_buf->len) {
			data->rx_buf = net_buf_frag_del(NULL, data->rx_buf);
		}
	}
}

static uint16_t findcrlf(struct modem_cmd_handler_data *data,
		      struct net_buf **frag, uint16_t *offset)
{
	struct net_buf *buf = data->rx_buf;
	uint16_t len = 0U, pos = 0U;

	while (buf && buf->len && !is_crlf(*(buf->data + pos))) {
		if (pos + 1 >= buf->len) {
			len += buf->len;
			buf = buf->frags;
			pos = 0U;
		} else {
			pos++;
		}
	}

	if (buf && buf->len && is_crlf(*(buf->data + pos))) {
		len += pos;
		*offset = pos;
		*frag = buf;
		return len;
	}

	return 0;
}

static bool starts_with(struct net_buf *buf, const char *str)
{
	int pos = 0;

	while (buf && buf->len && *str) {
		if (*(buf->data + pos) == *str) {
			str++;
			pos++;
			if (pos >= buf->len) {
				buf = buf->frags;
				pos = 0;
			}
		} else {
			return false;
		}
	}

	if (*str == 0) {
		return true;
	}

	return false;
}

/*
 * Cmd Handler Functions
 */

static inline struct net_buf *read_rx_allocator(k_timeout_t timeout,
						void *user_data)
{
	return net_buf_alloc((struct net_buf_pool *)user_data, timeout);
}

/* return scanned length for params */
static int parse_params(struct modem_cmd_handler_data *data,  size_t match_len,
			const struct modem_cmd *cmd,
			uint8_t **argv, size_t argv_len, uint16_t *argc)
{
	int count = 0;
	size_t begin, end, i;

	if (!data || !data->match_buf || !match_len || !cmd || !argv || !argc) {
		return -EINVAL;
	}

	begin = cmd->cmd_len;
	end = cmd->cmd_len;
	while (end < match_len) {
		for (i = 0; i < strlen(cmd->delim); i++) {
			if (data->match_buf[end] == cmd->delim[i]) {
				/* mark a parameter beginning */
				argv[*argc] = &data->match_buf[begin];
				/* end parameter with NUL char */
				data->match_buf[end] = '\0';
				/* bump begin */
				begin = end + 1;
				count += 1;
				(*argc)++;
				break;
			}
		}

		if (count >= cmd->arg_count_max) {
			break;
		}

		if (*argc == argv_len) {
			break;
		}

		end++;
	}

	/* consider the ending portion a param if end > begin */
	if (end > begin) {
		/* mark a parameter beginning */
		argv[*argc] = &data->match_buf[begin];
		/* end parameter with NUL char
		 * NOTE: if this is at the end of match_len will probably
		 * be overwriting a NUL that's already there
		 */
		data->match_buf[end] = '\0';
		(*argc)++;
	}

	/* missing arguments */
	if (*argc < cmd->arg_count_min) {
		/* Do not return -EAGAIN here as there is no way new argument
		 * can be parsed later because match_len is computed to be
		 * the minimum of the distance to the first CRLF and the size
		 * of the buffer.
		 * Therefore, waiting more data on the interface won't change
		 * match_len value, which mean there is no point in waiting
		 * for more arguments, this will just end in a infinite loop
		 * parsing data and finding that some arguments are missing.
		 */
		return -EINVAL;
	}

	/*
	 * return the beginning of the next unfinished param so we don't
	 * "skip" any data that could be parsed later.
	 */
	return begin - cmd->cmd_len;
}

/* process a "matched" command */
static int process_cmd(const struct modem_cmd *cmd, size_t match_len,
			struct modem_cmd_handler_data *data)
{
	int parsed_len = 0, ret = 0;
	uint8_t *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
	uint16_t argc = 0U;

	/* reset params */
	memset(argv, 0, sizeof(argv[0]) * ARRAY_SIZE(argv));

	/* do we need to parse arguments? */
	if (cmd->arg_count_max > 0U) {
		/* returns < 0 on error and > 0 for parsed len */
		parsed_len = parse_params(data, match_len, cmd,
					  argv, ARRAY_SIZE(argv), &argc);
		if (parsed_len < 0) {
			return parsed_len;
		}
	}

	/* skip cmd_len + parsed len */
	data->rx_buf = net_buf_skip(data->rx_buf, cmd->cmd_len + parsed_len);

	/* call handler */
	if (cmd->func) {
		ret = cmd->func(data, match_len - cmd->cmd_len - parsed_len,
				argv, argc);
		if (ret == -EAGAIN) {
			/* wait for more data */
			net_buf_push(data->rx_buf, cmd->cmd_len + parsed_len);
		}
	}

	return ret;
}

/*
 * check 3 arrays of commands for a match in match_buf:
 * - response handlers[0]
 * - unsolicited handlers[1]
 * - current assigned handlers[2]
 */
static const struct modem_cmd *find_cmd_match(
		struct modem_cmd_handler_data *data)
{
	int j;
	size_t i;

	for (j = 0; j < ARRAY_SIZE(data->cmds); j++) {
		if (!data->cmds[j] || data->cmds_len[j] == 0U) {
			continue;
		}

		for (i = 0; i < data->cmds_len[j]; i++) {
			/* match on "empty" cmd */
			if (strlen(data->cmds[j][i].cmd) == 0 ||
			    strncmp(data->match_buf, data->cmds[j][i].cmd,
				    data->cmds[j][i].cmd_len) == 0) {
				return &data->cmds[j][i];
			}
		}
	}

	return NULL;
}

static const struct modem_cmd *find_cmd_direct_match(
		struct modem_cmd_handler_data *data)
{
	size_t j, i;

	for (j = 0; j < ARRAY_SIZE(data->cmds); j++) {
		if (!data->cmds[j] || data->cmds_len[j] == 0U) {
			continue;
		}

		for (i = 0; i < data->cmds_len[j]; i++) {
			/* match start of cmd */
			if (data->cmds[j][i].direct &&
			    (data->cmds[j][i].cmd[0] == '\0' ||
			     starts_with(data->rx_buf, data->cmds[j][i].cmd))) {
				return &data->cmds[j][i];
			}
		}
	}

	return NULL;
}

static int cmd_handler_process_iface_data(struct modem_cmd_handler_data *data,
					  struct modem_iface *iface)
{
	struct net_buf *last;
	size_t bytes_read = 0;
	int ret;

	if (!data->rx_buf) {
		data->rx_buf = net_buf_alloc(data->buf_pool,
					     data->alloc_timeout);
		if (!data->rx_buf) {
			/* there is potentially more data waiting */
			return -ENOMEM;
		}
	}

	last = net_buf_frag_last(data->rx_buf);

	/* read all of the data from modem iface */
	while (true) {
		struct net_buf *frag = last;
		size_t frag_room = net_buf_tailroom(frag);

		if (!frag_room) {
			frag = net_buf_alloc(data->buf_pool,
					    data->alloc_timeout);
			if (!frag) {
				/* there is potentially more data waiting */
				return -ENOMEM;
			}

			net_buf_frag_insert(last, frag);
			last = frag;

			frag_room = net_buf_tailroom(frag);
		}

		ret = iface->read(iface, net_buf_tail(frag), frag_room,
				  &bytes_read);
		if (ret < 0 || bytes_read == 0) {
			/* modem context buffer is empty */
			return 0;
		}

		net_buf_add(frag, bytes_read);
	}
}

static void cmd_handler_process_rx_buf(struct modem_cmd_handler_data *data)
{
	const struct modem_cmd *cmd;
	struct net_buf *frag = NULL;
	size_t match_len;
	int ret;
	uint16_t offset, len;

	/* process all of the data in the net_buf */
	while (data->rx_buf && data->rx_buf->len) {
		skipcrlf(data);
		if (!data->rx_buf || !data->rx_buf->len) {
			break;
		}

		cmd = find_cmd_direct_match(data);
		if (cmd && cmd->func) {
			ret = cmd->func(data, cmd->cmd_len, NULL, 0);
			if (ret == -EAGAIN) {
				/* Wait for more data */
				break;
			} else if (ret > 0) {
				LOG_DBG("match direct cmd [%s] (ret:%d)",
					cmd->cmd, ret);
				data->rx_buf = net_buf_skip(data->rx_buf, ret);
			}

			continue;
		}

		frag = NULL;
		/* locate next CR/LF */
		len = findcrlf(data, &frag, &offset);
		if (!frag) {
			/*
			 * No CR/LF found.  Let's exit and leave any data
			 * for next time
			 */
			break;
		}

		/* load match_buf with content up to the next CR/LF */
		/* NOTE: keep room in match_buf for ending NUL char */
		match_len = net_buf_linearize(data->match_buf,
					      data->match_buf_len - 1,
					      data->rx_buf, 0, len);
		if ((data->match_buf_len - 1) < match_len) {
			LOG_ERR("Match buffer size (%zu) is too small for "
				"incoming command size: %zu!  Truncating!",
				data->match_buf_len - 1, match_len);
		}

#if defined(CONFIG_MODEM_CONTEXT_VERBOSE_DEBUG)
		LOG_HEXDUMP_DBG(data->match_buf, match_len, "RECV");
#endif

		k_sem_take(&data->sem_parse_lock, K_FOREVER);

		cmd = find_cmd_match(data);
		if (cmd) {
			LOG_DBG("match cmd [%s] (len:%zu)",
				cmd->cmd, match_len);

			ret = process_cmd(cmd, match_len, data);
			if (ret == -EAGAIN) {
				k_sem_give(&data->sem_parse_lock);
				break;
			} else if (ret < 0) {
				LOG_ERR("process cmd [%s] (len:%zu, ret:%d)",
					cmd->cmd, match_len, ret);
			}

			/*
			 * make sure we didn't run out of data during
			 * command processing
			 */
			if (!data->rx_buf) {
				/* we're out of data, exit early */
				k_sem_give(&data->sem_parse_lock);
				break;
			}

			frag = NULL;
			/*
			 * We've handled the current line.
			 * Let's skip any "extra" data in that
			 * line, and look for the next CR/LF.
			 * This leaves us ready for the next
			 * handler search.
			 * Ignore the length returned.
			 */
			(void)findcrlf(data, &frag, &offset);
		}

		k_sem_give(&data->sem_parse_lock);

		if (frag && data->rx_buf) {
			/* clear out processed line (net_buf's) */
			while (frag && data->rx_buf != frag) {
				data->rx_buf = net_buf_frag_del(NULL,
								data->rx_buf);
			}

			net_buf_pull(data->rx_buf, offset);
		}
	}
}

static void cmd_handler_process(struct modem_cmd_handler *cmd_handler,
				struct modem_iface *iface)
{
	struct modem_cmd_handler_data *data;
	int err;

	if (!cmd_handler || !cmd_handler->cmd_handler_data ||
	    !iface || !iface->read) {
		return;
	}

	data = (struct modem_cmd_handler_data *)(cmd_handler->cmd_handler_data);

	do {
		err = cmd_handler_process_iface_data(data, iface);
		cmd_handler_process_rx_buf(data);
	} while (err);
}

int modem_cmd_handler_get_error(struct modem_cmd_handler_data *data)
{
	if (!data) {
		return -EINVAL;
	}

	return data->last_error;
}

int modem_cmd_handler_set_error(struct modem_cmd_handler_data *data,
				int error_code)
{
	if (!data) {
		return -EINVAL;
	}

	data->last_error = error_code;
	return 0;
}

int modem_cmd_handler_update_cmds(struct modem_cmd_handler_data *data,
				  const struct modem_cmd *handler_cmds,
				  size_t handler_cmds_len,
				  bool reset_error_flag)
{
	if (!data) {
		return -EINVAL;
	}

	data->cmds[CMD_HANDLER] = handler_cmds;
	data->cmds_len[CMD_HANDLER] = handler_cmds_len;
	if (reset_error_flag) {
		data->last_error = 0;
	}

	return 0;
}

int modem_cmd_send_ext(struct modem_iface *iface,
		       struct modem_cmd_handler *handler,
		       const struct modem_cmd *handler_cmds,
		       size_t handler_cmds_len, const uint8_t *buf,
		       struct k_sem *sem, k_timeout_t timeout, int flags)
{
	struct modem_cmd_handler_data *data;
	int ret = 0;

	if (!iface || !handler || !handler->cmd_handler_data || !buf) {
		return -EINVAL;
	}

	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		/* semaphore is not needed if there is no timeout */
		sem = NULL;
	} else if (!sem) {
		/* cannot respect timeout without semaphore */
		return -EINVAL;
	}

	data = (struct modem_cmd_handler_data *)(handler->cmd_handler_data);
	if (!(flags & MODEM_NO_TX_LOCK)) {
		k_sem_take(&data->sem_tx_lock, K_FOREVER);
	}

	if (!(flags & MODEM_NO_SET_CMDS)) {
		ret = modem_cmd_handler_update_cmds(data, handler_cmds,
						    handler_cmds_len, true);
		if (ret < 0) {
			goto unlock_tx_lock;
		}
	}

#if defined(CONFIG_MODEM_CONTEXT_VERBOSE_DEBUG)
	LOG_HEXDUMP_DBG(buf, strlen(buf), "SENT DATA");

	if (data->eol_len > 0) {
		if (data->eol[0] != '\r') {
			/* Print the EOL only if it is not \r, otherwise there
			 * is just too much printing.
			 */
			LOG_HEXDUMP_DBG(data->eol, data->eol_len, "SENT EOL");
		}
	} else {
		LOG_DBG("EOL not set!!!");
	}
#endif
	if (sem) {
		k_sem_reset(sem);
	}

	iface->write(iface, buf, strlen(buf));
	iface->write(iface, data->eol, data->eol_len);

	if (sem) {
		ret = k_sem_take(sem, timeout);

		if (ret == 0) {
			ret = data->last_error;
		} else if (ret == -EAGAIN) {
			ret = -ETIMEDOUT;
		}
	}

	if (!(flags & MODEM_NO_UNSET_CMDS)) {
		/* unset handlers and ignore any errors */
		(void)modem_cmd_handler_update_cmds(data, NULL, 0U, false);
	}

unlock_tx_lock:
	if (!(flags & MODEM_NO_TX_LOCK)) {
		k_sem_give(&data->sem_tx_lock);
	}

	return ret;
}

/* run a set of AT commands */
int modem_cmd_handler_setup_cmds(struct modem_iface *iface,
				 struct modem_cmd_handler *handler,
				 const struct setup_cmd *cmds, size_t cmds_len,
				 struct k_sem *sem, k_timeout_t timeout)
{
	int ret = 0;
	size_t i;

	for (i = 0; i < cmds_len; i++) {

		if (cmds[i].handle_cmd.cmd && cmds[i].handle_cmd.func) {
			ret = modem_cmd_send(iface, handler,
					     &cmds[i].handle_cmd, 1U,
					     cmds[i].send_cmd,
					     sem, timeout);
		} else {
			ret = modem_cmd_send(iface, handler,
					     NULL, 0, cmds[i].send_cmd,
					     sem, timeout);
		}

		k_sleep(K_MSEC(50));

		if (ret < 0) {
			LOG_ERR("command %s ret:%d",
				cmds[i].send_cmd, ret);
			break;
		}
	}

	return ret;
}

/* run a set of AT commands, without lock */
int modem_cmd_handler_setup_cmds_nolock(struct modem_iface *iface,
					struct modem_cmd_handler *handler,
					const struct setup_cmd *cmds,
					size_t cmds_len, struct k_sem *sem,
					k_timeout_t timeout)
{
	int ret = 0;
	size_t i;

	for (i = 0; i < cmds_len; i++) {

		if (cmds[i].handle_cmd.cmd && cmds[i].handle_cmd.func) {
			ret = modem_cmd_send_nolock(iface, handler,
						    &cmds[i].handle_cmd, 1U,
						    cmds[i].send_cmd,
						    sem, timeout);
		} else {
			ret = modem_cmd_send_nolock(iface, handler,
						    NULL, 0, cmds[i].send_cmd,
						    sem, timeout);
		}

		k_sleep(K_MSEC(50));

		if (ret < 0) {
			LOG_ERR("command %s ret:%d",
				cmds[i].send_cmd, ret);
			break;
		}
	}

	return ret;
}

int modem_cmd_handler_tx_lock(struct modem_cmd_handler *handler,
			      k_timeout_t timeout)
{
	struct modem_cmd_handler_data *data;
	data = (struct modem_cmd_handler_data *)(handler->cmd_handler_data);

	return k_sem_take(&data->sem_tx_lock, timeout);
}

void modem_cmd_handler_tx_unlock(struct modem_cmd_handler *handler)
{
	struct modem_cmd_handler_data *data;
	data = (struct modem_cmd_handler_data *)(handler->cmd_handler_data);

	k_sem_give(&data->sem_tx_lock);
}

int modem_cmd_handler_init(struct modem_cmd_handler *handler,
			   struct modem_cmd_handler_data *data,
			   const struct modem_cmd_handler_config *config)
{
	/* Verify arguments */
	if (handler == NULL || data == NULL || config == NULL) {
		return -EINVAL;
	}

	/* Verify config */
	if ((config->match_buf == NULL) ||
	    (config->match_buf_len == 0) ||
	    (config->buf_pool == NULL) ||
	    (NULL != config->response_cmds && 0 == config->response_cmds_len) ||
	    (NULL != config->unsol_cmds && 0 == config->unsol_cmds_len)) {
		return -EINVAL;
	}

	/* Assign data to command handler */
	handler->cmd_handler_data = data;

	/* Assign command process implementation to command handler */
	handler->process = cmd_handler_process;

	/* Store arguments */
	data->match_buf = config->match_buf;
	data->match_buf_len = config->match_buf_len;
	data->buf_pool = config->buf_pool;
	data->alloc_timeout = config->alloc_timeout;
	data->eol = config->eol;
	data->cmds[CMD_RESP] = config->response_cmds;
	data->cmds_len[CMD_RESP] = config->response_cmds_len;
	data->cmds[CMD_UNSOL] = config->unsol_cmds;
	data->cmds_len[CMD_UNSOL] = config->unsol_cmds_len;

	/* Process end of line */
	data->eol_len = data->eol == NULL ? 0 : strlen(data->eol);

	/* Store optional user data */
	data->user_data = config->user_data;

	/* Initialize command handler data members */
	k_sem_init(&data->sem_tx_lock, 1, 1);
	k_sem_init(&data->sem_parse_lock, 1, 1);

	return 0;
}
