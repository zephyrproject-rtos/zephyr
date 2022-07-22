/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/sys/byteorder.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <zcbor_bulk/zcbor_bulk_priv.h>

#define zcbor_true_put(zse) zcbor_bool_put(zse, true)

ZTEST(zcbor_bulk, test_correct)
{
	uint8_t buffer[512];
	struct zcbor_string world;
	uint32_t one = 0;
	bool bool_val = false;
	zcbor_state_t zsd[4] = { 0 };
	size_t decoded = 0;
	bool ok;
	struct zcbor_map_decode_key_val dm[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(hello, zcbor_tstr_decode, &world),
		ZCBOR_MAP_DECODE_KEY_VAL(one, zcbor_uint32_decode, &one),
		ZCBOR_MAP_DECODE_KEY_VAL(bool_val, zcbor_bool_decode, &bool_val)
	};

	zcbor_new_encode_state(zsd, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* { "hello":"world", "one":1, "bool_val":true } */
	ok = zcbor_map_start_encode(zsd, 10) &&
	     zcbor_tstr_put_lit(zsd, "hello") && zcbor_tstr_put_lit(zsd, "world")	&&
	     zcbor_tstr_put_lit(zsd, "one") && zcbor_uint32_put(zsd, 1)			&&
	     zcbor_tstr_put_lit(zsd, "bool_val") && zcbor_true_put(zsd)			&&
	     zcbor_map_end_encode(zsd, 10);

	zassert_true(ok, "Expected to be successful in encoding test pattern");

	zcbor_new_decode_state(zsd, 4, buffer, ARRAY_SIZE(buffer), 1);

	int rc = zcbor_map_decode_bulk(zsd, dm, ARRAY_SIZE(dm), &decoded);

	zassert_ok(rc, "Expected 0, got %d", rc);
	zassert_equal(decoded, ARRAY_SIZE(dm), "Expected %d got %d",
		ARRAY_SIZE(dm), decoded);
	zassert_equal(one, 1, "Expected 1");
	zassert_equal(sizeof("world") - 1, world.len, "Expected length %d",
		sizeof("world") - 1);
	zassert_equal(0, memcmp(world.value, "world", world.len),
		"Expected \"world\", got %.*s", world.len, world.value);
	zassert_true(bool_val, "Expected bool_val == true");
}

ZTEST(zcbor_bulk, test_correct_out_of_order)
{
	uint8_t buffer[512];
	struct zcbor_string world;
	uint32_t one = 0;
	bool bool_val = false;
	zcbor_state_t zsd[4] = { 0 };
	size_t decoded = 0;
	bool ok;
	struct zcbor_map_decode_key_val dm[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(hello, zcbor_tstr_decode, &world),
		ZCBOR_MAP_DECODE_KEY_VAL(one, zcbor_uint32_decode, &one),
		ZCBOR_MAP_DECODE_KEY_VAL(bool_val, zcbor_bool_decode, &bool_val)
	};

	zcbor_new_encode_state(zsd, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* { "hello":"world", "one":1, "bool_val":true } */
	ok = zcbor_map_start_encode(zsd, 10) &&
	     zcbor_tstr_put_lit(zsd, "bool_val") && zcbor_true_put(zsd)			&&
	     zcbor_tstr_put_lit(zsd, "one") && zcbor_uint32_put(zsd, 1)			&&
	     zcbor_tstr_put_lit(zsd, "hello") && zcbor_tstr_put_lit(zsd, "world")	&&
	     zcbor_map_end_encode(zsd, 10);

	zassert_true(ok, "Expected to be successful in encoding test pattern");

	zcbor_new_decode_state(zsd, 4, buffer, ARRAY_SIZE(buffer), 1);

	int rc = zcbor_map_decode_bulk(zsd, dm, ARRAY_SIZE(dm), &decoded);

	zassert_ok(rc, "Expected 0, got %d", rc);
	zassert_equal(decoded, ARRAY_SIZE(dm), "Expected %d got %d",
		ARRAY_SIZE(dm), decoded);
	zassert_equal(one, 1, "Expected 1");
	zassert_equal(sizeof("world") - 1, world.len, "Expected length %d",
		sizeof("world") - 1);
	zassert_equal(0, memcmp(world.value, "world", world.len),
		"Expected \"world\", got %.*s", world.len, world.value);
	zassert_true(bool_val, "Expected bool_val == true");
}

ZTEST(zcbor_bulk, test_not_map)
{
	uint8_t buffer[512];
	struct zcbor_string world;
	uint32_t one = 0;
	bool bool_val = false;
	zcbor_state_t zsd[4] = { 0 };
	size_t decoded = 1111;
	bool ok;
	struct zcbor_map_decode_key_val dm[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(hello, zcbor_tstr_decode, &world),
		ZCBOR_MAP_DECODE_KEY_VAL(one, zcbor_uint32_decode, &one),
		ZCBOR_MAP_DECODE_KEY_VAL(bool_val, zcbor_bool_decode, &bool_val)
	};

	zcbor_new_encode_state(zsd, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* List "hello", "world" */
	ok = zcbor_list_start_encode(zsd, 10) &&
	     zcbor_tstr_put_lit(zsd, "hello") && zcbor_tstr_put_lit(zsd, "world")	&&
	     zcbor_list_end_encode(zsd, 10);

	zassert_true(ok, "Expected to be successful in encoding test pattern");

	zcbor_new_decode_state(zsd, 4, buffer, ARRAY_SIZE(buffer), 1);

	int rc = zcbor_map_decode_bulk(zsd, dm, ARRAY_SIZE(dm), &decoded);

	zassert_equal(rc, -EBADMSG, "Expected -EBADMSG(%d), got %d", -EBADMSG, rc);
	zassert_equal(decoded, 1111, "Expected decoded value to be unmodified");
}

ZTEST(zcbor_bulk, test_bad_type)
{
	uint8_t buffer[512];
	struct zcbor_string world = { 0 };
	uint32_t one = 0;
	bool bool_val = false;
	zcbor_state_t zsd[4] = { 0 };
	size_t decoded = 0;
	bool ok;
	struct zcbor_map_decode_key_val dm[] = {
		/* First entry has bad decoder given instead of tstr */
		ZCBOR_MAP_DECODE_KEY_VAL(hello, zcbor_uint32_decode, &world),
		ZCBOR_MAP_DECODE_KEY_VAL(one, zcbor_uint32_decode, &one),
		ZCBOR_MAP_DECODE_KEY_VAL(bool_val, zcbor_bool_decode, &bool_val)
	};

	zcbor_new_encode_state(zsd, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* { "hello":"world", "one":1, "bool_val":true } */
	ok = zcbor_map_start_encode(zsd, 10) &&
	     zcbor_tstr_put_lit(zsd, "hello") && zcbor_tstr_put_lit(zsd, "world")	&&
	     zcbor_tstr_put_lit(zsd, "one") && zcbor_uint32_put(zsd, 1)			&&
	     zcbor_tstr_put_lit(zsd, "bool_val") && zcbor_true_put(zsd)			&&
	     zcbor_map_end_encode(zsd, 10);

	zassert_true(ok, "Expected to be successful in encoding test pattern");

	zcbor_new_decode_state(zsd, 4, buffer, ARRAY_SIZE(buffer), 1);

	int rc = zcbor_map_decode_bulk(zsd, dm, ARRAY_SIZE(dm), &decoded);

	zassert_equal(rc, -ENOMSG, "Expected -ENOMSG, got %d", rc);
	zassert_equal(decoded, 0, "Expected 0 got %d", decoded);
	zassert_equal(one, 0, "Expected 0");
	zassert_equal(0, world.len, "Expected to be unmodified");
	zassert_false(bool_val, "Expected bool_val == false");
}

ZTEST(zcbor_bulk, test_bad_type_2)
{
	uint8_t buffer[512];
	struct zcbor_string world = { 0 };
	uint32_t one = 0;
	bool bool_val = false;
	zcbor_state_t zsd[4] = { 0 };
	size_t decoded = 0;
	bool ok;
	struct zcbor_map_decode_key_val dm[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(hello, zcbor_tstr_decode, &world),
		ZCBOR_MAP_DECODE_KEY_VAL(one, zcbor_uint32_decode, &one),
		/* This is bad decoder for type bool */
		ZCBOR_MAP_DECODE_KEY_VAL(bool_val, zcbor_tstr_decode, &bool_val)
	};

	zcbor_new_encode_state(zsd, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* { "hello":"world", "one":1, "bool_val":true } */
	ok = zcbor_map_start_encode(zsd, 10) &&
	     zcbor_tstr_put_lit(zsd, "hello") && zcbor_tstr_put_lit(zsd, "world")	&&
	     zcbor_tstr_put_lit(zsd, "one") && zcbor_uint32_put(zsd, 1)			&&
	     zcbor_tstr_put_lit(zsd, "bool_val") && zcbor_true_put(zsd)			&&
	     zcbor_map_end_encode(zsd, 10);

	zcbor_new_decode_state(zsd, 4, buffer, ARRAY_SIZE(buffer), 1);

	int rc = zcbor_map_decode_bulk(zsd, dm, ARRAY_SIZE(dm), &decoded);

	zassert_equal(rc, -ENOMSG, "Expected -ENOMSG, got %d", rc);
	zassert_equal(decoded, ARRAY_SIZE(dm) - 1, "Expected %d got %d",
		ARRAY_SIZE(dm) - 1, decoded);
	zassert_equal(one, 1, "Expected 1");
	zassert_equal(sizeof("world") - 1, world.len, "Expected length %d",
		sizeof("world") - 1);
	zassert_equal(0, memcmp(world.value, "world", world.len),
		"Expected \"world\", got %.*s", world.len, world.value);
	zassert_false(bool_val, "Expected bool_val unmodified");
}

ZTEST(zcbor_bulk, test_bad_type_encoded)
{
	uint8_t buffer[512];
	struct zcbor_string world = { 0 };
	uint32_t one = 0;
	bool bool_val = false;
	zcbor_state_t zsd[4] = { 0 };
	size_t decoded = 0;
	bool ok;
	struct zcbor_map_decode_key_val dm[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(hello, zcbor_tstr_decode, &world),
		ZCBOR_MAP_DECODE_KEY_VAL(one, zcbor_uint32_decode, &one),
		ZCBOR_MAP_DECODE_KEY_VAL(bool_val, zcbor_bool_decode, &bool_val)
	};

	zcbor_new_encode_state(zsd, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* { "hello":"world", "one":1, "bool_val":true } */
	ok = zcbor_map_start_encode(zsd, 10) &&
	     zcbor_tstr_put_lit(zsd, "hello") && zcbor_uint32_put(zsd, 10)		&&
	     zcbor_tstr_put_lit(zsd, "one") && zcbor_uint32_put(zsd, 1)			&&
	     zcbor_tstr_put_lit(zsd, "bool_val") && zcbor_true_put(zsd)			&&
	     zcbor_map_end_encode(zsd, 10);

	zassert_true(ok, "Expected to be successful in encoding test pattern");

	zcbor_new_decode_state(zsd, 4, buffer, ARRAY_SIZE(buffer), 1);

	int rc = zcbor_map_decode_bulk(zsd, dm, ARRAY_SIZE(dm), &decoded);

	zassert_equal(rc, -ENOMSG, "Expected -ENOMSG, got %d", rc);
	zassert_equal(decoded, 0, "Expected 0 got %d", decoded);
	zassert_equal(one, 0, "Expected 0");
	zassert_equal(0, world.len, "Expected to be unmodified");
	zassert_false(bool_val, "Expected bool_val == false");
}

ZTEST(zcbor_bulk, test_duplicate)
{
	/* Duplicate key is error and should never happen */
	uint8_t buffer[512];
	struct zcbor_string world = { 0 };
	uint32_t one = 0;
	bool bool_val = false;
	zcbor_state_t zsd[4] = { 0 };
	size_t decoded = 0;
	bool ok;
	struct zcbor_map_decode_key_val dm[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(hello, zcbor_tstr_decode, &world),
		ZCBOR_MAP_DECODE_KEY_VAL(one, zcbor_uint32_decode, &one),
		ZCBOR_MAP_DECODE_KEY_VAL(bool_val, zcbor_bool_decode, &bool_val)
	};

	zcbor_new_encode_state(zsd, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* { "hello":"world", "hello":"world" } */
	ok = zcbor_map_start_encode(zsd, 10) &&
	     zcbor_tstr_put_lit(zsd, "hello") && zcbor_tstr_put_lit(zsd, "world")	&&
	     zcbor_tstr_put_lit(zsd, "hello") && zcbor_tstr_put_lit(zsd, "world")	&&
	     zcbor_map_end_encode(zsd, 10);

	zcbor_new_decode_state(zsd, 4, buffer, ARRAY_SIZE(buffer), 1);

	int rc = zcbor_map_decode_bulk(zsd, dm, ARRAY_SIZE(dm), &decoded);

	zassert_equal(rc, -EADDRINUSE, "Expected -EADDRINUSE, got %d", rc);
	zassert_equal(decoded, 1, "Expected %d got %d",
		ARRAY_SIZE(dm) - 1, decoded);
	zassert_equal(one, 0, "Expected unmodified");
	zassert_equal(sizeof("world") - 1, world.len, "Expected length %d",
		sizeof("world") - 1);
	zassert_equal(0, memcmp(world.value, "world", world.len),
		"Expected \"world\", got %.*s", world.len, world.value);
	zassert_false(bool_val, "Expected bool_val unmodified");
}

ZTEST_SUITE(zcbor_bulk, NULL, NULL, NULL, NULL, NULL);
