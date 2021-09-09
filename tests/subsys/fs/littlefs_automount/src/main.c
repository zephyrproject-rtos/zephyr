/*
 * Copyright (c) 2021 SILA Embedded Solutions GmbH <office@embedded-solutions.at>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <ztest.h>
#include "test_littlefs_automount.h"

void test_main(void)
{
	ztest_test_suite(littlefs_automount_test,
			 ztest_unit_test(test_automount_opendir),
			 ztest_unit_test(test_automount_check_mounted)
			 );
	ztest_run_test_suite(littlefs_automount_test);
}
