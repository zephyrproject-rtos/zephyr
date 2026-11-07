/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

#include "rstp_internal.h"

/* Reference BPDUs built with Scapy and checked with the tcpdump STP printer:
 *
 * STP 802.1d, Config, Flags [Topology change, Topology change ACK],
 *   bridge-id 9000.66:77:88:99:aa:bb.8002, length 35
 *   message-age 1.00s, max-age 20.00s, hello-time 2.00s, forwarding-delay 15.00s
 *   root-id 8000.00:11:22:33:44:55, root-pathcost 4
 */
static const uint8_t config_bpdu[] = {
	0x00, 0x00, 0x00, 0x00, 0x81, 0x80, 0x00, 0x00, 0x11, 0x22, 0x33, 0x44,
	0x55, 0x00, 0x00, 0x00, 0x04, 0x90, 0x00, 0x66, 0x77, 0x88, 0x99, 0xaa,
	0xbb, 0x80, 0x02, 0x01, 0x00, 0x14, 0x00, 0x02, 0x00, 0x0f, 0x00,
};

/* STP 802.1w, Rapid STP, Flags [Proposal, Learn, Forward, Agreement],
 *   bridge-id 8000.66:77:88:99:aa:bb.8001, length 36
 *   message-age 2.00s, max-age 20.00s, hello-time 2.00s, forwarding-delay 15.00s
 *   root-id 1000.00:11:22:33:44:55, root-pathcost 200000, port-role Designated
 */
static const uint8_t rst_bpdu[] = {
	0x00, 0x00, 0x02, 0x02, 0x7e, 0x10, 0x00, 0x00, 0x11, 0x22, 0x33, 0x44,
	0x55, 0x00, 0x03, 0x0d, 0x40, 0x80, 0x00, 0x66, 0x77, 0x88, 0x99, 0xaa,
	0xbb, 0x80, 0x01, 0x02, 0x00, 0x14, 0x00, 0x02, 0x00, 0x0f, 0x00, 0x00,
};

/* STP 802.1d, Topology Change */
static const uint8_t tcn_bpdu[] = {0x00, 0x00, 0x00, 0x80};

static const struct eth_rstp_bpdu config_decoded = {
	.type = ETH_RSTP_BPDU_CONFIG,
	.flags = ETH_RSTP_FLAG_TC | ETH_RSTP_FLAG_TC_ACK,
	.root_id = 0x8000001122334455ULL,
	.root_path_cost = 4U,
	.bridge_id = 0x900066778899aabbULL,
	.port_id = 0x8002U,
	.times = {.message_age = 1U, .max_age = 20U, .hello_time = 2U, .forward_delay = 15U},
};

static const struct eth_rstp_bpdu rst_decoded = {
	.type = ETH_RSTP_BPDU_RST,
	.flags = ETH_RSTP_FLAG_PROPOSAL |
		 (ETH_RSTP_BPDU_ROLE_DESIGNATED << ETH_RSTP_FLAG_ROLE_SHIFT) |
		 ETH_RSTP_FLAG_LEARNING | ETH_RSTP_FLAG_FORWARDING | ETH_RSTP_FLAG_AGREEMENT,
	.root_id = 0x1000001122334455ULL,
	.root_path_cost = 200000U,
	.bridge_id = 0x800066778899aabbULL,
	.port_id = 0x8001U,
	.times = {.message_age = 2U, .max_age = 20U, .hello_time = 2U, .forward_delay = 15U},
};

static void expect_bpdu(const struct eth_rstp_bpdu *got, const struct eth_rstp_bpdu *want)
{
	zexpect_equal(got->type, want->type);
	zexpect_equal(got->flags, want->flags, "flags 0x%02x", got->flags);
	zexpect_equal(got->root_id, want->root_id);
	zexpect_equal(got->root_path_cost, want->root_path_cost);
	zexpect_equal(got->bridge_id, want->bridge_id);
	zexpect_equal(got->port_id, want->port_id);
	zexpect_equal(got->times.message_age, want->times.message_age);
	zexpect_equal(got->times.max_age, want->times.max_age);
	zexpect_equal(got->times.hello_time, want->times.hello_time);
	zexpect_equal(got->times.forward_delay, want->times.forward_delay);
}

ZTEST(rstp_bpdu, test_decode_config)
{
	struct eth_rstp_bpdu bpdu;

	zassert_ok(eth_rstp_bpdu_decode(config_bpdu, sizeof(config_bpdu), &bpdu));
	expect_bpdu(&bpdu, &config_decoded);
}

ZTEST(rstp_bpdu, test_decode_rst)
{
	struct eth_rstp_bpdu bpdu;

	zassert_ok(eth_rstp_bpdu_decode(rst_bpdu, sizeof(rst_bpdu), &bpdu));
	expect_bpdu(&bpdu, &rst_decoded);
}

ZTEST(rstp_bpdu, test_decode_tcn)
{
	struct eth_rstp_bpdu bpdu;
	uint8_t buf[sizeof(config_bpdu)];

	zassert_ok(eth_rstp_bpdu_decode(tcn_bpdu, sizeof(tcn_bpdu), &bpdu));
	zexpect_equal(bpdu.type, ETH_RSTP_BPDU_TCN);

	/* 14.5 b) checks neither the version nor the octets after the fourth */
	memcpy(buf, config_bpdu, sizeof(buf));
	buf[2] = 0x05;
	buf[3] = 0x80;
	zassert_ok(eth_rstp_bpdu_decode(buf, sizeof(buf), &bpdu));
	zexpect_equal(bpdu.type, ETH_RSTP_BPDU_TCN);
}

/* Flags not defined for the BPDU type are ignored on receipt (14.4 b to g) */
ZTEST(rstp_bpdu, test_decode_ignores_undefined_flags)
{
	struct eth_rstp_bpdu bpdu;
	uint8_t buf[sizeof(rst_bpdu)];

	memcpy(buf, config_bpdu, sizeof(config_bpdu));
	buf[4] = 0xff;
	zassert_ok(eth_rstp_bpdu_decode(buf, sizeof(config_bpdu), &bpdu));
	zexpect_equal(bpdu.flags, ETH_RSTP_FLAG_TC | ETH_RSTP_FLAG_TC_ACK);

	memcpy(buf, rst_bpdu, sizeof(rst_bpdu));
	buf[4] = 0xff;
	zassert_ok(eth_rstp_bpdu_decode(buf, sizeof(rst_bpdu), &bpdu));
	zexpect_equal(bpdu.flags, (uint8_t)~ETH_RSTP_FLAG_TC_ACK);
}

/* An MST BPDU carries an RST BPDU in its first 36 octets */
ZTEST(rstp_bpdu, test_decode_mst_as_rst)
{
	struct eth_rstp_bpdu bpdu;
	uint8_t buf[102] = {0};

	memcpy(buf, rst_bpdu, sizeof(rst_bpdu));
	buf[2] = 3U;
	zassert_ok(eth_rstp_bpdu_decode(buf, sizeof(buf), &bpdu));
	expect_bpdu(&bpdu, &rst_decoded);
}

ZTEST(rstp_bpdu, test_decode_invalid)
{
	struct eth_rstp_bpdu bpdu;
	uint8_t buf[sizeof(rst_bpdu)];

	/* Shorter than any BPDU */
	zexpect_equal(eth_rstp_bpdu_decode(tcn_bpdu, 3U, &bpdu), -EINVAL);

	/* Protocol Identifier other than 0 */
	memcpy(buf, rst_bpdu, sizeof(rst_bpdu));
	buf[1] = 0x01;
	zexpect_equal(eth_rstp_bpdu_decode(buf, sizeof(rst_bpdu), &bpdu), -EINVAL);

	/* Configuration BPDU shorter than 35 octets */
	zexpect_equal(eth_rstp_bpdu_decode(config_bpdu, sizeof(config_bpdu) - 1U, &bpdu), -EINVAL);

	/* RST BPDU shorter than 36 octets */
	zexpect_equal(eth_rstp_bpdu_decode(rst_bpdu, sizeof(rst_bpdu) - 1U, &bpdu), -EINVAL);

	/* BPDU Type 0000 0010 with a Protocol Version below 2 */
	memcpy(buf, rst_bpdu, sizeof(rst_bpdu));
	buf[2] = 1U;
	zexpect_equal(eth_rstp_bpdu_decode(buf, sizeof(rst_bpdu), &bpdu), -EINVAL);

	/* Unknown BPDU Type */
	memcpy(buf, rst_bpdu, sizeof(rst_bpdu));
	buf[3] = 0x01;
	zexpect_equal(eth_rstp_bpdu_decode(buf, sizeof(rst_bpdu), &bpdu), -EINVAL);
}

/* Timer values are units of 1/256 s (14.2.8), held in whole seconds */
ZTEST(rstp_bpdu, test_decode_rounds_times)
{
	struct eth_rstp_bpdu bpdu;
	uint8_t buf[sizeof(rst_bpdu)];

	memcpy(buf, rst_bpdu, sizeof(rst_bpdu));
	buf[27] = 0x01;
	buf[28] = 0x7f;
	buf[29] = 0x13;
	buf[30] = 0x80;
	zassert_ok(eth_rstp_bpdu_decode(buf, sizeof(buf), &bpdu));
	zexpect_equal(bpdu.times.message_age, 1U);
	zexpect_equal(bpdu.times.max_age, 20U);
}

ZTEST(rstp_bpdu, test_encode)
{
	uint8_t buf[ETH_RSTP_BPDU_MAX_LEN];
	struct eth_rstp_bpdu tcn = {.type = ETH_RSTP_BPDU_TCN};

	zassert_equal(eth_rstp_bpdu_encode(&config_decoded, buf, sizeof(buf)), sizeof(config_bpdu));
	zexpect_mem_equal(buf, config_bpdu, sizeof(config_bpdu));

	zassert_equal(eth_rstp_bpdu_encode(&rst_decoded, buf, sizeof(buf)), sizeof(rst_bpdu));
	zexpect_mem_equal(buf, rst_bpdu, sizeof(rst_bpdu));

	zassert_equal(eth_rstp_bpdu_encode(&tcn, buf, sizeof(buf)), sizeof(tcn_bpdu));
	zexpect_mem_equal(buf, tcn_bpdu, sizeof(tcn_bpdu));
}

/* Flags not defined for the BPDU type are transmitted as 0 (14.4 b to g) */
ZTEST(rstp_bpdu, test_encode_clears_undefined_flags)
{
	uint8_t buf[ETH_RSTP_BPDU_MAX_LEN];
	struct eth_rstp_bpdu bpdu = config_decoded;

	bpdu.flags = 0xff;
	zassert_equal(eth_rstp_bpdu_encode(&bpdu, buf, sizeof(buf)), sizeof(config_bpdu));
	zexpect_equal(buf[4], ETH_RSTP_FLAG_TC | ETH_RSTP_FLAG_TC_ACK);

	bpdu = rst_decoded;
	bpdu.flags = 0xff;
	zassert_equal(eth_rstp_bpdu_encode(&bpdu, buf, sizeof(buf)), sizeof(rst_bpdu));
	zexpect_equal(buf[4], (uint8_t)~ETH_RSTP_FLAG_TC_ACK);
}

ZTEST(rstp_bpdu, test_encode_short_buffer)
{
	uint8_t buf[ETH_RSTP_BPDU_MAX_LEN];

	zexpect_equal(eth_rstp_bpdu_encode(&rst_decoded, buf, sizeof(rst_bpdu) - 1U), 0U);
	zexpect_equal(eth_rstp_bpdu_encode(&config_decoded, buf, sizeof(config_bpdu) - 1U), 0U);
}

ZTEST_SUITE(rstp_bpdu, NULL, NULL, NULL, NULL, NULL);
