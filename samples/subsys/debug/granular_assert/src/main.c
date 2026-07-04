/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/zassert.h>
#include <setjmp.h>

ZASSERT_GROUP(MYGROUP);

static jmp_buf assert_escape;

/*
 * Custom assert hook. The ZASSERT() failure path is marked unreachable, so the
 * hook must not return normally. Here it longjmp()s back to main() to let the
 * sample continue (and exit cleanly) after demonstrating a failing assertion.
 */
FUNC_NORETURN void zassert_post_action(const char *file, unsigned int line)
{
	printk("(assertion caught[%s:%d]; recovering)\n", file, line);
	longjmp(assert_escape, 1);
}

int main(void)
{
	printk("Granular assert sample\n");
	printk("MYGROUP assert level        = %d\n", CONFIG_MYGROUP_ZASSERT_LEVEL);

	/*
	 * The MYGROUP group is raised, so ZASSERT() below is active.
	 */
	ZASSERT(1 == 1, "passing group assertion is a no-op");
	printk("Passed group ZASSERT(1 == 1)\n");

	printk("Triggering a failing group ZASSERT() ...\n");

	int x = 2;

	if (setjmp(assert_escape) == 0) {
		ZASSERT(x == 3, "x was %d, expected 3", x);
	}

	printk("Continued after the failing ZASSERT()\n");

	return 0;
}
