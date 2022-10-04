/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <string.h>

#include "settings_test_fs.h"
#include "settings_test.h"
#include "settings_priv.h"

void test_config_setup_littlefs(void);

void test_main(void)
{
	ztest_test_suite(test_config,
			 /* Config tests */
			 ztest_unit_test(test_config_empty_lookups),
			 ztest_unit_test(test_config_insert),
			 ztest_unit_test(test_config_getset_unknown),
			 ztest_unit_test(test_config_getset_int),
			 ztest_unit_test(test_config_getset_int64),
			 ztest_unit_test(test_config_commit),
			 /* Littlefs as backing storage. */
			 ztest_unit_test(test_config_setup_littlefs),
			 ztest_unit_test(test_config_empty_file),
			 ztest_unit_test(test_config_small_file),
			 ztest_unit_test(test_config_multiple_in_file),
			 ztest_unit_test(test_config_save_in_file),
			 ztest_unit_test(test_config_save_one_file),
			 ztest_unit_test(test_config_compress_file)
			);

	ztest_run_test_suite(test_config);
}
