/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
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

void cborattr_test_util_encode(const struct cbor_out_attr_t *attrs,
                               const uint8_t *expected, int len);
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

