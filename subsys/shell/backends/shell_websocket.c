/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/websocket.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_websocket.h>
#include <zephyr/logging/log_backend_ws.h>

LOG_MODULE_REGISTER(shell_websocket, CONFIG_SHELL_WEBSOCKET_LOG_LEVEL);

#define WEBSOCKET_LINE_SIZE CONFIG_SHELL_WEBSOCKET_LINE_BUF_SIZE
#define WEBSOCKET_TIMEOUT   CONFIG_SHELL_WEBSOCKET_SEND_TIMEOUT

/* Pause between retries while the stack is out of buffers, growing like the
 * socket layer's own retry for blocking sends.
 */

#define WEBSOCKET_MIN_COMMAND_LEN 2
#define WEBSOCKET_WILL_DO_COMMAND_LEN 3

static void ws_server_cb(struct net_socket_service_event *evt);

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(websocket_server, ws_server_cb,
				      SHELL_WEBSOCKET_SERVICE_COUNT);

/* The send work handler must not wait for itself, so it passes in_send_work
 * and only prevents further submissions.
 */
static void ws_end_client_connection(struct shell_websocket *ws, bool in_send_work)
{
	int ret;

	LOG_DBG("Closing connection to #%d", ws->fds[0].fd);

	/* Serialize with the output writers. An in-flight send keeps the
	 * WebSocket context alive; closing it concurrently would free the
	 * context under the sender. The wait is bounded by the send deadline.
	 */
	k_mutex_lock(&ws->tx_lock, K_FOREVER);

	k_mutex_lock(&ws->socket_lock, K_FOREVER);

	if (IS_ENABLED(CONFIG_LOG_BACKEND_WS)) {
		(void)log_backend_ws_unregister(ws->fds[0].fd);
	}

	(void)websocket_unregister(ws->fds[0].fd);

	ws->fds[0].fd = -1;
	ws->output_lock = false;
	ws->session++;

	if (in_send_work) {
		(void)k_work_cancel_delayable(&ws->send_work);
	} else {
		struct k_work_sync sync;

		(void)k_work_cancel_delayable_sync(&ws->send_work, &sync);
	}

	ret = net_socket_service_register(&websocket_server, ws->fds,
					  ARRAY_SIZE(ws->fds), NULL);
	if (ret < 0) {
		LOG_ERR("Failed to re-register socket service (%d)", ret);
	}

	k_mutex_unlock(&ws->socket_lock);

	k_mutex_unlock(&ws->tx_lock);
}

/* Waits with socket_lock released until a send may succeed again, so a
 * client that stops reading never holds up the connection close: for a full
 * window until the socket is writable, for a buffer shortage a growing pause.
 */



static bool ws_session_changed(struct shell_websocket *ws, uint32_t session)
{
	bool changed;

	k_mutex_lock(&ws->socket_lock, K_FOREVER);
	changed = ws->session != session || ws->output_lock || ws->fds[0].fd < 0;
	k_mutex_unlock(&ws->socket_lock);

	return changed;
}

static int ws_send(struct shell_websocket *ws, int fd, uint32_t session, bool block)
{
	int64_t deadline = k_uptime_get() + CONFIG_SHELL_WEBSOCKET_TX_TIMEOUT;
	int32_t timeout = 0;
	int ret;

	if (ws->line_out.len == 0) {
		return 0;
	}

	if (fd < 0) {
		return -ENOTCONN;
	}

	if (block) {
		timeout = (int32_t)(deadline - k_uptime_get());
		if (timeout < 0) {
			return -ETIMEDOUT;
		}
	}

	/* websocket_send_msg() frames the line as a single text frame and
	 * honors the given timeout. The socket lock is not held here, so the
	 * connection state can change while the frame is being sent; that is
	 * detected below through the connection generation.
	 */
	ret = websocket_send_msg(fd, ws->line_out.buf, ws->line_out.len,
				 WEBSOCKET_OPCODE_DATA_TEXT, false, true, timeout);
	if (ret >= 0) {
		/* We reinitialize the line buffer */
		ws->line_out.len = 0;
		return 0;
	}

	if (!block && ret == -EAGAIN) {
		/* Nothing was sent, keep the line for the next attempt. */
		return -EAGAIN;
	}

	/* A frame that could not be completed must not be resumed: a partially
	 * sent frame would make the following frame be parsed as its payload.
	 * Drop the pending output, the caller closes the connection.
	 */
	ws->line_out.len = 0;

	if (ws_session_changed(ws, session)) {
		return -ECANCELED;
	}

	return ret;
}


static void ws_send_prematurely(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct shell_websocket *ws = CONTAINER_OF(dwork,
						  struct shell_websocket,
						  send_work);
	uint32_t session;
	int fd;
	int ret;

	/* Do not make the system workqueue wait for a shell writer or for the
	 * connection state; retry later when either is busy.
	 */
	if (k_mutex_lock(&ws->tx_lock, K_NO_WAIT) != 0) {
		k_work_reschedule(&ws->send_work, K_MSEC(WEBSOCKET_TIMEOUT));
		return;
	}

	if (k_mutex_lock(&ws->socket_lock, K_NO_WAIT) != 0) {
		k_mutex_unlock(&ws->tx_lock);
		k_work_reschedule(&ws->send_work, K_MSEC(WEBSOCKET_TIMEOUT));
		return;
	}

	if (ws->fds[0].fd < 0 || ws->output_lock) {
		k_mutex_unlock(&ws->socket_lock);
		k_mutex_unlock(&ws->tx_lock);
		return;
	}

	fd = ws->fds[0].fd;
	session = ws->session;
	k_mutex_unlock(&ws->socket_lock);

	/* Use non-blocking send to prevent system workqueue blocking. */
	ret = ws_send(ws, fd, session, false);
	k_mutex_unlock(&ws->tx_lock);

	if (ret == -EAGAIN) {
		/* Nothing was sent, retry the same line later. */
		k_work_reschedule(&ws->send_work, K_MSEC(WEBSOCKET_TIMEOUT));
	} else if (ret < 0 && ret != -ENOTCONN && ret != -ECANCELED) {
		LOG_ERR("Failed to send %d, closing connection", -ret);
		ws_end_client_connection(ws, true);
	}
}


static void ws_recv(struct shell_websocket *ws, struct zsock_pollfd *pollfd)
{
	size_t len, buf_left;
	uint8_t *buf;
	int ret;

	k_mutex_lock(&ws->rx_lock, K_FOREVER);

	buf_left = sizeof(ws->rx_buf) - ws->rx_len;
	if (buf_left == 0) {
		/* No space left to read TCP stream, try again later. */
		k_mutex_unlock(&ws->rx_lock);
		k_msleep(10);
		return;
	}

	buf = ws->rx_buf + ws->rx_len;

	ret = zsock_recv(pollfd->fd, buf, buf_left, 0);
	if (ret < 0) {
		LOG_DBG("Websocket client error %d", ret);
		goto error;
	} else if (ret == 0) {
		LOG_DBG("Websocket client closed connection");
		goto error;
	}

	len = ret;
	ws->rx_len += len;

	k_mutex_unlock(&ws->rx_lock);

	ws->shell_handler(SHELL_TRANSPORT_EVT_RX_RDY, ws->shell_context);

	return;

error:
	k_mutex_unlock(&ws->rx_lock);
	ws_end_client_connection(ws, false);
}

static void ws_server_cb(struct net_socket_service_event *evt)
{
	net_socklen_t optlen = sizeof(int);
	struct shell_websocket *ws;
	int sock_error;

	ws = (struct shell_websocket *)evt->user_data;

	if ((evt->event.revents & ZSOCK_POLLERR) ||
	    (evt->event.revents & ZSOCK_POLLNVAL)) {
		(void)zsock_getsockopt(evt->event.fd, ZSOCK_SOL_SOCKET,
				       ZSOCK_SO_ERROR, &sock_error, &optlen);
		LOG_ERR("Websocket socket %d error (%d)", evt->event.fd, sock_error);

		if (evt->event.fd == ws->fds[0].fd) {
			return ws_end_client_connection(ws, false);
		}

		return;
	}

	if (!(evt->event.revents & ZSOCK_POLLIN)) {
		return;
	}

	if (evt->event.fd == ws->fds[0].fd) {
		return ws_recv(ws, &ws->fds[0]);
	}
}

static int shell_ws_init(struct shell_websocket *ctx, int ws_socket)
{
	int ret;

	if (ws_socket < 0) {
		LOG_ERR("Invalid socket %d", ws_socket);
		return -EBADF;
	}

	if (ctx->fds[0].fd >= 0) {
		/* There is already a websocket connection to this shell,
		 * kick the previous connection out.
		 */
		ws_end_client_connection(ctx, false);
	}

	ctx->line_out.len = 0;
	ctx->output_lock = false;
	ctx->session++;
	ctx->fds[0].fd = ws_socket;
	ctx->fds[0].events = ZSOCK_POLLIN;

	ret = net_socket_service_register(&websocket_server, ctx->fds,
					  ARRAY_SIZE(ctx->fds), ctx);
	if (ret < 0) {
		LOG_ERR("Failed to register socket service, %d", ret);
		goto error;
	}
	if (IS_ENABLED(CONFIG_LOG_BACKEND_WS)) {
		log_backend_ws_register(ws_socket);
	}

	return 0;

error:
	if (ctx->fds[0].fd >= 0) {
		(void)zsock_close(ctx->fds[0].fd);
		ctx->fds[0].fd = -1;
	}

	return ret;
}

/* Shell API */

static int init(const struct shell_transport *transport,
		const void *config,
		shell_transport_handler_t evt_handler,
		void *context)
{
	struct shell_websocket *ws;

	ws = (struct shell_websocket *)transport->ctx;

	memset(ws, 0, sizeof(struct shell_websocket));
	for (int i = 0; i < ARRAY_SIZE(ws->fds); i++) {
		ws->fds[i].fd = -1;
	}

	ws->shell_handler = evt_handler;
	ws->shell_context = context;

	k_work_init_delayable(&ws->send_work, ws_send_prematurely);
	k_mutex_init(&ws->rx_lock);
	k_mutex_init(&ws->socket_lock);
	k_mutex_init(&ws->tx_lock);

	return 0;
}

static int uninit(const struct shell_transport *transport)
{
	ARG_UNUSED(transport);

	return 0;
}

static int enable(const struct shell_transport *transport, bool blocking)
{
	ARG_UNUSED(transport);
	ARG_UNUSED(blocking);

	return 0;
}

static int sh_write(const struct shell_transport *transport,
		    const void *data, size_t length, size_t *cnt)
{
	struct shell_websocket_line_buf *lb;
	struct shell_websocket *ws;
	struct k_work_sync sync;
	uint32_t timeout;
	bool was_running;
	size_t copy_len;
	int ret;

	ws = (struct shell_websocket *)transport->ctx;

	*cnt = 0;
	lb = &ws->line_out;

	/* Serialize the writers. The socket lock itself must not be held across
	 * a send: sending may block while the peer is not reading, and holding
	 * it would also keep the socket service thread from closing the
	 * connection.
	 */
	k_mutex_lock(&ws->tx_lock, K_FOREVER);

	/* Stop the transmission timer, so it does not interrupt the operation.
	 */
	timeout = k_ticks_to_ms_ceil32(k_work_delayable_remaining_get(&ws->send_work));
	was_running = k_work_cancel_delayable_sync(&ws->send_work, &sync);

	do {
		uint32_t session;
		int fd;

		if (lb->len + length - *cnt > WEBSOCKET_LINE_SIZE) {
			copy_len = WEBSOCKET_LINE_SIZE - lb->len;
		} else {
			copy_len = length - *cnt;
		}

		memcpy(lb->buf + lb->len, (uint8_t *)data + *cnt, copy_len);
		lb->len += copy_len;

		/* Send the data immediately if the buffer is full or line feed
		 * is recognized.
		 */
		if (lb->buf[lb->len - 1] == '\n' || lb->len == WEBSOCKET_LINE_SIZE) {
			k_mutex_lock(&ws->socket_lock, K_FOREVER);
			if (ws->fds[0].fd < 0 || ws->output_lock) {
				*cnt = length;
				k_mutex_unlock(&ws->socket_lock);
				k_mutex_unlock(&ws->tx_lock);
				return 0;
			}

			fd = ws->fds[0].fd;
			session = ws->session;
			k_mutex_unlock(&ws->socket_lock);

			ret = ws_send(ws, fd, session, true);
			if (ret != 0) {
				k_mutex_lock(&ws->socket_lock, K_FOREVER);
				if (ws->session == session) {
					ws->output_lock = true;
				}
				k_mutex_unlock(&ws->socket_lock);
				k_mutex_unlock(&ws->tx_lock);

				*cnt = length;
				if (ret != -ENOTCONN && ret != -ECANCELED) {
					LOG_ERR("Failed to send %d, closing connection", -ret);
					ws_end_client_connection(ws, false);
				}
				return 0; /* Return 0 to not trigger ASSERT in shell_ops.c */
			}
		}

		*cnt += copy_len;
	} while (*cnt < length);

	if (lb->len > 0) {
		/* Check if the timer was already running, initialize otherwise.
		 */
		timeout = was_running ? timeout : WEBSOCKET_TIMEOUT;

		k_work_reschedule(&ws->send_work, K_MSEC(timeout));
	}

	ws->shell_handler(SHELL_TRANSPORT_EVT_TX_RDY, ws->shell_context);

	k_mutex_unlock(&ws->tx_lock);

	return 0;
}


static int sh_read(const struct shell_transport *transport,
		   void *data, size_t length, size_t *cnt)
{
	struct shell_websocket *ws;
	size_t read_len;

	ws = (struct shell_websocket *)transport->ctx;

	if (ws->fds[0].fd < 0) {
		goto no_data;
	}

	k_mutex_lock(&ws->rx_lock, K_FOREVER);

	if (ws->rx_len == 0) {
		k_mutex_unlock(&ws->rx_lock);
		goto no_data;
	}

	read_len = ws->rx_len;
	if (read_len > length) {
		read_len = length;
	}

	memcpy(data, ws->rx_buf, read_len);
	*cnt = read_len;

	ws->rx_len -= read_len;
	if (ws->rx_len > 0) {
		memmove(ws->rx_buf, ws->rx_buf + read_len, ws->rx_len);
	}

	k_mutex_unlock(&ws->rx_lock);

	return 0;

no_data:
	*cnt = 0;
	return 0;
}

const struct shell_transport_api shell_websocket_transport_api = {
	.init = init,
	.uninit = uninit,
	.enable = enable,
	.write = sh_write,
	.read = sh_read
};

int shell_websocket_setup(int ws_socket, struct http_request_ctx *request_ctx, void *user_data)
{
	struct shell_websocket *ws = user_data;

	return shell_ws_init(ws, ws_socket);
}

int shell_websocket_enable(const struct shell *sh)
{
	bool log_backend = CONFIG_SHELL_WEBSOCKET_INIT_LOG_LEVEL > 0 &&
			   !IS_ENABLED(CONFIG_LOG_BACKEND_WS);
	uint32_t level = (CONFIG_SHELL_WEBSOCKET_INIT_LOG_LEVEL > LOG_LEVEL_DBG) ?
		CONFIG_LOG_MAX_LEVEL : CONFIG_SHELL_WEBSOCKET_INIT_LOG_LEVEL;
	static const struct shell_backend_config_flags cfg_flags =
		SHELL_DEFAULT_BACKEND_CONFIG_FLAGS;
	int ret;

	ret = shell_init(sh, NULL, cfg_flags, log_backend, level);
	if (ret < 0) {
		LOG_DBG("Cannot init websocket shell %p", sh);
	}

	return ret;
}
