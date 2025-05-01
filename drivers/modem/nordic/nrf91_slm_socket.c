/*
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nrf91_slm.h"

#include <zephyr/sys/ring_buffer.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(nrf91_slm, CONFIG_MODEM_LOG_LEVEL);

struct nrf91_slm_parse_context {
	uint8_t *start;
	uint8_t *end;
	size_t size;
	uintptr_t len;
};

static void nrf91_slm_parse_xpoll(struct nrf91_slm_data *data, struct nrf91_slm_parse_context *ctx)
{
	struct modem_socket *sock;
	int revents;
	int id;

	/*
	 * #XPOLL: %d,"%x"\r\n
	 * │                │
	 * │                └ end
	 * └ start
	 */
	id = strtoul((char *)ctx->start + 7, (char **)&ctx->start, 10);

	sock = modem_socket_from_id(&data->socket_config, id);
	if (sock == NULL) {
		LOG_WRN("invalid socket id (%d)", id);
		return;
	}

	/*
	 * #XPOLL: %d,"%x"\r\n
	 *           │      │
	 *           │      └ end
	 *           └ start
	 */
	revents = strtoul((char *)ctx->start + 1, NULL, 16);

	for (int i = 0; i < data->poll_nfds; i++) {
		if (data->poll_fds[i].fd == sock->sock_fd) {
			data->poll_fds[i].revents = revents;
			data->poll_count++;
			break;
		}
	}
}

static void nrf91_slm_parse_xrecv(struct nrf91_slm_data *data, struct nrf91_slm_parse_context *ctx)
{
	/*
	 * #XRECV: %d\r\n......\r\nOK\r\n
	 * │           │
	 * │           └ end
	 * └ start
	 */
	int pending = strtoul((char *)ctx->start + 7, NULL, 10);

	if (ctx->size < pending) {
		LOG_WRN("lost %d bytes", pending - ctx->size);
		pending = ctx->size;
	}

	ctx->start = ctx->end + 1;
	ctx->size -= ctx->len + 1;
	ctx->end = ctx->start + pending;
	ctx->len = pending;

	LOG_HEXDUMP_DBG(ctx->start, ctx->len, "received: ");

	/*
	 *               |-len-|
	 * #XRECV: %d\r\n......\r\nOK\r\n
	 *               │     │
	 *               │     └ end
	 *               └ start
	 */
	ring_buf_put(&data->sock_recv_rb, ctx->start, ctx->len);
}

static void nrf91_slm_pipe_callback(struct modem_pipe *pipe, enum modem_pipe_event event,
				    void *user_data)
{
	struct nrf91_slm_data *data = (struct nrf91_slm_data *)user_data;
	struct nrf91_slm_parse_context parse_ctx;
	int ret;

	if (event == MODEM_PIPE_EVENT_RECEIVE_READY) {
		parse_ctx.start = data->sock_receive_buf;
		parse_ctx.size = sizeof(data->sock_receive_buf);

		ret = modem_pipe_receive(pipe, parse_ctx.start, parse_ctx.size);
		if (ret < 0) {
			LOG_ERR("failed to receive data (%d)", ret);
			return;
		}

		parse_ctx.size = ret;
		parse_ctx.end = memchr(parse_ctx.start, '\n', parse_ctx.size);

		while (parse_ctx.end != NULL) {
			parse_ctx.len = parse_ctx.end - parse_ctx.start;

			if (parse_ctx.len > 1) {
				/* Print non-blank lines */
				LOG_DBG("%.*s", (int)parse_ctx.len, parse_ctx.start);
			}

			if (memcmp(parse_ctx.start, "#XPOLL:", 7) == 0) {
				nrf91_slm_parse_xpoll(data, &parse_ctx);
			} else if (memcmp(parse_ctx.start, "#XRECV:", 7) == 0) {
				nrf91_slm_parse_xrecv(data, &parse_ctx);
			} else if (memcmp(parse_ctx.start, "OK", 2) == 0) {
				k_sem_give(&data->sock_recv_sem);
				break;
			} else if (memcmp(parse_ctx.start, "ERROR", 5) == 0) {
				k_sem_give(&data->sock_recv_sem);
				break;
			}


			/* Next line */
			parse_ctx.start = parse_ctx.end + 1;
			parse_ctx.size -= parse_ctx.len + 1;
			parse_ctx.end = memchr(parse_ctx.start, '\n', parse_ctx.size);
		}
	} else if (event == MODEM_PIPE_EVENT_TRANSMIT_IDLE) {
		if (data->sock_send_buf_len == 0) {
			return;
		}

		ret = modem_pipe_transmit(pipe, data->sock_send_buf,
						data->sock_send_buf_len);

		if (ret < 0) {
			LOG_ERR("error during pipe transmit (%d)", ret);
			data->sock_send_buf_len = 0;
		} else {
			LOG_DBG("transmitted %d bytes", ret);
			data->sock_send_count += ret;
			data->sock_send_buf += ret;
			data->sock_send_buf_len -= ret;
		}

		if (data->sock_send_buf_len == 0) {
			k_sem_give(&data->sock_send_sem);
		}
	}
}

static void nrf91_slm_chat_on_xsocket(struct modem_chat *chat, char **argv, uint16_t argc,
				      void *user_data)
{
	struct nrf91_slm_data *data = (struct nrf91_slm_data *)user_data;
	struct modem_socket *sock = &data->sockets[0];

	/* TODO: support more than one socket */
	if (argc == 4) {
		/* New modem socket created */
		sock->id = atoi(argv[1]);
		LOG_INF("socket id %d assigned to fd %d", sock->id, sock->sock_fd);
	} else {
		/* Active modem socket closed */
		LOG_INF("closed socket");
	}
}

static void nrf91_slm_chat_on_xconnect(struct modem_chat *chat, char **argv, uint16_t argc,
				       void *user_data)
{
	struct nrf91_slm_data *data = (struct nrf91_slm_data *)user_data;

	if (atoi(argv[1]) == 1) {
		/* TODO: We only support one socket right now */
		data->sockets[0].is_connected = true;
	}
}

MODEM_CHAT_MATCH_DEFINE(ok_match, "OK", "", NULL);
MODEM_CHAT_MATCH_DEFINE(abort_match, "ERROR", "", NULL);
MODEM_CHAT_MATCH_DEFINE(xsocket_match, "#XSOCKET: ", ",", nrf91_slm_chat_on_xsocket);
MODEM_CHAT_MATCH_DEFINE(xconnect_match, "#XCONNECT: ", "", nrf91_slm_chat_on_xconnect);
MODEM_CHAT_MATCH_DEFINE(xdatamode_match, "#XDATAMODE: ", "", NULL);

/* AT#XSOCKET=<op>[,<type>,<role>] */
static int nrf91_slm_xsocket(struct nrf91_slm_data *data, int op, int type)
{
	struct modem_chat_script script;
	struct modem_chat_script_chat script_chats[2];
	char request[sizeof("AT#XSOCKET=#,#,#")];
	int ret;

	if (op == 0) {
		/* Close the active socket */
		strncpy(request, "AT#XSOCKET=0", sizeof(request));
	} else {
		/* Open a new socket */
		ret = snprintk(request, sizeof(request), "AT#XSOCKET=%d,%d,0", op, type);
		if (ret < 0) {
			return ret;
		}
	}

	modem_chat_script_chat_init(&script_chats[0]);
	modem_chat_script_chat_set_request(&script_chats[0], request);
	modem_chat_script_chat_set_response_matches(&script_chats[0], &xsocket_match, 1);

	modem_chat_script_chat_init(&script_chats[1]);
	modem_chat_script_chat_set_request(&script_chats[1], "");
	modem_chat_script_chat_set_response_matches(&script_chats[1], &ok_match, 1);
	modem_chat_script_chat_set_timeout(&script_chats[1], 100);

	modem_chat_script_init(&script);
	modem_chat_script_set_name(&script, "xsocket");
	modem_chat_script_set_script_chats(&script, script_chats, 2);
	modem_chat_script_set_abort_matches(&script, &abort_match, 1);
	modem_chat_script_set_timeout(&script, 10);

	return modem_chat_run_script(&data->chat, &script);
}

/* AT#XCONNECT=<url>,<port> */
static int nrf91_slm_xconnect(struct nrf91_slm_data *data, const char *ip_str, uint16_t port)
{
	struct modem_chat_script script;
	struct modem_chat_script_chat script_chats[2];
	char request[sizeof("AT#XCONNECT=##,####") + NET_IPV6_ADDR_LEN];
	int ret;

	ret = snprintk(request, sizeof(request), "AT#XCONNECT=\"%s\",%d", ip_str, port);
	if (ret < 0) {
		return ret;
	}

	modem_chat_script_chat_init(&script_chats[0]);
	modem_chat_script_chat_set_request(&script_chats[0], request);
	modem_chat_script_chat_set_response_matches(&script_chats[0], &xconnect_match, 1);

	modem_chat_script_chat_init(&script_chats[1]);
	modem_chat_script_chat_set_request(&script_chats[1], "");
	modem_chat_script_chat_set_response_matches(&script_chats[1], &ok_match, 1);
	modem_chat_script_chat_set_timeout(&script_chats[1], 100);

	modem_chat_script_init(&script);
	modem_chat_script_set_name(&script, "xconnect");
	modem_chat_script_set_script_chats(&script, script_chats, 2);
	modem_chat_script_set_abort_matches(&script, &abort_match, 1);
	modem_chat_script_set_timeout(&script, 160);

	return modem_chat_run_script(&data->chat, &script);
}

/* AT#XSEND */
static int nrf91_slm_xsend(struct nrf91_slm_data *data, const void *buf, size_t len)
{
	struct modem_chat_script script;
	struct modem_chat_script_chat script_chat;
	char request[sizeof("AT#XSEND")];
	int ret;

	/* Enter SLM data mode */
	strncpy(request, "AT#XSEND", sizeof(request));
	modem_chat_script_chat_init(&script_chat);
	modem_chat_script_chat_set_request(&script_chat, request);
	modem_chat_script_chat_set_response_matches(&script_chat, &ok_match, 1);

	modem_chat_script_init(&script);
	modem_chat_script_set_name(&script, "xsend");
	modem_chat_script_set_script_chats(&script, &script_chat, 1);
	modem_chat_script_set_abort_matches(&script, &abort_match, 1);
	modem_chat_script_set_timeout(&script, 31);

	ret = modem_chat_run_script(&data->chat, &script);
	if (ret < 0) {
		LOG_ERR("failed to enter data mode (%d)", ret);
		return ret;
	}

	LOG_HEXDUMP_DBG(buf, len, "sending: ");

	data->sock_send_buf = buf;
	data->sock_send_buf_len = len;
	data->sock_send_count = 0;

	k_sem_reset(&data->sock_send_sem);

	modem_chat_release(&data->chat);
	modem_pipe_attach(data->uart_pipe, nrf91_slm_pipe_callback, data);

	/* Wait for transmit */
	ret = k_sem_take(&data->sock_send_sem, K_SECONDS(30));
	if (ret < 0) {
		LOG_ERR("failed to take semaphore (%d)", ret);
	}

	modem_chat_attach(&data->chat, data->uart_pipe);

	/* Exit SLM data mode */
	/* TODO: The '+++' terminator should be configurable */
	strncpy(request, "+++", sizeof(request));
	modem_chat_script_chat_init(&script_chat);
	modem_chat_script_chat_set_request(&script_chat, request);
	modem_chat_script_chat_set_response_matches(&script_chat, &xdatamode_match, 1);

	modem_chat_script_init(&script);
	modem_chat_script_set_name(&script, "xsend");
	modem_chat_script_set_script_chats(&script, &script_chat, 1);
	modem_chat_script_set_abort_matches(&script, &abort_match, 1);
	modem_chat_script_set_timeout(&script, 31);

	ret = modem_chat_run_script(&data->chat, &script);
	if (ret < 0) {
		LOG_ERR("failed to exit data mode (%d)", ret);
	}

	return data->sock_send_count;
}

/* AT#XRECV=<timeout>[,<flags>] */
static int nrf91_slm_xrecv(struct nrf91_slm_data *data, int timeout_s, int flags)
{
	struct modem_chat *chat = &data->chat;
	char request[sizeof("AT#XRECV=####,###") + chat->delimiter_size];
	int ret;
	int i;

	__ASSERT(timeout_s >= 0, "Timeout must be >= 0");

	ret = snprintk(request, sizeof(request) - chat->delimiter_size, "AT#XRECV=%d,%d", timeout_s,
		       flags);

	if (ret < 0) {
		return ret;
	}

	LOG_DBG("%.*s", ret, request);

	/* Append the delimiter */
	for (i = 0; i < chat->delimiter_size; i++) {
		request[ret++] = chat->delimiter[i];
	}

	data->sock_send_buf = request;
	data->sock_send_buf_len = ret;
	data->sock_send_count = 0;

	k_sem_reset(&data->sock_recv_sem);

	modem_chat_release(chat);
	modem_pipe_attach(data->uart_pipe, nrf91_slm_pipe_callback, data);

	/* Wait for 'OK' */
	ret = k_sem_take(&data->sock_recv_sem, K_SECONDS(timeout_s + 1));
	if (ret < 0) {
		LOG_ERR("failed to take semaphore (%d)", ret);
	}

	modem_chat_attach(chat, data->uart_pipe);
	return ret;
}

/* AT#XPOLL=<timeout>[,<handle>,...] */
static int nrf91_slm_xpoll(struct nrf91_slm_data *data, struct zsock_pollfd *fds, int nfds,
			   int timeout_ms)
{
	char request[64];
	struct modem_chat *chat = &data->chat;
	struct modem_socket *sock;
	int len;
	int ret;
	int i;

	__ASSERT(fds != NULL, "fds must not be NULL");

	data->poll_fds = fds;
	data->poll_nfds = nfds;
	data->poll_count = 0;

	ret = snprintk(request, sizeof(request), "AT#XPOLL=%d", timeout_ms);
	if (ret < 0) {
		return ret;
	}

	len = ret;

	/* Append the socket ids */
	for (i = 0; i < nfds; i++) {
		sock = modem_socket_from_fd(&data->socket_config, fds[i].fd);

		ret = snprintk(request + len, sizeof(request) - len, ",%d", sock->id);
		if (ret < 0) {
			return ret;
		}

		len += ret;
	}

	/* Append the delimiter */
	if (len + chat->delimiter_size >= sizeof(request)) {
		return -ENOMEM;
	}

	LOG_DBG("%.*s", len, request);

	for (i = 0; i < chat->delimiter_size; i++) {
		request[len++] = chat->delimiter[i];
	}

	data->sock_send_buf = request;
	data->sock_send_buf_len = len;
	data->sock_send_count = 0;

	k_sem_reset(&data->sock_recv_sem);

	modem_chat_release(chat);
	modem_pipe_attach(data->uart_pipe, nrf91_slm_pipe_callback, data);

	/* Wait for "OK" */
	ret = k_sem_take(&data->sock_recv_sem, K_MSEC(timeout_ms + 500));
	if (ret < 0) {
		LOG_ERR("failed to take semaphore (%d)", ret);
	}

	modem_chat_attach(chat, data->uart_pipe);
	return ret;
}

int nrf91_slm_socket(struct nrf91_slm_data *data, int family, int type, int proto)
{
	int sock_fd;
	int ret;

	ret = modem_socket_get(&data->socket_config, family, type, proto);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	sock_fd = ret;

	ret = k_mutex_lock(&data->chat_lock, K_SECONDS(10));
	if (ret < 0) {
		modem_socket_put(&data->socket_config, sock_fd);
		errno = -ret;
		return -1;
	}

	ret = nrf91_slm_xsocket(data, family, type);
	k_mutex_unlock(&data->chat_lock);

	if (ret < 0) {
		LOG_ERR("failed to create socket (%d)", ret);
		modem_socket_put(&data->socket_config, sock_fd);
		errno = -ret;
		return -1;
	}

	errno = 0;
	return sock_fd;
}

int nrf91_slm_connect(struct nrf91_slm_data *data, void *obj, const struct sockaddr *addr,
		      socklen_t addrlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	char ip_str[NET_IPV6_ADDR_LEN];
	void *src = NULL;
	uint16_t port = 0;
	int ret;

	if (!addr) {
		errno = EINVAL;
		return -1;
	}

	if (data->state != NRF91_SLM_STATE_CARRIER_ON) {
		LOG_ERR("modem is not attached to the network!");
		errno = EAGAIN;
		return -1;
	}

	memcpy(&sock->dst, addr, sizeof(*addr));

	switch (addr->sa_family) {
	case AF_INET6: {
		src = &net_sin6(addr)->sin6_addr;
		port = ntohs(net_sin6(addr)->sin6_port);
		break;
	}
	case AF_INET: {
		src = &net_sin(addr)->sin_addr;
		port = ntohs(net_sin(addr)->sin_port);
		break;
	}
	default:
		errno = EAFNOSUPPORT;
		return -1;
	}

	net_addr_ntop(addr->sa_family, src, ip_str, sizeof(ip_str));

	ret = k_mutex_lock(&data->chat_lock, K_SECONDS(1));
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	ret = nrf91_slm_xconnect(data, ip_str, port);
	k_mutex_unlock(&data->chat_lock);

	if (ret < 0) {
		LOG_ERR("failed to connect socket (%d)", ret);
		errno = -ret;
		return -1;
	}

	if (sock->is_connected) {
		LOG_INF("socket %d connected to %s:%d", sock->id, ip_str, port);
	}

	errno = 0;
	return 0;
}

ssize_t nrf91_slm_recvfrom(struct nrf91_slm_data *data, void *obj, void *buf, size_t max_len,
			   int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	int ret = 0;

	if (ring_buf_size_get(&data->sock_recv_rb) < max_len) {
		if (data->state != NRF91_SLM_STATE_CARRIER_ON) {
			LOG_ERR("modem is not attached to the network!");
			errno = EAGAIN;
			return -1;
		}

		ret = k_mutex_lock(&data->chat_lock, K_SECONDS(1));
		if (ret < 0) {
			errno = -ret;
			return -1;
		}

		/* Request more data from the modem */
		if (sock->type == SOCK_STREAM) {
			ret = nrf91_slm_xrecv(data, 1, flags);
		} else {
			/* TODO: Add XRECVFROM for SOCK_DGRAM */
			ret = -ESOCKTNOSUPPORT;
		}

		k_mutex_unlock(&data->chat_lock);

		if (ret < 0) {
			errno = -ret;
			return -1;
		}
	}

	errno = 0;
	return ring_buf_get(&data->sock_recv_rb, buf, max_len);
}

ssize_t nrf91_slm_sendto(struct nrf91_slm_data *data, void *obj, const void *buf, size_t len,
			 int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	char ip_str[NET_IPV6_ADDR_LEN];
	void *src = NULL;
	uint16_t port = 0;
	int ret;

	ARG_UNUSED(flags);

	if (data->state != NRF91_SLM_STATE_CARRIER_ON) {
		LOG_ERR("modem is not attached to the network!");
		errno = EAGAIN;
		return -1;
	}

	if (dest_addr != NULL && addrlen > 0) {
		if (sock->type == SOCK_STREAM) {
			errno = EISCONN;
			return -1;
		}

		switch (dest_addr->sa_family) {
		case AF_INET6: {
			src = &net_sin6(dest_addr)->sin6_addr;
			port = ntohs(net_sin6(dest_addr)->sin6_port);
			break;
		}
		case AF_INET: {
			src = &net_sin(dest_addr)->sin_addr;
			port = ntohs(net_sin(dest_addr)->sin_port);
			break;
		}
		default:
			errno = EAFNOSUPPORT;
			return -1;
		}

		net_addr_ntop(dest_addr->sa_family, src, ip_str, sizeof(ip_str));
	}

	ret = k_mutex_lock(&data->chat_lock, K_SECONDS(1));
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	if (sock->type == SOCK_STREAM) {
		ret = nrf91_slm_xsend(data, buf, len);
	} else {
		/* TODO: Add XSENDTO for SOCK_DGRAM */
		ret = -ESOCKTNOSUPPORT;
	}

	k_mutex_unlock(&data->chat_lock);

	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	errno = 0;
	return ret;
}

int nrf91_slm_close(struct nrf91_slm_data *data, void *obj)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	int ret;

	ret = k_mutex_lock(&data->chat_lock, K_SECONDS(1));
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	ret = nrf91_slm_xsocket(data, 0, 0);
	k_mutex_unlock(&data->chat_lock);

	modem_socket_put(&data->socket_config, sock->sock_fd);

	if (ret < 0 && ret != -EAGAIN) {
		LOG_WRN("failed to close socket (%d)", ret);
		errno = -ret;
		return -1;
	}

	return 0;
}

int nrf91_slm_poll(struct nrf91_slm_data *data, struct zsock_pollfd *fds, int nfds, int timeout_ms)
{
	int ret;
	int64_t start_ms;
	int64_t delta_ms;
	k_timeout_t timeout;

	if (data->state != NRF91_SLM_STATE_CARRIER_ON) {
		LOG_ERR("modem is not attached to the network!");
		errno = EAGAIN;
		return -1;
	}

	if (timeout_ms < 0) {
		timeout = K_FOREVER;
	} else if (timeout_ms == 0) {
		timeout = K_NO_WAIT;
	} else {
		timeout = K_MSEC(timeout_ms);
	}

	start_ms = k_uptime_get();
	ret = k_mutex_lock(&data->chat_lock, timeout);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	delta_ms = k_uptime_delta(&start_ms);
	if (timeout_ms >= delta_ms) {
		timeout_ms -= delta_ms;
	}

	ret = nrf91_slm_xpoll(data, fds, nfds, timeout_ms);
	k_mutex_unlock(&data->chat_lock);

	if (ret < 0) {
		LOG_ERR("failed to poll sockets (%d)", ret);
		errno = -ret;
		return -1;
	}

	LOG_DBG("poll count: %d", data->poll_count);

	errno = 0;
	return data->poll_count;
}
