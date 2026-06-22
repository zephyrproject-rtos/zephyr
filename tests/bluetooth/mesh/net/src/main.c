/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net_buf.h>
#include <zephyr/bluetooth/mesh.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <errno.h>
#include <string.h>
#include "crypto.h"
#include "net.h"

/**** Mocked functions ****/

/* Mocked address check: always treat any address as non-local for this test. */
bool bt_mesh_has_addr(uint16_t addr)
{
	return false;
}

/* Mocked obfuscation/decryption: keep header readable and report success. */
int bt_mesh_net_obfuscate(uint8_t *pdu, uint32_t iv_index, const struct bt_mesh_key *privacy_key)
{
	ARG_UNUSED(pdu);
	ARG_UNUSED(iv_index);
	ARG_UNUSED(privacy_key);

	return 0;
}

/* Mocked decryption function. */
int bt_mesh_net_decrypt(const struct bt_mesh_key *key, struct net_buf_simple *buf,
			uint32_t iv_index, enum bt_mesh_nonce_type type)
{
	ARG_UNUSED(key);
	ARG_UNUSED(buf);
	ARG_UNUSED(iv_index);
	ARG_UNUSED(type);

	return 0;
}

/* Mocked bt_hex logging function. */
const char *bt_hex(const void *buf, size_t len)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	return "";
}

/* Test-local subnets tied to NID values so cache includes net_idx. */
static struct bt_mesh_subnet test_subnet_1 = { .net_idx = 0x0001 };
static struct bt_mesh_subnet test_subnet_2 = { .net_idx = 0x0002 };

/* Minimalistic mock credential finder: pick subnet by NID and invoke callback once. */
bool bt_mesh_net_cred_find(struct bt_mesh_net_rx *rx, struct net_buf_simple *in,
				struct net_buf_simple *out,
				bool (*cb)(struct bt_mesh_net_rx *rx,
					struct net_buf_simple *in,
					struct net_buf_simple *out,
					const struct bt_mesh_net_cred *cred))
{
	struct bt_mesh_net_cred cred = { 0 };
	uint8_t nid = in->data[0] & 0x7f; /* NID(pdu) */

	if (nid == 0x11) {
		rx->sub = &test_subnet_1;
		cred.nid = 0x11;
	} else if (nid == 0x22) {
		rx->sub = &test_subnet_2;
		cred.nid = 0x22;
	} else {
		return false;
	}

	if (cb(rx, in, out, &cred)) {
		rx->new_key = 0U;
		rx->friend_cred = 0U;
		rx->ctx.net_idx = rx->sub->net_idx;
		return true;
	}

	return false;
}

/**** Mocked functions - end ****/

/**** Tests ****/

ZTEST_SUITE(bt_mesh_net_msg_cache, NULL, NULL, NULL, NULL, NULL);

/* Helper to build a minimal Network PDU: 9-byte header + 1B payload + 8B MIC. */
static void build_pdu(uint8_t *dst, uint8_t nid, uint8_t ttl, uint32_t seq,
			uint16_t src, uint16_t daddr, uint8_t mic_tag)
{
		/* IVI(0) | NID | CTL(1) | TTL | SEQ | SRC | DST | PAYLOAD | MIC */
		dst[0] = (0u << 7) | (nid & 0x7f);
		dst[1] = (1u << 7) | (ttl & 0x7f);
		sys_put_be24(seq & 0xFFFFFF, &dst[2]);
		sys_put_be16(src, &dst[5]);
		sys_put_be16(daddr, &dst[7]);
		dst[9] = 0xAA;

		/* 8B MIC - Set the MIC to a unique, easily distinguishable pattern per PDU. */
		dst[10] = mic_tag;
		dst[11] = mic_tag;
		dst[12] = mic_tag;
		dst[13] = mic_tag;
		dst[14] = 0x01;
		dst[15] = 0x01;
		dst[16] = 0x01;
		dst[17] = 0x01;
}

/* Verify that identical SRC+SEQ are accepted if NetKey Index differs, and rejected if same.
 *
 * This test verifies that the message cache differentiates between PDUs with the
 * same SRC and SEQ but different NetKey Index. For this, the test builds three PDUs with the same
 * SRC and SEQ but different NetKey Index, and verifies that the first two PDUs are accepted and
 * the third PDU is rejected. Inorder to bypass 'check_dup()' function, the test builds PDUs with
 * different MICs, and PDU builder function is coded in a way that the MICs are different.
 */
ZTEST(bt_mesh_net_msg_cache, test_cache_differentiates_by_net_idx)
{
		uint8_t pdu1[18];
		uint8_t pdu2[18];
		uint8_t pdu3[18];
		uint8_t out_buf[18];
		struct net_buf_simple in = { 0 };
		struct net_buf_simple out = { 0 };
		struct bt_mesh_net_rx rx = { 0 };

		/* Same SRC and SEQ across PDUs */
		const uint16_t src = 0x1234;
		const uint32_t seq = 0x000123;
		const uint16_t dst = 0xC001; /* arbitrary */

		/* Initialize arbitrary bt_mesh global IV index used in decode path */
		bt_mesh.iv_index = 0;

		/* Create three PDUs:
		 * - pdu1: NID 0x11 (net_idx 0x0001)
		 * - pdu2: NID 0x22 (net_idx 0x0002), same SRC/SEQ (should be accepted)
		 * - pdu3: NID 0x11 (net_idx 0x0001), same SRC/SEQ (should be rejected)
		 * MICs are different on purpose so that check_dup() function does not affect the
		 * test logic for network message cache testing.
		 */
		build_pdu(pdu1, 0x11, 5, seq, src, dst, 0x02);
		build_pdu(pdu2, 0x22, 5, seq, src, dst, 0x03);
		build_pdu(pdu3, 0x11, 5, seq, src, dst, 0x04);

		/* First PDU: expect success */
		net_buf_simple_init_with_data(&in, pdu1, sizeof(pdu1));
		net_buf_simple_init_with_data(&out, out_buf, sizeof(out_buf));

		int err = bt_mesh_net_decode(&in, BT_MESH_NET_IF_ADV, &rx, &out);

		zassert_equal(err, 0, "First PDU decode failed: %d", err);

		/* Second PDU: expect success (not duplicate) */
		net_buf_simple_init_with_data(&in, pdu2, sizeof(pdu2));
		net_buf_simple_reset(&out);
		err = bt_mesh_net_decode(&in, BT_MESH_NET_IF_ADV, &rx, &out);
		zassert_equal(err, 0, "Second PDU decode (different net_idx) failed: %d", err);

		/* Decode third PDU with same NID/net_idx: expect -ENOENT due to cache duplicate */
		net_buf_simple_init_with_data(&in, pdu3, sizeof(pdu3));
		net_buf_simple_reset(&out);
		err = bt_mesh_net_decode(&in, BT_MESH_NET_IF_ADV, &rx, &out);
		zassert_equal(err, -ENOENT, "Third PDU (same net_idx) not rejected: %d", err);
}
