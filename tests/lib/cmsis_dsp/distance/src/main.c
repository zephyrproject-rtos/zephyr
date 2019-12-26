/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr.h>

extern void test_distance_u32(void);
extern void test_distance_f32(void);

void test_main(void)
{
	test_distance_u32();
	test_distance_f32();
}
