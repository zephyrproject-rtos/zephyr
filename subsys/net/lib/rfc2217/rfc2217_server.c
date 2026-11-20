/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/rfc2217.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/sys/ring_buffer.h>

LOG_MODULE_REGISTER(net_rfc2217_server, CONFIG_NET_RFC2217_SERVER_LOG_LEVEL);

#define FD_IPV4   0
#define FD_IPV6   1
#define FD_CLIENT 2

/* Room kept in the output buffer for replies */
#define REPLY_ROOM 64
/* Serial data escaped at once: up to twice its size once escaped */
#define CHUNK      ((CONFIG_NET_RFC2217_SERVER_OUT_SIZE - REPLY_ROOM) / 2)
/* Retry a send the socket could not take */
#define RETRY      K_MSEC(10)

BUILD_ASSERT(CHUNK >= 16, "CONFIG_NET_RFC2217_SERVER_OUT_SIZE is too small");

static struct rfc2217_server *server_of(struct rfc2217_session *session)
{
	return CONTAINER_OF(session, struct rfc2217_server, session);
}

static void uart_isr(const struct device *dev, void *user_data)
{
	struct rfc2217_server *s = user_data;

	while (true) {
		uart_irq_update(dev);
		if (uart_irq_is_pending(dev) <= 0) {
			break;
		}

		if (uart_irq_rx_ready(dev) > 0) {
			uint8_t *data;
			uint32_t len = ring_buf_put_ptr(s->rx, &data, 0);
			uint8_t drop[16];
			int n;

			if (len > 0) {
				n = uart_fifo_read(dev, data, len);
				if (n > 0) {
					ring_buf_commit(s->rx, n);
					k_work_reschedule(&s->work, K_NO_WAIT);
				}
			} else {
				(void)uart_fifo_read(dev, drop, sizeof(drop));
			}
		}

		if (uart_irq_tx_ready(dev) > 0) {
			uint8_t *data;
			uint32_t len = ring_buf_get_ptr(s->tx, &data, 0);
			int n;

			if (len > 0) {
				n = uart_fifo_fill(dev, data, len);
				if (n > 0) {
					ring_buf_consume(s->tx, n);
				}
			} else {
				uart_irq_tx_disable(dev);
				/* Room again for a client that was paused */
				k_work_reschedule(&s->work, K_NO_WAIT);
			}
		}
	}
}

static void discard(struct ring_buf *rb)
{
	ring_buf_consume(rb, ring_buf_size_get(rb));
}

static void update_service(struct rfc2217_server *s)
{
	struct zsock_pollfd *client = &s->fds[FD_CLIENT];
	int ret;

	if (client->fd >= 0 && client->events == 0 && ring_buf_space_get(s->tx) > 0) {
		client->events = ZSOCK_POLLIN;
	}

	if (memcmp(s->fds, s->registered, sizeof(s->fds)) == 0) {
		return;
	}

	ret = net_socket_service_register(s->service, s->fds, ARRAY_SIZE(s->fds), s);
	if (ret < 0) {
		LOG_ERR("Cannot register the sockets (%d)", ret);
		return;
	}
	memcpy(s->registered, s->fds, sizeof(s->fds));
}

static void close_client(struct rfc2217_server *s)
{
	if (s->fds[FD_CLIENT].fd < 0) {
		return;
	}

	(void)zsock_close(s->fds[FD_CLIENT].fd);
	s->fds[FD_CLIENT].fd = -1;
	s->fds[FD_CLIENT].events = 0;
	s->out_len = 0;
	s->out_off = 0;

	if (s->brk && s->cb != NULL && s->cb->set_break != NULL &&
	    s->cb->set_break(s, false) == 0) {
		s->brk = false;
	}

	LOG_DBG("Client disconnected");
}

static bool fill_out(struct rfc2217_server *s)
{
	uint8_t *data;
	uint32_t len = MIN(ring_buf_get_ptr(s->rx, &data, 0), CHUNK);
	size_t used;

	if (len == 0) {
		return false;
	}

	s->out_len = rfc2217_escape(data, len, s->out, 2 * CHUNK, &used);
	s->out_off = 0;
	ring_buf_consume(s->rx, used);

	return true;
}

static void send_out(struct rfc2217_server *s)
{
	int fd = s->fds[FD_CLIENT].fd;

	if (fd < 0) {
		discard(s->rx);
		return;
	}

	while (true) {
		ssize_t n;

		if (s->out_off == s->out_len && (s->suspended || !fill_out(s))) {
			s->out_len = 0;
			s->out_off = 0;
			return;
		}

		n = zsock_send(fd, &s->out[s->out_off], s->out_len - s->out_off,
			       ZSOCK_MSG_DONTWAIT);
		if (n < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				k_work_reschedule(&s->work, RETRY);
			} else {
				close_client(s);
			}
			return;
		}
		s->out_off += n;
	}
}

static void work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct rfc2217_server *s = CONTAINER_OF(dwork, struct rfc2217_server, work);

	k_mutex_lock(&s->lock, K_FOREVER);
	if (s->running) {
		send_out(s);
		update_service(s);
	}
	k_mutex_unlock(&s->lock);
}

/* Session callbacks, with the server's lock held */

static void on_data(struct rfc2217_session *session, const uint8_t *buf, size_t len)
{
	struct rfc2217_server *s = server_of(session);

	if (ring_buf_put(s->tx, buf, len) > 0) {
		uart_irq_tx_enable(s->uart);
	}
}

static void on_send(struct rfc2217_session *session, const uint8_t *buf, size_t len)
{
	struct rfc2217_server *s = server_of(session);

	if (s->out_off > 0) {
		memmove(s->out, &s->out[s->out_off], s->out_len - s->out_off);
		s->out_len -= s->out_off;
		s->out_off = 0;
	}
	if (s->out_len + len > sizeof(s->out)) {
		LOG_WRN("Reply dropped");
		return;
	}
	memcpy(&s->out[s->out_len], buf, len);
	s->out_len += len;
}

static int set_line(struct rfc2217_server *s, struct uart_config *cfg)
{
	int ret = uart_configure(s->uart, cfg);

	if (ret == 0) {
		s->line = *cfg;
	} else {
		LOG_DBG("Line settings refused (%d)", ret);
	}

	return ret;
}

static uint32_t on_baud_rate(struct rfc2217_session *session, uint32_t baud_rate)
{
	struct rfc2217_server *s = server_of(session);
	struct uart_config cfg = s->line;

	if (baud_rate != 0) {
		cfg.baudrate = baud_rate;
		(void)set_line(s, &cfg);
	}

	return s->line.baudrate;
}

static uint8_t on_data_size(struct rfc2217_session *session, uint8_t data_size)
{
	struct rfc2217_server *s = server_of(session);
	struct uart_config cfg = s->line;

	if (data_size >= 5 && data_size <= 8) {
		cfg.data_bits = UART_CFG_DATA_BITS_5 + (data_size - 5);
		(void)set_line(s, &cfg);
	}

	return 5 + MIN(s->line.data_bits, UART_CFG_DATA_BITS_8);
}

static uint8_t on_parity(struct rfc2217_session *session, uint8_t parity)
{
	struct rfc2217_server *s = server_of(session);
	struct uart_config cfg = s->line;

	/* The RFC 2217 values are the UART API's plus one */
	if (parity >= RFC2217_PARITY_NONE && parity <= RFC2217_PARITY_SPACE) {
		cfg.parity = parity - RFC2217_PARITY_NONE;
		(void)set_line(s, &cfg);
	}

	return s->line.parity + RFC2217_PARITY_NONE;
}

static uint8_t on_stop_size(struct rfc2217_session *session, uint8_t stop_size)
{
	static const uint8_t to_uart[] = {
		[RFC2217_STOP_SIZE_1] = UART_CFG_STOP_BITS_1,
		[RFC2217_STOP_SIZE_2] = UART_CFG_STOP_BITS_2,
		[RFC2217_STOP_SIZE_1_5] = UART_CFG_STOP_BITS_1_5,
	};
	struct rfc2217_server *s = server_of(session);
	struct uart_config cfg = s->line;

	if (stop_size >= RFC2217_STOP_SIZE_1 && stop_size <= RFC2217_STOP_SIZE_1_5) {
		cfg.stop_bits = to_uart[stop_size];
		(void)set_line(s, &cfg);
	}

	switch (s->line.stop_bits) {
	case UART_CFG_STOP_BITS_2:
		return RFC2217_STOP_SIZE_2;
	case UART_CFG_STOP_BITS_1_5:
		return RFC2217_STOP_SIZE_1_5;
	default:
		return RFC2217_STOP_SIZE_1;
	}
}

static bool set_modem_line(struct rfc2217_server *s, uint32_t line, bool on)
{
	int ret;

	if (s->cb != NULL && s->cb->set_line != NULL) {
		ret = s->cb->set_line(s, line, on);
	} else {
		ret = uart_line_ctrl_set(s->uart, line, on ? 1 : 0);
	}

	return ret == 0;
}

static uint8_t on_control(struct rfc2217_session *session, uint8_t control)
{
	struct rfc2217_server *s = server_of(session);
	bool hw_flow = s->line.flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS;
	struct uart_config cfg = s->line;

	switch (control) {
	case RFC2217_CONTROL_FLOW_NONE:
	case RFC2217_CONTROL_FLOW_HARDWARE:
		cfg.flow_ctrl = control == RFC2217_CONTROL_FLOW_HARDWARE
					? UART_CFG_FLOW_CTRL_RTS_CTS
					: UART_CFG_FLOW_CTRL_NONE;
		if (set_line(s, &cfg) == 0) {
			hw_flow = control == RFC2217_CONTROL_FLOW_HARDWARE;
		}
		return hw_flow ? RFC2217_CONTROL_FLOW_HARDWARE : RFC2217_CONTROL_FLOW_NONE;
	case RFC2217_CONTROL_BREAK_ON:
	case RFC2217_CONTROL_BREAK_OFF:
		if (s->cb != NULL && s->cb->set_break != NULL &&
		    s->cb->set_break(s, control == RFC2217_CONTROL_BREAK_ON) == 0) {
			s->brk = control == RFC2217_CONTROL_BREAK_ON;
		}
		__fallthrough;
	case RFC2217_CONTROL_BREAK_REQUEST:
		return s->brk ? RFC2217_CONTROL_BREAK_ON : RFC2217_CONTROL_BREAK_OFF;
	case RFC2217_CONTROL_DTR_ON:
	case RFC2217_CONTROL_DTR_OFF:
		if (set_modem_line(s, UART_LINE_CTRL_DTR, control == RFC2217_CONTROL_DTR_ON)) {
			s->dtr = control == RFC2217_CONTROL_DTR_ON;
		}
		__fallthrough;
	case RFC2217_CONTROL_DTR_REQUEST:
		return s->dtr ? RFC2217_CONTROL_DTR_ON : RFC2217_CONTROL_DTR_OFF;
	case RFC2217_CONTROL_RTS_ON:
	case RFC2217_CONTROL_RTS_OFF:
		if (set_modem_line(s, UART_LINE_CTRL_RTS, control == RFC2217_CONTROL_RTS_ON)) {
			s->rts = control == RFC2217_CONTROL_RTS_ON;
		}
		__fallthrough;
	case RFC2217_CONTROL_RTS_REQUEST:
		return s->rts ? RFC2217_CONTROL_RTS_ON : RFC2217_CONTROL_RTS_OFF;
	case RFC2217_CONTROL_INBOUND_FLOW_REQUEST:
	case RFC2217_CONTROL_INBOUND_FLOW_NONE:
	case RFC2217_CONTROL_INBOUND_FLOW_XON_XOFF:
	case RFC2217_CONTROL_INBOUND_FLOW_HARDWARE:
	case RFC2217_CONTROL_INBOUND_FLOW_DTR:
		/* Inbound flow control follows the outbound setting */
		return hw_flow ? RFC2217_CONTROL_INBOUND_FLOW_HARDWARE
			       : RFC2217_CONTROL_INBOUND_FLOW_NONE;
	default:
		return hw_flow ? RFC2217_CONTROL_FLOW_HARDWARE : RFC2217_CONTROL_FLOW_NONE;
	}
}

static void on_purge(struct rfc2217_session *session, uint8_t purge)
{
	struct rfc2217_server *s = server_of(session);

	if (purge & RFC2217_PURGE_RX) {
		discard(s->rx);
	}
	if (purge & RFC2217_PURGE_TX) {
		uart_irq_tx_disable(s->uart);
		ring_buf_reset(s->tx);
	}
}

static void on_suspend(struct rfc2217_session *session, bool suspend)
{
	struct rfc2217_server *s = server_of(session);

	s->suspended = suspend;
	if (!suspend) {
		k_work_reschedule(&s->work, K_NO_WAIT);
	}
}

static uint8_t on_modem_state(struct rfc2217_session *session)
{
	struct rfc2217_server *s = server_of(session);
	uint8_t state = 0;
	uint32_t val;

	if (uart_line_ctrl_get(s->uart, UART_LINE_CTRL_DCD, &val) == 0 && val != 0) {
		state |= RFC2217_MODEM_CD;
	}
	if (uart_line_ctrl_get(s->uart, UART_LINE_CTRL_DSR, &val) == 0 && val != 0) {
		state |= RFC2217_MODEM_DSR;
	}

	return state;
}

static const struct rfc2217_session_ops session_ops = {
	.data = on_data,
	.send = on_send,
	.baud_rate = on_baud_rate,
	.data_size = on_data_size,
	.parity = on_parity,
	.stop_size = on_stop_size,
	.control = on_control,
	.purge = on_purge,
	.suspend = on_suspend,
	.modem_state = on_modem_state,
};

/* Socket service */

static void accept_client(struct rfc2217_server *s, int listener)
{
	struct net_sockaddr_storage addr;
	net_socklen_t addr_len = sizeof(addr);
	int fd = zsock_accept(listener, net_sad(&addr), &addr_len);

	if (fd < 0) {
		LOG_ERR("Accept failed (%d)", -errno);
		return;
	}

	if (s->fds[FD_CLIENT].fd >= 0) {
		LOG_INF("New client replaces the current one");
		close_client(s);
	}

	s->fds[FD_CLIENT].fd = fd;
	s->fds[FD_CLIENT].events = ZSOCK_POLLIN;
	s->suspended = false;
	discard(s->rx);
	rfc2217_session_init(&s->session, &session_ops, s);

	LOG_DBG("Client connected");
}

static void read_client(struct rfc2217_server *s)
{
	struct zsock_pollfd *client = &s->fds[FD_CLIENT];
	uint8_t buf[64];
	uint32_t room = ring_buf_space_get(s->tx);
	ssize_t n;

	if (room == 0) {
		/* Wait for the UART to take what it has */
		client->events = 0;
		return;
	}

	n = zsock_recv(client->fd, buf, MIN(room, sizeof(buf)), ZSOCK_MSG_DONTWAIT);
	if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
		close_client(s);
		return;
	}
	if (n > 0) {
		rfc2217_session_input(&s->session, buf, n);
	}
	if (ring_buf_space_get(s->tx) == 0) {
		client->events = 0;
	}
}

void z_rfc2217_server_service(struct net_socket_service_event *pev)
{
	struct rfc2217_server *s = pev->user_data;
	int fd = pev->event.fd;

	k_mutex_lock(&s->lock, K_FOREVER);

	if (!s->running) {
		goto out;
	}

	if (fd == s->fds[FD_IPV4].fd || fd == s->fds[FD_IPV6].fd) {
		accept_client(s, fd);
	} else if (fd == s->fds[FD_CLIENT].fd) {
		if (pev->event.revents & ZSOCK_POLLIN) {
			read_client(s);
		} else if (pev->event.revents & (ZSOCK_POLLERR | ZSOCK_POLLHUP | ZSOCK_POLLNVAL)) {
			close_client(s);
		}
	}

	send_out(s);
	update_service(s);
out:
	k_mutex_unlock(&s->lock);
}

static int listen_on(struct zsock_pollfd *pfd, net_sa_family_t family, uint16_t port)
{
	struct net_sockaddr_storage addr = {0};
	net_socklen_t addr_len;
	int reuse = 1;
	int ret;

	if (family == NET_AF_INET) {
		struct net_sockaddr_in *in = (struct net_sockaddr_in *)&addr;

		in->sin_family = NET_AF_INET;
		in->sin_port = net_htons(port);
		addr_len = sizeof(*in);
	} else {
		struct net_sockaddr_in6 *in6 = (struct net_sockaddr_in6 *)&addr;

		in6->sin6_family = NET_AF_INET6;
		in6->sin6_port = net_htons(port);
		addr_len = sizeof(*in6);
	}

	pfd->fd = zsock_socket(family, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	if (pfd->fd < 0) {
		return -errno;
	}

	(void)zsock_setsockopt(pfd->fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_REUSEADDR, &reuse,
			       sizeof(reuse));
	if (zsock_bind(pfd->fd, net_sad(&addr), addr_len) < 0 || zsock_listen(pfd->fd, 1) < 0) {
		ret = -errno;
		(void)zsock_close(pfd->fd);
		pfd->fd = -1;
		return ret;
	}
	pfd->events = ZSOCK_POLLIN;

	return 0;
}

static void close_listeners(struct rfc2217_server *s)
{
	for (int i = FD_IPV4; i <= FD_IPV6; i++) {
		if (s->fds[i].fd >= 0) {
			(void)zsock_close(s->fds[i].fd);
			s->fds[i].fd = -1;
		}
	}
}

int rfc2217_server_start(struct rfc2217_server *server, const struct rfc2217_server_config *config)
{
	static const struct uart_config line = {
		.baudrate = 115200,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
	};
	int ret = -EAFNOSUPPORT;

	if (server->running) {
		return -EALREADY;
	}
	if (!device_is_ready(config->uart)) {
		return -ENODEV;
	}

	k_mutex_init(&server->lock);
	k_work_init_delayable(&server->work, work_handler);

	server->uart = config->uart;
	server->port = config->port;
	if (uart_config_get(server->uart, &server->line) < 0) {
		server->line = line;
	}

	for (int i = 0; i < ARRAY_SIZE(server->fds); i++) {
		server->fds[i] = (struct zsock_pollfd){.fd = -1};
		server->registered[i] = server->fds[i];
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		ret = listen_on(&server->fds[FD_IPV4], NET_AF_INET, config->port);
	}
	if (IS_ENABLED(CONFIG_NET_IPV6) &&
	    listen_on(&server->fds[FD_IPV6], NET_AF_INET6, config->port) == 0) {
		ret = 0;
	}
	if (ret < 0) {
		LOG_ERR("Cannot listen on port %u (%d)", config->port, ret);
		close_listeners(server);
		return ret;
	}

	ret = uart_irq_callback_user_data_set(server->uart, uart_isr, server);
	if (ret < 0) {
		close_listeners(server);
		return ret;
	}

	ring_buf_reset(server->rx);
	ring_buf_reset(server->tx);
	server->out_len = 0;
	server->out_off = 0;

	k_mutex_lock(&server->lock, K_FOREVER);
	server->running = true;
	uart_irq_rx_enable(server->uart);
	update_service(server);
	k_mutex_unlock(&server->lock);

	LOG_DBG("Listening on port %u", config->port);

	return 0;
}

int rfc2217_server_stop(struct rfc2217_server *server)
{
	struct k_work_sync sync;

	k_mutex_lock(&server->lock, K_FOREVER);

	if (!server->running) {
		k_mutex_unlock(&server->lock);
		return -EALREADY;
	}

	server->running = false;
	uart_irq_rx_disable(server->uart);
	uart_irq_tx_disable(server->uart);
	(void)uart_irq_callback_user_data_set(server->uart, NULL, NULL);
	(void)net_socket_service_unregister(server->service);
	close_client(server);
	close_listeners(server);

	k_mutex_unlock(&server->lock);

	(void)k_work_cancel_delayable_sync(&server->work, &sync);

	return 0;
}

void rfc2217_server_set_callbacks(struct rfc2217_server *server,
				  const struct rfc2217_server_callbacks *cb, void *user_data)
{
	server->cb = cb;
	server->user_data = user_data;
}

#define DT_DRV_COMPAT zephyr_rfc2217_server

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define SERVER_DEFINE(name) Z_RFC2217_SERVER_DEFINE(name)

#define SERVER_DT_DEFINE(inst) SERVER_DEFINE(Z_RFC2217_SERVER_DT_NAME(DT_DRV_INST(inst)));

DT_INST_FOREACH_STATUS_OKAY(SERVER_DT_DEFINE)

#define SERVER_DT_ENTRY(inst)                                                                      \
	{                                                                                          \
		.server = RFC2217_SERVER_DT_GET(DT_DRV_INST(inst)),                                \
		.config =                                                                          \
			{                                                                          \
				.uart = DEVICE_DT_GET(DT_INST_PHANDLE(inst, uart)),                \
				.port = DT_INST_PROP(inst, port),                                  \
			},                                                                         \
	},

static int rfc2217_server_dt_init(void)
{
	static const struct {
		struct rfc2217_server *server;
		struct rfc2217_server_config config;
	} servers[] = {DT_INST_FOREACH_STATUS_OKAY(SERVER_DT_ENTRY)};

	ARRAY_FOR_EACH_PTR(servers, entry) {
		int ret = rfc2217_server_start(entry->server, &entry->config);

		if (ret < 0) {
			LOG_ERR("Server on port %u not started (%d)", entry->config.port, ret);
		}
	}

	return 0;
}

SYS_INIT(rfc2217_server_dt_init, APPLICATION, CONFIG_NET_RFC2217_SERVER_INIT_PRIORITY);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
