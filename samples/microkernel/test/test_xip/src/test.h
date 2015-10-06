/* test.h - XIP test header */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
