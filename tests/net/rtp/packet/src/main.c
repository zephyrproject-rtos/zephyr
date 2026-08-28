/*
 *  SPDX-FileCopyrightText: 2026 Basalte bv
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/rtp.h>
#include <zephyr/ztest.h>

#include "rtp_packet.h"

#define TEST_SSRC 0xdeadbeef
#define TEST_SEQ  0x1234
#define TEST_TS   0x89abcdef

static uint8_t payload[] = {0x10, 0x20, 0x30, 0x40, 0x50};

static void fill_header(struct rtp_header *header)
{
	rtp_header_set_v(header, RTP_VERSION);
	rtp_header_set_pt(header, 96);
	header->seq = TEST_SEQ;
	header->ts = TEST_TS;
	header->ssrc = TEST_SSRC;
}

ZTEST(rtp_packet_tests, test_serialize_fixed)
{
	struct rtp_packet packet = {};
	uint8_t buf[64];
	int len;
	/* clang-format off */
	static const uint8_t expected[] = {
		0x80, 0xe0, 0x12, 0x34,             /* V=2, M=1, PT=96, seq */
		0x89, 0xab, 0xcd, 0xef,             /* timestamp */
		0xde, 0xad, 0xbe, 0xef,             /* SSRC */
		0x10, 0x20, 0x30, 0x40, 0x50,       /* payload */
	};
	/* clang-format on */

	fill_header(&packet.header);
	rtp_header_set_m(&packet.header, 1);
	packet.payload = payload;
	packet.payload_len = sizeof(payload);

	len = rtp_packet_serialize(&packet, 0, buf, sizeof(buf));
	zexpect_equal(len, sizeof(expected));
	zexpect_mem_equal(buf, expected, sizeof(expected));
}

ZTEST(rtp_packet_tests, test_roundtrip)
{
	struct rtp_packet packet = {};
	struct rtp_packet parsed = {};
	uint8_t buf[64];
	int len;

	fill_header(&packet.header);
	packet.payload = payload;
	packet.payload_len = sizeof(payload);

	len = rtp_packet_serialize(&packet, 0, buf, sizeof(buf));
	zassert_true(len > 0);
	zassert_ok(rtp_packet_deserialize(&parsed, buf, len));

	zexpect_equal(rtp_header_get_v(&parsed.header), RTP_VERSION);
	zexpect_equal(rtp_header_get_pt(&parsed.header), 96);
	zexpect_equal(parsed.header.seq, TEST_SEQ);
	zexpect_equal(parsed.header.ts, TEST_TS);
	zexpect_equal(parsed.header.ssrc, TEST_SSRC);
	zexpect_equal(parsed.payload_len, sizeof(payload));
	zexpect_mem_equal(parsed.payload, payload, sizeof(payload));
}

ZTEST(rtp_packet_tests, test_roundtrip_padding)
{
	struct rtp_packet packet = {};
	struct rtp_packet parsed = {};
	uint8_t buf[64];
	int len;

	fill_header(&packet.header);
	rtp_header_set_p(&packet.header, 1);
	packet.payload = payload;
	packet.payload_len = sizeof(payload);

	len = rtp_packet_serialize(&packet, 4, buf, sizeof(buf));
	zexpect_equal(len, RTP_MIN_HEADER_LEN + sizeof(payload) + 4);
	zexpect_equal(buf[len - 1], 4, "Last padding byte holds the padding count");

	zassert_ok(rtp_packet_deserialize(&parsed, buf, len));
	zexpect_equal(rtp_header_get_p(&parsed.header), 1);
	zexpect_equal(parsed.payload_len, sizeof(payload), "Padding stripped from payload");
	zexpect_mem_equal(parsed.payload, payload, sizeof(payload));
}

ZTEST(rtp_packet_tests, test_roundtrip_extension)
{
	struct rtp_packet packet = {};
	struct rtp_packet parsed = {};
	uint8_t x_data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	uint8_t buf[64];
	int len;

	fill_header(&packet.header);
	rtp_header_set_x(&packet.header, 1);
	zassert_ok(rtp_init_header_extension(&packet.header.header_extension, 0xabcd, x_data,
					     sizeof(x_data)));
	packet.payload = payload;
	packet.payload_len = sizeof(payload);

	len = rtp_packet_serialize(&packet, 0, buf, sizeof(buf));
	zexpect_equal(len, RTP_MIN_HEADER_LEN + 4 + sizeof(x_data) + sizeof(payload));

	zassert_ok(rtp_packet_deserialize(&parsed, buf, len));
	zexpect_equal(rtp_header_get_x(&parsed.header), 1);
	zexpect_equal(parsed.header.header_extension.definition, 0xabcd);
	zexpect_equal(parsed.header.header_extension.length, sizeof(x_data) / sizeof(uint32_t));
	zexpect_mem_equal(parsed.header.header_extension.data, x_data, sizeof(x_data));
	zexpect_equal(parsed.payload_len, sizeof(payload));
	zexpect_mem_equal(parsed.payload, payload, sizeof(payload));
}

ZTEST(rtp_packet_tests, test_roundtrip_csrc)
{
#if CONFIG_RTP_MAX_CSRC_COUNT == 0
	ztest_test_skip();
#else
	struct rtp_packet packet = {};
	struct rtp_packet parsed = {};
	uint8_t buf[64];
	int len;

	fill_header(&packet.header);
	rtp_header_set_cc(&packet.header, 2);
	packet.header.csrc[0] = 0x11223344;
	packet.header.csrc[1] = 0x55667788;
	packet.payload = payload;
	packet.payload_len = sizeof(payload);

	len = rtp_packet_serialize(&packet, 0, buf, sizeof(buf));
	zexpect_equal(len, RTP_MIN_HEADER_LEN + 2 * sizeof(uint32_t) + sizeof(payload));

	zassert_ok(rtp_packet_deserialize(&parsed, buf, len));
	zexpect_equal(rtp_header_get_cc(&parsed.header), 2);
	zexpect_equal(parsed.header.csrc[0], 0x11223344);
	zexpect_equal(parsed.header.csrc[1], 0x55667788);
	zexpect_equal(parsed.payload_len, sizeof(payload));
#endif /* CONFIG_RTP_MAX_CSRC_COUNT == 0 */
}

ZTEST(rtp_packet_tests, test_hdr_len)
{
	struct rtp_packet packet = {};
	uint8_t x_data[4] = {0};

	fill_header(&packet.header);
	zexpect_equal(rtp_packet_hdr_len(&packet), RTP_MIN_HEADER_LEN);

	rtp_header_set_x(&packet.header, 1);
	zassert_ok(rtp_init_header_extension(&packet.header.header_extension, 1, x_data,
					     sizeof(x_data)));
	zexpect_equal(rtp_packet_hdr_len(&packet), RTP_MIN_HEADER_LEN + 4 + sizeof(x_data));

#if CONFIG_RTP_MAX_CSRC_COUNT > 0
	rtp_header_set_cc(&packet.header, 2);
	zexpect_equal(rtp_packet_hdr_len(&packet),
		      RTP_MIN_HEADER_LEN + 2 * sizeof(uint32_t) + 4 + sizeof(x_data));
#endif /* CONFIG_RTP_MAX_CSRC_COUNT > 0 */
}

ZTEST(rtp_packet_tests, test_deserialize_errors)
{
	struct rtp_packet packet = {};
	uint8_t buf[64] = {};

	/* Too short */
	zexpect_equal(rtp_packet_deserialize(&packet, buf, RTP_MIN_HEADER_LEN - 1), -EINVAL);

	/* Wrong version (V=0) */
	zexpect_equal(rtp_packet_deserialize(&packet, buf, RTP_MIN_HEADER_LEN), -EINVAL);

	/* Extension flag set but no extension words present */
	buf[0] = 0x90; /* V=2, X=1 */
	zexpect_equal(rtp_packet_deserialize(&packet, buf, RTP_MIN_HEADER_LEN), -EINVAL);

	/* Extension length larger than remaining data */
	buf[12] = 0x00;
	buf[13] = 0x01; /* definition */
	buf[14] = 0x00;
	buf[15] = 0x10; /* length = 16 words, not present */
	zexpect_equal(rtp_packet_deserialize(&packet, buf, RTP_MIN_HEADER_LEN + 4), -EINVAL);

	/* Padding flag set with zero padding value */
	buf[0] = 0xa0; /* V=2, P=1 */
	buf[12] = 0x00;
	zexpect_equal(rtp_packet_deserialize(&packet, buf, RTP_MIN_HEADER_LEN + 1), -EINVAL);

	/* Padding larger than payload */
	buf[12] = 0xff;
	zexpect_equal(rtp_packet_deserialize(&packet, buf, RTP_MIN_HEADER_LEN + 1), -EINVAL);

	/* Padding flag set without payload */
	zexpect_equal(rtp_packet_deserialize(&packet, buf, RTP_MIN_HEADER_LEN), -EINVAL);
}

ZTEST(rtp_packet_tests, test_serialize_errors)
{
	struct rtp_packet packet = {};
	uint8_t small[RTP_MIN_HEADER_LEN];
	uint8_t buf[16];

	fill_header(&packet.header);

	/* Buffer smaller than the fixed header */
	zexpect_equal(rtp_packet_serialize(&packet, 0, buf, RTP_MIN_HEADER_LEN - 1), -EINVAL);

	/* Payload with NULL pointer */
	packet.payload = NULL;
	packet.payload_len = 4;
	zexpect_equal(rtp_packet_serialize(&packet, 0, buf, sizeof(buf)), -EINVAL);

	/* Payload does not fit */
	packet.payload = payload;
	packet.payload_len = sizeof(payload);
	zexpect_equal(rtp_packet_serialize(&packet, 0, small, sizeof(small)), -EINVAL);

	/* Padding does not fit */
	packet.payload_len = 0;
	zexpect_equal(rtp_packet_serialize(&packet, 16, small, sizeof(small)), -EINVAL);
}

ZTEST_SUITE(rtp_packet_tests, NULL, NULL, NULL, NULL, NULL);
