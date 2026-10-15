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

/* Pause between retries while the stack is out of buffers, growing like the
 * socket layer's own retry for blocking sends.
 */
#define TELNET_BUFS_WAIT_MS     10
#define TELNET_BUFS_WAIT_MAX_MS 100

#define TELNET_MIN_COMMAND_LEN 2
#define TELNET_WILL_DO_COMMAND_LEN 3

#define SOCK_ID_IPV4_LISTEN 0
#define SOCK_ID_IPV6_LISTEN 1
#define SOCK_ID_CLIENT      2
#define SOCK_ID_MAX         3

/* Basic TELNET implementation. */

static void telnet_server_cb(struct net_socket_service_event *evt);
static int telnet_init(struct shell_telnet *ctx);

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(telnet_server, telnet_server_cb,
				      SHELL_TELNET_POLLFD_COUNT);


/* Only the socket service thread closes the client socket, and only under
 * tx_lock: no zsock_send() runs on a closed descriptor, and a send that waits
 * with the lock released re-checks the session afterwards. Every synchronous
 * cancel of send_work also runs under tx_lock, so no two threads ever wait
 * for the same cancel.
 */
static void telnet_close_client(void)
{
	struct k_work_sync sync;

	k_mutex_lock(&sh_telnet->tx_lock, K_FOREVER);

	(void)k_work_cancel_delayable_sync(&sh_telnet->send_work, &sync);

	(void)zsock_close(sh_telnet->fds[SOCK_ID_CLIENT].fd);

	sh_telnet->fds[SOCK_ID_CLIENT].fd = -1;
	sh_telnet->output_lock = false;
	sh_telnet->session++;

	k_mutex_unlock(&sh_telnet->tx_lock);
}

/* A failed send on another thread shuts the socket down for reading instead
 * of closing it; that wakes the service thread with EOF and the close happens
 * here.
 */
static void telnet_end_client_connection(void)
{
	int ret;

	telnet_close_client();

	ret = net_socket_service_register(&telnet_server, sh_telnet->fds,
					  ARRAY_SIZE(sh_telnet->fds), NULL);
	if (ret < 0) {
		LOG_ERR("Failed to register socket service, %d", ret);
	}
}

/* Replies are a few bytes. One that cannot start is dropped rather than
 * block the socket service thread; one that has partly left must be
 * completed, with tx_lock held so that no output lands in the middle of
 * it, within a short bounded wait.
 */
static void telnet_command_send_reply(uint8_t *msg, uint16_t len)
{
	int64_t deadline = k_uptime_get() + TELNET_TIMEOUT;
	bool started = false;
	int ret = 0;
	int fd;

	k_mutex_lock(&sh_telnet->tx_lock, K_FOREVER);

	fd = sh_telnet->fds[SOCK_ID_CLIENT].fd;

	while (fd >= 0 && len > 0) {
		ret = zsock_send(fd, msg, len, ZSOCK_MSG_DONTWAIT);
		if (ret >= 0) {
			started = started || ret > 0;
			msg += ret;
			len -= ret;
			continue;
		}

		ret = -errno;
		if (ret != -EAGAIN && ret != -ENOBUFS) {
			break;
		}

		if (!started) {
			ret = 0;
			break;
		}

		if (k_uptime_get() >= deadline) {
			ret = -ETIMEDOUT;
			break;
		}

		k_sleep(K_MSEC(TELNET_BUFS_WAIT_MS));
	}

	k_mutex_unlock(&sh_telnet->tx_lock);

	if (ret < 0) {
		LOG_ERR("Failed to send command %d, shutting down", ret);
		telnet_end_client_connection();
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
	struct k_work_sync sync;

	if (!cmd->iac) {
		return;
	}

	switch (cmd->op) {
	case NVT_CMD_AO:
		/* OK, no output then */
		k_mutex_lock(&sh_telnet->tx_lock, K_FOREVER);
		sh_telnet->output_lock = true;
		sh_telnet->line_out.len = 0;
		(void)k_work_cancel_delayable_sync(&sh_telnet->send_work, &sync);
		k_mutex_unlock(&sh_telnet->tx_lock);
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

/* Waits with tx_lock released until a send may succeed again, so a client
 * that stops reading never holds up a close: for a full window until the
 * socket is writable, for a buffer shortage a growing pause.
 */
static int telnet_send_wait(int fd, int err, int64_t deadline, uint32_t *pause)
{
	int64_t remaining = deadline - k_uptime_get();
	int ret = 0;

	if (remaining <= 0) {
		return -ETIMEDOUT;
	}

	k_mutex_unlock(&sh_telnet->tx_lock);

	if (err == -EAGAIN) {
		struct zsock_pollfd pfd = {
			.fd = fd,
			.events = ZSOCK_POLLOUT,
		};

		ret = zsock_poll(&pfd, 1, (int)remaining);
		if (ret < 0) {
			ret = -errno;
		} else if (ret == 0) {
			ret = -ETIMEDOUT;
		} else {
			ret = 0;
		}
	} else {
		k_sleep(K_MSEC(MIN(*pause, (uint32_t)remaining)));
		*pause = MIN(*pause * 2, TELNET_BUFS_WAIT_MAX_MS);
	}

	k_mutex_lock(&sh_telnet->tx_lock, K_FOREVER);

	return ret;
}

/* Called with tx_lock held. Returns -ECANCELED when the pending output was
 * dropped on purpose: the session changed or was told to stop while the
 * lock was released for the wait.
 */
static int telnet_send(bool block)
{
	int64_t deadline = k_uptime_get() + CONFIG_NET_SOCKET_MAX_SEND_WAIT;
	uint32_t pause = TELNET_BUFS_WAIT_MS;
	uint8_t *msg = sh_telnet->line_out.buf;
	uint16_t len = sh_telnet->line_out.len;
	int fd = sh_telnet->fds[SOCK_ID_CLIENT].fd;
	uint32_t session = sh_telnet->session;
	int ret;

	if (len == 0) {
		return 0;
	}

	if (fd < 0) {
		return -ENOTCONN;
	}

	if (sh_telnet->output_lock) {
		return -ECANCELED;
	}

	while (len > 0) {
		ret = zsock_send(fd, msg, len, ZSOCK_MSG_DONTWAIT);
		if (ret >= 0) {
			msg += ret;
			len -= ret;
			continue;
		}

		ret = -errno;
		if (ret == -EAGAIN || ret == -ENOBUFS) {
			if (!block) {
				/* Keep the rest for the next attempt. */
				memmove(sh_telnet->line_out.buf, msg, len);
				sh_telnet->line_out.len = len;
				return -EAGAIN;
			}

			ret = telnet_send_wait(fd, ret, deadline, &pause);
			if (sh_telnet->session != session || sh_telnet->output_lock) {
				return -ECANCELED;
			}

			if (ret == 0) {
				continue;
			}
		}

		/* Stop the output before anything else: the caller logs the
		 * error once tx_lock is released, and a log backend on this
		 * shell must find the output already stopped.
		 */
		sh_telnet->output_lock = true;
		(void)zsock_shutdown(fd, ZSOCK_SHUT_RD);
		return ret;
	}

	/* We reinitialize the line buffer */
	sh_telnet->line_out.len = 0;

	return 0;
}

static void telnet_send_prematurely(struct k_work *work)
{
	int ret = -EAGAIN;

	/* While the shell thread writes it owns the line buffer and reschedules
	 * this work itself if output remains. Use non-blocking send to prevent
	 * system workqueue blocking.
	 */
	if (k_mutex_lock(&sh_telnet->tx_lock, K_NO_WAIT) == 0) {
		ret = sh_telnet->tx_active ? 0 : telnet_send(false);
		k_mutex_unlock(&sh_telnet->tx_lock);
	}

	if (ret == -EAGAIN) {
		/* Not all data was sent, reschedule the work. */
		k_work_reschedule(&sh_telnet->send_work, K_MSEC(TELNET_TIMEOUT));
	} else if (ret < 0 && ret != -ENOTCONN && ret != -ECANCELED) {
		LOG_ERR("Failed to send %d, shutting down", -ret);
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
		telnet_close_client();
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
	struct net_sockaddr_storage addr = {};
	net_socklen_t addrlen = sizeof(addr);

	sock = zsock_accept(pollfd->fd, net_sad(&addr), &addrlen);
	if (sock < 0) {
		ret = -errno;
		LOG_ERR("Telnet accept error (%d)", ret);
		return;
	}

	if (sh_telnet->fds[SOCK_ID_CLIENT].fd > 0) {
		/* Too many connections. */
		ret = 0;
		LOG_ERR("Telnet client already connected.");
		goto error;
	}

	k_mutex_lock(&sh_telnet->tx_lock, K_FOREVER);

	sh_telnet->fds[SOCK_ID_CLIENT].fd = sock;
	sh_telnet->fds[SOCK_ID_CLIENT].events = ZSOCK_POLLIN;
	sh_telnet->rx_len = 0;
	sh_telnet->cmd_len = 0;
	sh_telnet->line_out.len = 0;
	sh_telnet->output_lock = false;
	sh_telnet->session++;

	k_mutex_unlock(&sh_telnet->tx_lock);

	ret = net_socket_service_register(&telnet_server, sh_telnet->fds,
					  ARRAY_SIZE(sh_telnet->fds), NULL);
	if (ret < 0) {
		LOG_ERR("Failed to register socket service, (%d)", ret);
		telnet_close_client();
		sock = -1;
		goto error;
	}

	LOG_DBG("Telnet client connected (family NET_AF_INET%s)",
		addr.ss_family == NET_AF_INET ? "" : "6");

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

static void telnet_server_cb(struct net_socket_service_event *evt)
{
	int sock_error = 0;
	int ret;
	net_socklen_t optlen = sizeof(int);

	if (sh_telnet == NULL) {
		return;
	}

	if ((evt->event.revents & ZSOCK_POLLERR) ||
	    (evt->event.revents & ZSOCK_POLLNVAL)) {
		ret = zsock_getsockopt(evt->event.fd, ZSOCK_SOL_SOCKET,
				       ZSOCK_SO_ERROR, &sock_error, &optlen);
		if (ret < 0) {
			sock_error = errno;
		}

		ret = -sock_error;

		if (ret == -ENETDOWN) {
			LOG_INF("Network is down");
		} else {
			LOG_ERR("Telnet socket %d error (%d)", evt->event.fd, ret);
		}

		if (evt->event.fd == sh_telnet->fds[SOCK_ID_CLIENT].fd) {
			telnet_end_client_connection();
			return;
		}

		goto error;
	}

	if (!(evt->event.revents & ZSOCK_POLLIN)) {
		return;
	}

	if (evt->event.fd == sh_telnet->fds[SOCK_ID_IPV4_LISTEN].fd) {
		telnet_accept(&sh_telnet->fds[SOCK_ID_IPV4_LISTEN]);
		return;
	} else if (evt->event.fd == sh_telnet->fds[SOCK_ID_IPV6_LISTEN].fd) {
		telnet_accept(&sh_telnet->fds[SOCK_ID_IPV6_LISTEN]);
		return;
	} else if (evt->event.fd == sh_telnet->fds[SOCK_ID_CLIENT].fd) {
		telnet_recv(&sh_telnet->fds[SOCK_ID_CLIENT]);
		return;
	}

	LOG_ERR("Unexpected FD received for telnet, restarting service.");

error:
	telnet_restart_server();
}

static int telnet_setup_server(struct zsock_pollfd *pollfd, net_sa_family_t family,
			       struct net_sockaddr *addr, net_socklen_t addrlen)
{
	int ret = 0;

	pollfd->fd = zsock_socket(family, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	if (pollfd->fd < 0) {
		ret = -errno;
		LOG_ERR("Failed to create telnet NET_AF_INET%s socket",
			family == NET_AF_INET ? "" : "6");
		goto error;
	}

	if (zsock_bind(pollfd->fd, addr, addrlen) < 0) {
		ret = -errno;
		LOG_ERR("Cannot bind on family NET_AF_INET%s (%d)",
			family == NET_AF_INET ? "" : "6", ret);
		goto error;
	}

	if (zsock_listen(pollfd->fd, 1)) {
		ret = -errno;
		LOG_ERR("Cannot listen on family NET_AF_INET%s (%d)",
			family == NET_AF_INET ? "" : "6", ret);
		goto error;
	}

	pollfd->events = ZSOCK_POLLIN;

	LOG_DBG("Telnet console enabled on NET_AF_INET%s",
		family == NET_AF_INET ? "" : "6");

	return 0;

error:
	LOG_ERR("Unable to start telnet on NET_AF_INET%s (%d)",
		family == NET_AF_INET ? "" : "6", ret);

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
		struct net_sockaddr_in any_addr4 = {
			.sin_family = NET_AF_INET,
			.sin_port = net_htons(TELNET_PORT),
			.sin_addr = NET_INADDR_ANY_INIT
		};

		ret = telnet_setup_server(&ctx->fds[SOCK_ID_IPV4_LISTEN],
					  NET_AF_INET, (struct net_sockaddr *)&any_addr4,
					  sizeof(any_addr4));
		if (ret < 0) {
			goto error;
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		struct net_sockaddr_in6 any_addr6 = {
			.sin6_family = NET_AF_INET6,
			.sin6_port = net_htons(TELNET_PORT),
			.sin6_addr = NET_IN6ADDR_ANY_INIT
		};

		ret = telnet_setup_server(&ctx->fds[SOCK_ID_IPV6_LISTEN],
					  NET_AF_INET6, (struct net_sockaddr *)&any_addr6,
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
	k_mutex_init(&sh_telnet->tx_lock);

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
	struct k_work_sync sync;
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

	k_mutex_lock(&sh_telnet->tx_lock, K_FOREVER);

	/* Stop the transmission timer, so it does not interrupt the operation.
	 */
	timeout = k_ticks_to_ms_ceil32(
			k_work_delayable_remaining_get(&sh_telnet->send_work));
	was_running = k_work_cancel_delayable_sync(&sh_telnet->send_work, &sync);
	sh_telnet->tx_active = true;

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
				sh_telnet->tx_active = false;
				k_mutex_unlock(&sh_telnet->tx_lock);
				if ((err == -ENOTCONN) || (err == -ENETDOWN)) {
					LOG_ERR("Network disconnected, shutting down");
				} else if (err != -ECANCELED) {
					LOG_ERR("Error %d, shutting down", err);
				}
				return 0; /* Return 0 to not trigger ASSERT in shell_ops.c */
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

	sh_telnet->tx_active = false;
	k_mutex_unlock(&sh_telnet->tx_lock);

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
