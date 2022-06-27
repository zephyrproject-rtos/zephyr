/*
 * Copyright (c) 2021 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/zephyr.h>

extern void test_quaternionmath_f32(void);

void test_main(void)
{
	test_quaternionmath_f32();
}
