/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * End-to-end tests of the MCUmgr UART transports on vnd,serial test UARTs: requests are queued
 * as received data and the packets each UART transmits are read back. The scenarios use the
 * zephyr,uart-mcumgr chosen UART alone, or zephyr,smp-uart and zephyr,smp-uart-raw nodes with
 * the chosen UART still set, which must then be ignored.
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart/serial_test.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>
#include <zephyr/mgmt/mcumgr/transport/serial.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <mgmt/mcumgr/transport/smp_internal.h>
#include "smp_test_util.h"

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT)
#include <zephyr/mgmt/mcumgr/grp/transport_mgmt/transport_mgmt.h>
#endif

#define RESPONSE_TIMEOUT_MS 1000
#define QUIET_PERIOD_MS     100
#define MAX_PACKETS         4
#define PACKET_BUFFER_SIZE  256
#define TX_DATA_SIZE        1024

/* With nodes of enabled transports, the transports must ignore the chosen UART */
#define HAS_NODES DT_NODE_EXISTS(DT_NODELABEL(uart_console0))

struct test_uart {
	const struct device *dev;
	/* Raw framing, else SMP over console */
	bool raw;
	/* Whether a transport is expected to run on the UART */
	bool served;
	/* Data the UART transmitted that is not parsed into packets yet */
	uint8_t tx_data[TX_DATA_SIZE];
	size_t tx_data_len;
	/* Decodes SMP over console frames */
	struct mcumgr_serial_rx_ctxt rx_ctxt;
	/* Packets parsed from the transmitted data, header included */
	uint8_t packets[MAX_PACKETS][CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE];
	size_t packet_len[MAX_PACKETS];
	int packet_count;
};

#define TEST_UART(label, _raw, _served)							\
	{										\
		.dev = DEVICE_DT_GET(DT_NODELABEL(label)),				\
		.raw = (_raw),								\
		.served = (_served),							\
	},

static struct test_uart test_uarts[] = {
	TEST_UART(uart_chosen, IS_ENABLED(CONFIG_UART_MCUMGR_RAW_PROTOCOL), !HAS_NODES)
#if DT_NODE_EXISTS(DT_NODELABEL(uart_console0))
	TEST_UART(uart_console0, false, true)
	TEST_UART(uart_raw0, true, true)
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(uart_console1))
	TEST_UART(uart_console1, false, true)
	TEST_UART(uart_raw1, true, true)
#endif
};

#if HAS_NODES
#define CONSOLE0 (&test_uarts[1])
#define RAW0     (&test_uarts[2])
#endif

static void store_packet(struct test_uart *uart, const uint8_t *data, size_t len)
{
	zassert_true(uart->packet_count < MAX_PACKETS, "%s: too many packets", uart->dev->name);
	zassert_true(len <= sizeof(uart->packets[0]), "%s: packet too large", uart->dev->name);
	memcpy(uart->packets[uart->packet_count], data, len);
	uart->packet_len[uart->packet_count] = len;
	++uart->packet_count;
}

static void consume_tx_data(struct test_uart *uart, size_t len)
{
	memmove(uart->tx_data, &uart->tx_data[len], uart->tx_data_len - len);
	uart->tx_data_len -= len;
}

/* Raw packets are delimited by the length in their header */
static void parse_raw(struct test_uart *uart)
{
	while (uart->tx_data_len >= sizeof(struct smp_hdr)) {
		struct smp_hdr hdr;
		size_t total;

		memcpy(&hdr, uart->tx_data, sizeof(hdr));
		total = sizeof(hdr) + sys_be16_to_cpu(hdr.nh_len);
		if (uart->tx_data_len < total) {
			break;
		}

		store_packet(uart, uart->tx_data, total);
		consume_tx_data(uart, total);
	}
}

#if defined(CONFIG_MCUMGR_TRANSPORT_UART)
/* SMP over console frames end in a newline */
static void parse_console(struct test_uart *uart)
{
	uint8_t *end;

	while ((end = memchr(uart->tx_data, '\n', uart->tx_data_len)) != NULL) {
		size_t frame_len = end - uart->tx_data + 1;
		struct net_buf *nb;

		nb = mcumgr_serial_process_frag(&uart->rx_ctxt, uart->tx_data, frame_len);
		if (nb != NULL) {
			store_packet(uart, nb->data, nb->len);
			smp_packet_free(nb);
		}

		consume_tx_data(uart, frame_len);
	}
}

struct frame_buffer {
	uint8_t data[TX_DATA_SIZE];
	size_t len;
};

static int frames_append(const void *data, int len, void *ctx)
{
	struct frame_buffer *frames = ctx;

	zassert_true(frames->len + len <= sizeof(frames->data), "Frame buffer too small");
	memcpy(&frames->data[frames->len], data, len);
	frames->len += len;

	return 0;
}
#endif

static void read_packets(struct test_uart *uart)
{
	uart->tx_data_len += serial_vnd_read_out_data(uart->dev, &uart->tx_data[uart->tx_data_len],
						      sizeof(uart->tx_data) - uart->tx_data_len);

	if (uart->raw) {
		parse_raw(uart);
	} else {
#if defined(CONFIG_MCUMGR_TRANSPORT_UART)
		parse_console(uart);
#endif
	}
}

/*
 * Reads what the UART transmits until it has sent @p count packets or the response timeout
 * passes, then for a quiet period to catch extra packets, and returns the packet count.
 */
static int collect_packets(struct test_uart *uart, int count)
{
	int64_t end = k_uptime_get() + RESPONSE_TIMEOUT_MS;

	while (uart->packet_count < count && k_uptime_get() < end) {
		read_packets(uart);
		k_msleep(1);
	}

	k_msleep(QUIET_PERIOD_MS);
	read_packets(uart);

	return uart->packet_count;
}

/* Queues a packet as received data of the UART, in the UART's framing */
static void send_packet(struct test_uart *uart, const uint8_t *packet, size_t len)
{
	const uint8_t *data = packet;

#if defined(CONFIG_MCUMGR_TRANSPORT_UART)
	static struct frame_buffer frames;

	if (!uart->raw) {
		frames.len = 0;
		zassert_ok(mcumgr_serial_tx_pkt(packet, len, frames_append, &frames),
			   "Failed to encode packet");
		data = frames.data;
		len = frames.len;
	}
#endif

	zassert_equal(serial_vnd_queue_in_data(uart->dev, data, len), len, "%s: receive queue full",
		      uart->dev->name);
}

ZTEST(transport_uart, test_echo)
{
	uint8_t request[PACKET_BUFFER_SIZE];
	size_t len;

	/* A request to every UART before reading any, each with its own sequence number */
	ARRAY_FOR_EACH(test_uarts, i) {
		len = smp_test_build_echo_request(request, sizeof(request), TEST_ECHO_STRING,
						  i + 1);
		send_packet(&test_uarts[i], request, len);
	}

	ARRAY_FOR_EACH(test_uarts, i) {
		struct test_uart *uart = &test_uarts[i];

		if (!uart->served) {
			collect_packets(uart, 0);
			zassert_true(uart->tx_data_len == 0 && uart->packet_count == 0,
				     "%s: expected no data", uart->dev->name);
			continue;
		}

		zassert_equal(collect_packets(uart, 1), 1, "%s: expected one response",
			      uart->dev->name);
		smp_test_check_echo_response(uart->packets[0], uart->packet_len[0],
					     TEST_ECHO_STRING, i + 1);
	}
}

#if defined(CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT)
/* Long enough for the input timeout to have passed */
#define INPUT_TIMEOUT_WAIT_MS (CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT_TIME_MS * 3)

ZTEST(transport_uart, test_raw_input_timeout)
{
	uint8_t request[PACKET_BUFFER_SIZE];
	size_t len;

	/* An incomplete request on each raw UART, to be discarded once the input timeout passes */
	len = smp_test_build_echo_request(request, sizeof(request), TEST_ECHO_STRING, 1);
	ARRAY_FOR_EACH_PTR(test_uarts, uart) {
		if (uart->served && uart->raw) {
			send_packet(uart, request, len - 4);
		}
	}

	k_msleep(INPUT_TIMEOUT_WAIT_MS);

	len = smp_test_build_echo_request(request, sizeof(request), TEST_ECHO_STRING, 2);
	ARRAY_FOR_EACH_PTR(test_uarts, uart) {
		if (uart->served && uart->raw) {
			send_packet(uart, request, len);
			zassert_equal(collect_packets(uart, 1), 1, "%s: expected one response",
				      uart->dev->name);
			smp_test_check_echo_response(uart->packets[0], uart->packet_len[0],
						     TEST_ECHO_STRING, 2);
		}
	}
}
#endif

#if HAS_NODES
static K_SEM_DEFINE(sysworkq_blocked, 0, 1);
static K_SEM_DEFINE(sysworkq_release, 0, 1);
static bool sysworkq_held;

static void block_sysworkq(struct k_work *work)
{
	ARG_UNUSED(work);

	k_sem_give(&sysworkq_blocked);
	k_sem_take(&sysworkq_release, K_FOREVER);
}

static K_WORK_DEFINE(sysworkq_blocker, block_sysworkq);

static void release_sysworkq(void)
{
	if (sysworkq_held) {
		k_sem_give(&sysworkq_release);
		sysworkq_held = false;
	}
}

ZTEST(transport_uart, test_rx_pool_isolation)
{
	uint8_t request[PACKET_BUFFER_SIZE];
	size_t len;

	/* Received SMP over console lines wait in receive buffers for the system workqueue */
	k_work_submit(&sysworkq_blocker);
	zassert_ok(k_sem_take(&sysworkq_blocked, K_SECONDS(1)), "System workqueue not blocked");
	sysworkq_held = true;

	/* One line more than the console UART has receive buffers for */
	for (int i = 0; i <= CONFIG_UART_MCUMGR_RX_BUF_COUNT; i++) {
		len = smp_test_build_echo_request(request, sizeof(request), TEST_ECHO_STRING,
						  i + 1);
		send_packet(CONSOLE0, request, len);
	}

	/* The raw UART has receive buffers of its own */
	len = smp_test_build_echo_request(request, sizeof(request), TEST_ECHO_STRING, 1);
	send_packet(RAW0, request, len);
	zassert_equal(collect_packets(RAW0, 1), 1, "Expected one raw response");
	smp_test_check_echo_response(RAW0->packets[0], RAW0->packet_len[0], TEST_ECHO_STRING, 1);

	release_sysworkq();

	zassert_equal(collect_packets(CONSOLE0, CONFIG_UART_MCUMGR_RX_BUF_COUNT),
		      CONFIG_UART_MCUMGR_RX_BUF_COUNT, "Expected a response per receive buffer");
	for (int i = 0; i < CONFIG_UART_MCUMGR_RX_BUF_COUNT; i++) {
		smp_test_check_echo_response(CONSOLE0->packets[i], CONSOLE0->packet_len[i],
					     TEST_ECHO_STRING, i + 1);
	}
}
#endif

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT)
ZTEST(transport_uart, test_bridge)
{
	uint8_t request[PACKET_BUFFER_SIZE];
	uint8_t response[PACKET_BUFFER_SIZE];
	size_t request_len;
	size_t len;

	/* Bridge requests received on the raw UART to the SMP over console transport */
	len = smp_test_build_connect_request(request, sizeof(request), SMP_SERIAL_TRANSPORT, 1);
	send_packet(RAW0, request, len);
	zassert_equal(collect_packets(RAW0, 1), 1, "Expected a connect response");

	/* A request received on the raw UART goes out of the console UART unchanged */
	request_len = smp_test_build_echo_request(request, sizeof(request), TEST_ECHO_STRING, 2);
	send_packet(RAW0, request, request_len);
	zassert_equal(collect_packets(CONSOLE0, 1), 1, "Expected the request on the console UART");
	zassert_equal(CONSOLE0->packet_len[0], request_len, "Bridged request length mismatch");
	zassert_mem_equal(CONSOLE0->packets[0], request, request_len, "Bridged request mismatch");

	/* Its response, received on the console UART, goes out of the raw UART */
	len = smp_test_build_echo_response(response, sizeof(response), TEST_ECHO_STRING, 2);
	send_packet(CONSOLE0, response, len);
	zassert_equal(collect_packets(RAW0, 2), 2, "Expected the response on the raw UART");
	smp_test_check_echo_response(RAW0->packets[1], RAW0->packet_len[1], TEST_ECHO_STRING, 2);
}
#endif

static void transport_uart_before(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Let anything still in flight from the previous test finish, then start clean */
	k_msleep(QUIET_PERIOD_MS);

	ARRAY_FOR_EACH_PTR(test_uarts, uart) {
		uint8_t discard[64];

		while (serial_vnd_read_out_data(uart->dev, discard, sizeof(discard)) > 0) {
			/* Discard what earlier tests left */
		}

		uart->tx_data_len = 0;
		uart->packet_count = 0;

		if (uart->rx_ctxt.nb != NULL) {
			smp_packet_free(uart->rx_ctxt.nb);
			uart->rx_ctxt.nb = NULL;
		}
	}
}

static void transport_uart_after(void *fixture)
{
	ARG_UNUSED(fixture);

#if HAS_NODES
	release_sysworkq();
#endif

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT)
	(void)transport_mgmt_disconnect_all();
#endif
}

ZTEST_SUITE(transport_uart, NULL, NULL, transport_uart_before, transport_uart_after, NULL);
