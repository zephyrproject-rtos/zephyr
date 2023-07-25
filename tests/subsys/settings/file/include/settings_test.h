/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SETTINGS_TEST_H
#define _SETTINGS_TEST_H

#include "settings_test_file.h"

#define TEST_FS_MPTR "/fs"
#define TEST_CONFIG_DIR TEST_FS_MPTR"/settings"

void *config_setup_fs(void);

void *settings_config_setup(void);
void settings_config_teardown(void *fixture);

#endif /* _SETTINGS_TEST_H */
