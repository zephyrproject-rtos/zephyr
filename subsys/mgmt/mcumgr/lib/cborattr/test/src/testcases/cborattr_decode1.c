/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_cborattr.h"

/*
 * Simple decoding.
 */
TEST_CASE(test_cborattr_decode1)
{
	const uint8_t *data;
	int len;
	int rc;
	char test_str_a[4] = { '\0' };
	char test_str_b[4] = { '\0' };
	char test_str_c[4] = { '\0' };
	char test_str_d[4] = { '\0' };
	char test_str_e[4] = { '\0' };
	struct cbor_attr_t test_attrs[] = {
		[0] = {
			.attribute = "a",
			.type = CborAttrTextStringType,
			.addr.string = test_str_a,
			.len = sizeof(test_str_a),
			.nodefault = true
		},
		[1] = {
			.attribute = "b",
			.type = CborAttrTextStringType,
			.addr.string = test_str_b,
			.len = sizeof(test_str_b),
			.nodefault = true
			},
		[2] = {
			.attribute = "c",
			.type = CborAttrTextStringType,
			.addr.string = test_str_c,
			.len = sizeof(test_str_c),
			.nodefault = true
			},
		[3] = {
			.attribute = "d",
			.type = CborAttrTextStringType,
			.addr.string = test_str_d,
			.len = sizeof(test_str_d),
			.nodefault = true,
			},
		[4] = {
			.attribute = "e",
			.type = CborAttrTextStringType,
			.addr.string = test_str_e,
			.len = sizeof(test_str_e),
			.nodefault = true,
		},
		[5] = {
			.attribute = NULL
		}
	};

	data = test_str1(&len);
	rc = cbor_read_flat_attrs(data, len, test_attrs);
	TEST_ASSERT(rc == 0);
	TEST_ASSERT(!strcmp(test_str_a, "A"));
	TEST_ASSERT(!strcmp(test_str_b, "B"));
	TEST_ASSERT(!strcmp(test_str_c, "C"));
	TEST_ASSERT(!strcmp(test_str_d, "D"));
	TEST_ASSERT(!strcmp(test_str_e, "E"));
}
