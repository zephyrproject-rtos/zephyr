/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Tests of mcumgr_serial_process_raw(), fed the way the UART driver delivers data: in chunks, with
 * each chunk processed until it has been consumed.
 */

#include <string.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/transport/serial.h>
#include <mgmt/mcumgr/transport/smp_internal.h>
#include "smp_test_util.h"

#define MAX_PACKETS      2
#define DATA_BUFFER_SIZE 256
/* Feed everything in one chunk */
#define WHOLE            SIZE_MAX

static struct mcumgr_serial_rx_ctxt rx_ctxt = {
#if defined(CONFIG_MCUMGR_TRANSPORT_SERIAL_HAS_SMP_OVER_CONSOLE) &&                                \
	defined(CONFIG_MCUMGR_TRANSPORT_SERIAL_HAS_RAW_BINARY_NON_SMP_OVER_CONSOLE)
	.raw_transport = true,
#endif
};

static struct net_buf *packets[MAX_PACKETS];
static int packet_count;

/* Feeds data in chunks of chunk_size bytes, collecting the completed packets */
static void feed(const uint8_t *data, size_t len, size_t chunk_size)
{
	while (len > 0) {
		size_t chunk_len = MIN(len, chunk_size);

		len -= chunk_len;

		while (chunk_len > 0) {
			struct net_buf *nb;
			size_t consumed;

			nb = mcumgr_serial_process_raw(&rx_ctxt, data, chunk_len, &consumed);
			zassert_true(consumed >= 1 && consumed <= chunk_len,
				     "Consumed %zu of %zu bytes", consumed, chunk_len);
			data += consumed;
			chunk_len -= consumed;

			if (nb != NULL) {
				zassert_true(packet_count < MAX_PACKETS, "Unexpected extra packet");
				packets[packet_count++] = nb;
			}
		}
	}
}

static void check_packet(int index, const uint8_t *expected, size_t len)
{
	zassert_true(index < packet_count, "Missing packet %d", index);
	zassert_equal(packets[index]->len, len, "Packet %d length mismatch", index);
	zassert_mem_equal(packets[index]->data, expected, len, "Packet %d content mismatch", index);
}

/* Builds two echo requests back to back, returning their combined length */
static size_t build_two_requests(uint8_t *data, size_t size, size_t *first_len)
{
	*first_len = smp_test_build_echo_request(data, size, "first", 1);

	return *first_len +
	       smp_test_build_echo_request(&data[*first_len], size - *first_len, "second", 2);
}

static void raw_parser_after(void *fixture)
{
	ARG_UNUSED(fixture);

	for (int i = 0; i < packet_count; i++) {
		smp_packet_free(packets[i]);
	}

	packet_count = 0;

	if (rx_ctxt.nb != NULL) {
		smp_packet_free(rx_ctxt.nb);
		rx_ctxt.nb = NULL;
	}
}

ZTEST(transport_uart_raw_parser, test_two_packets_one_chunk)
{
	uint8_t data[DATA_BUFFER_SIZE];
	size_t first_len;
	size_t len = build_two_requests(data, sizeof(data), &first_len);

	feed(data, len, WHOLE);

	zassert_equal(packet_count, 2, "Expected two packets");
	check_packet(0, data, first_len);
	check_packet(1, &data[first_len], len - first_len);
}

ZTEST(transport_uart_raw_parser, test_split_header)
{
	uint8_t data[DATA_BUFFER_SIZE];
	size_t len = smp_test_build_echo_request(data, sizeof(data), TEST_ECHO_STRING, 1);

	/* 3 byte chunks split the 8 byte header across three of them */
	feed(data, len, 3);

	zassert_equal(packet_count, 1, "Expected one packet");
	check_packet(0, data, len);
}

ZTEST(transport_uart_raw_parser, test_split_payload)
{
	uint8_t data[DATA_BUFFER_SIZE];
	size_t len = smp_test_build_echo_request(data, sizeof(data), TEST_ECHO_STRING, 1);

	feed(data, sizeof(struct smp_hdr) + 2, WHOLE);
	zassert_equal(packet_count, 0, "Expected no packet before the payload is complete");

	feed(&data[sizeof(struct smp_hdr) + 2], len - sizeof(struct smp_hdr) - 2, WHOLE);
	zassert_equal(packet_count, 1, "Expected one packet");
	check_packet(0, data, len);
}

ZTEST(transport_uart_raw_parser, test_byte_by_byte)
{
	uint8_t data[DATA_BUFFER_SIZE];
	size_t first_len;
	size_t len = build_two_requests(data, sizeof(data), &first_len);

	/* The way the UART driver used to deliver raw data */
	feed(data, len, 1);

	zassert_equal(packet_count, 2, "Expected two packets");
	check_packet(0, data, first_len);
	check_packet(1, &data[first_len], len - first_len);
}

ZTEST(transport_uart_raw_parser, test_chunk_sizes)
{
	uint8_t data[DATA_BUFFER_SIZE];
	size_t first_len;
	size_t len = build_two_requests(data, sizeof(data), &first_len);

	/* Every chunk size puts the packet boundaries at a different place within a chunk */
	for (size_t chunk_size = 1; chunk_size <= len; chunk_size++) {
		feed(data, len, chunk_size);

		zassert_equal(packet_count, 2, "Expected two packets with %zu byte chunks",
			      chunk_size);
		check_packet(0, data, first_len);
		check_packet(1, &data[first_len], len - first_len);
		raw_parser_after(NULL);
	}
}

ZTEST(transport_uart_raw_parser, test_invalid_header_resync)
{
	uint8_t data[DATA_BUFFER_SIZE];
	struct smp_hdr invalid = {
		/* Not a valid operation */
		.nh_op = 7,
		.nh_len = sys_cpu_to_be16(4),
	};
	size_t request_len;
	size_t consumed;

	BUILD_ASSERT(7 >= MGMT_OP_COUNT, "Operation 7 must be invalid");

	memcpy(data, &invalid, sizeof(invalid));
	request_len = smp_test_build_echo_request(
		&data[sizeof(invalid)], sizeof(data) - sizeof(invalid), TEST_ECHO_STRING, 1);

	/* The invalid header is dropped as a whole, on its last byte */
	zassert_is_null(mcumgr_serial_process_raw(&rx_ctxt, data, sizeof(invalid) + request_len,
						  &consumed));
	zassert_equal(consumed, sizeof(invalid), "Expected the invalid header to be consumed");
	zassert_is_null(rx_ctxt.nb, "Expected the invalid header to be dropped");

	/* Parsing restarts with the next byte */
	feed(&data[sizeof(invalid)], request_len, WHOLE);
	zassert_equal(packet_count, 1, "Expected one packet");
	check_packet(0, &data[sizeof(invalid)], request_len);
}

ZTEST(transport_uart_raw_parser, test_oversize)
{
	uint8_t data[DATA_BUFFER_SIZE];
	struct smp_hdr oversize = {
		.nh_op = MGMT_OP_READ,
		/* Longer than a packet buffer can hold */
		.nh_len = sys_cpu_to_be16(CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE),
	};
	size_t request_len;

	memcpy(data, &oversize, sizeof(oversize));
	request_len = smp_test_build_echo_request(
		&data[sizeof(oversize)], sizeof(data) - sizeof(oversize), TEST_ECHO_STRING, 1);
	feed(data, sizeof(oversize) + request_len, WHOLE);

	zassert_equal(packet_count, 1, "Expected only the valid packet");
	check_packet(0, &data[sizeof(oversize)], request_len);
}

ZTEST(transport_uart_raw_parser, test_empty_payload)
{
	struct smp_hdr empty = {
		.nh_op = MGMT_OP_READ,
		.nh_len = 0,
	};

	feed((const uint8_t *)&empty, sizeof(empty), WHOLE);

	zassert_equal(packet_count, 1, "Expected a packet with an empty payload");
	check_packet(0, (const uint8_t *)&empty, sizeof(empty));
}

ZTEST(transport_uart_raw_parser, test_no_data)
{
	uint8_t data = 0;
	size_t consumed = 1;

	zassert_is_null(mcumgr_serial_process_raw(&rx_ctxt, &data, 0, &consumed));
	zassert_equal(consumed, 0, "Expected nothing to be consumed");
}

ZTEST_SUITE(transport_uart_raw_parser, NULL, NULL, NULL, raw_parser_after, NULL);
