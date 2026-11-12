/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

#include "snmp_ber.h"

static uint8_t buf[64];

static void check_encoding(const uint8_t *expected, size_t expected_len,
			   const struct snmp_ber_writer *w)
{
	zassert_equal(w->pos, expected_len, "Encoded %zu bytes, expected %zu", w->pos,
		      expected_len);
	zassert_mem_equal(w->buf, expected, expected_len, "Encoding differs");
}

ZTEST(snmp_ber, test_int32_roundtrip)
{
	static const int32_t values[] = {0,     1,     -1,     127,    128,       -128,     -129,
					 32767, 32768, -32768, -32769, INT32_MAX, INT32_MIN};

	for (size_t i = 0; i < ARRAY_SIZE(values); i++) {
		struct snmp_ber_writer w;
		struct snmp_ber_reader r;
		int32_t got;

		snmp_ber_writer_init(&w, buf, sizeof(buf));
		zassert_ok(snmp_ber_write_int32(&w, values[i]));
		zassert_equal(w.pos, snmp_ber_sizeof_int32(values[i]), "Size mismatch for %d",
			      values[i]);

		snmp_ber_reader_init(&r, buf, w.pos);
		zassert_ok(snmp_ber_read_int32(&r, &got));
		zassert_equal(got, values[i], "Got %d for %d", got, values[i]);
		zassert_true(snmp_ber_reader_at_end(&r), "Trailing bytes");
	}
}

ZTEST(snmp_ber, test_int32_encoding)
{
	static const uint8_t exp_128[] = {0x02, 0x02, 0x00, 0x80};
	static const uint8_t exp_neg129[] = {0x02, 0x02, 0xFF, 0x7F};
	static const uint8_t exp_neg1[] = {0x02, 0x01, 0xFF};
	struct snmp_ber_writer w;

	snmp_ber_writer_init(&w, buf, sizeof(buf));
	zassert_ok(snmp_ber_write_int32(&w, 128));
	check_encoding(exp_128, sizeof(exp_128), &w);

	snmp_ber_writer_init(&w, buf, sizeof(buf));
	zassert_ok(snmp_ber_write_int32(&w, -129));
	check_encoding(exp_neg129, sizeof(exp_neg129), &w);

	snmp_ber_writer_init(&w, buf, sizeof(buf));
	zassert_ok(snmp_ber_write_int32(&w, -1));
	check_encoding(exp_neg1, sizeof(exp_neg1), &w);
}

ZTEST(snmp_ber, test_uint32_encoding)
{
	static const uint8_t exp_128[] = {0x41, 0x02, 0x00, 0x80};
	static const uint8_t exp_max[] = {0x42, 0x05, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
	static const uint8_t exp_zero[] = {0x43, 0x01, 0x00};
	struct snmp_ber_writer w;
	struct snmp_ber_reader r;
	uint32_t got;

	snmp_ber_writer_init(&w, buf, sizeof(buf));
	zassert_ok(snmp_ber_write_uint32(&w, SNMP_BER_TAG_COUNTER32, 128U));
	check_encoding(exp_128, sizeof(exp_128), &w);

	snmp_ber_writer_init(&w, buf, sizeof(buf));
	zassert_ok(snmp_ber_write_uint32(&w, SNMP_BER_TAG_GAUGE32, UINT32_MAX));
	check_encoding(exp_max, sizeof(exp_max), &w);
	zassert_equal(w.pos, snmp_ber_sizeof_uint32(UINT32_MAX));

	snmp_ber_reader_init(&r, buf, w.pos);
	zassert_ok(snmp_ber_read_uint32(&r, SNMP_BER_TAG_GAUGE32, &got));
	zassert_equal(got, UINT32_MAX);

	snmp_ber_writer_init(&w, buf, sizeof(buf));
	zassert_ok(snmp_ber_write_uint32(&w, SNMP_BER_TAG_TIMETICKS, 0U));
	check_encoding(exp_zero, sizeof(exp_zero), &w);
}

ZTEST(snmp_ber, test_uint64_roundtrip)
{
	static const uint64_t values[] = {
		0U,        1U, 0x80U, UINT32_MAX, (uint64_t)UINT32_MAX + 1U, 0x7FFFFFFFFFFFFFFFULL,
		UINT64_MAX};

	for (size_t i = 0; i < ARRAY_SIZE(values); i++) {
		struct snmp_ber_writer w;
		struct snmp_ber_reader r;
		uint64_t got;

		snmp_ber_writer_init(&w, buf, sizeof(buf));
		zassert_ok(snmp_ber_write_uint64(&w, SNMP_BER_TAG_COUNTER64, values[i]));
		zassert_equal(w.pos, snmp_ber_sizeof_uint64(values[i]));

		snmp_ber_reader_init(&r, buf, w.pos);
		zassert_ok(snmp_ber_read_uint64(&r, SNMP_BER_TAG_COUNTER64, &got));
		zassert_equal(got, values[i], "Got %llu for %llu", got, values[i]);
	}

	zassert_equal(buf[1], 9U, "UINT64_MAX needs a leading zero byte");
}

ZTEST(snmp_ber, test_octets_and_empty)
{
	static const uint8_t expected[] = {0x04, 0x03, 'a', 'b', 'c', 0x05, 0x00, 0x80, 0x00};
	struct snmp_ber_writer w;
	struct snmp_ber_reader r;
	const uint8_t *data;
	size_t len;

	snmp_ber_writer_init(&w, buf, sizeof(buf));
	zassert_ok(
		snmp_ber_write_octets(&w, SNMP_BER_TAG_OCTET_STRING, (const uint8_t *)"abc", 3U));
	zassert_ok(snmp_ber_write_empty(&w, SNMP_BER_TAG_NULL));
	zassert_ok(snmp_ber_write_empty(&w, SNMP_BER_TAG_NO_SUCH_OBJECT));
	check_encoding(expected, sizeof(expected), &w);

	snmp_ber_reader_init(&r, buf, w.pos);
	zassert_ok(snmp_ber_read_octets(&r, SNMP_BER_TAG_OCTET_STRING, &data, &len));
	zassert_equal(len, 3U);
	zassert_mem_equal(data, "abc", 3U);
	zassert_ok(snmp_ber_read_empty(&r, SNMP_BER_TAG_NULL));
	zassert_ok(snmp_ber_read_empty(&r, SNMP_BER_TAG_NO_SUCH_OBJECT));
	zassert_true(snmp_ber_reader_at_end(&r));

	/* An empty OCTET STRING is legal. */
	snmp_ber_writer_init(&w, buf, sizeof(buf));
	zassert_ok(snmp_ber_write_octets(&w, SNMP_BER_TAG_OCTET_STRING, NULL, 0U));
	zassert_equal(w.pos, 2U);
}

ZTEST(snmp_ber, test_oid_encoding)
{
	static const uint32_t sys_descr[] = {1, 3, 6, 1, 2, 1, 1, 1, 0};
	static const uint8_t exp_sys_descr[] = {0x06, 0x08, 0x2B, 0x06, 0x01,
						0x02, 0x01, 0x01, 0x01, 0x00};
	static const uint32_t zero_dot_zero[] = {0, 0};
	static const uint8_t exp_zero_dot_zero[] = {0x06, 0x01, 0x00};
	static const uint32_t enterprise[] = {1, 3, 6, 1, 4, 1, 99999};
	static const uint8_t exp_enterprise[] = {0x06, 0x08, 0x2B, 0x06, 0x01,
						 0x04, 0x01, 0x86, 0x8D, 0x1F};
	static const uint32_t big[] = {2, 4294967215U, UINT32_MAX};
	struct snmp_ber_writer w;
	struct snmp_ber_reader r;
	uint32_t arcs[16];
	size_t len;

	snmp_ber_writer_init(&w, buf, sizeof(buf));
	zassert_ok(snmp_ber_write_oid(&w, sys_descr, ARRAY_SIZE(sys_descr)));
	check_encoding(exp_sys_descr, sizeof(exp_sys_descr), &w);
	zassert_equal(w.pos, snmp_ber_sizeof_oid(sys_descr, ARRAY_SIZE(sys_descr)));

	snmp_ber_reader_init(&r, buf, w.pos);
	zassert_ok(snmp_ber_read_oid(&r, arcs, ARRAY_SIZE(arcs), &len));
	zassert_equal(len, ARRAY_SIZE(sys_descr));
	zassert_mem_equal(arcs, sys_descr, sizeof(sys_descr));

	snmp_ber_writer_init(&w, buf, sizeof(buf));
	zassert_ok(snmp_ber_write_oid(&w, zero_dot_zero, ARRAY_SIZE(zero_dot_zero)));
	check_encoding(exp_zero_dot_zero, sizeof(exp_zero_dot_zero), &w);

	snmp_ber_reader_init(&r, buf, w.pos);
	zassert_ok(snmp_ber_read_oid(&r, arcs, ARRAY_SIZE(arcs), &len));
	zassert_equal(len, 2U);
	zassert_equal(arcs[0], 0U);
	zassert_equal(arcs[1], 0U);

	snmp_ber_writer_init(&w, buf, sizeof(buf));
	zassert_ok(snmp_ber_write_oid(&w, enterprise, ARRAY_SIZE(enterprise)));
	check_encoding(exp_enterprise, sizeof(exp_enterprise), &w);

	/* The largest values that fit: second arc 2.x combines to UINT32_MAX. */
	snmp_ber_writer_init(&w, buf, sizeof(buf));
	zassert_ok(snmp_ber_write_oid(&w, big, ARRAY_SIZE(big)));
	snmp_ber_reader_init(&r, buf, w.pos);
	zassert_ok(snmp_ber_read_oid(&r, arcs, ARRAY_SIZE(arcs), &len));
	zassert_equal(len, 3U);
	zassert_mem_equal(arcs, big, sizeof(big));
}

ZTEST(snmp_ber, test_oid_invalid)
{
	static const uint32_t one_arc[] = {1};
	static const uint32_t bad_first[] = {3, 1};
	static const uint32_t bad_second[] = {1, 40};
	static const uint32_t overflow[] = {2, 4294967216U};
	/* Continuation bit set on the last byte. */
	static const uint8_t truncated[] = {0x06, 0x02, 0x2B, 0x86};
	/* Non-minimal leading 0x80. */
	static const uint8_t leading_zero[] = {0x06, 0x03, 0x2B, 0x80, 0x06};
	/* Sub-identifier wider than 32 bits. */
	static const uint8_t too_wide[] = {0x06, 0x07, 0x2B, 0x90, 0x80, 0x80, 0x80, 0x80, 0x00};
	static const uint8_t empty[] = {0x06, 0x00};
	static const uint8_t valid[] = {0x06, 0x03, 0x2B, 0x06, 0x01};
	struct snmp_ber_writer w;
	struct snmp_ber_reader r;
	uint32_t arcs[16];
	size_t len;

	snmp_ber_writer_init(&w, buf, sizeof(buf));
	zassert_equal(snmp_ber_write_oid(&w, one_arc, ARRAY_SIZE(one_arc)), -EINVAL);
	zassert_equal(snmp_ber_write_oid(&w, bad_first, ARRAY_SIZE(bad_first)), -EINVAL);
	zassert_equal(snmp_ber_write_oid(&w, bad_second, ARRAY_SIZE(bad_second)), -EINVAL);
	zassert_equal(snmp_ber_write_oid(&w, overflow, ARRAY_SIZE(overflow)), -EINVAL);
	zassert_equal(w.pos, 0U, "Nothing may be written on error");
	zassert_equal(snmp_ber_sizeof_oid(one_arc, ARRAY_SIZE(one_arc)), 0U);

	snmp_ber_reader_init(&r, truncated, sizeof(truncated));
	zassert_equal(snmp_ber_read_oid(&r, arcs, ARRAY_SIZE(arcs), &len), -EBADMSG);

	snmp_ber_reader_init(&r, leading_zero, sizeof(leading_zero));
	zassert_equal(snmp_ber_read_oid(&r, arcs, ARRAY_SIZE(arcs), &len), -EBADMSG);

	snmp_ber_reader_init(&r, too_wide, sizeof(too_wide));
	zassert_equal(snmp_ber_read_oid(&r, arcs, ARRAY_SIZE(arcs), &len), -EBADMSG);

	snmp_ber_reader_init(&r, empty, sizeof(empty));
	zassert_equal(snmp_ber_read_oid(&r, arcs, ARRAY_SIZE(arcs), &len), -EBADMSG);

	snmp_ber_reader_init(&r, valid, sizeof(valid));
	zassert_equal(snmp_ber_read_oid(&r, arcs, 2U, &len), -EMSGSIZE);

	snmp_ber_reader_init(&r, valid, sizeof(valid));
	zassert_equal(snmp_ber_read_oid(&r, arcs, 1U, &len), -EMSGSIZE);
}

ZTEST(snmp_ber, test_malformed_input)
{
	static const uint8_t indefinite[] = {0x30, 0x80, 0x02, 0x01, 0x00, 0x00, 0x00};
	static const uint8_t truncated_len[] = {0x30, 0x82, 0x01};
	static const uint8_t too_long[] = {0x30, 0x05, 0x02, 0x01, 0x00};
	static const uint8_t too_wide_len[] = {0x30, 0x89, 0x01, 0x00, 0x00, 0x00,
					       0x00, 0x00, 0x00, 0x00, 0x01, 0x00};
	static const uint8_t reserved_len[] = {0x30, 0xFF, 0x00};
	static const uint8_t multibyte_tag[] = {0x1F, 0x01, 0x00};
	static const uint8_t int_too_wide[] = {0x02, 0x05, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
	static const uint8_t int_empty[] = {0x02, 0x00};
	static const uint8_t uint_negative[] = {0x41, 0x01, 0x80};
	static const uint8_t uint_too_wide[] = {0x41, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00};
	static const uint8_t wrong_tag[] = {0x04, 0x01, 0x00};
	static const uint8_t null_with_content[] = {0x05, 0x01, 0x00};
	static const uint8_t single[] = {0x02};
	struct snmp_ber_reader r, inner;
	int32_t i32;
	uint32_t u32;
	uint8_t tag;

	snmp_ber_reader_init(&r, indefinite, sizeof(indefinite));
	zassert_equal(snmp_ber_read_sequence(&r, SNMP_BER_TAG_SEQUENCE, &inner), -EBADMSG);

	snmp_ber_reader_init(&r, truncated_len, sizeof(truncated_len));
	zassert_equal(snmp_ber_read_sequence(&r, SNMP_BER_TAG_SEQUENCE, &inner), -EBADMSG);

	snmp_ber_reader_init(&r, too_long, sizeof(too_long));
	zassert_equal(snmp_ber_read_sequence(&r, SNMP_BER_TAG_SEQUENCE, &inner), -EBADMSG);

	snmp_ber_reader_init(&r, too_wide_len, sizeof(too_wide_len));
	zassert_equal(snmp_ber_read_sequence(&r, SNMP_BER_TAG_SEQUENCE, &inner), -EBADMSG);

	snmp_ber_reader_init(&r, reserved_len, sizeof(reserved_len));
	zassert_equal(snmp_ber_read_sequence(&r, SNMP_BER_TAG_SEQUENCE, &inner), -EBADMSG);

	snmp_ber_reader_init(&r, multibyte_tag, sizeof(multibyte_tag));
	zassert_equal(snmp_ber_skip(&r), -EBADMSG);

	snmp_ber_reader_init(&r, int_too_wide, sizeof(int_too_wide));
	zassert_equal(snmp_ber_read_int32(&r, &i32), -EBADMSG);

	snmp_ber_reader_init(&r, int_empty, sizeof(int_empty));
	zassert_equal(snmp_ber_read_int32(&r, &i32), -EBADMSG);

	snmp_ber_reader_init(&r, uint_negative, sizeof(uint_negative));
	zassert_equal(snmp_ber_read_uint32(&r, SNMP_BER_TAG_COUNTER32, &u32), -EBADMSG);

	snmp_ber_reader_init(&r, uint_too_wide, sizeof(uint_too_wide));
	zassert_equal(snmp_ber_read_uint32(&r, SNMP_BER_TAG_COUNTER32, &u32), -EBADMSG);

	snmp_ber_reader_init(&r, wrong_tag, sizeof(wrong_tag));
	zassert_equal(snmp_ber_read_int32(&r, &i32), -EBADMSG);

	snmp_ber_reader_init(&r, null_with_content, sizeof(null_with_content));
	zassert_equal(snmp_ber_read_empty(&r, SNMP_BER_TAG_NULL), -EBADMSG);

	snmp_ber_reader_init(&r, single, sizeof(single));
	zassert_equal(snmp_ber_skip(&r), -EBADMSG);
	zassert_ok(snmp_ber_peek_tag(&r, &tag));
	zassert_equal(tag, 0x02U);

	snmp_ber_reader_init(&r, single, 0U);
	zassert_equal(snmp_ber_peek_tag(&r, &tag), -EBADMSG);
}

/* RFC 3417 section 8 (1): a sender may use more length octets than needed. */
ZTEST(snmp_ber, test_non_minimal_lengths)
{
	static const uint8_t short_as_long[] = {0x04, 0x81, 0x02, 'a', 'b'};
	static const uint8_t two_octets[] = {0x04, 0x82, 0x00, 0x02, 'a', 'b'};
	static const uint8_t five_octets[] = {0x04, 0x85, 0x00, 0x00, 0x00, 0x00, 0x02, 'a', 'b'};
	struct snmp_ber_reader r;
	const uint8_t *data;
	size_t len;

	snmp_ber_reader_init(&r, short_as_long, sizeof(short_as_long));
	zassert_ok(snmp_ber_read_octets(&r, SNMP_BER_TAG_OCTET_STRING, &data, &len));
	zassert_equal(len, 2U);

	snmp_ber_reader_init(&r, two_octets, sizeof(two_octets));
	zassert_ok(snmp_ber_read_octets(&r, SNMP_BER_TAG_OCTET_STRING, &data, &len));
	zassert_equal(len, 2U);

	snmp_ber_reader_init(&r, five_octets, sizeof(five_octets));
	zassert_ok(snmp_ber_read_octets(&r, SNMP_BER_TAG_OCTET_STRING, &data, &len));
	zassert_equal(len, 2U);
	zassert_true(snmp_ber_reader_at_end(&r));
}

/* X.690 8.3.2 forbids senders to pad integers; the decoder tolerates it as
 * long as the value still fits its type.
 */
ZTEST(snmp_ber, test_non_minimal_integers)
{
	static const uint8_t padded_pos[] = {0x02, 0x02, 0x00, 0x05};
	static const uint8_t padded_neg[] = {0x02, 0x02, 0xFF, 0xFF};
	static const uint8_t padded_wide[] = {0x02, 0x05, 0x00, 0x00, 0x00, 0x00, 0x05};
	static const uint8_t padded_min[] = {0x02, 0x05, 0xFF, 0x80, 0x00, 0x00, 0x00};
	static const uint8_t padded_u32[] = {0x41, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05};
	static const uint8_t padded_max[] = {0x41, 0x06, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
	static const uint8_t really_wide[] = {0x02, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00};
	struct snmp_ber_reader r;
	int32_t i32;
	uint32_t u32;

	snmp_ber_reader_init(&r, padded_pos, sizeof(padded_pos));
	zassert_ok(snmp_ber_read_int32(&r, &i32));
	zassert_equal(i32, 5);

	snmp_ber_reader_init(&r, padded_neg, sizeof(padded_neg));
	zassert_ok(snmp_ber_read_int32(&r, &i32));
	zassert_equal(i32, -1);

	snmp_ber_reader_init(&r, padded_wide, sizeof(padded_wide));
	zassert_ok(snmp_ber_read_int32(&r, &i32));
	zassert_equal(i32, 5);

	snmp_ber_reader_init(&r, padded_min, sizeof(padded_min));
	zassert_ok(snmp_ber_read_int32(&r, &i32));
	zassert_equal(i32, INT32_MIN);

	snmp_ber_reader_init(&r, padded_u32, sizeof(padded_u32));
	zassert_ok(snmp_ber_read_uint32(&r, SNMP_BER_TAG_COUNTER32, &u32));
	zassert_equal(u32, 5U);

	snmp_ber_reader_init(&r, padded_max, sizeof(padded_max));
	zassert_ok(snmp_ber_read_uint32(&r, SNMP_BER_TAG_COUNTER32, &u32));
	zassert_equal(u32, UINT32_MAX);

	/* Padding never lets a value exceed its type */
	snmp_ber_reader_init(&r, really_wide, sizeof(really_wide));
	zassert_equal(snmp_ber_read_int32(&r, &i32), -EBADMSG);
}

ZTEST(snmp_ber, test_sequence_and_skip)
{
	/* SEQUENCE { INTEGER 5, OCTET STRING "x" } INTEGER 7 */
	static const uint8_t msg[] = {0x30, 0x06, 0x02, 0x01, 0x05, 0x04,
				      0x01, 'x',  0x02, 0x01, 0x07};
	struct snmp_ber_reader r, inner;
	int32_t val;

	snmp_ber_reader_init(&r, msg, sizeof(msg));
	zassert_ok(snmp_ber_read_sequence(&r, SNMP_BER_TAG_SEQUENCE, &inner));
	zassert_equal(inner.len, 6U);
	zassert_ok(snmp_ber_read_int32(&inner, &val));
	zassert_equal(val, 5);
	zassert_ok(snmp_ber_skip(&inner));
	zassert_true(snmp_ber_reader_at_end(&inner));

	zassert_ok(snmp_ber_read_int32(&r, &val));
	zassert_equal(val, 7);
	zassert_true(snmp_ber_reader_at_end(&r));
}

ZTEST(snmp_ber, test_writer_full)
{
	struct snmp_ber_writer w;

	snmp_ber_writer_init(&w, buf, 3U);
	zassert_ok(snmp_ber_write_int32(&w, 1));
	zassert_equal(w.pos, 3U);
	zassert_equal(snmp_ber_sizeof_header(1U), 2U);
	zassert_equal(snmp_ber_sizeof_header(200U), 3U);
	zassert_equal(snmp_ber_sizeof_header(70000U), 5U);
	zassert_equal(snmp_ber_write_int32(&w, 1), -EMSGSIZE);
	zassert_equal(snmp_ber_write_empty(&w, SNMP_BER_TAG_NULL), -EMSGSIZE);
	zassert_equal(snmp_ber_write_header(&w, SNMP_BER_TAG_SEQUENCE, 0U), -EMSGSIZE);
	zassert_equal(w.pos, 3U, "A failed write must not advance");
}

ZTEST(snmp_ber, test_long_lengths)
{
	static uint8_t big[300];
	static uint8_t out[310];
	struct snmp_ber_writer w;
	struct snmp_ber_reader r;
	const uint8_t *data;
	size_t len;

	memset(big, 0xAB, sizeof(big));

	snmp_ber_writer_init(&w, out, sizeof(out));
	zassert_ok(snmp_ber_write_octets(&w, SNMP_BER_TAG_OCTET_STRING, big, 200U));
	zassert_equal(out[1], 0x81U);
	zassert_equal(out[2], 200U);
	zassert_equal(w.pos, 203U);

	snmp_ber_writer_init(&w, out, sizeof(out));
	zassert_ok(snmp_ber_write_octets(&w, SNMP_BER_TAG_OCTET_STRING, big, sizeof(big)));
	zassert_equal(out[1], 0x82U);
	zassert_equal(out[2], 0x01U);
	zassert_equal(out[3], 0x2CU);
	zassert_equal(w.pos, snmp_ber_sizeof_octets(sizeof(big)));

	snmp_ber_reader_init(&r, out, w.pos);
	zassert_ok(snmp_ber_read_octets(&r, SNMP_BER_TAG_OCTET_STRING, &data, &len));
	zassert_equal(len, sizeof(big));
}

ZTEST(snmp_ber, test_prepend)
{
	/* SEQUENCE { INTEGER 1, OCTET STRING "public", INTEGER 300 } */
	static const uint8_t expected[] = {0x30, 0x0F, 0x02, 0x01, 0x01, 0x04, 0x06, 'p', 'u',
					   'b',  'l',  'i',  'c',  0x02, 0x02, 0x01, 0x2C};
	size_t start = sizeof(buf);

	zassert_ok(snmp_ber_prepend_int32(buf, &start, 300));
	zassert_ok(snmp_ber_prepend_octets(buf, &start, SNMP_BER_TAG_OCTET_STRING,
					   (const uint8_t *)"public", 6U));
	zassert_ok(snmp_ber_prepend_int32(buf, &start, 1));
	zassert_ok(
		snmp_ber_prepend_header(buf, &start, SNMP_BER_TAG_SEQUENCE, sizeof(buf) - start));
	zassert_equal(sizeof(buf) - start, sizeof(expected));
	zassert_mem_equal(&buf[start], expected, sizeof(expected));

	start = 1U;
	zassert_equal(snmp_ber_prepend_int32(buf, &start, 1), -EMSGSIZE);
	zassert_equal(snmp_ber_prepend_header(buf, &start, SNMP_BER_TAG_SEQUENCE, 0U), -EMSGSIZE);
	zassert_equal(start, 1U, "A failed prepend must not move the start");
}

ZTEST_SUITE(snmp_ber, NULL, NULL, NULL, NULL, NULL);
