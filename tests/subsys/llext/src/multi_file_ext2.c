/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* File 2 of the multi-file linking test */

extern int number;
int ext_number = 0x18;

int ext_sum_fn(int arg)
{
	return arg + number;
}
