/*
 * Copyright (c) 2024 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdbool.h>

#include <zephyr/kernel.h>

static void func1(int a);
static void func2(int a);

static void __noinline func2(int a)
{
	printf("%d: %s\n", a, __func__);

	if (a >= 5) {
		k_oops();
	}

	func1(a + 1);
	printf("bottom %d: %s\n", a, __func__);
}

static void __noinline func1(int a)
{
	printf("%d: %s\n", a, __func__);
	func2(a + 1);
	printf("bottom %d: %s\n", a, __func__);
}

int main(void)
{
	printf("Hello World! %s\n", CONFIG_BOARD);

	func1(1);

	return 0;
}
