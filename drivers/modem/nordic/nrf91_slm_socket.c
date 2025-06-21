/*
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nrf91_slm.h"

#include <zephyr/sys/ring_buffer.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(nrf91_slm, CONFIG_MODEM_LOG_LEVEL);

static void nrf91_slm_pipe_callback(struct modem_pipe *pipe, enum modem_pipe_event event,
				    void *user_data)
{
	struct nrf91_slm_data *data = (struct nrf91_slm_data *)user_data;
	int ret;

	if (event == MODEM_PIPE_EVENT_RECEIVE_READY) {
		uint8_t *start = data->sock_receive_buf;
		size_t size = sizeof(data->sock_receive_buf);

		ret = modem_pipe_receive(pipe, start, size);
		if (ret < 0) {
			LOG_ERR("failed to receive data (%d)", ret);
			return;
		}

		size = ret;

		uint8_t *end = (uint8_t *)memchr(start, '\n', size);

		while (end != NULL) {
			uintptr_t len = end - start;

			if (len > 1) {
				/* Print non-blank lines */
				LOG_DBG("%.*s", (int)len, start);
			}

			if (memcmp(start, "#XPOLL:", 7) == 0) {
				/*
				 * #XPOLL: %d,"%x"\r\n
				 * │                │
				 * │                └ end
				 * └ start
				 */
				int id = strtoul((char *)start + 7, (char **)&start, 10);

				struct modem_socket *sock =
					modem_socket_from_id(&data->socket_config, id);

				if (sock == NULL) {
					LOG_WRN("invalid socket id (%d)", id);
					continue;
				}

				/*
				 * #XPOLL: %d,"%x"\r\n
				 *           │      │
				 *           │      └ end
				 *           └ start
				 */
				int revents = strtoul((char *)start + 1, NULL, 16);

				for (int i = 0; i < data->poll_nfds; i++) {
					if (data->poll_fds[i].fd == sock->sock_fd) {
						data->poll_fds[i].revents = revents;
						data->poll_count++;
						break;
					}
				}
			} else if (memcmp(start, "#XRECV:", 7) == 0) {
				/*
				 * #XRECV: %d\r\n......\r\nOK\r\n
				 * │           │
				 * │           └ end
				 * └ start
				 */
				int pending = strtoul((char *)start + 7, NULL, 10);

				if (size < pending) {
					LOG_WRN("lost %d bytes", pending - size);
					pending = size;
				}

				start = end + 1;
				size -= len + 1;
				end = start + pending;
				len = pending;

				LOG_HEXDUMP_DBG(start, len, "received: ");

				/*
				 *               |-len-|
				 * #XRECV: %d\r\n......\r\nOK\r\n
				 *               │     │
				 *               │     └ end
				 *               └ start
				 */

				ring_buf_put(&data->sock_recv_rb, start, len);
			} else if (memcmp(start, "#XRECVFROM:", 11) == 0) {
				/*
				 * #XRECVFROM: %d,"%s",%d\r\n......\r\nOK\r\n
				 * │                       │
				 * │                       └ end
				 * └ start
				 */
				int pending = strtoul((char *)start + 11, (char **)&start, 10);

				if (size < pending) {
					LOG_WRN("lost %d bytes", pending - size);
					pending = size;
				}

				/*
				 * #XRECVFROM: %d,"%s",%d\r\n
				 *               │         │
				 *               │         └ end
				 *               └ start
				 */

				/* TODO: Parse the IP address and port */

				start = end + 1;
				size -= len + 1;
				end = start + pending;
				len = pending;

				LOG_HEXDUMP_DBG(start, len, "received: ");

				/*
				 *                           |-len-|
				 * #XRECVFROM: %d,"%s",%d\r\n......\r\nOK\r\n
				 *                           │     │
				 *                           │     └ end
				 *                           └ start
				 */

				ring_buf_put(&data->sock_recv_rb, start, len);
			} else if (memcmp(start, "OK", 2) == 0) {
				k_sem_give(&data->sock_sem);
				break;
			} else if (memcmp(start, "ERROR", 5) == 0) {
				k_sem_give(&data->sock_sem);
				break;
			}

			/* Next line */
			start = end + 1;
			size -= len + 1;
			end = (uint8_t *)memchr(start, '\n', size);
		}
	}
}

static void nrf91_slm_xsocket_callback(struct modem_chat *chat, char **argv, uint16_t argc,
				       void *user_data)
{
	struct nrf91_slm_data *data = (struct nrf91_slm_data *)user_data;
	struct modem_socket *sock = NULL;
	int ret;

	if (argc == 4) {
		/* New socket created */
		sock = modem_socket_from_newid(&data->socket_config);
		if (sock) {
			ret = modem_socket_id_assign(&data->socket_config, sock, atoi(argv[1]));

			if (ret < 0) {
				LOG_ERR("failed to assign socket id (%d)", ret);
				modem_socket_put(&data->socket_config, sock->sock_fd);
			} else {
				LOG_INF("created socket with id %d", sock->id);
			}
		}
	} else {
		/* Active socket closed */
		LOG_INF("closed socket");
	}
}

static int nrf91_slm_xsocket(struct nrf91_slm_data *data, int op, int type)
{
	/* AT#XSOCKET=<op>[,<type>,<role>] */
	char request[sizeof("AT#XSOCKET=#,#,#")];
	static const char match[] = "#XSOCKET: ";
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

	modem_chat_script_chat_set_request(&data->sock_script_chat, request);
	modem_chat_match_set_match(&data->sock_match, match);
	modem_chat_match_set_separators(&data->sock_match, ",");
	modem_chat_match_set_callback(&data->sock_match, nrf91_slm_xsocket_callback);
	modem_chat_script_set_timeout(&data->sock_script, 10);

	return modem_chat_run_script(&data->chat, &data->sock_script);
}

static void nrf91_slm_xconnect_callback(struct modem_chat *chat, char **argv, uint16_t argc,
					void *user_data)
{
	struct nrf91_slm_data *data = (struct nrf91_slm_data *)user_data;

	if (atoi(argv[1]) == 1) {
		/* TODO: We only support one socket right now */
		data->sockets[0].is_connected = true;
	}
}

static int nrf91_slm_xconnect(struct nrf91_slm_data *data, const char *ip_str, uint16_t port)
{
	/* AT#XCONNECT=<url>,<port> */
	char request[sizeof("AT#XCONNECT=##,####") + NET_IPV6_ADDR_LEN];
	static const char match[] = "#XCONNECT: ";
	int ret;

	ret = snprintk(request, sizeof(request), "AT#XCONNECT=\"%s\",%d", ip_str, port);
	if (ret < 0) {
		return ret;
	}

	modem_chat_script_chat_set_request(&data->sock_script_chat, request);
	modem_chat_match_set_match(&data->sock_match, match);
	modem_chat_match_set_separators(&data->sock_match, "");
	modem_chat_match_set_callback(&data->sock_match, nrf91_slm_xconnect_callback);
	modem_chat_script_set_timeout(&data->sock_script, 160);

	return modem_chat_run_script(&data->chat, &data->sock_script);
}

static int nrf91_slm_xsend(struct nrf91_slm_data *data, const void *buf, size_t len)
{
	/* AT#XSEND */
	char request[sizeof("AT#XSEND")];
	char match[sizeof("#XDATAMODE: ")];
	int count = 0;
	int ret;

	strncpy(request, "AT#XSEND", sizeof(request));
	strncpy(match, "OK", sizeof(match));

	modem_chat_script_chat_set_request(&data->sock_script_chat, request);
	modem_chat_match_set_match(&data->sock_match, match);
	modem_chat_match_set_separators(&data->sock_match, "");
	modem_chat_match_set_callback(&data->sock_match, NULL);

	/* #XDATAMODE can take up to 30 seconds to return */
	modem_chat_script_set_timeout(&data->sock_script, 35);

	/* Enter SLM data mode */
	ret = modem_chat_run_script(&data->chat, &data->sock_script);
	if (ret < 0) {
		LOG_ERR("failed to enter data mode (%d)", ret);
		return ret;
	}

	/* Send the payload directly on the pipe */
	LOG_HEXDUMP_DBG(buf, len, "sending: ");
	ret = modem_pipe_transmit(data->uart_pipe, buf, len);
	if (ret < 0) {
		LOG_WRN("failed to send data (%d)", ret);
	} else {
		count = ret;
	}

	/* TODO: The '+++' terminator is configurable in the modem.
	 * It should be configurable here as well.
	 */
	strncpy(request, "+++", sizeof(request));
	strncpy(match, "#XDATAMODE: ", sizeof(match));

	modem_chat_script_chat_set_request(&data->sock_script_chat, request);
	modem_chat_match_set_match(&data->sock_match, match);

	ret = modem_chat_run_script(&data->chat, &data->sock_script);
	if (ret < 0) {
		LOG_ERR("failed to exit data mode (%d)", ret);
		return ret;
	}

	return count;
}

static int nrf91_slm_xsendto(struct nrf91_slm_data *data, const void *buf, size_t len,
			     const char *ip_str, uint16_t port)
{
	/* AT#XSEND */
	char request[sizeof("AT#XSENDTO=##,####") + NET_IPV6_ADDR_LEN];
	char match[sizeof("#XDATAMODE: ")];
	int count = 0;
	int ret;

	ret = snprintk(request, sizeof(request), "AT#XSENDTO=\"%s\",%d", ip_str, port);
	if (ret < 0) {
		return ret;
	}

	strncpy(match, "OK", sizeof(match));

	modem_chat_script_chat_set_request(&data->sock_script_chat, request);
	modem_chat_match_set_match(&data->sock_match, match);
	modem_chat_match_set_separators(&data->sock_match, "");
	modem_chat_match_set_callback(&data->sock_match, NULL);

	/* #XDATAMODE can take up to 30 seconds to return */
	modem_chat_script_set_timeout(&data->sock_script, 35);

	/* Enter SLM data mode */
	ret = modem_chat_run_script(&data->chat, &data->sock_script);
	if (ret < 0) {
		LOG_ERR("failed to enter data mode (%d)", ret);
		return ret;
	}

	/* Send the payload directly on the pipe */
	LOG_HEXDUMP_DBG(buf, len, "sending: ");
	ret = modem_pipe_transmit(data->uart_pipe, buf, len);
	if (ret < 0) {
		LOG_WRN("failed to send data (%d)", ret);
	} else {
		count = ret;
	}

	/* TODO: The '+++' terminator is configurable in the modem.
	 * It should be configurable here as well.
	 */
	strncpy(request, "+++", sizeof(request));
	strncpy(match, "#XDATAMODE: ", sizeof(match));

	modem_chat_script_chat_set_request(&data->sock_script_chat, request);
	modem_chat_match_set_match(&data->sock_match, match);

	ret = modem_chat_run_script(&data->chat, &data->sock_script);
	if (ret < 0) {
		LOG_ERR("failed to exit data mode (%d)", ret);
		return ret;
	}

	return count;
}

static int nrf91_slm_xrecv(struct nrf91_slm_data *data, int timeout_s, int flags)
{
	/* AT#XRECV=<timeout>[,<flags>] */
	char request[sizeof("AT#XRECV=####,###")];
	struct modem_chat *chat = &data->chat;
	int ret;

	__ASSERT(timeout_s >= 0, "Timeout must be >= 0");

	/* The chat module only behaves nicely with "AT" like data. We're going to
	 * detach it for now and install our own callback.
	 */
	modem_chat_release(chat);
	modem_pipe_attach(data->uart_pipe, nrf91_slm_pipe_callback, data);

	ret = snprintk(request, sizeof(request), "AT#XRECV=%d,%d", timeout_s, flags);
	if (ret < 0) {
		goto reattach;
	}

	LOG_DBG("%.*s", ret, request);
	ret = modem_pipe_transmit(data->uart_pipe, request, ret);
	if (ret < 0) {
		goto reattach;
	}

	/* We are bypassing the chat module, so we need to send the delimiter */
	ret = modem_pipe_transmit(data->uart_pipe, chat->delimiter, chat->delimiter_size);
	if (ret < 0) {
		goto reattach;
	}

	ret = k_sem_take(&data->sock_sem, K_SECONDS(timeout_s + 1));
	if (ret < 0) {
		LOG_ERR("failed to take semaphore (%d)", ret);
		goto reattach;
	}

reattach:
	modem_chat_attach(chat, data->uart_pipe);
	return ret;
}

static int nrf91_slm_xrecvfrom(struct nrf91_slm_data *data, int timeout_s, int flags)
{
	/* AT#XRECVFROM=<timeout>[,<flags>] */
	char request[sizeof("AT#XRECVFROM=####,###")];
	struct modem_chat *chat = &data->chat;
	int ret;

	__ASSERT(timeout_s >= 0, "Timeout must be >= 0");

	/* The chat module only behaves nicely with "AT" like data. We're going to
	 * detach it for now and install our own callback.
	 */
	modem_chat_release(chat);
	modem_pipe_attach(data->uart_pipe, nrf91_slm_pipe_callback, data);

	ret = snprintk(request, sizeof(request), "AT#XRECVFROM=%d,%d", timeout_s, flags);
	if (ret < 0) {
		goto reattach;
	}

	LOG_DBG("%.*s", ret, request);
	ret = modem_pipe_transmit(data->uart_pipe, request, ret);
	if (ret < 0) {
		goto reattach;
	}

	/* We are bypassing the chat module, so we need to send the delimiter */
	ret = modem_pipe_transmit(data->uart_pipe, chat->delimiter, chat->delimiter_size);
	if (ret < 0) {
		goto reattach;
	}

	ret = k_sem_take(&data->sock_sem, K_SECONDS(timeout_s + 1));
	if (ret < 0) {
		LOG_ERR("failed to take semaphore (%d)", ret);
		goto reattach;
	}

reattach:
	modem_chat_attach(chat, data->uart_pipe);
	return ret;
}

static int nrf91_slm_xpoll(struct nrf91_slm_data *data, struct zsock_pollfd *fds, int nfds,
			   int timeout_ms)
{
	/* AT#XPOLL=<timeout>[,<handle>,...] */
	char request[64];
	struct modem_chat *chat = &data->chat;
	struct modem_socket *sock;
	int len;
	int ret;

	__ASSERT(fds != NULL, "fds must not be NULL");

	data->poll_fds = fds;
	data->poll_nfds = nfds;
	data->poll_count = 0;

	/* XPOLL returns a line for each socket handle, which chat does not like.
	 * We will detach the chat module and install our own callback.
	 */
	modem_chat_release(chat);
	modem_pipe_attach(data->uart_pipe, nrf91_slm_pipe_callback, data);

	/* Send AT#XPOLL=<timeout> */
	ret = snprintk(request, sizeof(request), "AT#XPOLL=%d", timeout_ms);
	if (ret < 0) {
		goto reattach;
	}

	len = ret;

	/* Translate the fds into socket ids and send them to the modem */
	for (int i = 0; i < nfds; i++) {
		sock = modem_socket_from_fd(&data->socket_config, fds[i].fd);
		if (!modem_socket_id_is_assigned(&data->socket_config, sock)) {
			continue;
		}

		ret = snprintk(request + len, sizeof(request) - len, ",%d", sock->id);
		if (ret < 0) {
			goto reattach;
		}

		len += ret;
	}

	LOG_DBG("%.*s", len, request);
	ret = modem_pipe_transmit(data->uart_pipe, request, len);
	if (ret < 0) {
		goto reattach;
	}

	/* Send the delimiter */
	ret = modem_pipe_transmit(data->uart_pipe, chat->delimiter, chat->delimiter_size);
	if (ret < 0) {
		goto reattach;
	}

	/* Wait for "OK" */
	ret = k_sem_take(&data->sock_sem, K_MSEC(timeout_ms + 500));
	if (ret < 0) {
		LOG_ERR("failed to take semaphore (%d)", ret);
		goto reattach;
	}

reattach:
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
	return ret;
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

	if (!modem_socket_id_is_assigned(&data->socket_config, sock)) {
		LOG_ERR("invalid socket_id (%d) from fd %d", sock->id, sock->sock_fd);
		errno = EINVAL;
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

	if (!modem_socket_id_is_assigned(&data->socket_config, sock)) {
		return 0;
	}

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
		} else if (sock->type == SOCK_DGRAM) {
			ret = nrf91_slm_xrecvfrom(data, 1, flags);
		} else {
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

	if (!modem_socket_id_is_assigned(&data->socket_config, sock)) {
		return 0;
	}

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
	} else if (sock->type == SOCK_DGRAM) {
		ret = nrf91_slm_xsendto(data, buf, len, ip_str, port);
	} else {
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

	if (!modem_socket_id_is_assigned(&data->socket_config, sock)) {
		return 0;
	}

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
