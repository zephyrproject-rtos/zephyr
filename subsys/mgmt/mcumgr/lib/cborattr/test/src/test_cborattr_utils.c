/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_cborattr.h"
/*
 * {"a": "A", "b": "B", "c": "C", "d": "D", "e": "E"}
 */
static const uint8_t test_data1[] = {
	0xa5, 0x61, 0x61, 0x61, 0x41, 0x61, 0x62, 0x61,
	0x42, 0x61, 0x63, 0x61, 0x43, 0x61, 0x64, 0x61,
	0x44, 0x61, 0x65, 0x61, 0x45
};

const uint8_t *
test_str1(int *len)
{
	*len = sizeof(test_data1);
	return test_data1;
}

void
cborattr_test_util_encode(const struct cbor_out_attr_t *attrs, const uint8_t *expected, int len)
{
	struct os_mbuf *om;
	int rc;

	rc = cbor_write_object_msys(attrs, &om);
	TEST_ASSERT_FATAL(rc == 0);

	TEST_ASSERT(os_mbuf_cmpf(om, 0, expected, len) == 0);

	os_mbuf_free_chain(om);
}
