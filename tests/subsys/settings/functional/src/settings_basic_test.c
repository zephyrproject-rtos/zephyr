/*
 * Copyright (c) 2018 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief Settings functional test suite
 *
 */

#include <zephyr.h>
#include <ztest.h>
#include <errno.h>
#include <settings/settings.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(settings_basic_test);

#if defined(CONFIG_SETTINGS_FCB) || defined(CONFIG_SETTINGS_NVS)
#include <storage/flash_map.h>
#endif
#if IS_ENABLED(CONFIG_SETTINGS_FS)
#include <fs/fs.h>
#include <fs/littlefs.h>
#endif

/* The standard test expects a cleared flash area.  Make sure it has
 * one.
 */
static void test_clear_settings(void)
{
#if IS_ENABLED(CONFIG_SETTINGS_FCB) || IS_ENABLED(CONFIG_SETTINGS_NVS)
	const struct flash_area *fap;
	int rc = flash_area_open(FLASH_AREA_ID(storage), &fap);

	if (rc == 0) {
		rc = flash_area_erase(fap, 0, fap->fa_size);
		flash_area_close(fap);
	}
	zassert_true(rc == 0, "clear settings failed");
#endif
#if IS_ENABLED(CONFIG_SETTINGS_FS)
	FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(cstorage);

	/* mounting info */
	static struct fs_mount_t littlefs_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &cstorage,
	.storage_dev = (void *)FLASH_AREA_ID(storage),
	.mnt_point = "/ff"
};

	int rc;

	rc = fs_mount(&littlefs_mnt);
	zassert_true(rc == 0, "mounting littlefs [%d]\n", rc);

	rc = fs_unlink(CONFIG_SETTINGS_FS_FILE);
	zassert_true(rc == 0 || rc == -ENOENT,
		     "can't delete config file%d\n", rc);
#endif
}

/*
 * Test the two support routines that settings provides:
 *
 *   settings_name_steq(name, key, next): compares the start of name with key
 *   settings_name_next(name, next): returns the location of the first
 *                                   separator
 */

static void test_support_rtn(void)
{
	const char test1[] = "bt/a/b/c/d";
	const char test2[] = "bt/a/b/c/d=";
	const char *next1, *next2;
	int rc;

	/* complete match: return 1, next = NULL */
	rc = settings_name_steq(test1, "bt/a/b/c/d", &next1);
	zassert_true(rc == 1, "_steq comparison failure");
	zassert_is_null(next1, "_steq comparison next error");
	rc = settings_name_steq(test2, "bt/a/b/c/d", &next2);
	zassert_true(rc == 1, "_steq comparison failure");
	zassert_is_null(next2, "_steq comparison next error");

	/* partial match: return 1, next <> NULL */
	rc = settings_name_steq(test1, "bt/a/b/c", &next1);
	zassert_true(rc == 1, "_steq comparison failure");
	zassert_not_null(next1, "_steq comparison next error");
	zassert_equal_ptr(next1, test1+9, "next points to wrong location");
	rc = settings_name_steq(test2, "bt/a/b/c", &next2);
	zassert_true(rc == 1, "_steq comparison failure");
	zassert_not_null(next2, "_steq comparison next error");
	zassert_equal_ptr(next2, test2+9, "next points to wrong location");

	/* no match: return 0, next = NULL */
	rc = settings_name_steq(test1, "bta", &next1);
	zassert_true(rc == 0, "_steq comparison failure");
	zassert_is_null(next1, "_steq comparison next error");
	rc = settings_name_steq(test2, "bta", &next2);
	zassert_true(rc == 0, "_steq comparison failure");
	zassert_is_null(next2, "_steq comparison next error");

	/* no match: return 0, next = NULL */
	rc = settings_name_steq(test1, "b", &next1);
	zassert_true(rc == 0, "_steq comparison failure");
	zassert_is_null(next1, "_steq comparison next error");
	rc = settings_name_steq(test2, "b", &next2);
	zassert_true(rc == 0, "_steq comparison failure");
	zassert_is_null(next2, "_steq comparison next error");

	/* first separator: return 2, next <> NULL */
	rc = settings_name_next(test1, &next1);
	zassert_true(rc == 2, "_next wrong return value");
	zassert_not_null(next1, "_next wrong next");
	zassert_equal_ptr(next1, test1+3, "next points to wrong location");
	rc = settings_name_next(test2, &next2);
	zassert_true(rc == 2, "_next wrong return value");
	zassert_not_null(next2, "_next wrong next");
	zassert_equal_ptr(next2, test2+3, "next points to wrong location");

	/* second separator: return 1, next <> NULL */
	rc = settings_name_next(next1, &next1);
	zassert_true(rc == 1, "_next wrong return value");
	zassert_not_null(next1, "_next wrong next");
	zassert_equal_ptr(next1, test1+5, "next points to wrong location");
	rc = settings_name_next(next2, &next2);
	zassert_true(rc == 1, "_next wrong return value");
	zassert_not_null(next2, "_next wrong next");
	zassert_equal_ptr(next2, test2+5, "next points to wrong location");

	/* third separator: return 1, next <> NULL */
	rc = settings_name_next(next1, &next1);
	zassert_true(rc == 1, "_next wrong return value");
	zassert_not_null(next1, "_next wrong next");
	rc = settings_name_next(next2, &next2);
	zassert_true(rc == 1, "_next wrong return value");
	zassert_not_null(next2, "_next wrong next");

	/* fourth separator: return 1, next <> NULL */
	rc = settings_name_next(next1, &next1);
	zassert_true(rc == 1, "_next wrong return value");
	zassert_not_null(next1, "_next wrong next");
	rc = settings_name_next(next2, &next2);
	zassert_true(rc == 1, "_next wrong return value");
	zassert_not_null(next2, "_next wrong next");

	/* fifth separator: return 1, next == NULL */
	rc = settings_name_next(next1, &next1);
	zassert_true(rc == 1, "_next wrong return value");
	zassert_is_null(next1, "_next wrong next");
	rc = settings_name_next(next2, &next2);
	zassert_true(rc == 1, "_next wrong return value");
	zassert_is_null(next2, "_next wrong next");

}

struct stored_data {
	uint8_t val1;
	uint8_t val2;
	uint8_t val3;
	bool en1;
	bool en2;
	bool en3;
};

struct stored_data data;

int val1_set(const char *key, size_t len, settings_read_cb read_cb,
	     void *cb_arg)
{
	data.val1 = 1;
	return 0;
}
int val1_commit(void)
{
	data.en1 = true;
	return 0;
}
static struct settings_handler val1_settings = {
	.name = "ps",
	.h_set = val1_set,
	.h_commit = val1_commit,
};

int val2_set(const char *key, size_t len, settings_read_cb read_cb,
	     void *cb_arg)
{
	data.val2 = 2;
	return 0;
}
int val2_commit(void)
{
	data.en2 = true;
	return 0;
}
static struct settings_handler val2_settings = {
	.name = "ps/ss/ss",
	.h_set = val2_set,
	.h_commit = val2_commit,
};

int val3_set(const char *key, size_t len, settings_read_cb read_cb,
	     void *cb_arg)
{
	data.val3 = 3;
	return 0;
}
int val3_commit(void)
{
	data.en3 = true;
	return 0;
}
static struct settings_handler val3_settings = {
	.name = "ps/ss",
	.h_set = val3_set,
	.h_commit = val3_commit,
};

/* helper routine to remove a handler from settings */
int settings_deregister(struct settings_handler *handler)
{
	extern sys_slist_t settings_handlers;

	return sys_slist_find_and_remove(&settings_handlers, &handler->node);
}

static void test_register_and_loading(void)
{
	int rc, err;
	uint8_t val = 0;

	rc = settings_subsys_init();
	zassert_true(rc == 0, "subsys init failed");


	settings_save_one("ps/ss/ss/val2", &val, sizeof(uint8_t));

	memset(&data, 0, sizeof(struct stored_data));

	rc = settings_register(&val1_settings);
	zassert_true(rc == 0, "register of val1 settings failed");

	/* when we load settings now data.val1 should receive the value*/
	rc = settings_load();
	zassert_true(rc == 0, "settings_load failed");
	err = (data.val1 == 1) && (data.val2 == 0) && (data.val3 == 0);
	zassert_true(err, "wrong data value found");
	/* commit is only called for val1_settings */
	err = (data.en1) && (!data.en2) && (!data.en3);
	zassert_true(err, "wrong data enable found");

	/* Next register should be ok */
	rc = settings_register(&val2_settings);
	zassert_true(rc == 0, "register of val2 settings failed");

	/* Next register should fail (same name) */
	rc = settings_register(&val2_settings);
	zassert_true(rc == -EEXIST, "double register of val2 settings allowed");

	memset(&data, 0, sizeof(struct stored_data));
	/* when we load settings now data.val2 should receive the value*/
	rc = settings_load();
	zassert_true(rc == 0, "settings_load failed");
	err = (data.val1 == 0) && (data.val2 == 2) && (data.val3 == 0);
	zassert_true(err, "wrong data value found");
	/* commit is called for val1_settings and val2_settings*/
	err = (data.en1) && (data.en2) && (!data.en3);
	zassert_true(err, "wrong data enable found");

	settings_save_one("ps/ss/val3", &val, sizeof(uint8_t));
	memset(&data, 0, sizeof(struct stored_data));
	/* when we load settings now data.val2 and data.val1 should receive a
	 * value
	 */
	rc = settings_load();
	zassert_true(rc == 0, "settings_load failed");
	err = (data.val1 == 1) && (data.val2 == 2) && (data.val3 == 0);
	zassert_true(err, "wrong data value found");
	/* commit is called for val1_settings and val2_settings*/
	err = (data.en1) && (data.en2) && (!data.en3);
	zassert_true(err, "wrong data enable found");

	/* val3 settings should be inserted in between val1_settings and
	 * val2_settings
	 */
	rc = settings_register(&val3_settings);
	zassert_true(rc == 0, "register of val3 settings failed");
	memset(&data, 0, sizeof(struct stored_data));
	/* when we load settings now data.val2 and data.val3 should receive a
	 * value
	 */
	rc = settings_load();
	zassert_true(rc == 0, "settings_load failed");
	err = (data.val1 == 0) && (data.val2 == 2) && (data.val3 == 3);
	zassert_true(err, "wrong data value found");
	/* commit is called for val1_settings, val2_settings and val3_settings
	 */
	err = (data.en1) && (data.en2) && (data.en3);
	zassert_true(err, "wrong data enable found");

	settings_save_one("ps/val1", &val, sizeof(uint8_t));
	memset(&data, 0, sizeof(struct stored_data));
	/* when we load settings all data should receive a value loaded */
	rc = settings_load();
	zassert_true(rc == 0, "settings_load failed");
	err = (data.val1 == 1) && (data.val2 == 2) && (data.val3 == 3);
	zassert_true(err, "wrong data value found");
	/* commit is called for all settings*/
	err = (data.en1) && (data.en2) && (data.en3);
	zassert_true(err, "wrong data enable found");

	memset(&data, 0, sizeof(struct stored_data));
	/* test subtree loading: subtree "ps/ss" data.val2 and data.val3 should
	 * receive a value
	 */
	rc = settings_load_subtree("ps/ss");
	zassert_true(rc == 0, "settings_load failed");
	err = (data.val1 == 0) && (data.val2 == 2) && (data.val3 == 3);
	zassert_true(err, "wrong data value found");
	/* commit is called for val2_settings and val3_settings */
	err = (!data.en1) && (data.en2) && (data.en3);
	zassert_true(err, "wrong data enable found");

	memset(&data, 0, sizeof(struct stored_data));
	/* test subtree loading: subtree "ps/ss/ss" only data.val2 should
	 * receive a value
	 */
	rc = settings_load_subtree("ps/ss/ss");
	zassert_true(rc == 0, "settings_load failed");
	err = (data.val1 == 0) && (data.val2 == 2) && (data.val3 == 0);
	zassert_true(err, "wrong data value found");
	/* commit is called only for val2_settings */
	err = (!data.en1) && (data.en2) && (!data.en3);
	zassert_true(err, "wrong data enable found");

	/* clean up by deregisterring settings_handler */
	rc = settings_deregister(&val1_settings);
	zassert_true(rc, "deregistering val1_settings failed");

	rc = settings_deregister(&val2_settings);
	zassert_true(rc, "deregistering val1_settings failed");

	rc = settings_deregister(&val3_settings);
	zassert_true(rc, "deregistering val1_settings failed");
}

int val123_set(const char *key, size_t len,
	       settings_read_cb read_cb, void *cb_arg)
{
	int rc;
	uint8_t val;

	zassert_equal(1, len, "Unexpected size");

	rc = read_cb(cb_arg, &val, sizeof(val));
	zassert_equal(sizeof(val), rc, "read_cb failed");

	if (!strcmp("1", key)) {
		data.val1 = val;
		data.en1 = true;
		return 0;
	}
	if (!strcmp("2", key)) {
		data.val2 = val;
		data.en2 = true;
		return 0;
	}
	if (!strcmp("3", key)) {
		data.val3 = val;
		data.en3 = true;
		return 0;
	}

	zassert_unreachable("Unexpected key value: %s", key);

	return 0;
}

static struct settings_handler val123_settings = {
	.name = "val",
	.h_set = val123_set,
};

unsigned int direct_load_cnt;
uint8_t val_directly_loaded;

int direct_loader(
	const char *key,
	size_t len,
	settings_read_cb read_cb,
	void *cb_arg,
	void *param)
{
	int rc;
	uint8_t val;

	zassert_equal(0x1234, (size_t)param, NULL);

	zassert_equal(1, len, NULL);
	zassert_is_null(key, "Unexpected key: %s", key);


	zassert_not_null(cb_arg, NULL);
	rc = read_cb(cb_arg, &val, sizeof(val));
	zassert_equal(sizeof(val), rc, NULL);

	val_directly_loaded = val;
	direct_load_cnt += 1;
	return 0;
}


static void test_direct_loading(void)
{
	int rc;
	uint8_t val;

	val = 11;
	settings_save_one("val/1", &val, sizeof(uint8_t));
	val = 23;
	settings_save_one("val/2", &val, sizeof(uint8_t));
	val = 35;
	settings_save_one("val/3", &val, sizeof(uint8_t));

	rc = settings_register(&val123_settings);
	zassert_true(rc == 0, NULL);
	memset(&data, 0, sizeof(data));

	rc = settings_load();
	zassert_true(rc == 0, NULL);

	zassert_equal(11, data.val1, NULL);
	zassert_equal(23, data.val2, NULL);
	zassert_equal(35, data.val3, NULL);

	/* Load subtree */
	memset(&data, 0, sizeof(data));

	rc = settings_load_subtree("val/2");
	zassert_true(rc == 0, NULL);

	zassert_equal(0,  data.val1, NULL);
	zassert_equal(23, data.val2, NULL);
	zassert_equal(0,  data.val3, NULL);

	/* Direct loading now */
	memset(&data, 0, sizeof(data));
	val_directly_loaded = 0;
	direct_load_cnt = 0;
	rc = settings_load_subtree_direct(
		"val/2",
		direct_loader,
		(void *)0x1234);
	zassert_true(rc == 0, NULL);
	zassert_equal(0, data.val1, NULL);
	zassert_equal(0, data.val2, NULL);
	zassert_equal(0, data.val3, NULL);

	zassert_equal(1, direct_load_cnt, NULL);
	zassert_equal(23, val_directly_loaded, NULL);
}

struct test_loading_data {
	const char *n;
	const char *v;
};

/* Final data */
static const struct test_loading_data data_final[] = {
	{ .n = "val/1", .v = "final 1" },
	{ .n = "val/2", .v = "final 2" },
	{ .n = "val/3", .v = "final 3" },
	{ .n = "val/4", .v = "final 4" },
	{ .n = NULL }
};

/* The counter of the callback called */
static unsigned int data_final_called[ARRAY_SIZE(data_final)];


static int filtered_loader(
	const char *key,
	size_t len,
	settings_read_cb read_cb,
	void *cb_arg)
{
	int rc;
	const char *next;
	char buf[32];
	const struct test_loading_data *ldata;

	printk("-- Called: %s\n", key);

	/* Searching for a element in an array */
	for (ldata = data_final; ldata->n; ldata += 1) {
		if (settings_name_steq(key, ldata->n, &next)) {
			break;
		}
	}
	zassert_not_null(ldata->n, "Unexpected data name: %s", key);
	zassert_is_null(next, NULL);
	zassert_equal(strlen(ldata->v) + 1, len, "e: \"%s\", a:\"%s\"", ldata->v, buf);
	zassert_true(len <= sizeof(buf), NULL);

	rc = read_cb(cb_arg, buf, len);
	zassert_equal(len, rc, NULL);

	zassert_false(strcmp(ldata->v, buf), "e: \"%s\", a:\"%s\"", ldata->v, buf);

	/* Count an element that was properly loaded */
	data_final_called[ldata - data_final] += 1;

	return 0;
}

static struct settings_handler filtered_loader_settings = {
	.name = "filtered_test",
	.h_set = filtered_loader,
};


static int direct_filtered_loader(
	const char *key,
	size_t len,
	settings_read_cb read_cb,
	void *cb_arg,
	void *param)
{
	zassert_equal(0x3456, (size_t)param, NULL);
	return filtered_loader(key, len, read_cb, cb_arg);
}


static void test_direct_loading_filter(void)
{
	int rc;
	const struct test_loading_data *ldata;
	const char *prefix = filtered_loader_settings.name;
	char buffer[48];
	size_t n;

	/* Duplicated data */
	static const struct test_loading_data data_duplicates[] = {
		{ .n = "val/1", .v = "dup abc" },
		{ .n = "val/2", .v = "dup 123" },
		{ .n = "val/3", .v = "dup 11" },
		{ .n = "val/4", .v = "dup 34" },
		{ .n = "val/1", .v = "dup 56" },
		{ .n = "val/2", .v = "dup 7890" },
		{ .n = "val/4", .v = "dup niety" },
		{ .n = "val/3", .v = "dup er" },
		{ .n = "val/3", .v = "dup super" },
		{ .n = "val/3", .v = "dup xxx" },
		{ .n = NULL }
	};

	/* Data that is going to be deleted */
	strcpy(buffer, prefix);
	strcat(buffer, "/to_delete");
	settings_save_one(buffer, "1", 2);
	settings_delete(buffer);

	/* Saving all the data */
	for (ldata = data_duplicates; ldata->n; ++ldata) {
		strcpy(buffer, prefix);
		strcat(buffer, "/");
		strcat(buffer, ldata->n);
		settings_save_one(buffer, ldata->v, strlen(ldata->v) + 1);
	}
	for (ldata = data_final; ldata->n; ++ldata) {
		strcpy(buffer, prefix);
		strcat(buffer, "/");
		strcat(buffer, ldata->n);
		settings_save_one(buffer, ldata->v, strlen(ldata->v) + 1);
	}


	memset(data_final_called, 0, sizeof(data_final_called));

	rc = settings_load_subtree_direct(
		prefix,
		direct_filtered_loader,
		(void *)0x3456);
	zassert_equal(0, rc, NULL);

	/* Check if all the data was called */
	for (n = 0; data_final[n].n; ++n) {
		zassert_equal(1, data_final_called[n],
			"Unexpected number of calls (%u) of (%s) element",
			n, data_final[n].n);
	}

	rc = settings_register(&filtered_loader_settings);
	zassert_true(rc == 0, NULL);

	rc = settings_load_subtree(prefix);
	zassert_equal(0, rc, NULL);

	/* Check if all the data was called */
	for (n = 0; data_final[n].n; ++n) {
		zassert_equal(2, data_final_called[n],
			"Unexpected number of calls (%u) of (%s) element",
			n, data_final[n].n);
	}
}


void test_main(void)
{
	ztest_test_suite(settings_test_suite,
			 ztest_unit_test(test_clear_settings),
			 ztest_unit_test(test_support_rtn),
			 ztest_unit_test(test_register_and_loading),
			 ztest_unit_test(test_direct_loading),
			 ztest_unit_test(test_direct_loading_filter)
			);

	ztest_run_test_suite(settings_test_suite);
}
