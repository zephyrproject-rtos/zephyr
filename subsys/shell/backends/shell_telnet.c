/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/shell/shell_telnet.h>

#include "shell_telnet_protocol.h"

SHELL_TELNET_DEFINE(shell_transport_telnet);
SHELL_DEFINE(shell_telnet, CONFIG_SHELL_PROMPT_TELNET, &shell_transport_telnet,
	     CONFIG_SHELL_TELNET_LOG_MESSAGE_QUEUE_SIZE,
	     CONFIG_SHELL_TELNET_LOG_MESSAGE_QUEUE_TIMEOUT,
	     SHELL_FLAG_OLF_CRLF);

LOG_MODULE_REGISTER(shell_telnet, CONFIG_SHELL_TELNET_LOG_LEVEL);

struct shell_telnet *sh_telnet;

/* Various definitions mapping the TELNET service configuration options */
#define TELNET_PORT      CONFIG_SHELL_TELNET_PORT
#define TELNET_LINE_SIZE CONFIG_SHELL_TELNET_LINE_BUF_SIZE
#define TELNET_TIMEOUT   CONFIG_SHELL_TELNET_SEND_TIMEOUT

#define TELNET_MIN_COMMAND_LEN 2
#define TELNET_WILL_DO_COMMAND_LEN 3

#define SOCK_ID_IPV4_LISTEN 0
#define SOCK_ID_IPV6_LISTEN 1
#define SOCK_ID_CLIENT      2
#define SOCK_ID_MAX         3

/* Basic TELNET implementation. */

static void telnet_server_cb(struct k_work *work);
static int telnet_init(struct shell_telnet *ctx);

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(telnet_server, NULL, telnet_server_cb,
				      SHELL_TELNET_POLLFD_COUNT);


static void telnet_end_client_connection(void)
{
	int ret;

	(void)zsock_close(sh_telnet->fds[SOCK_ID_CLIENT].fd);

	sh_telnet->fds[SOCK_ID_CLIENT].fd = -1;
	sh_telnet->output_lock = false;

	k_work_cancel_delayable_sync(&sh_telnet->send_work,
				     &sh_telnet->work_sync);

	ret = net_socket_service_register(&telnet_server, sh_telnet->fds,
					  ARRAY_SIZE(sh_telnet->fds), NULL);
	if (ret < 0) {
		LOG_ERR("Failed to register socket service, %d", ret);
	}
}

static void telnet_command_send_reply(uint8_t *msg, uint16_t len)
{
	if (sh_telnet->fds[SOCK_ID_CLIENT].fd < 0) {
		return;
	}

	while (len > 0) {
		int ret;

		ret = zsock_send(sh_telnet->fds[SOCK_ID_CLIENT].fd, msg, len, 0);
		if (ret < 0) {
			LOG_ERR("Failed to send command %d, shutting down", ret);
			telnet_end_client_connection();
			break;
		}

		msg += ret;
		len -= ret;
	}
}

static void telnet_reply_ay_command(void)
{
	static const char alive[] = "Zephyr at your service\r\n";

	telnet_command_send_reply((uint8_t *)alive, strlen(alive));
}

static int telnet_echo_set(const struct shell *sh, bool val)
{
	int ret = shell_echo_set(sh_telnet->shell_context, val);

	if (ret < 0) {
		LOG_ERR("Failed to set echo to: %d, err: %d", val, ret);
	}
	return ret;
}

static void telnet_reply_dont_command(struct telnet_simple_command *cmd)
{
	switch (cmd->opt) {
	case NVT_OPT_ECHO:
	{
		int ret = telnet_echo_set(sh_telnet->shell_context, false);

		if (ret >= 0) {
			cmd->op = NVT_CMD_WILL_NOT;
		} else {
			cmd->op = NVT_CMD_WILL;
		}
		break;
	}
	default:
		cmd->op = NVT_CMD_WILL_NOT;
		break;
	}

	telnet_command_send_reply((uint8_t *)cmd,
				  sizeof(struct telnet_simple_command));
}

static void telnet_reply_do_command(struct telnet_simple_command *cmd)
{
	switch (cmd->opt) {
	case NVT_OPT_SUPR_GA:
		cmd->op = NVT_CMD_WILL;
		break;
	case NVT_OPT_ECHO:
	{
		int ret = telnet_echo_set(sh_telnet->shell_context, true);

		if (ret >= 0) {
			cmd->op = NVT_CMD_WILL;
		} else {
			cmd->op = NVT_CMD_WILL_NOT;
		}
		break;
	}
	default:
		cmd->op = NVT_CMD_WILL_NOT;
		break;
	}

	telnet_command_send_reply((uint8_t *)cmd,
				  sizeof(struct telnet_simple_command));
}

static void telnet_reply_command(struct telnet_simple_command *cmd)
{
	if (!cmd->iac) {
		return;
	}

	switch (cmd->op) {
	case NVT_CMD_AO:
		/* OK, no output then */
		sh_telnet->output_lock = true;
		sh_telnet->line_out.len = 0;
		k_work_cancel_delayable_sync(&sh_telnet->send_work,
					     &sh_telnet->work_sync);
		break;
	case NVT_CMD_AYT:
		telnet_reply_ay_command();
		break;
	case NVT_CMD_DO:
		telnet_reply_do_command(cmd);
		break;
	case NVT_CMD_DO_NOT:
		telnet_reply_dont_command(cmd);
		break;
	default:
		LOG_DBG("Operation %u not handled", cmd->op);
		break;
	}
}

static int telnet_send(bool block)
{
	int ret;
	uint8_t *msg = sh_telnet->line_out.buf;
	uint16_t len = sh_telnet->line_out.len;

	if (sh_telnet->line_out.len == 0) {
		return 0;
	}

	if (sh_telnet->fds[SOCK_ID_CLIENT].fd < 0) {
		return -ENOTCONN;
	}

	while (len > 0) {
		ret = zsock_send(sh_telnet->fds[SOCK_ID_CLIENT].fd, msg, len,
				 block ? 0 : ZSOCK_MSG_DONTWAIT);
		if (!block && (ret < 0) && (errno == EAGAIN)) {
			/* Not all data was sent - move the remaining data and
			 * update length.
			 */
			memmove(sh_telnet->line_out.buf, msg, len);
			sh_telnet->line_out.len = len;
			return -EAGAIN;
		}

		if (ret < 0) {
			ret = -errno;
			LOG_ERR("Failed to send %d, shutting down", -ret);
			telnet_end_client_connection();
			return ret;
		}

		msg += ret;
		len -= ret;
	}

	/* We reinitialize the line buffer */
	sh_telnet->line_out.len = 0;

	return 0;
}

static void telnet_send_prematurely(struct k_work *work)
{
	int ret;

	/* Use non-blocking send to prevent system workqueue blocking. */
	ret = telnet_send(false);
	if (ret == -EAGAIN) {
		/* Not all data was sent, reschedule the work. */
		k_work_reschedule(&sh_telnet->send_work, K_MSEC(TELNET_TIMEOUT));
	}
}

static int telnet_command_length(uint8_t op)
{
	if (op == NVT_CMD_SB || op == NVT_CMD_WILL || op == NVT_CMD_WILL_NOT ||
	    op == NVT_CMD_DO || op == NVT_CMD_DO_NOT) {
		return TELNET_WILL_DO_COMMAND_LEN;
	}

	return TELNET_MIN_COMMAND_LEN;
}

static inline int telnet_handle_command(struct telnet_simple_command *cmd)
{
	/* Commands are two or three bytes. */
	if (cmd->iac != NVT_CMD_IAC) {
		return 0;
	}

	if (IS_ENABLED(CONFIG_SHELL_TELNET_SUPPORT_COMMAND)) {
		LOG_DBG("Got a command %u/%u/%u", cmd->iac, cmd->op, cmd->opt);
		telnet_reply_command(cmd);
	}

	if (cmd->op == NVT_CMD_SB) {
		/* TODO Add subnegotiation support. */
		return -EOPNOTSUPP;
	}

	return 0;
}

static void telnet_recv(struct zsock_pollfd *pollfd)
{
	struct telnet_simple_command *cmd =
			(struct telnet_simple_command *)sh_telnet->cmd_buf;
	size_t len, off, buf_left, cmd_total_len;
	uint8_t *buf;
	int ret;

	k_mutex_lock(&sh_telnet->rx_lock, K_FOREVER);

	buf_left = sizeof(sh_telnet->rx_buf) - sh_telnet->rx_len;
	if (buf_left == 0) {
		/* No space left to read TCP stream, try again later. */
		k_mutex_unlock(&sh_telnet->rx_lock);
		k_msleep(10);
		return;
	}

	buf = sh_telnet->rx_buf + sh_telnet->rx_len;

	ret = zsock_recv(pollfd->fd, buf, buf_left, 0);
	if (ret < 0) {
		LOG_DBG("Telnet client error %d", ret);
		goto error;
	} else if (ret == 0) {
		LOG_DBG("Telnet client closed connection");
		goto error;
	}

	off = 0;
	len = ret;
	cmd_total_len = 0;
	/* Filter out and process commands from the data buffer. */
	while (off < len) {
		if (sh_telnet->cmd_len > 0) {
			/* Command mode */
			if (sh_telnet->cmd_len == 1) {
				/* Operation */
				cmd->op = *(buf + off);
				sh_telnet->cmd_len++;
				cmd_total_len++;
				off++;

				if (telnet_command_length(cmd->op) >
							TELNET_MIN_COMMAND_LEN) {
					continue;
				}
			} else if (sh_telnet->cmd_len == 2) {
				/* Option */
				cmd->opt = *(buf + off);
				sh_telnet->cmd_len++;
				cmd_total_len++;
				off++;
			}

			ret = telnet_handle_command(cmd);
			if (ret < 0) {
				goto error;
			} else {
				LOG_DBG("Handled command");
			}

			memset(cmd, 0, sizeof(*cmd));
			sh_telnet->cmd_len = 0;

			continue;
		}

		if (*(buf + off) == NVT_CMD_IAC) {
			cmd->iac = *(buf + off);
			sh_telnet->cmd_len++;
			cmd_total_len++;
			off++;
			continue;
		}

		/* Data byte, remove command bytes from the buffer, if any. */
		if (cmd_total_len > 0) {
			size_t data_off = off;

			off -= cmd_total_len;
			len -= cmd_total_len;
			cmd_total_len = 0;

			memmove(buf + off, buf + data_off, len);
		}

		off++;
	}

	if (cmd_total_len > 0) {
		/* In case the buffer ended with command, trim the buffer size
		 * here.
		 */
		len -=	cmd_total_len;
	}

	if (len == 0) {
		k_mutex_unlock(&sh_telnet->rx_lock);
		return;
	}

	sh_telnet->rx_len += len;

	k_mutex_unlock(&sh_telnet->rx_lock);

	sh_telnet->shell_handler(SHELL_TRANSPORT_EVT_RX_RDY,
				 sh_telnet->shell_context);

	return;

error:
	k_mutex_unlock(&sh_telnet->rx_lock);
	telnet_end_client_connection();
}

static void telnet_restart_server(void)
{
	int ret;

	if (sh_telnet->fds[SOCK_ID_IPV4_LISTEN].fd >= 0) {
		(void)zsock_close(sh_telnet->fds[SOCK_ID_IPV4_LISTEN].fd);
		sh_telnet->fds[SOCK_ID_IPV4_LISTEN].fd = -1;
	}

	if (sh_telnet->fds[SOCK_ID_IPV6_LISTEN].fd >= 0) {
		(void)zsock_close(sh_telnet->fds[SOCK_ID_IPV6_LISTEN].fd);
		sh_telnet->fds[SOCK_ID_IPV6_LISTEN].fd = -1;
	}

	if (sh_telnet->fds[SOCK_ID_CLIENT].fd >= 0) {
		(void)zsock_close(sh_telnet->fds[SOCK_ID_CLIENT].fd);
		sh_telnet->fds[SOCK_ID_CLIENT].fd = -1;
	}

	ret = telnet_init(sh_telnet);
	if (ret < 0) {
		LOG_ERR("Telnet fatal error, failed to restart server (%d)", ret);
		(void)net_socket_service_unregister(&telnet_server);
	}
}

static void telnet_accept(struct zsock_pollfd *pollfd)
{
	int sock, ret = 0;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(struct sockaddr);

	sock = zsock_accept(pollfd->fd, &addr, &addrlen);
	if (sock < 0) {
		ret = -errno;
		NET_ERR("Telnet accept error (%d)", ret);
		return;
	}

	if (sh_telnet->fds[SOCK_ID_CLIENT].fd > 0) {
		/* Too many connections. */
		ret = 0;
		NET_ERR("Telnet client already connected.");
		goto error;
	}

	sh_telnet->fds[SOCK_ID_CLIENT].fd = sock;
	sh_telnet->fds[SOCK_ID_CLIENT].events = ZSOCK_POLLIN;
	sh_telnet->rx_len = 0;
	sh_telnet->cmd_len = 0;
	sh_telnet->line_out.len = 0;

	ret = net_socket_service_register(&telnet_server, sh_telnet->fds,
					  ARRAY_SIZE(sh_telnet->fds), NULL);
	if (ret < 0) {
		LOG_ERR("Failed to register socket service, (%d)", ret);
		sh_telnet->fds[SOCK_ID_CLIENT].fd = -1;
		goto error;
	}

	LOG_DBG("Telnet client connected (family AF_INET%s)",
		addr.sa_family == AF_INET ? "" : "6");

	/* Disable echo - if command handling is enabled we reply that we
	 * support echo.
	 */
	(void)telnet_echo_set(sh_telnet->shell_context, false);

	return;

error:
	if (sock > 0) {
		(void)zsock_close(sock);
	}

	if (ret < 0) {
		telnet_restart_server();
	}
}

static void telnet_server_cb(struct k_work *work)
{
	struct net_socket_service_event *evt =
		CONTAINER_OF(work, struct net_socket_service_event, work);
	int sock_error;
	socklen_t optlen = sizeof(int);

	if (sh_telnet == NULL) {
		return;
	}

	if ((evt->event.revents & ZSOCK_POLLERR) ||
	    (evt->event.revents & ZSOCK_POLLNVAL)) {
		(void)zsock_getsockopt(evt->event.fd, SOL_SOCKET,
				       SO_ERROR, &sock_error, &optlen);
		NET_ERR("Telnet socket %d error (%d)", evt->event.fd, sock_error);

		if (evt->event.fd == sh_telnet->fds[SOCK_ID_CLIENT].fd) {
			return telnet_end_client_connection();
		}

		goto error;
	}

	if (!(evt->event.revents & ZSOCK_POLLIN)) {
		return;
	}

	if (evt->event.fd == sh_telnet->fds[SOCK_ID_IPV4_LISTEN].fd) {
		return telnet_accept(&sh_telnet->fds[SOCK_ID_IPV4_LISTEN]);
	} else if (evt->event.fd == sh_telnet->fds[SOCK_ID_IPV6_LISTEN].fd) {
		return telnet_accept(&sh_telnet->fds[SOCK_ID_IPV6_LISTEN]);
	} else if (evt->event.fd == sh_telnet->fds[SOCK_ID_CLIENT].fd) {
		return telnet_recv(&sh_telnet->fds[SOCK_ID_CLIENT]);
	}

	NET_ERR("Unexpected FD received for telnet, restarting service.");

error:
	telnet_restart_server();
}

static int telnet_setup_server(struct zsock_pollfd *pollfd, sa_family_t family,
			       struct sockaddr *addr, socklen_t addrlen)
{
	int ret = 0;

	pollfd->fd = zsock_socket(family, SOCK_STREAM, IPPROTO_TCP);
	if (pollfd->fd < 0) {
		ret = -errno;
		LOG_ERR("Failed to create telnet AF_INET%s socket",
			family == AF_INET ? "" : "6");
		goto error;
	}

	if (zsock_bind(pollfd->fd, addr, addrlen) < 0) {
		ret = -errno;
		LOG_ERR("Cannot bind on family AF_INET%s (%d)",
			family == AF_INET ? "" : "6", ret);
		goto error;
	}

	if (zsock_listen(pollfd->fd, 1)) {
		ret = -errno;
		LOG_ERR("Cannot listen on family AF_INET%s (%d)",
			family == AF_INET ? "" : "6", ret);
		goto error;
	}

	pollfd->events = ZSOCK_POLLIN;

	LOG_DBG("Telnet console enabled on AF_INET%s",
		family == AF_INET ? "" : "6");

	return 0;

error:
	LOG_ERR("Unable to start telnet on AF_INET%s (%d)",
		family == AF_INET ? "" : "6", ret);

	if (pollfd->fd >= 0) {
		(void)zsock_close(pollfd->fd);
		pollfd->fd = -1;
	}

	return ret;
}

static int telnet_init(struct shell_telnet *ctx)
{
	int ret;

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		struct sockaddr_in any_addr4 = {
			.sin_family = AF_INET,
			.sin_port = htons(TELNET_PORT),
			.sin_addr = INADDR_ANY_INIT
		};

		ret = telnet_setup_server(&ctx->fds[SOCK_ID_IPV4_LISTEN],
					  AF_INET, (struct sockaddr *)&any_addr4,
					  sizeof(any_addr4));
		if (ret < 0) {
			goto error;
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		struct sockaddr_in6 any_addr6 = {
			.sin6_family = AF_INET6,
			.sin6_port = htons(TELNET_PORT),
			.sin6_addr = IN6ADDR_ANY_INIT
		};

		ret = telnet_setup_server(&ctx->fds[SOCK_ID_IPV6_LISTEN],
					  AF_INET6, (struct sockaddr *)&any_addr6,
					  sizeof(any_addr6));
		if (ret < 0) {
			goto error;
		}
	}

	ret = net_socket_service_register(&telnet_server, ctx->fds,
					  ARRAY_SIZE(ctx->fds), NULL);
	if (ret < 0) {
		LOG_ERR("Failed to register socket service, %d", ret);
		goto error;
	}

	LOG_INF("Telnet shell backend initialized");

	return 0;

error:
	if (ctx->fds[SOCK_ID_IPV4_LISTEN].fd >= 0) {
		(void)zsock_close(ctx->fds[SOCK_ID_IPV4_LISTEN].fd);
		ctx->fds[SOCK_ID_IPV4_LISTEN].fd = -1;
	}

	if (ctx->fds[SOCK_ID_IPV6_LISTEN].fd >= 0) {
		(void)zsock_close(ctx->fds[SOCK_ID_IPV6_LISTEN].fd);
		ctx->fds[SOCK_ID_IPV6_LISTEN].fd = -1;
	}

	return ret;
}

/* Shell API */

static int init(const struct shell_transport *transport,
		const void *config,
		shell_transport_handler_t evt_handler,
		void *context)
{
	int err;

	sh_telnet = (struct shell_telnet *)transport->ctx;

	memset(sh_telnet, 0, sizeof(struct shell_telnet));
	for (int i = 0; i < ARRAY_SIZE(sh_telnet->fds); i++) {
		sh_telnet->fds[i].fd = -1;
	}

	sh_telnet->shell_handler = evt_handler;
	sh_telnet->shell_context = context;

	err = telnet_init(sh_telnet);
	if (err != 0) {
		return err;
	}

	k_work_init_delayable(&sh_telnet->send_work, telnet_send_prematurely);
	k_mutex_init(&sh_telnet->rx_lock);

	return 0;
}

static int uninit(const struct shell_transport *transport)
{
	if (sh_telnet == NULL) {
		return -ENODEV;
	}

	return 0;
}

static int enable(const struct shell_transport *transport, bool blocking)
{
	if (sh_telnet == NULL) {
		return -ENODEV;
	}

	return 0;
}

static int telnet_write(const struct shell_transport *transport,
			const void *data, size_t length, size_t *cnt)
{
	struct shell_telnet_line_buf *lb;
	size_t copy_len;
	int err;
	uint32_t timeout;
	bool was_running;

	if (sh_telnet == NULL) {
		*cnt = 0;
		return -ENODEV;
	}

	if (sh_telnet->fds[SOCK_ID_CLIENT].fd < 0 || sh_telnet->output_lock) {
		*cnt = length;
		return 0;
	}

	*cnt = 0;
	lb = &sh_telnet->line_out;

	/* Stop the transmission timer, so it does not interrupt the operation.
	 */
	timeout = k_ticks_to_ms_ceil32(
			k_work_delayable_remaining_get(&sh_telnet->send_work));
	was_running = k_work_cancel_delayable_sync(&sh_telnet->send_work,
						   &sh_telnet->work_sync);

	do {
		if (lb->len + length - *cnt > TELNET_LINE_SIZE) {
			copy_len = TELNET_LINE_SIZE - lb->len;
		} else {
			copy_len = length - *cnt;
		}

		memcpy(lb->buf + lb->len, (uint8_t *)data + *cnt, copy_len);
		lb->len += copy_len;

		/* Send the data immediately if the buffer is full or line feed
		 * is recognized.
		 */
		if (lb->buf[lb->len - 1] == '\n' ||
		    lb->len == TELNET_LINE_SIZE) {
			err = telnet_send(true);
			if (err != 0) {
				*cnt = length;
				return err;
			}
		}

		*cnt += copy_len;
	} while (*cnt < length);

	if (lb->len > 0) {
		/* Check if the timer was already running, initialize otherwise.
		 */
		timeout = was_running ? timeout : TELNET_TIMEOUT;

		k_work_reschedule(&sh_telnet->send_work, K_MSEC(timeout));
	}

	sh_telnet->shell_handler(SHELL_TRANSPORT_EVT_TX_RDY,
				 sh_telnet->shell_context);

	return 0;
}

static int telnet_read(const struct shell_transport *transport,
		       void *data, size_t length, size_t *cnt)
{
	size_t read_len;

	if (sh_telnet == NULL) {
		return -ENODEV;
	}

	if (sh_telnet->fds[SOCK_ID_CLIENT].fd < 0) {
		goto no_data;
	}

	k_mutex_lock(&sh_telnet->rx_lock, K_FOREVER);

	if (sh_telnet->rx_len == 0) {
		k_mutex_unlock(&sh_telnet->rx_lock);
		goto no_data;
	}

	read_len = sh_telnet->rx_len;
	if (read_len > length) {
		read_len = length;
	}

	memcpy(data, sh_telnet->rx_buf, read_len);
	*cnt = read_len;

	sh_telnet->rx_len -= read_len;
	if (sh_telnet->rx_len > 0) {
		memmove(sh_telnet->rx_buf, sh_telnet->rx_buf + read_len,
			sh_telnet->rx_len);
	}

	k_mutex_unlock(&sh_telnet->rx_lock);

	return 0;

no_data:
	*cnt = 0;
	return 0;
}

const struct shell_transport_api shell_telnet_transport_api = {
	.init = init,
	.uninit = uninit,
	.enable = enable,
	.write = telnet_write,
	.read = telnet_read
};

static int enable_shell_telnet(void)
{
	bool log_backend = CONFIG_SHELL_TELNET_INIT_LOG_LEVEL > 0;
	uint32_t level = (CONFIG_SHELL_TELNET_INIT_LOG_LEVEL > LOG_LEVEL_DBG) ?
		      CONFIG_LOG_MAX_LEVEL : CONFIG_SHELL_TELNET_INIT_LOG_LEVEL;
	static const struct shell_backend_config_flags cfg_flags =
					SHELL_DEFAULT_BACKEND_CONFIG_FLAGS;

	return shell_init(&shell_telnet, NULL, cfg_flags, log_backend, level);
}

SYS_INIT(enable_shell_telnet, APPLICATION, CONFIG_SHELL_TELNET_INIT_PRIORITY);

const struct shell *shell_backend_telnet_get_ptr(void)
{
	return &shell_telnet;
}
