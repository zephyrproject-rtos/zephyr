/*
 *  Copyright (c) 2023 KNS Group LLC (YADRO)
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <stdio.h>

#define WAIT_KOEF 10000

int __noinline func_0_0(void)
{
	k_busy_wait(1*WAIT_KOEF);
	return 0;
}

int __noinline func_0_1(void)
{
	k_busy_wait(2*WAIT_KOEF);
	return 0;
}

int __noinline func_0(void)
{
	k_busy_wait(1*WAIT_KOEF);
	func_0_0();
	func_0_1();
	return 0;
}

int __noinline func_1(void)
{
	k_busy_wait(3*WAIT_KOEF);
	return 0;
}

int __noinline func_2(void)
{
	k_busy_wait(4*WAIT_KOEF);
	return 0;
}

int main(void)
{
	while (1) {
		k_usleep(1000);
		func_0();
		func_1();
		func_2();
	}
	return 0;
}
