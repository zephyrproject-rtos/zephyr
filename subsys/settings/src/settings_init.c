/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include <errno.h>

#include <zephyr/settings/settings.h>
#include "settings/settings_file.h"
#include <zephyr/zephyr.h>


bool settings_subsys_initialized;

void settings_init(void);

int settings_backend_init(void);

int settings_subsys_init(void)
{

	int err = 0;

	if (settings_subsys_initialized) {
		return 0;
	}

	settings_init();

	err = settings_backend_init(); /* func rises kernel panic once error */

	if (!err) {
		settings_subsys_initialized = true;
	}

	return err;
}
