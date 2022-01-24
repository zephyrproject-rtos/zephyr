/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_CBORATTR_H
#define TEST_CBORATTR_H

#include <assert.h>
#include <string.h>
#include "testutil/testutil.h"
#include "test_cborattr.h"
#include "tinycbor/cbor.h"
#include "cborattr/cborattr.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Returns test data.
 */
const uint8_t *test_str1(int *len);

void cborattr_test_util_encode(const struct cbor_out_attr_t *attrs, const uint8_t *expected,
			       int len);
/*
 * Testcases
 */
TEST_CASE_DECL(test_cborattr_decode1);
TEST_CASE_DECL(test_cborattr_decode_partial);
TEST_CASE_DECL(test_cborattr_decode_simple);
TEST_CASE_DECL(test_cborattr_decode_object);
TEST_CASE_DECL(test_cborattr_decode_int_array);
TEST_CASE_DECL(test_cborattr_decode_bool_array);
TEST_CASE_DECL(test_cborattr_decode_string_array);
TEST_CASE_DECL(test_cborattr_decode_object_array);
TEST_CASE_DECL(test_cborattr_decode_unnamed_array);
TEST_CASE_DECL(test_cborattr_decode_substring_key);

#ifdef __cplusplus
}
#endif

#endif /* TEST_CBORATTR_H */
