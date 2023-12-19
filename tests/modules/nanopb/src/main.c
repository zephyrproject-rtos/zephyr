/*
 * Copyright (c) 2023 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <pb_encode.h>
#include <pb_decode.h>

#include <proto/simple.pb.h>
#include <proto/complex.pb.h>

#include "lib.h"

ZTEST(nanopb_tests, test_nanopb_simple)
{
	uint8_t buffer[SimpleMessage_size];
	SimpleMessage msg = SimpleMessage_init_zero;

	for (size_t i = 0; i < sizeof(msg.buffer); ++i) {
		msg.buffer[i] = i;
	}

	pb_ostream_t ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));

	zassert_true(pb_encode(&ostream, SimpleMessage_fields, &msg),
		     "Encoding failed: %s", PB_GET_ERROR(&ostream));

	/* Sanity check, clear data */
	memset(&msg, 0, sizeof(SimpleMessage));

	pb_istream_t istream = pb_istream_from_buffer(buffer, ostream.bytes_written);

	zassert_true(pb_decode(&istream, SimpleMessage_fields, &msg),
		     "Decoding failed: %s", PB_GET_ERROR(&ostream));

	for (size_t i = 0; i < sizeof(msg.buffer); ++i) {
		zassert_equal(msg.buffer[i], i);
	}
}

ZTEST(nanopb_tests, test_nanopb_nested)
{
	uint8_t buffer[ComplexMessage_size];
	ComplexMessage msg = ComplexMessage_init_zero;

	msg.has_nested = true;
	msg.nested.id = 42;
	strcpy(msg.nested.name, "Test name");

	pb_ostream_t ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));

	zassert_true(pb_encode(&ostream, ComplexMessage_fields, &msg),
		     "Encoding failed: %s", PB_GET_ERROR(&ostream));

	/* Sanity check, clear data */
	memset(&msg, 0, sizeof(ComplexMessage));

	pb_istream_t istream = pb_istream_from_buffer(buffer, ostream.bytes_written);

	zassert_true(pb_decode(&istream, ComplexMessage_fields, &msg),
		     "Decoding failed: %s", PB_GET_ERROR(&istream));

	zassert_equal(42, msg.nested.id);
	zassert_true(msg.has_nested);
	zassert_equal(0, strcmp(msg.nested.name, "Test name"));
}

ZTEST(nanopb_tests, test_nanopb_lib)
{
	SimpleMessage msg = SimpleMessage_init_zero;

	lib_fill_message(&msg);

	for (size_t i = 0; i < sizeof(msg.buffer); ++i) {
		zassert_equal(msg.buffer[i], sizeof(msg.buffer) - i);
	}
}

ZTEST_SUITE(nanopb_tests, NULL, NULL, NULL, NULL, NULL);
