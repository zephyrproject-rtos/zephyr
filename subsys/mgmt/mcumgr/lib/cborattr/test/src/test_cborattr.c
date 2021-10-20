/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sysinit/sysinit.h"
#include "syscfg/syscfg.h"
#include "testutil/testutil.h"
#include "test_cborattr.h"

TEST_SUITE(test_cborattr_suite)
{
	test_cborattr_decode1();
	test_cborattr_decode_partial();
	test_cborattr_decode_simple();
	test_cborattr_decode_object();
	test_cborattr_decode_int_array();
	test_cborattr_decode_bool_array();
	test_cborattr_decode_string_array();
	test_cborattr_decode_object_array();
	test_cborattr_decode_unnamed_array();
	test_cborattr_decode_substring_key();
}
