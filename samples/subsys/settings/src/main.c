/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>

#include "settings/settings.h"

#include <errno.h>
#include <sys/printk.h>

#if IS_ENABLED(CONFIG_SETTINGS_FS)
#include <fs/fs.h>
#include <fs/littlefs.h>
#endif

#define GAMMA_DEFAULT_VAl 0
#define FAIL_MSG "fail (err %d)\n"
#define SECTION_BEGIN_LINE \
	"\n=================================================\n"
/* Default valuse are assigned to settings valuses consuments
 * All of them will be overwritten if storage contain proper key-values
 */
uint8_t angle_val;
uint64_t length_val = 100;
uint16_t length_1_val = 40;
uint32_t length_2_val = 60;
int32_t voltage_val = -3000;
char source_name_val[6] = "";

int alpha_handle_set(const char *name, size_t len, settings_read_cb read_cb,
		  void *cb_arg);
int alpha_handle_commit(void);
int alpha_handle_export(int (*cb)(const char *name,
			       const void *value, size_t val_len));

int beta_handle_set(const char *name, size_t len, settings_read_cb read_cb,
		  void *cb_arg);
int beta_handle_commit(void);
int beta_handle_export(int (*cb)(const char *name,
			       const void *value, size_t val_len));
int beta_handle_get(const char *name, char *val, int val_len_max);

/* dynamic main tree handler */
struct settings_handler alph_handler = {
		.name = "alpha",
		.h_get = NULL,
		.h_set = alpha_handle_set,
		.h_commit = alpha_handle_commit,
		.h_export = alpha_handle_export
};

/* static subtree handler */
SETTINGS_STATIC_HANDLER_DEFINE(beta, "alpha/beta", beta_handle_get,
			       beta_handle_set, beta_handle_commit,
			       beta_handle_export);

int alpha_handle_set(const char *name, size_t len, settings_read_cb read_cb,
		  void *cb_arg)
{
	const char *next;
	size_t next_len;
	int rc;

	if (settings_name_steq(name, "angle/1", &next) && !next) {
		if (len != sizeof(angle_val)) {
			return -EINVAL;
		}
		rc = read_cb(cb_arg, &angle_val, sizeof(angle_val));
		printk("<alpha/angle/1> = %d\n", angle_val);
		return 0;
	}

	next_len = settings_name_next(name, &next);

	if (!next) {
		return -ENOENT;
	}

	if (!strncmp(name, "length", next_len)) {
		next_len = settings_name_next(name, &next);

		if (!next) {
			rc = read_cb(cb_arg, &length_val, sizeof(length_val));
			printk("<alpha/length> = %" PRId64 "\n", length_val);
			return 0;
		}

		if (!strncmp(next, "1", next_len)) {
			rc = read_cb(cb_arg, &length_1_val,
				     sizeof(length_1_val));
			printk("<alpha/length/1> = %d\n", length_1_val);
			return 0;
		}

		if (!strncmp(next, "2", next_len)) {
			rc = read_cb(cb_arg, &length_2_val,
				     sizeof(length_2_val));
			printk("<alpha/length/2> = %d\n", length_2_val);
			return 0;
		}

		return -ENOENT;
	}

	return -ENOENT;
}

int beta_handle_set(const char *name, size_t len, settings_read_cb read_cb,
		  void *cb_arg)
{
	const char *next;
	size_t name_len;
	int rc;

	name_len = settings_name_next(name, &next);

	if (!next) {
		if (!strncmp(name, "voltage", name_len)) {
			rc = read_cb(cb_arg, &voltage_val, sizeof(voltage_val));
			printk("<alpha/beta/voltage> = %d\n", voltage_val);
			return 0;
		}

		if (!strncmp(name, "source", name_len)) {
			if (len > sizeof(source_name_val) - 1) {
				printk("<alpha/beta/source> is not compatible "
				       "with the application\n");
				return -EINVAL;
			}
			rc = read_cb(cb_arg, source_name_val,
				     sizeof(source_name_val));
			if (rc < 0) {
				return rc;
			} else if (rc > 0) {
				printk("<alpha/beta/source> = %s\n",
				       source_name_val);
			}
			return 0;
		}
	}

	return -ENOENT;
}

int alpha_handle_commit(void)
{
	printk("loading all settings under <alpha> handler is done\n");
	return 0;
}

int alpha_handle_export(int (*cb)(const char *name,
			       const void *value, size_t val_len))
{
	printk("export keys under <alpha> handler\n");
	(void)cb("alpha/angle/1", &angle_val, sizeof(angle_val));
	(void)cb("alpha/length", &length_val, sizeof(length_val));
	(void)cb("alpha/length/1", &length_1_val, sizeof(length_1_val));
	(void)cb("alpha/length/2", &length_2_val, sizeof(length_2_val));

	return 0;
}

int beta_handle_export(int (*cb)(const char *name,
			       const void *value, size_t val_len))
{
	printk("export keys under <beta> handler\n");
	(void)cb("alpha/beta/voltage", &voltage_val, sizeof(voltage_val));
	(void)cb("alpha/beta/source", source_name_val, strlen(source_name_val) +
						       1);

	return 0;
}

int beta_handle_commit(void)
{
	printk("loading all settings under <beta> handler is done\n");
	return 0;
}

int beta_handle_get(const char *name, char *val, int val_len_max)
{
	const char *next;

	if (settings_name_steq(name, "source", &next) && !next) {
		val_len_max = MIN(val_len_max, strlen(source_name_val));
		memcpy(val, source_name_val, val_len_max);
		return val_len_max;
	}

	return -ENOENT;
}

static void example_save_and_load_basic(void)
{
	int i, rc;
	int32_t val_s32;

	printk(SECTION_BEGIN_LINE);
	printk("basic load and save using registered handlers\n");
	/* load all key-values at once
	 * In case a key-value doesn't exist in the storage
	 * default valuse should be assigned to settings consuments variable
	 * before any settings load call
	 */
	printk("\nload all key-value pairs using registered handlers\n");
	settings_load();

	val_s32 = voltage_val - 25;
	/* save certain key-value directly*/
	printk("\nsave <alpha/beta/voltage> key directly: ");
	rc = settings_save_one("alpha/beta/voltage", (const void *)&val_s32,
			       sizeof(val_s32));
	if (rc) {
		printk(FAIL_MSG, rc);
	}

	printk("OK.\n");

	printk("\nload <alpha/beta> key-value pairs using registered "
	       "handlers\n");
	settings_load_subtree("alpha/beta");

	/* save only modified values
	 * or those that were not saved
	 * before
	 */
	i = strlen(source_name_val);
	if (i < sizeof(source_name_val) - 1) {
		source_name_val[i] = 'a' + i;
		source_name_val[i + 1] = 0;
	} else {
		source_name_val[0] = 0;
	}

	angle_val += 1;

	printk("\nsave all key-value pairs using registered handlers\n");
	settings_save();

	if (++length_1_val > 100) {
		length_1_val = 0;
	}

	if (--length_2_val > 100) {
		length_2_val = 100;
	}

	/*---------------------------
	 * save only modified values
	 * or those that were deleted
	 * before
	 */
	printk("\nload all key-value pairs using registered handlers\n");
	settings_save();
}

struct direct_length_data {
	uint64_t length;
	uint16_t length_1;
	uint32_t length_2;
};

static int direct_loader(const char *name, size_t len, settings_read_cb read_cb,
			  void *cb_arg, void *param)
{
	const char *next;
	size_t name_len;
	int rc;
	struct direct_length_data *dest = (struct direct_length_data *)param;

	printk("direct load: ");

	name_len = settings_name_next(name, &next);

	if (name_len == 0) {
		rc = read_cb(cb_arg, &(dest->length), sizeof(dest->length));
		printk("<alpha/length>\n");
		return 0;
	}

	name_len = settings_name_next(name, &next);
	if (next) {
		printk("nothing\n");
		return -ENOENT;
	}

	if (!strncmp(name, "1", name_len)) {
		rc = read_cb(cb_arg, &(dest->length_1), sizeof(dest->length_1));
		printk("<alpha/length/1>\n");
		return 0;
	}

	if (!strncmp(name, "2", name_len)) {
		rc = read_cb(cb_arg, &(dest->length_2), sizeof(dest->length_2));
		printk("<alpha/length/2>\n");
		return 0;
	}

	printk("nothing\n");
	return -ENOENT;
}

static void example_direct_load_subtree(void)
{
	struct direct_length_data dld;
	int rc;

	/* load subtree directly using call-specific handler `direct_loader'
	 * This handder loads subtree values to call-speciffic structure of type
	 * 'direct_length_data`.
	 */
	printk(SECTION_BEGIN_LINE);
	printk("loading subtree to destination provided by the caller\n\n");
	rc = settings_load_subtree_direct("alpha/length", direct_loader,
					  (void *)&dld);
	if (rc == 0) {
		printk("  direct.length = %" PRId64 "\n", dld.length);
		printk("  direct.length_1 = %d\n", dld.length_1);
		printk("  direct.length_2 = %d\n", dld.length_2);
	} else {
		printk("  direct load fails unexpectedly\n");
	}
}

struct direct_immediate_value {
	size_t len;
	void *dest;
	uint8_t fetched;
};

static int direct_loader_immediate_value(const char *name, size_t len,
					 settings_read_cb read_cb, void *cb_arg,
					 void *param)
{
	const char *next;
	size_t name_len;
	int rc;
	struct direct_immediate_value *one_value =
					(struct direct_immediate_value *)param;

	name_len = settings_name_next(name, &next);

	if (name_len == 0) {
		if (len == one_value->len) {
			rc = read_cb(cb_arg, one_value->dest, len);
			if (rc >= 0) {
				one_value->fetched = 1;
				printk("immediate load: OK.\n");
				return 0;
			}

			printk(FAIL_MSG, rc);
			return rc;
		}
		return -EINVAL;
	}

	/* other keys aren't served by the calback
	 * Return success in order to skip them
	 * and keep storage processing.
	 */
	return 0;
}

int load_immediate_value(const char *name, void *dest, size_t len)
{
	int rc;
	struct direct_immediate_value dov;

	dov.fetched = 0;
	dov.len = len;
	dov.dest = dest;

	rc = settings_load_subtree_direct(name, direct_loader_immediate_value,
					  (void *)&dov);
	if (rc == 0) {
		if (!dov.fetched) {
			rc = -ENOENT;
		}
	}

	return rc;
}

static void example_without_handler(void)
{
	uint8_t val_u8;
	int rc;

	printk(SECTION_BEGIN_LINE);
	printk("Service a key-value pair without dedicated handlers\n\n");
	rc = load_immediate_value("gamma", &val_u8, sizeof(val_u8));
	if (rc == -ENOENT) {
		val_u8 = GAMMA_DEFAULT_VAl;
		printk("<gamma> = %d (default)\n", val_u8);
	} else if (rc == 0) {
		printk("<gamma> = %d\n", val_u8);
	} else {
		printk("unexpected"FAIL_MSG, rc);
	}

	val_u8++;

	printk("save <gamma> key directly: ");
	rc = settings_save_one("gamma", (const void *)&val_u8,
			       sizeof(val_u8));
	if (rc) {
		printk(FAIL_MSG, rc);
	} else {
		printk("OK.\n");
	}
}

static void example_initialization(void)
{
	int rc;

#if IS_ENABLED(CONFIG_SETTINGS_FS)
	FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(cstorage);

	/* mounting info */
	static struct fs_mount_t littlefs_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &cstorage,
	.storage_dev = (void *)FLASH_AREA_ID(storage),
	.mnt_point = "/ff"
	};

	rc = fs_mount(&littlefs_mnt);
	if (rc != 0) {
		printk("mounting littlefs error: [%d]\n", rc);
	} else {

		rc = fs_unlink(CONFIG_SETTINGS_FS_FILE);
		if ((rc != 0) && (rc != -ENOENT)) {
			printk("can't delete config file%d\n", rc);
		} else {
			printk("FS initiqalized: OK\n");
		}
	}
#endif

	rc = settings_subsys_init();
	if (rc) {
		printk("settings subsys initialization: fail (err %d)\n", rc);
		return;
	}

	printk("settings subsys initialization: OK.\n");

	rc = settings_register(&alph_handler);
	if (rc) {
		printk("subtree <%s> handler registered: fail (err %d)\n",
		       alph_handler.name, rc);
	}

	printk("subtree <%s> handler registered: OK\n", alph_handler.name);
	printk("subtree <alpha/beta> has static handler\n");
}

static void example_delete(void)
{
	uint64_t val_u64;
	int rc;

	printk(SECTION_BEGIN_LINE);
	printk("Delete a key-value pair\n\n");

	rc = load_immediate_value("alpha/length", &val_u64, sizeof(val_u64));
	if (rc == 0) {
		printk("  <alpha/length> value exist in the storage\n");
	}

	printk("delete <alpha/length>: ");
	rc = settings_delete("alpha/length");
	if (rc) {
		printk(FAIL_MSG, rc);
	} else {
		printk("OK.\n");
	}

	rc = load_immediate_value("alpha/length", &val_u64, sizeof(val_u64));
	if (rc == -ENOENT) {
		printk("  Can't to load the <alpha/length> value as "
		       "expected\n");
	}
}

void example_runtime_usage(void)
{
	int rc;
	uint8_t injected_str[sizeof(source_name_val)] = "RT";

	printk(SECTION_BEGIN_LINE);
	printk("Inject the value to the setting destination in runtime\n\n");

	rc = settings_runtime_set("alpha/beta/source", (void *) injected_str,
				  strlen(injected_str) + 1);

	printk("injected <alpha/beta/source>: ");
	if (rc) {
		printk(FAIL_MSG, rc);
	} else {
		printk("OK.\n");
	}

	printk("  The settings destination off the key <alpha/beta/source> has "
	       "got value: \"%s\"\n\n", source_name_val);

	/* set settins destination value "by hand" for next example */
	(void) strcpy(source_name_val, "rtos");

	printk(SECTION_BEGIN_LINE);
	printk("Read a value from the setting destination in runtime\n\n");

	rc = settings_runtime_get("alpha/beta/source", (void *) injected_str,
				  strlen(injected_str) + 1);
	printk("fetched <alpha/beta/source>: ");
	if (rc < 0) {
		printk(FAIL_MSG, rc);
	} else {
		printk("OK.\n");
	}

	printk("  String value \"%s\" was retrieved from the settings "
	       "destination off the key <alpha/beta/source>\n",
	       source_name_val);
}

void main(void)
{

	int i;

	printk("\n*** Settings usage example ***\n\n");

	/* settings initialization */
	example_initialization();

	for (i = 0; i < 6; i++) {
		printk("\n##############\n");
		printk("# iteration %d", i);
		printk("\n##############\n");

		/*---------------------------------------------
		 * basic save and load using registered handler
		 */
		example_save_and_load_basic();

		/*-------------------------------------------------
		 *load subtree directly using call-specific handler
		 */
		example_direct_load_subtree();

		/*-------------------------
		 * delete certain kay-value
		 */
		example_delete();

		/*---------------------------------------
		 * a key-value without dedicated handler
		 */
		example_without_handler();
	}

	/*------------------------------------------------------
	 * write and read settings destination using runtime API
	 */
	example_runtime_usage();

	printk("\n*** THE END  ***\n");
}
