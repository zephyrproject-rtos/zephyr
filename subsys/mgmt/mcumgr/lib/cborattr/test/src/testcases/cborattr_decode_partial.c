/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_cborattr.h"

/*
 * Simple decoding. Only have key for one of the key/value pairs.
 */
TEST_CASE(test_cborattr_decode_partial)
{
	const uint8_t *data;
	int len;
	int rc;
	char test_str_b[4] = { '\0' };
	struct cbor_attr_t test_attrs[] = {
		[0] = {
			.attribute = "b",
			.type = CborAttrTextStringType,
			.addr.string = test_str_b,
			.len = sizeof(test_str_b),
			.nodefault = true
		},
		[1] = {
			.attribute = NULL
		}
	};

	data = test_str1(&len);
	rc = cbor_read_flat_attrs(data, len, test_attrs);
	TEST_ASSERT(rc == 0);
	TEST_ASSERT(!strcmp(test_str_b, "B"));
}
