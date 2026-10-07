/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * End-to-end tests of the MCUmgr UART transports: requests are written to an emulated UART set as
 * the zephyr,uart-mcumgr chosen device, and the responses the device transmits are read back.
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/transport/serial.h>
#include <mgmt/mcumgr/transport/smp_internal.h>
#include "smp_test_util.h"

BUILD_ASSERT(IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_UART) !=
		     IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_RAW_UART),
	     "Each scenario must enable exactly one of the MCUmgr UART transports");

#define RESPONSE_TIMEOUT_MS 1000
#define QUIET_PERIOD_MS     50
#define MAX_RESPONSES       2
#define REQUEST_BUFFER_SIZE 256

static const struct device *const uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_uart_mcumgr));

/* Data transmitted by the device that has not been parsed into responses yet */
static uint8_t tx_data[1024];
static size_t tx_data_len;

/* Responses parsed from the transmitted data, header included */
static uint8_t responses[MAX_RESPONSES][CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE];
static size_t response_len[MAX_RESPONSES];
static int response_count;

#if defined(CONFIG_MCUMGR_TRANSPORT_UART)
static struct mcumgr_serial_rx_ctxt console_rx_ctxt;
static uint8_t console_frames[1024];
static size_t console_frames_len;

static int console_capture(const void *data, int len)
{
	zassert_true(console_frames_len + len <= sizeof(console_frames), "Frame buffer too small");
	memcpy(&console_frames[console_frames_len], data, len);
	console_frames_len += len;

	return 0;
}
#endif

static void store_response(const uint8_t *data, size_t len)
{
	zassert_true(response_count < MAX_RESPONSES, "Unexpected extra response");
	zassert_true(len <= sizeof(responses[0]), "Response too large");
	memcpy(responses[response_count], data, len);
	response_len[response_count] = len;
	++response_count;
}

static void consume_tx_data(size_t len)
{
	memmove(tx_data, &tx_data[len], tx_data_len - len);
	tx_data_len -= len;
}

static void parse_tx_data(void)
{
#if defined(CONFIG_MCUMGR_TRANSPORT_RAW_UART)
	/* Raw responses are bare SMP packets, delimited by the length in their header */
	while (tx_data_len >= sizeof(struct smp_hdr)) {
		const struct smp_hdr *hdr = (const struct smp_hdr *)tx_data;
		size_t total = sizeof(*hdr) + sys_be16_to_cpu(hdr->nh_len);

		if (tx_data_len < total) {
			break;
		}

		store_response(tx_data, total);
		consume_tx_data(total);
	}
#else
	/* SMP over console responses are newline terminated frames */
	uint8_t *end;

	while ((end = memchr(tx_data, '\n', tx_data_len)) != NULL) {
		size_t frame_len = end - tx_data + 1;
		struct net_buf *nb;

		nb = mcumgr_serial_process_frag(&console_rx_ctxt, tx_data, frame_len);
		if (nb != NULL) {
			store_response(nb->data, nb->len);
			smp_packet_free(nb);
		}

		consume_tx_data(frame_len);
	}
#endif
}

/* Collects responses until there are at least expected of them, or the timeout passes */
static int wait_for_responses(int expected, int timeout_ms)
{
	int64_t end = k_uptime_get() + timeout_ms;

	do {
		tx_data_len += uart_emul_get_tx_data(uart_dev, &tx_data[tx_data_len],
						     sizeof(tx_data) - tx_data_len);
		parse_tx_data();

		if (response_count >= expected) {
			break;
		}

		k_msleep(1);
	} while (k_uptime_get() < end);

	return response_count;
}

static void put_rx_data(const uint8_t *data, size_t len)
{
	zassert_equal(uart_emul_put_rx_data(uart_dev, data, len), len, "Emulated UART RX full");
}

/* Sends a request packet in the framing used by the transport under test */
static void send_request(const uint8_t *packet, size_t len)
{
#if defined(CONFIG_MCUMGR_TRANSPORT_RAW_UART)
	put_rx_data(packet, len);
#else
	console_frames_len = 0;
	zassert_equal(mcumgr_serial_tx_pkt(packet, len, console_capture), 0,
		      "Failed to encode request");
	put_rx_data(console_frames, console_frames_len);
#endif
}

static void transport_uart_before(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Let anything still in flight from the previous test finish, then start clean */
	k_msleep(QUIET_PERIOD_MS);
	uart_emul_flush_rx_data(uart_dev);
	uart_emul_flush_tx_data(uart_dev);
	tx_data_len = 0;
	response_count = 0;

#if defined(CONFIG_MCUMGR_TRANSPORT_UART)
	if (console_rx_ctxt.nb != NULL) {
		smp_packet_free(console_rx_ctxt.nb);
		console_rx_ctxt.nb = NULL;
	}
#endif
}

ZTEST(transport_uart, test_echo)
{
	uint8_t request[REQUEST_BUFFER_SIZE];
	size_t len = smp_test_build_echo_request(request, sizeof(request), TEST_ECHO_STRING, 1);

	send_request(request, len);

	zassert_equal(wait_for_responses(1, RESPONSE_TIMEOUT_MS), 1, "Expected one response");
	smp_test_check_echo_response(responses[0], response_len[0], TEST_ECHO_STRING, 1);
}

#if defined(CONFIG_MCUMGR_TRANSPORT_RAW_UART)
/* Long enough for the input timeout to have passed */
#define INPUT_TIMEOUT_WAIT_MS (CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT_TIME_MS * 3)

ZTEST(transport_uart, test_raw_back_to_back)
{
	uint8_t requests[REQUEST_BUFFER_SIZE];
	size_t first_len;
	size_t second_len;

	first_len = smp_test_build_echo_request(requests, sizeof(requests), "first", 1);
	second_len = smp_test_build_echo_request(&requests[first_len], sizeof(requests) - first_len,
						 "second", 2);

	/* Both requests in one write, so they reach the transport in the same chunk */
	put_rx_data(requests, first_len + second_len);

	zassert_equal(wait_for_responses(2, RESPONSE_TIMEOUT_MS), 2, "Expected two responses");
	smp_test_check_echo_response(responses[0], response_len[0], "first", 1);
	smp_test_check_echo_response(responses[1], response_len[1], "second", 2);
}

ZTEST(transport_uart, test_raw_split_across_writes)
{
	uint8_t request[REQUEST_BUFFER_SIZE];
	size_t len = smp_test_build_echo_request(request, sizeof(request), TEST_ECHO_STRING, 1);

	put_rx_data(request, len / 2);
	k_msleep(5);
	put_rx_data(&request[len / 2], len - (len / 2));

	zassert_equal(wait_for_responses(1, RESPONSE_TIMEOUT_MS), 1, "Expected one response");
	smp_test_check_echo_response(responses[0], response_len[0], TEST_ECHO_STRING, 1);
	zassert_equal(wait_for_responses(2, QUIET_PERIOD_MS), 1, "Expected no other response");
}

ZTEST(transport_uart, test_raw_invalid_header_then_packet)
{
	uint8_t data[REQUEST_BUFFER_SIZE];
	struct smp_hdr invalid = {
		/* Not a valid operation, so the whole header is dropped */
		.nh_op = 7,
		.nh_len = sys_cpu_to_be16(4),
	};
	size_t len;

	BUILD_ASSERT(7 >= MGMT_OP_COUNT, "Operation 7 must be invalid");

	memcpy(data, &invalid, sizeof(invalid));
	len = sizeof(invalid) + smp_test_build_echo_request(&data[sizeof(invalid)],
							    sizeof(data) - sizeof(invalid),
							    TEST_ECHO_STRING, 1);
	put_rx_data(data, len);

	zassert_equal(wait_for_responses(1, RESPONSE_TIMEOUT_MS), 1, "Expected one response");
	smp_test_check_echo_response(responses[0], response_len[0], TEST_ECHO_STRING, 1);
	zassert_equal(wait_for_responses(2, QUIET_PERIOD_MS), 1, "Expected no other response");
}

ZTEST(transport_uart, test_raw_input_timeout)
{
	uint8_t request[REQUEST_BUFFER_SIZE];
	size_t len;

	/* An incomplete request, which must be discarded once the input timeout passes */
	len = smp_test_build_echo_request(request, sizeof(request), TEST_ECHO_STRING, 1);
	put_rx_data(request, len - 4);
	zassert_equal(wait_for_responses(1, INPUT_TIMEOUT_WAIT_MS), 0,
		      "Expected no response to an incomplete request");

	len = smp_test_build_echo_request(request, sizeof(request), TEST_ECHO_STRING, 2);
	put_rx_data(request, len);

	zassert_equal(wait_for_responses(1, RESPONSE_TIMEOUT_MS), 1, "Expected one response");
	smp_test_check_echo_response(responses[0], response_len[0], TEST_ECHO_STRING, 2);
}
#endif

#if defined(CONFIG_MCUMGR_TRANSPORT_UART)
ZTEST(transport_uart, test_console_multi_frame)
{
	/* Long enough that the request and the response each span several frames */
	char long_string[MCUMGR_SERIAL_MAX_FRAME + 32];
	uint8_t request[REQUEST_BUFFER_SIZE];
	size_t len;

	for (size_t i = 0; i < sizeof(long_string) - 1; i++) {
		long_string[i] = 'a' + (i % 26);
	}

	long_string[sizeof(long_string) - 1] = '\0';
	len = smp_test_build_echo_request(request, sizeof(request), long_string, 1);

	zassert_true(len > MCUMGR_SERIAL_MAX_FRAME, "Request fits in one frame");
	send_request(request, len);

	zassert_equal(wait_for_responses(1, RESPONSE_TIMEOUT_MS), 1, "Expected one response");
	smp_test_check_echo_response(responses[0], response_len[0], long_string, 1);
}

ZTEST(transport_uart, test_console_line_too_long)
{
	uint8_t line[CONFIG_UART_MCUMGR_RX_BUF_SIZE + 16];
	uint8_t request[REQUEST_BUFFER_SIZE];
	size_t len;

	/* A frame start followed by more data than a receive buffer can hold */
	memset(line, 'A', sizeof(line));
	line[0] = MCUMGR_SERIAL_HDR_PKT_1;
	line[1] = MCUMGR_SERIAL_HDR_PKT_2;
	line[sizeof(line) - 1] = '\n';
	put_rx_data(line, sizeof(line));

	len = smp_test_build_echo_request(request, sizeof(request), TEST_ECHO_STRING, 1);
	send_request(request, len);

	zassert_equal(wait_for_responses(1, RESPONSE_TIMEOUT_MS), 1, "Expected one response");
	smp_test_check_echo_response(responses[0], response_len[0], TEST_ECHO_STRING, 1);
	zassert_equal(wait_for_responses(2, QUIET_PERIOD_MS), 1, "Expected no other response");
}
#endif

ZTEST_SUITE(transport_uart, NULL, NULL, transport_uart_before, NULL, NULL);
