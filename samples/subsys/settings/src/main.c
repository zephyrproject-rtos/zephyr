/*
 * Copyright (c) 2020 Intive
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <sys/printk.h>
#include <settings/settings.h>
#include <string.h>
#include <stdio.h>

static int prefs_set_handler(const char *key, size_t len_rd,
			     settings_read_cb read_cb, void *cb_arg);
static int prefs_export_handler(int (*storage_func)(const char *name,
						    const void *value,
						    size_t val_len));

struct settings_handler prefs = { .name = "my_prefs",
				  .h_set = prefs_set_handler,
				  .h_export = prefs_export_handler };

#define STRING_SIZE 30

#define NUMBER_NAME "number"
#define ANOTHER_NUMBER_NAME "anthr_number"
#define STRING_NAME "str"

#define FULL_NUMBER_NAME "my_prefs/number"
#define FULL_ANOTHER_NUMBER_NAME "my_prefs/anthr_number"
#define FULL_STRING_NAME "my_prefs/str"

static double number;
static int another_number;
static char str[STRING_SIZE];

static int prefs_set_handler(const char *key, size_t len_rd,
			     settings_read_cb read_cb, void *cb_arg)
{
	ssize_t len = 0;
	int key_len;
	const char *next;

	key_len = settings_name_next(key, &next);

	if (!next) {
		/* Note: We compare to name WITHOUT prefix! */
		if (strncmp(key, NUMBER_NAME, key_len) == 0) {
			if (len_rd == sizeof(number)) {
				len = read_cb(cb_arg, &number, sizeof(number));
			} else {
				len = -EIO;
			}
		}
		if (strncmp(key, ANOTHER_NUMBER_NAME, key_len) == 0) {
			if (len_rd == sizeof(another_number)) {
				len = read_cb(cb_arg, &another_number,
					      sizeof(another_number));
			} else {
				len = -EIO;
			}
		}
		if (strncmp(key, STRING_NAME, key_len) == 0) {
			if (len_rd == sizeof(str)) {
				len = read_cb(cb_arg, &str, sizeof(str));
			} else {
				len = -EIO;
			}
		}
		printk("Restoring key %s, result:%d\n", key, len);
	}
	return len >= 0 ? 0 : len;
}

static int prefs_export_handler(int (*storage_func)(const char *name,
						    const void *value,
						    size_t val_len))
{
	printk("%s is called\n", __func__);
	/* Note: To save value we use full name (with "my_prefs/" prefix) */
	int err = storage_func(FULL_NUMBER_NAME, &number, sizeof(number));

	if (err != 0) {
		printk("Error %d while storing number.\n", err);
		goto exit;
	}
	err = storage_func(FULL_ANOTHER_NUMBER_NAME, &another_number,
			   sizeof(another_number));
	if (err != 0) {
		printk("Error %d while storing another_number.\n", err);
		goto exit;
	}
	err = storage_func(FULL_STRING_NAME, &str, sizeof(str));
	if (err != 0) {
		printk("Error %d while storing str.\n", err);
		goto exit;
	}
exit:
	return err;
}

static void show_values(void)
{
	char tmp[10];

	snprintf(tmp, sizeof(tmp), "%f", number);
	printk("Number = %s\n", tmp);
	printk("Another Number = %d\n", another_number);
	printk("Str = %s\n\n", str);
}

void main(void)
{
	printk("Settings sample\n");
	int err = settings_subsys_init();

	if (err) {
		printk("settings_subsys_init failed (err %d)\n", err);
	}
	err = settings_register(&prefs);
	if (err) {
		printk("settings_register failed (err %d)\n", err);
		return;
	}

	/* Set values to variables, and save it to settings */
	printk("Setup and save variables:\n");
	number = 33.8;
	another_number = 2;
	strncpy(str, "This is string", STRING_SIZE);
	show_values();
	settings_save(); /* This will call prefs_export_handler under hood */
	k_sleep(K_SECONDS(2));

	/* rewrite variables, and load it from settings */
	printk("Cleanup of variables:\n");
	number = 0;
	another_number = 0;
	strncpy(str, "--- NOTHING :D ---", STRING_SIZE);
	show_values();
	k_sleep(K_SECONDS(2));

	printk("Now we are loading settings:\n");
	settings_load(); /* This will call prefs_set_handler under hood */
	k_sleep(K_SECONDS(2));
	show_values();
	k_sleep(K_SECONDS(2));

	printk("Now we save just a single item:\n");
	int this_will_be_saved = 444;
	/* none of those will be saved */
	number = 0;
	another_number = 0;
	strncpy(str, "--- NOTHING :D ---", STRING_SIZE);
	show_values();
	/* This call will not run prefs_export_handler */
	settings_save_one(FULL_ANOTHER_NUMBER_NAME, &this_will_be_saved,
			  sizeof(this_will_be_saved));
	k_sleep(K_SECONDS(2));

	printk("Now we loading settings with just one value change:\n");
	settings_load(); /* This will call prefs_set_handler under hood */
	show_values();
}
