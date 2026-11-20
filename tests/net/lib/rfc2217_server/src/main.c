/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The RFC 2217 server on emulated UARTs, with clients over the loopback
 * interface: a server defined in the devicetree on euart0, and one defined
 * with RFC2217_SERVER_DEFINE() and started by the test on euart1.
 */

#include <string.h>

#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/net/rfc2217.h>
#include <zephyr/net/socket.h>
#include <zephyr/ztest.h>

#define IAC      0xff
#define WILL     0xfb
#define DO       0xfd
#define SB       0xfa
#define SE       0xf0
#define COM_PORT 44

#define DT_SERVER DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_rfc2217_server)
#define DT_PORT   DT_PROP(DT_SERVER, port)
#define API_PORT  (DT_PORT + 1)

static const struct device *const dt_uart = DEVICE_DT_GET(DT_PHANDLE(DT_SERVER, uart));
static const struct device *const api_uart = DEVICE_DT_GET(DT_NODELABEL(euart1));
static struct rfc2217_server *const dt_server = RFC2217_SERVER_DT_GET(DT_SERVER);

RFC2217_SERVER_DEFINE(api_server);

static const uint8_t opening[] = {IAC, WILL, 1, IAC, WILL, 3, IAC, DO, 0, IAC, WILL, COM_PORT};

static struct {
	int breaks;
	bool brk;
	uint32_t line;
	bool line_on;
	void *user_data;
} calls;

static int set_break(struct rfc2217_server *server, bool on)
{
	calls.breaks++;
	calls.brk = on;
	calls.user_data = server->user_data;

	return 0;
}

static int set_line(struct rfc2217_server *server, uint32_t line, bool on)
{
	calls.line = line;
	calls.line_on = on;

	return 0;
}

static const struct rfc2217_server_callbacks callbacks = {
	.set_break = set_break,
	.set_line = set_line,
};

static int client(uint16_t port)
{
	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(port),
	};
	struct zsock_timeval timeout = {.tv_sec = 2};
	int fd = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);

	zassert_true(fd >= 0, "socket: %d", errno);
	zassert_equal(zsock_inet_pton(NET_AF_INET, "127.0.0.1", &addr.sin_addr), 1);
	zassert_ok(zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &timeout,
				    sizeof(timeout)));
	zassert_ok(zsock_connect(fd, (struct net_sockaddr *)&addr, sizeof(addr)), "connect: %d",
		   errno);

	return fd;
}

static void expect(int fd, const uint8_t *data, size_t len)
{
	uint8_t buf[64];
	size_t got = 0;

	zassert_true(len <= sizeof(buf));
	while (got < len) {
		ssize_t n = zsock_recv(fd, &buf[got], len - got, 0);

		zassert_true(n > 0, "recv: %zd (%d) after %zu bytes", n, errno, got);
		got += n;
	}
	zassert_mem_equal(buf, data, len);
}

#define EXPECT(fd, ...)                                                                            \
	do {                                                                                       \
		const uint8_t _exp[] = {__VA_ARGS__};                                              \
		expect(fd, _exp, sizeof(_exp));                                                    \
	} while (false)

#define SEND(fd, ...)                                                                              \
	do {                                                                                       \
		const uint8_t _out[] = {__VA_ARGS__};                                              \
		zassert_equal(zsock_send(fd, _out, sizeof(_out), 0), sizeof(_out));                \
	} while (false)

static int session(uint16_t port)
{
	int fd = client(port);

	expect(fd, opening, sizeof(opening));

	return fd;
}

static void expect_closed(int fd)
{
	uint8_t c;

	zassert_true(zsock_recv(fd, &c, 1, 0) <= 0, "connection still open");
}

static void expect_uart_tx(const struct device *uart, const uint8_t *data, size_t len)
{
	uint8_t buf[64];
	size_t got = 0;

	for (int i = 0; i < 100 && got < len; i++) {
		got += uart_emul_get_tx_data(uart, &buf[got], sizeof(buf) - got);
		k_msleep(10);
	}
	zassert_equal(got, len, "UART sent %zu bytes", got);
	zassert_mem_equal(buf, data, len);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(&calls, 0, sizeof(calls));
	rfc2217_server_set_callbacks(dt_server, NULL, NULL);
	uart_emul_flush_rx_data(dt_uart);
	uart_emul_flush_tx_data(dt_uart);
	uart_emul_flush_rx_data(api_uart);
	uart_emul_flush_tx_data(api_uart);
}

ZTEST(rfc2217_server, test_data)
{
	const uint8_t to_client[] = {'c', IAC, 'd'};
	const uint8_t to_uart[] = {'a', IAC, 'b'};
	int fd = session(DT_PORT);

	/* 0xff doubled on the connection, single on the UART */
	SEND(fd, 'a', IAC, IAC, 'b');
	expect_uart_tx(dt_uart, to_uart, sizeof(to_uart));

	uart_emul_put_rx_data(dt_uart, to_client, sizeof(to_client));
	EXPECT(fd, 'c', IAC, IAC, 'd');

	zsock_close(fd);
}

ZTEST(rfc2217_server, test_line_settings)
{
	struct uart_config cfg;
	int fd = session(DT_PORT);

	SEND(fd, IAC, SB, COM_PORT, 1, 0x00, 0x00, 0x25, 0x80, IAC, SE);
	EXPECT(fd, IAC, SB, COM_PORT, 101, 0x00, 0x00, 0x25, 0x80, IAC, SE);
	SEND(fd, IAC, SB, COM_PORT, 2, 7, IAC, SE);
	EXPECT(fd, IAC, SB, COM_PORT, 102, 7, IAC, SE);
	SEND(fd, IAC, SB, COM_PORT, 3, RFC2217_PARITY_EVEN, IAC, SE);
	EXPECT(fd, IAC, SB, COM_PORT, 103, RFC2217_PARITY_EVEN, IAC, SE);
	SEND(fd, IAC, SB, COM_PORT, 4, RFC2217_STOP_SIZE_2, IAC, SE);
	EXPECT(fd, IAC, SB, COM_PORT, 104, RFC2217_STOP_SIZE_2, IAC, SE);

	zassert_ok(uart_config_get(dt_uart, &cfg));
	zassert_equal(cfg.baudrate, 9600);
	zassert_equal(cfg.data_bits, UART_CFG_DATA_BITS_7);
	zassert_equal(cfg.parity, UART_CFG_PARITY_EVEN);
	zassert_equal(cfg.stop_bits, UART_CFG_STOP_BITS_2);

	/* A zero asks for the current value */
	SEND(fd, IAC, SB, COM_PORT, 1, 0, 0, 0, 0, IAC, SE);
	EXPECT(fd, IAC, SB, COM_PORT, 101, 0x00, 0x00, 0x25, 0x80, IAC, SE);

	zsock_close(fd);
}

ZTEST(rfc2217_server, test_break_without_callback)
{
	int fd = session(DT_PORT);

	SEND(fd, IAC, SB, COM_PORT, 5, RFC2217_CONTROL_BREAK_ON, IAC, SE);
	EXPECT(fd, IAC, SB, COM_PORT, 105, RFC2217_CONTROL_BREAK_OFF, IAC, SE);

	zsock_close(fd);
}

ZTEST(rfc2217_server, test_callbacks)
{
	int marker;
	int fd;

	rfc2217_server_set_callbacks(dt_server, &callbacks, &marker);
	fd = session(DT_PORT);

	SEND(fd, IAC, SB, COM_PORT, 5, RFC2217_CONTROL_BREAK_ON, IAC, SE);
	EXPECT(fd, IAC, SB, COM_PORT, 105, RFC2217_CONTROL_BREAK_ON, IAC, SE);
	zassert_true(calls.brk);
	zassert_equal(calls.user_data, &marker);

	SEND(fd, IAC, SB, COM_PORT, 5, RFC2217_CONTROL_DTR_ON, IAC, SE);
	EXPECT(fd, IAC, SB, COM_PORT, 105, RFC2217_CONTROL_DTR_ON, IAC, SE);
	zassert_equal(calls.line, UART_LINE_CTRL_DTR);
	zassert_true(calls.line_on);

	SEND(fd, IAC, SB, COM_PORT, 5, RFC2217_CONTROL_RTS_OFF, IAC, SE);
	EXPECT(fd, IAC, SB, COM_PORT, 105, RFC2217_CONTROL_RTS_OFF, IAC, SE);
	zassert_equal(calls.line, UART_LINE_CTRL_RTS);
	zassert_false(calls.line_on);

	/* A client leaving releases its BREAK */
	zsock_close(fd);
	for (int i = 0; i < 100 && calls.brk; i++) {
		k_msleep(10);
	}
	zassert_false(calls.brk, "BREAK still on");
	zassert_equal(calls.breaks, 2);
}

ZTEST(rfc2217_server, test_suspend)
{
	const uint8_t data[] = {'x'};
	struct zsock_timeval timeout = {.tv_usec = 300000};
	uint8_t c;
	int fd = session(DT_PORT);

	SEND(fd, IAC, SB, COM_PORT, 8, IAC, SE);
	k_msleep(100);
	uart_emul_put_rx_data(dt_uart, data, sizeof(data));

	zassert_ok(zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &timeout,
				    sizeof(timeout)));
	zassert_true(zsock_recv(fd, &c, 1, 0) < 0, "data sent while suspended");

	SEND(fd, IAC, SB, COM_PORT, 9, IAC, SE);
	EXPECT(fd, 'x');

	zsock_close(fd);
}

ZTEST(rfc2217_server, test_new_client_replaces)
{
	const uint8_t data[] = {'y'};
	int first = session(DT_PORT);
	int second = session(DT_PORT);

	expect_closed(first);
	uart_emul_put_rx_data(dt_uart, data, sizeof(data));
	EXPECT(second, 'y');

	zsock_close(first);
	zsock_close(second);
}

ZTEST(rfc2217_server, test_api_server)
{
	const struct rfc2217_server_config config = {
		.uart = api_uart,
		.port = API_PORT,
	};
	const uint8_t to_uart[] = {'s'};
	const uint8_t to_client[] = {'u'};
	int fd;

	zassert_ok(rfc2217_server_start(&api_server, &config));
	zassert_equal(rfc2217_server_start(&api_server, &config), -EALREADY);

	fd = session(API_PORT);
	SEND(fd, 's');
	expect_uart_tx(api_uart, to_uart, sizeof(to_uart));
	uart_emul_put_rx_data(api_uart, to_client, sizeof(to_client));
	EXPECT(fd, 'u');

	/* Stopping closes the client and the port */
	zassert_ok(rfc2217_server_stop(&api_server));
	zassert_equal(rfc2217_server_stop(&api_server), -EALREADY);
	expect_closed(fd);
	zsock_close(fd);

	fd = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	{
		struct net_sockaddr_in addr = {
			.sin_family = NET_AF_INET,
			.sin_port = net_htons(API_PORT),
		};

		zsock_inet_pton(NET_AF_INET, "127.0.0.1", &addr.sin_addr);
		zassert_true(zsock_connect(fd, (struct net_sockaddr *)&addr, sizeof(addr)) < 0,
			     "port still open");
	}
	zsock_close(fd);

	/* And it starts again */
	zassert_ok(rfc2217_server_start(&api_server, &config));
	fd = session(API_PORT);
	zsock_close(fd);
	zassert_ok(rfc2217_server_stop(&api_server));
}

ZTEST_SUITE(rfc2217_server, NULL, NULL, before, NULL, NULL);
