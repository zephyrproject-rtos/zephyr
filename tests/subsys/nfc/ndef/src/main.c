/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/nfc/ndef.h>
#include <zephyr/ztest.h>

/* RTD Text record, "en" / "Hello": D1 01 08 54 | 02 65 6E 48 65 6C 6C 6F */
static const uint8_t text_msg[] = {0xD1, 0x01, 0x08, 0x54, 0x02, 0x65,
				   0x6E, 0x48, 0x65, 0x6C, 0x6C, 0x6F};

/* Two short records: MB rec 'A', ME rec 'B'. */
static const uint8_t two_msg[] = {0x91, 0x01, 0x01, 0x54, 0x41, 0x51, 0x01, 0x01, 0x54, 0x42};

ZTEST(nfc_ndef, test_parse_text)
{
	struct nfc_ndef_record recs[4];
	size_t count = ARRAY_SIZE(recs);

	zassert_ok(nfc_ndef_msg_parse(text_msg, sizeof(text_msg), recs, &count));
	zassert_equal(count, 1);
	zassert_equal(recs[0].tnf, NFC_NDEF_TNF_WELL_KNOWN);
	zassert_equal(recs[0].type_len, 1);
	zassert_equal(recs[0].type[0], 'T');
	zassert_equal(recs[0].id_len, 0);
	zassert_equal(recs[0].payload_len, 8);
	zassert_equal(recs[0].payload[0], 0x02);
	zassert_equal(recs[0].payload[7], 'o');
}

ZTEST(nfc_ndef, test_roundtrip)
{
	struct nfc_ndef_record recs[4];
	size_t count = ARRAY_SIZE(recs);
	uint8_t out[32];
	size_t out_len = sizeof(out);

	zassert_ok(nfc_ndef_msg_parse(text_msg, sizeof(text_msg), recs, &count));
	zassert_ok(nfc_ndef_msg_encode(recs, count, out, &out_len));
	zassert_equal(out_len, sizeof(text_msg));
	zassert_mem_equal(out, text_msg, out_len);
}

ZTEST(nfc_ndef, test_two_records)
{
	struct nfc_ndef_record recs[4];
	size_t count = ARRAY_SIZE(recs);

	zassert_ok(nfc_ndef_msg_parse(two_msg, sizeof(two_msg), recs, &count));
	zassert_equal(count, 2);
	zassert_equal(recs[0].payload[0], 'A');
	zassert_equal(recs[1].payload[0], 'B');
}

ZTEST(nfc_ndef, test_truncated)
{
	static const uint8_t bad[] = {0xD1, 0x01, 0x08, 0x54, 0x02};
	struct nfc_ndef_record recs[4];
	size_t count = ARRAY_SIZE(recs);

	zassert_equal(nfc_ndef_msg_parse(bad, sizeof(bad), recs, &count), -EBADMSG);
}

ZTEST(nfc_ndef, test_no_message_end)
{
	static const uint8_t bad[] = {0x91, 0x01, 0x01, 0x54, 0x41};
	struct nfc_ndef_record recs[4];
	size_t count = ARRAY_SIZE(recs);

	zassert_equal(nfc_ndef_msg_parse(bad, sizeof(bad), recs, &count), -EBADMSG);
}

ZTEST(nfc_ndef, test_capacity)
{
	struct nfc_ndef_record recs[1];
	size_t count = ARRAY_SIZE(recs);

	zassert_equal(nfc_ndef_msg_parse(two_msg, sizeof(two_msg), recs, &count), -ENOMEM);
}

ZTEST(nfc_ndef, test_encode_nomem)
{
	struct nfc_ndef_record recs[4];
	size_t count = ARRAY_SIZE(recs);
	uint8_t small[4];
	size_t small_len = sizeof(small);

	zassert_ok(nfc_ndef_msg_parse(text_msg, sizeof(text_msg), recs, &count));
	zassert_equal(nfc_ndef_msg_encode(recs, count, small, &small_len), -ENOMEM);
}

ZTEST_SUITE(nfc_ndef, NULL, NULL, NULL, NULL, NULL);
