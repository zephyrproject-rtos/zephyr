/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include <errno.h>

#include <settings/settings.h>
#include <zephyr.h>

void settings_init(void);
extern int settings_backend_init(void);

int settings_subsys_init(void)
{
	static bool settings_initialized;
	int rc;

	if (settings_initialized) {
		return 0;
	}

	settings_init();
	rc = settings_backend_init();

	if (!rc) {
		settings_initialized = true;
	}

	return rc;
}


