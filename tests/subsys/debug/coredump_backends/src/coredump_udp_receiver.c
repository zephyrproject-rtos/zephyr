/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/init.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include <zephyr/debug/coredump.h>

/* Frame format of subsys/debug/coredump/coredump_backend_logging_udp.c */
#define ZCDU_HDR_SIZE    16U
#define ZCDU_MAX_PAYLOAD 1024U
#define ZCDU_FLAG_END    0x0001U
#define ZCDU_PORT        17777U

#define DUMP_BUF_SIZE 8192U
#define RX_TIMEOUT_MS 2000

static int rx_sock = -1;
static uint8_t dump[DUMP_BUF_SIZE];

/* Bind the backend's peer address before the test suite raises the coredump. */
static int coredump_udp_receiver_init(void)
{
	const char *host = CONFIG_DEBUG_COREDUMP_LOGGING_UDP_HOST;
	struct net_sockaddr_storage addr = {0};

	if (!net_ipaddr_parse(host, strlen(host), net_sad(&addr)) ||
	    net_port_set_default(net_sad(&addr), ZCDU_PORT) < 0) {
		return 0;
	}

	rx_sock = zsock_socket(addr.ss_family, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	if (rx_sock < 0) {
		return 0;
	}

	if (zsock_bind(rx_sock, net_sad(&addr), sizeof(addr)) < 0) {
		zsock_close(rx_sock);
		rx_sock = -1;
	}

	return 0;
}

SYS_INIT(coredump_udp_receiver_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

ZTEST(coredump_backends, test_coredump_6_udp_received)
{
	uint8_t frame[ZCDU_HDR_SIZE + ZCDU_MAX_PAYLOAD];
	const struct coredump_hdr_t *hdr = (const struct coredump_hdr_t *)dump;
	uint32_t expected_seq = 0U;
	size_t total = 0U;
	bool end = false;

	if (IS_ENABLED(CONFIG_TEST_BACKEND_ERROR)) {
		ztest_test_skip();
	}

	zassert_true(rx_sock >= 0, "Receiving socket not open");

	while (!end) {
		struct zsock_pollfd pfd = {
			.fd = rx_sock,
			.events = ZSOCK_POLLIN,
		};
		ssize_t len;
		uint32_t offset;
		uint16_t payload_len;
		uint16_t flags;

		zassert_equal(zsock_poll(&pfd, 1, RX_TIMEOUT_MS), 1,
			      "No frame received after %u frames", expected_seq);

		len = zsock_recv(rx_sock, frame, sizeof(frame), ZSOCK_MSG_DONTWAIT);
		zassert_true(len >= 0, "recv failed (errno %d)", errno);
		zassert_true(len >= ZCDU_HDR_SIZE, "Short frame (%d bytes)", (int)len);
		zassert_mem_equal(frame, "ZCDU", 4, "Bad frame magic");

		offset = sys_get_le32(&frame[4]);
		payload_len = sys_get_le16(&frame[8]);
		flags = sys_get_le16(&frame[10]);

		zassert_equal(sys_get_le32(&frame[12]), expected_seq, "Unexpected sequence number");
		zassert_equal(offset, total, "Unexpected stream offset");
		zassert_equal(len, ZCDU_HDR_SIZE + payload_len, "Frame length mismatch");
		zassert_true(total + payload_len <= sizeof(dump), "Coredump too large for test");

		memcpy(&dump[total], &frame[ZCDU_HDR_SIZE], payload_len);
		total += payload_len;
		expected_seq++;
		end = (flags & ZCDU_FLAG_END) != 0U;
	}

	zassert_true(total > sizeof(*hdr), "Coredump too short (%zu bytes)", total);
	zassert_equal(hdr->id[0], 'Z', "Bad coredump header");
	zassert_equal(hdr->id[1], 'E', "Bad coredump header");

	TC_PRINT("Received %zu coredump bytes in %u frames\n", total, expected_seq);
}
