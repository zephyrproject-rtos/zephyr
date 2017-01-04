/*
 * Copyright (c) 2017 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <console.h>
#include <misc/printk.h>
#include <misc/util.h>

int test_fpgaio(void);

struct test {
	const char *title;
	int (*fn)(void);
};

static const struct test tests[] = {
	{"Buttons and LEDs", test_fpgaio},
};

static const unsigned int num_tests = ARRAY_SIZE(tests);

void main(void)
{
	printk("MPS2 Selftest\n");

	console_getchar_init();

	for (;;) {
		unsigned int t;
		int result;

		printk("\nSelect a test to run:\n");

		for (t = 0; t < num_tests; ++t) {
			printk("%x. %s\n", t, tests[t].title);
		}

		t = console_getchar() - '0';
		if (t >= num_tests) {
			printk("Invalid selection\n");
			continue;
		}

		printk("Running test '%s'...\n", tests[t].title);
		result =  tests[t].fn();
		if (!result) {
			printk("OK\n");
		} else {
			printk("FAIL %d\n", result);
		}
	}
}
