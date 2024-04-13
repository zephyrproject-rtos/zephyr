/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/init.h>

static int i_first, i_fourth, i_third, i_second, i_console;

static int third(void)
{
	printf("Third from zervice!\n");
	i_third++;

	return 0;
}

static int second(void)
{
	printf("Second from zervice!\n");
	i_second++;

	return 0;
}

static int first(void)
{
	printf("First from zervice!\n");
	i_first++;

	return 0;
}

static int fourth(void)
{
	printf("Fourth from zervice!\n");
	i_fourth++;

	return 0;
}

static int after_console(void)
{
	printf("After_console from zervice!\n");
	i_console++;

	return 0;
}

int main(void)
{
	printf("first: %d\nsecond: %d\nthird: %d\nfourth: %d\nconsole: %d",
			i_first, i_second, i_third, i_fourth, i_console);
	return 0;
}

ZERVICE_DEFINE(first, "nxp,kinetis-mcg;nxp,kinetis-gpio", "EARLY;k6x_init");
ZERVICE_DEFINE(second, "", "");
ZERVICE_DEFINE(third, "", "");
ZERVICE_DEFINE(fourth, "", "third");
ZERVICE_DEFINE(after_console, "", "zephyr,console");
