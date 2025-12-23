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
#include <zephyr/kernel.h>
#include "settings_priv.h"

bool settings_subsys_initialized;

void settings_init(void);

int settings_backend_init(void);

int settings_subsys_init(void)
{

	int err = 0;

	settings_lock_take();

	if (!settings_subsys_initialized) {
		settings_init();

		err = settings_backend_init();

		if (!err) {
			settings_subsys_initialized = true;
		}
	}

	settings_lock_release();

	return err;
}
