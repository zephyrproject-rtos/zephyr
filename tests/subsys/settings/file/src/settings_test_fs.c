/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <string.h>

#include "settings_test_file.h"
#include "settings_test.h"
#include "settings_priv.h"

ZTEST_SUITE(settings_config, NULL, settings_config_setup, NULL, NULL,
	    settings_config_teardown);
ZTEST_SUITE(settings_config_fs, NULL, config_setup_fs, NULL, NULL, NULL);
