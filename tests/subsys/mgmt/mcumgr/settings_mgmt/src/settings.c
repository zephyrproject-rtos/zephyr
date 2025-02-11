/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings.h"
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>

static bool set_called;
static bool get_called;
static bool export_called;
static bool commit_called;
static uint8_t val_aa[4];
static uint8_t val_bb;

static int val_handle_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg);
static int val_handle_commit(void);
static int val_handle_export(int (*cb)(const char *name, const void *value, size_t val_len));
static int val_handle_get(const char *name, char *val, int val_len_max);

SETTINGS_STATIC_HANDLER_DEFINE(val, "test_val", val_handle_get, val_handle_set, val_handle_commit,
			       val_handle_export);

static int val_handle_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	const char *next;
	int rc;

	set_called = true;

	if (settings_name_steq(name, "aa", &next) && !next) {
		if (len != sizeof(val_aa)) {
			return -EINVAL;
		}

		rc = read_cb(cb_arg, val_aa, sizeof(val_aa));

		return 0;
	} else if (settings_name_steq(name, "bb", &next) && !next) {
		if (len != sizeof(val_bb)) {
			return -EINVAL;
		}

		rc = read_cb(cb_arg, &val_bb, sizeof(val_bb));

		return 0;
	}

	return -ENOENT;
}

static int val_handle_commit(void)
{
	commit_called = true;
	return 0;
}

static int val_handle_export(int (*cb)(const char *name, const void *value, size_t val_len))
{
	export_called = true;

	(void)cb("test_val/aa", val_aa, sizeof(val_aa));
	(void)cb("test_val/bb", &val_bb, sizeof(val_bb));

	return 0;
}

static int val_handle_get(const char *name, char *val, int val_len_max)
{
	const char *next;

	get_called = true;

	if (settings_name_steq(name, "aa", &next) && !next) {
		val_len_max = MIN(val_len_max, sizeof(val_aa));
		memcpy(val, val_aa, val_len_max);
		return val_len_max;
	} else if (settings_name_steq(name, "bb", &next) && !next) {
		val_len_max = MIN(val_len_max, sizeof(val_bb));
		memcpy(val, &val_bb, val_len_max);
		return val_len_max;
	}

	return -ENOENT;
}

void settings_state_reset(void)
{
	set_called = false;
	get_called = false;
	export_called = false;
	commit_called = false;
}

void settings_state_get(bool *set, bool *get, bool *export, bool *commit)
{
	*set = set_called;
	*get = get_called;
	*export = export_called;
	*commit = commit_called;
}
