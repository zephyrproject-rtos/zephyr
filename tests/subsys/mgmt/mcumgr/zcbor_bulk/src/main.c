/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include "mgmt/mcumgr/util/zcbor_bulk.h"

#define zcbor_true_put(zse) zcbor_bool_put(zse, true)

ZTEST(zcbor_bulk, test_ZCBOR_MAP_DECODE_KEY_DECODER)
{
	struct zcbor_string world;
	struct zcbor_map_decode_key_val map_one[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("hello", zcbor_tstr_decode, &world),
	};
	/* ZCBOR_MAP_DECODE_KEY_DECODER should not be used but is still provided */
	struct zcbor_map_decode_key_val map_two[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(hello, zcbor_tstr_decode, &world),
	};

	zassert_ok(strcmp(map_one[0].key.value, "hello"));
	zassert_equal(map_one[0].key.len, sizeof("hello") - 1);
	zassert_equal((void *)map_one[0].decoder, (void *)&zcbor_tstr_decode);
	zassert_equal((void *)map_one[0].value_ptr, (void *)&world);
	/* Both maps should be the same */
	zassert_ok(strcmp(map_one[0].key.value, map_two[0].key.value));
	zassert_equal(map_one[0].key.len, map_two[0].key.len);
	zassert_equal(map_one[0].decoder, map_two[0].decoder);
	zassert_equal(map_one[0].value_ptr, map_two[0].value_ptr);
}

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
		ZCBOR_MAP_DECODE_KEY_DECODER("hello", zcbor_tstr_decode, &world),
		ZCBOR_MAP_DECODE_KEY_DECODER("one", zcbor_uint32_decode, &one),
		ZCBOR_MAP_DECODE_KEY_DECODER("bool val", zcbor_bool_decode, &bool_val)
	};

	zcbor_new_encode_state(zsd, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* { "hello":"world", "one":1, "bool_val":true } */
	ok = zcbor_map_start_encode(zsd, 10) &&
	     zcbor_tstr_put_lit(zsd, "hello") && zcbor_tstr_put_lit(zsd, "world")	&&
	     zcbor_tstr_put_lit(zsd, "one") && zcbor_uint32_put(zsd, 1)			&&
	     zcbor_tstr_put_lit(zsd, "bool val") && zcbor_true_put(zsd)			&&
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
	zassert_true(bool_val, "Expected bool val == true");
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
		ZCBOR_MAP_DECODE_KEY_DECODER("hello", zcbor_tstr_decode, &world),
		ZCBOR_MAP_DECODE_KEY_DECODER("one", zcbor_uint32_decode, &one),
		ZCBOR_MAP_DECODE_KEY_DECODER("bool val", zcbor_bool_decode, &bool_val)
	};

	zcbor_new_encode_state(zsd, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* { "hello":"world", "one":1, "bool_val":true } */
	ok = zcbor_map_start_encode(zsd, 10) &&
	     zcbor_tstr_put_lit(zsd, "bool val") && zcbor_true_put(zsd)			&&
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
	zassert_true(bool_val, "Expected bool val == true");
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
		ZCBOR_MAP_DECODE_KEY_DECODER("hello", zcbor_tstr_decode, &world),
		ZCBOR_MAP_DECODE_KEY_DECODER("one", zcbor_uint32_decode, &one),
		ZCBOR_MAP_DECODE_KEY_DECODER("bool val", zcbor_bool_decode, &bool_val)
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
		ZCBOR_MAP_DECODE_KEY_DECODER("hello", zcbor_uint32_decode, &world),
		ZCBOR_MAP_DECODE_KEY_DECODER("one", zcbor_uint32_decode, &one),
		ZCBOR_MAP_DECODE_KEY_DECODER("bool val", zcbor_bool_decode, &bool_val)
	};

	zcbor_new_encode_state(zsd, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* { "hello":"world", "one":1, "bool_val":true } */
	ok = zcbor_map_start_encode(zsd, 10) &&
	     zcbor_tstr_put_lit(zsd, "hello") && zcbor_tstr_put_lit(zsd, "world")	&&
	     zcbor_tstr_put_lit(zsd, "one") && zcbor_uint32_put(zsd, 1)			&&
	     zcbor_tstr_put_lit(zsd, "bool val") && zcbor_true_put(zsd)			&&
	     zcbor_map_end_encode(zsd, 10);

	zassert_true(ok, "Expected to be successful in encoding test pattern");

	zcbor_new_decode_state(zsd, 4, buffer, ARRAY_SIZE(buffer), 1);

	int rc = zcbor_map_decode_bulk(zsd, dm, ARRAY_SIZE(dm), &decoded);

	zassert_equal(rc, -ENOMSG, "Expected -ENOMSG, got %d", rc);
	zassert_equal(decoded, 0, "Expected 0 got %d", decoded);
	zassert_equal(one, 0, "Expected 0");
	zassert_equal(0, world.len, "Expected to be unmodified");
	zassert_false(bool_val, "Expected bool val == false");
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
		ZCBOR_MAP_DECODE_KEY_DECODER("hello", zcbor_tstr_decode, &world),
		ZCBOR_MAP_DECODE_KEY_DECODER("one", zcbor_uint32_decode, &one),
		/* This is bad decoder for type bool */
		ZCBOR_MAP_DECODE_KEY_DECODER("bool val", zcbor_tstr_decode, &bool_val)
	};

	zcbor_new_encode_state(zsd, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* { "hello":"world", "one":1, "bool_val":true } */
	ok = zcbor_map_start_encode(zsd, 10) &&
	     zcbor_tstr_put_lit(zsd, "hello") && zcbor_tstr_put_lit(zsd, "world")	&&
	     zcbor_tstr_put_lit(zsd, "one") && zcbor_uint32_put(zsd, 1)			&&
	     zcbor_tstr_put_lit(zsd, "bool val") && zcbor_true_put(zsd)			&&
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
	zassert_false(bool_val, "Expected bool val unmodified");
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
		ZCBOR_MAP_DECODE_KEY_DECODER("hello", zcbor_tstr_decode, &world),
		ZCBOR_MAP_DECODE_KEY_DECODER("one", zcbor_uint32_decode, &one),
		ZCBOR_MAP_DECODE_KEY_DECODER("bool val", zcbor_bool_decode, &bool_val)
	};

	zcbor_new_encode_state(zsd, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* { "hello":"world", "one":1, "bool_val":true } */
	ok = zcbor_map_start_encode(zsd, 10) &&
	     zcbor_tstr_put_lit(zsd, "hello") && zcbor_uint32_put(zsd, 10)		&&
	     zcbor_tstr_put_lit(zsd, "one") && zcbor_uint32_put(zsd, 1)			&&
	     zcbor_tstr_put_lit(zsd, "bool val") && zcbor_true_put(zsd)			&&
	     zcbor_map_end_encode(zsd, 10);

	zassert_true(ok, "Expected to be successful in encoding test pattern");

	zcbor_new_decode_state(zsd, 4, buffer, ARRAY_SIZE(buffer), 1);

	int rc = zcbor_map_decode_bulk(zsd, dm, ARRAY_SIZE(dm), &decoded);

	zassert_equal(rc, -ENOMSG, "Expected -ENOMSG, got %d", rc);
	zassert_equal(decoded, 0, "Expected 0 got %d", decoded);
	zassert_equal(one, 0, "Expected 0");
	zassert_equal(0, world.len, "Expected to be unmodified");
	zassert_false(bool_val, "Expected bool val == false");
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
		ZCBOR_MAP_DECODE_KEY_DECODER("hello", zcbor_tstr_decode, &world),
		ZCBOR_MAP_DECODE_KEY_DECODER("one", zcbor_uint32_decode, &one),
		ZCBOR_MAP_DECODE_KEY_DECODER("bool val", zcbor_bool_decode, &bool_val)
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
	zassert_false(bool_val, "Expected bool val unmodified");
}

struct in_map_decoding {
	size_t decoded;
	int ret;
	uint32_t number;
	struct zcbor_string str;
	uint32_t other_number;
};

static bool in_map_decoder(zcbor_state_t *zsd, struct in_map_decoding *imd)
{
	uint32_t dont_exist = 0x64;
	struct zcbor_map_decode_key_val in_map[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(number, zcbor_uint32_decode, &imd->number),
		ZCBOR_MAP_DECODE_KEY_VAL(str, zcbor_tstr_decode, &imd->str),
		ZCBOR_MAP_DECODE_KEY_VAL(dont_exist, zcbor_uint32_decode, &dont_exist),
	};

	imd->ret = zcbor_map_decode_bulk(zsd, in_map, ARRAY_SIZE(in_map), &imd->decoded);

	zassert_equal(dont_exist, 0x64, "dont_exist should not have get modified");

	return (imd->ret == 0);
}

ZTEST(zcbor_bulk, test_map_in_map_correct)
{
	uint8_t buffer[512];
	struct zcbor_string world;
	uint32_t one = 0;
	bool bool_val = false;
	zcbor_state_t zsd[4] = { 0 };
	size_t decoded = 0;
	bool ok;
	struct in_map_decoding imdd = { .ret = -1 };
	struct zcbor_map_decode_key_val dm[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(hello, zcbor_tstr_decode, &world),
		ZCBOR_MAP_DECODE_KEY_VAL(in_map_map, in_map_decoder, &imdd),
		ZCBOR_MAP_DECODE_KEY_VAL(one, zcbor_uint32_decode, &one),
		ZCBOR_MAP_DECODE_KEY_VAL(bool_val, zcbor_bool_decode, &bool_val)
	};

	zcbor_new_encode_state(zsd, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* { "hello":"world",
	 *   "in_map" : {
	 *	"number" : 30,
	 *	"str" : "in_str"
	 *	},
	 *   "one":1,
	 *   "bool_val":true }
	 */
	ok = zcbor_map_start_encode(zsd, 10)						&&
	     zcbor_tstr_put_lit(zsd, "hello") && zcbor_tstr_put_lit(zsd, "world")	&&
	     zcbor_tstr_put_lit(zsd, "in_map_map")					&&
		zcbor_map_start_encode(zsd, 10)						&&
		zcbor_tstr_put_lit(zsd, "number") && zcbor_uint32_put(zsd, 30)		&&
		zcbor_tstr_put_lit(zsd, "str") && zcbor_tstr_put_lit(zsd, "in_str")	&&
		zcbor_map_end_encode(zsd, 10)						&&
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

	/* Map within map */
	zassert_equal(imdd.ret, 0, "Expected successful decoding of inner map");
	zassert_equal(imdd.decoded, 2, "Expected two items in inner map");
	zassert_equal(imdd.number, 30);
	zassert_equal(imdd.str.len, sizeof("in_str") - 1);
	zassert_equal(memcmp("in_str", imdd.str.value, imdd.str.len), 0);
}

static bool in_map_decoder_bad(zcbor_state_t *zsd, struct in_map_decoding *imd)
{
	uint32_t dont_exist = 0x64;
	uint32_t wrong_type = 0x34;
	struct zcbor_map_decode_key_val in_map[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(number, zcbor_uint32_decode, &imd->number),
		ZCBOR_MAP_DECODE_KEY_VAL(str, zcbor_uint32_decode, &wrong_type),
		ZCBOR_MAP_DECODE_KEY_VAL(dont_exist, zcbor_uint32_decode, &dont_exist),
	};

	imd->ret = zcbor_map_decode_bulk(zsd, in_map, ARRAY_SIZE(in_map), &imd->decoded);

	zassert_equal(dont_exist, 0x64, "dont_exist should not have get modified");

	return (imd->ret == 0);
}

ZTEST(zcbor_bulk, test_map_in_map_bad)
{
	uint8_t buffer[512];
	struct zcbor_string world;
	uint32_t one = 0;
	bool bool_val = false;
	zcbor_state_t zsd[4] = { 0 };
	size_t decoded = 0;
	bool ok;
	struct in_map_decoding imdd = { .ret = -1 };
	struct zcbor_map_decode_key_val dm[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(hello, zcbor_tstr_decode, &world),
		ZCBOR_MAP_DECODE_KEY_VAL(in_map_map, in_map_decoder_bad, &imdd),
		ZCBOR_MAP_DECODE_KEY_VAL(one, zcbor_uint32_decode, &one),
		ZCBOR_MAP_DECODE_KEY_VAL(bool_val, zcbor_bool_decode, &bool_val)
	};

	zcbor_new_encode_state(zsd, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* { "hello":"world",
	 *   "in_map" : {
	 *	"number" : 30,
	 *	"str" : "in_str" # Decoding function will expect str to be int
	 *	},
	 *   "one":1,
	 *   "bool_val":true }
	 */
	ok = zcbor_map_start_encode(zsd, 10)						&&
	     zcbor_tstr_put_lit(zsd, "hello") && zcbor_tstr_put_lit(zsd, "world")	&&
	     zcbor_tstr_put_lit(zsd, "in_map_map")					&&
		zcbor_map_start_encode(zsd, 10)						&&
		zcbor_tstr_put_lit(zsd, "number") && zcbor_uint32_put(zsd, 30)		&&
		zcbor_tstr_put_lit(zsd, "str") && zcbor_tstr_put_lit(zsd, "in_str")	&&
		zcbor_map_end_encode(zsd, 10)						&&
	     zcbor_tstr_put_lit(zsd, "one") && zcbor_uint32_put(zsd, 1)			&&
	     zcbor_tstr_put_lit(zsd, "bool_val") && zcbor_true_put(zsd)			&&
	     zcbor_map_end_encode(zsd, 10);

	zassert_true(ok, "Expected to be successful in encoding test pattern");

	zcbor_new_decode_state(zsd, 4, buffer, ARRAY_SIZE(buffer), 1);

	int rc = zcbor_map_decode_bulk(zsd, dm, ARRAY_SIZE(dm), &decoded);

	/* in_map_decoder_bad should fail */
	zassert_equal(rc, -ENOMSG, "Expected ENOMSG(-42), got %d", rc);
	zassert_equal(decoded, 1, "Expected 1 got %d", decoded);

	/* Map within map */
	zassert_equal(imdd.ret, -ENOMSG, "Expected failure in decoding of inner map");
	zassert_equal(imdd.decoded, 1, "Expected 1 item before failure");
	zassert_equal(imdd.number, 30);
}

ZTEST(zcbor_bulk, test_key_found)
{
	uint8_t buffer[512];
	struct zcbor_string world;
	uint32_t one = 0;
	bool bool_val = false;
	zcbor_state_t zsd[4] = { 0 };
	size_t decoded = 0;
	bool ok;
	struct zcbor_map_decode_key_val dm[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("hello", zcbor_tstr_decode, &world),
		ZCBOR_MAP_DECODE_KEY_DECODER("one", zcbor_uint32_decode, &one),
		ZCBOR_MAP_DECODE_KEY_DECODER("bool val", zcbor_bool_decode, &bool_val)
	};

	zcbor_new_encode_state(zsd, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* { "hello":"world", "bool val":true }, "one" is missing and will not be
	 * found.
	 */
	ok = zcbor_map_start_encode(zsd, 10) &&
	     zcbor_tstr_put_lit(zsd, "hello") && zcbor_tstr_put_lit(zsd, "world")	&&
	     zcbor_tstr_put_lit(zsd, "bool val") && zcbor_true_put(zsd)			&&
	     zcbor_map_end_encode(zsd, 10);

	zassert_true(ok, "Expected to be successful in encoding test pattern");

	zcbor_new_decode_state(zsd, 4, buffer, ARRAY_SIZE(buffer), 1);

	int rc = zcbor_map_decode_bulk(zsd, dm, ARRAY_SIZE(dm), &decoded);

	zassert_ok(rc, "Expected 0, got %d", rc);
	zassert_equal(decoded, ARRAY_SIZE(dm) - 1, "Expected %d got %d",
		ARRAY_SIZE(dm), decoded);

	zassert_true(zcbor_map_decode_bulk_key_found(dm, ARRAY_SIZE(dm), "hello"),
		     "Expected \"hello\"");
	zassert_false(zcbor_map_decode_bulk_key_found(dm, ARRAY_SIZE(dm), "one"),
		     "Unexpected \"one\"");
	zassert_true(zcbor_map_decode_bulk_key_found(dm, ARRAY_SIZE(dm), "bool val"),
		     "Expected \"bool val\"");
}

ZTEST_SUITE(zcbor_bulk, NULL, NULL, NULL, NULL, NULL);
