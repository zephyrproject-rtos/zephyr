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

/* The standard test expects a cleared flash area.  Make sure it has
 * one.
 */
static void test_clear_settings(void)
{
	if (IS_ENABLED(CONFIG_SETTINGS_FCB)) {
		const struct flash_area *fap;
		int rc = flash_area_open(DT_FLASH_AREA_STORAGE_ID, &fap);

		if (rc == 0) {
			rc = flash_area_erase(fap, 0, fap->fa_size);
			flash_area_close(fap);
		}
		zassert_true(rc == 0, "clear settings failed");
	}
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
	u8_t val1;
	u8_t val2;
	u8_t val3;
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
	u8_t val = 0;

	rc = settings_subsys_init();
	zassert_true(rc == 0, "subsys init failed");


	settings_save_one("ps/ss/ss/val2", &val, sizeof(u8_t));

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

	settings_save_one("ps/ss/val3", &val, sizeof(u8_t));
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

	settings_save_one("ps/val1", &val, sizeof(u8_t));
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

void test_main(void)
{
	ztest_test_suite(settings_test_suite,
			 ztest_unit_test(test_clear_settings),
			 ztest_unit_test(test_support_rtn),
			 ztest_unit_test(test_register_and_loading)
			);

	ztest_run_test_suite(settings_test_suite);
}
