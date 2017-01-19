/* test.h - XIP test header */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
DESCRIPTION
This header contains defines, externs etc. for the XIP
regression test.
 */


/* This test relies on these values being one larger than the one before */

#define TEST_VAL_1  0x1
#define TEST_VAL_2  0x2
#define TEST_VAL_3  0x3
#define TEST_VAL_4  0x4

#define XIP_TEST_ARRAY_SZ 4

extern uint32_t xip_array[XIP_TEST_ARRAY_SZ];
