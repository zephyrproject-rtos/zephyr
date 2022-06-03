/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <ztest.h>

#include <zephyr.h>
#include <sys/reboot.h>

static uint8_t rb_type;

uint8_t test_init_setup(void)
{
	uint8_t type;

	type = sys_get_reboot_type();

	if (type == SYS_REBOOT_WARM) {
		TC_PRINT("Attempts to reboot the system.\n");
		k_sleep(K_MSEC(100));

		sys_reboot(SYS_REBOOT_COLD);
		zassert_unreachable("reboot didn't' happen");
		while (true) {
		}
	}
	return type;
}

void test_get_reboot_type(void)
{
	zassert_true(rb_type == SYS_REBOOT_COLD,
		     "get reboot type , was %d, expected %d", rb_type,
		     SYS_REBOOT_COLD);
}

void test_main(void)
{
	/* Bellow call is not used as a test setup intentionally.    */
	/* It causes device reboot at the first device run after it */
	/* was powered. */

	rb_type = test_init_setup();

	ztest_test_suite(test_sys_reboot,
			 ztest_unit_test(test_get_reboot_type)
			);

	ztest_run_test_suite(test_sys_reboot);
}
