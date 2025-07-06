/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/net_buf.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/transport/smp_dummy.h>
#include <zephyr/mgmt/mcumgr/grp/fs_mgmt/fs_mgmt.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <mgmt/mcumgr/transport/smp_internal.h>

#define SMP_RESPONSE_WAIT_TIME 3

/* Test fs_mgmt supported hash/checksum query command */
static const uint8_t command[] = {
	0x00, 0x00, 0x00, 0x02, 0x00, 0x08, 0x01, 0x03,
	0xbf, 0xff,
};

struct hash_checksum_type {
	uint8_t name[32];
	uint32_t format;
	uint32_t size;
	bool found;
	bool entries_matched;
};

ZTEST(fs_mgmt_hash_supported, test_supported)
{
	struct net_buf *nb;
	struct hash_checksum_type expected_types[] = {
#ifdef CONFIG_MCUMGR_GRP_FS_HASH_SHA256

		{
			.name = "sha256",
			.format = 1,
			.size = 32,
			.found = false,
			.entries_matched = false,
		},
#endif
#ifdef CONFIG_MCUMGR_GRP_FS_CHECKSUM_IEEE_CRC32
		{
			.name = "crc32",
			.format = 0,
			.size = 4,
			.found = false,
			.entries_matched = false,
		},
#endif
	};

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send test echo command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(command, sizeof(command));
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	bool received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check that the headers seem valid */
	struct smp_hdr *response_hdr = (struct smp_hdr *)nb->data;
	uint16_t len = ntohs(response_hdr->nh_len);
	uint16_t group = ntohs(response_hdr->nh_group);

	zassert_equal(response_hdr->nh_op, MGMT_OP_READ_RSP,
		      "Expected response to have rease response type");
	zassert_true((len > 20), "Expected response to be at least 20 bytes in length");
	zassert_equal(group, MGMT_GROUP_ID_FS, "Expected response to be FS group");
	zassert_equal(response_hdr->nh_id, FS_MGMT_ID_SUPPORTED_HASH_CHECKSUM,
		      "Expected response to be supported hash/checksum ID");

	/* Process the payload with zcbor and check expected types are present */
	zcbor_state_t state[10];
	struct zcbor_string key;
	uint32_t format_value;
	bool format_found;
	uint32_t size_value;
	bool size_found;
	int8_t entry = 0;
	bool ok = true;

	/* Search expected type array for this type and update details */
	zcbor_new_decode_state(state, 10, &nb->data[sizeof(struct smp_hdr)],
			       (nb->len - sizeof(struct smp_hdr)), 1, NULL, 0);

	ok = zcbor_map_start_decode(state);

	ok = zcbor_tstr_decode(state, &key);

	zassert_equal(key.len, strlen("types"),
		      "Expected CBOR response 'types' value length to match");
	zassert_mem_equal(key.value, "types", strlen("types"),
			  "Expected CBOR response 'types' value to match");

	ok = zcbor_map_start_decode(state);

	while (ok == true) {
		ok = zcbor_tstr_decode(state, &key);
		if (!ok) {
			break;
		}

		entry = 0;
		while (entry < ARRAY_SIZE(expected_types)) {
			if (memcmp(key.value, expected_types[entry].name, MIN(key.len,
					strlen(expected_types[entry].name))) == 0) {
				zassert_equal(expected_types[entry].found, false,
					      "Found entry multiple times");
				expected_types[entry].found = true;
				break;
			}

			++entry;
		}

		zassert_equal(entry < ARRAY_SIZE(expected_types), true,
			      "Did not find entry for type");

		ok = zcbor_map_start_decode(state);
		format_found = false;
		size_found = false;

		while (ok == true) {
			ok = zcbor_tstr_decode(state, &key);

			if (!ok) {
				break;
			}

			if (memcmp(key.value, "format", strlen("format")) == 0) {
				zassert_false(format_found,
					      "Expected format to only be found once");
				ok &= zcbor_uint32_decode(state, &format_value);
				format_found = true;
			} else if (memcmp(key.value, "size", strlen("size")) == 0) {
				zassert_false(size_found, "Expected size to be only found once");
				ok &= zcbor_uint32_decode(state, &size_value);
				size_found = true;
			} else {
				zassert_true(false, "Unexpected field in CBOR response");
			}

			if (format_found == true && size_found == true) {
				zassert_equal(expected_types[entry].format, format_value,
					      "Format value mismatch with expected value");
				zassert_equal(expected_types[entry].size, size_value,
					      "Size value mismatch with expected value");
				expected_types[entry].entries_matched = true;
			}
		}

		ok = zcbor_map_end_decode(state);
	}

	ok = zcbor_map_end_decode(state);
}

ZTEST_SUITE(fs_mgmt_hash_supported, NULL, NULL, NULL, NULL, NULL);
