/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/ztest.h>

#include "tag/tag_internal.h"

ZTEST(nfc_tag, test_t2t_find_ndef)
{
	/* T=03 L=03 V=D0 00 00, then terminator. */
	static const uint8_t data[] = {0x03, 0x03, 0xD0, 0x00, 0x00, 0xFE};
	uint16_t off, len;

	zassert_ok(z_nfc_t2t_find_ndef(data, sizeof(data), &off, &len));
	zassert_equal(off, 2);
	zassert_equal(len, 3);
}

ZTEST(nfc_tag, test_t2t_skip_other_tlv)
{
	/* T=01 (skipped, L=02), then the NDEF TLV. */
	static const uint8_t data[] = {0x01, 0x02, 0xAA, 0xBB, 0x03, 0x01, 0xD0, 0xFE};
	uint16_t off, len;

	zassert_ok(z_nfc_t2t_find_ndef(data, sizeof(data), &off, &len));
	zassert_equal(off, 6);
	zassert_equal(len, 1);
}

ZTEST(nfc_tag, test_t2t_terminator_only)
{
	static const uint8_t data[] = {0xFE};
	uint16_t off, len;

	zassert_equal(z_nfc_t2t_find_ndef(data, sizeof(data), &off, &len), -ENOENT);
}

ZTEST(nfc_tag, test_t2t_truncated)
{
	static const uint8_t data[] = {0x03, 0x05, 0x01};
	uint16_t off, len;

	zassert_equal(z_nfc_t2t_find_ndef(data, sizeof(data), &off, &len), -EAGAIN);
	zassert_equal(len, 5U, "the length must be reported before the value arrives");
}

ZTEST(nfc_tag, test_t4t_parse_cc)
{
	/* CCLEN=000F ver=20 MLe=00FB MLc=00FB, TLV 04 06 fileid=E104 max=00FF r=00 w=00. */
	static const uint8_t cc[] = {0x00, 0x0F, 0x20, 0x00, 0xFB, 0x00, 0xFB, 0x04,
				     0x06, 0xE1, 0x04, 0x00, 0xFF, 0x00, 0x00};
	uint16_t fileid, max_len;
	bool writable;

	zassert_ok(z_nfc_t4t_parse_cc(cc, sizeof(cc), &fileid, &max_len, &writable));
	zassert_equal(fileid, 0xE104);
	zassert_equal(max_len, 0x00FF);
	zassert_true(writable);
}

ZTEST(nfc_tag, test_t4t_cc_readonly)
{
	static const uint8_t cc[] = {0x00, 0x0F, 0x20, 0x00, 0xFB, 0x00, 0xFB, 0x04,
				     0x06, 0xE1, 0x04, 0x00, 0xFF, 0x00, 0xFF};
	uint16_t fileid, max_len;
	bool writable;

	zassert_ok(z_nfc_t4t_parse_cc(cc, sizeof(cc), &fileid, &max_len, &writable));
	zassert_false(writable);
}

ZTEST(nfc_tag, test_t4t_cc_too_short)
{
	static const uint8_t cc[] = {0x00, 0x0F, 0x20, 0x00, 0xFB};
	uint16_t fileid, max_len;
	bool writable;

	zassert_equal(z_nfc_t4t_parse_cc(cc, sizeof(cc), &fileid, &max_len, &writable), -EBADMSG);
}

ZTEST(nfc_tag, test_t4t_cc_bad_tlv)
{
	static const uint8_t cc[] = {0x00, 0x0F, 0x20, 0x00, 0xFB, 0x00, 0xFB, 0x05,
				     0x06, 0xE1, 0x04, 0x00, 0xFF, 0x00, 0x00};
	uint16_t fileid, max_len;
	bool writable;

	zassert_equal(z_nfc_t4t_parse_cc(cc, sizeof(cc), &fileid, &max_len, &writable), -EBADMSG);
}

ZTEST(nfc_tag, test_detect_t4t)
{
	struct nfc_target target = {.tech = NFC_TECH_A, .a.sak = 0x20};
	enum nfc_tag_type type;

	zassert_ok(z_nfc_tag_detect(&target, &type));
	zassert_equal(type, NFC_TAG_TYPE_T4T_A);
}

ZTEST(nfc_tag, test_detect_t2t)
{
	struct nfc_target target = {.tech = NFC_TECH_A, .a.sak = 0x00};
	enum nfc_tag_type type;

	zassert_ok(z_nfc_tag_detect(&target, &type));
	zassert_equal(type, NFC_TAG_TYPE_T2T);
}

ZTEST(nfc_tag, test_detect_nfcb_unsupported)
{
	struct nfc_target target = {.tech = NFC_TECH_B};
	enum nfc_tag_type type;

	zassert_equal(z_nfc_tag_detect(&target, &type), -ENOTSUP);
}

ZTEST_SUITE(nfc_tag, NULL, NULL, NULL, NULL, NULL);
