/*
 * Copyright (c) 2019 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
#include <settings/settings.h>
#include <flash_map.h>


/**
 * Settings entry element for sample settings array
 */
struct test_settings_entry {
	const char *name;
	u32_t data;
};


static void setup(void)
{
	/* Clearing the FLASH */
	int rc;
	const struct flash_area *fap;

	rc = flash_area_open(DT_FLASH_AREA_STORAGE_ID, &fap);
	zassert_equal(0, rc, "flash area open call failure (%d)", rc);

	rc = flash_area_erase(fap, 0, fap->fa_size);
	zassert_equal(0, rc, "erase call failure (%d)", rc);

	flash_area_close(fap);

	/* Remove all setting handlers */
	extern bool settings_subsys_initialized;

	settings_subsys_initialized = 0;
	rc = settings_subsys_init();
	zassert_equal(0, rc, "settings initialize failure (%d)", rc);
}

static void teardown(void)
{

}


/**
 * @brief Find a named element in given table
 *
 * @param table      The setting entry array to search in.
 * @param name       The name to find.
 * @param group_name The group to find (the first element in patch)
 *
 * @return The index of the in the table where named settings is found or
 *         negative value if entry was not found.
 */
static ssize_t find_settings_table(const struct test_settings_entry *table,
				   const char *name,
				   const char *group_name)
{
	ssize_t i = 0;

	while (table->name) {
		const char *name_in_table = table->name;

		if (!strncmp(name_in_table, group_name, strlen(group_name))) {
			name_in_table += strlen(group_name);
			if (*name_in_table++ == '/') {
				if (!strcmp(name_in_table, name)) {
					return i;
				}
			}
		}
		++table; ++i;
	}
	return -1;
}

/**
 * @brief Check given settings in an array and mark it if set as expected
 *
 * This function can be called in @c set callback to analyze the value
 * according to sample settings array.
 *
 * If everything looks good it is marked in an array witch pointer is provided.
 *
 * @param key           The key from set callback
 * @param len           The value length from set callback
 * @param read_cb       Read callback function pointer from set callback
 * @param cb_arg        Argument for read callback from set callback
 * @param group_name    The group name (that was stripped from @c key parameter)
 * @param sample_array  The array of sample settings data to search in
 * @param touched_array The array where to set an information if selected
 *                      settings entry was properly set
 *
 * @return 0 or error code
 */
static int set_check_and_mark(const char *key, size_t len,
			      settings_read_cb read_cb, void *cb_arg,
			      const char *group_name,
			      const struct test_settings_entry sample_array[],
			      bool touched_array[])
{
	ssize_t index;
	u32_t val;
	ssize_t  val_len;

	zassert_not_null(key, NULL);

	index = find_settings_table(sample_array, key, group_name);
	zassert_true(index >= 0, "Cannot find a key: \"%s/%s\" (%d)",
		     group_name, key, index);

	zassert_true(len == sizeof(val),
		     "Unexpected data length (%d) for \"%s/%s\"",
		     len, group_name, key);

	/* Loading the value */
	val_len = read_cb(cb_arg, &val, sizeof(val));
	zassert_equal(sizeof(val), val_len,
		      "Unexpected read size: %d for \"%s/%s\"", val_len,
		      group_name, key);

	zassert_equal(sample_array[index].data, val,
		      "Unexpected value (%d) for \"%s/%s\"",
		      val, group_name, key);

	/* Value loaded as expected */
	touched_array[index] = true;

	return 0;
}

/**
 * @brief Add an array to settings
 *
 * Function saves all the entries from the table to the settings.
 *
 * @param table The array with all the entries to import, with NULL at the end.
 *
 * @return 0 on success or negative error code.
 */
static int settings_table_import(const struct test_settings_entry *table)
{
	int rc = 0;

	while (table->name) {
		rc = settings_save_one(table->name,
				       &table->data,
				       sizeof(table->data));
		if (rc) {
			break;
		}
		++table;
	}
	return rc;
}


/* ****************************************************************************
 * simple_g12 - simple group 1 and 2 test.
 */

/**
 * Sample settings data
 */
static const struct test_settings_entry simple_g12[] = {
	{"group1/one",   1},
	{"group1/two",   2},
	{"group1/three", 3},
	{"group2/one",   21},
	{"group2/two",   22},
	{"group2/three", 23},
	{NULL, 0}
};

/**
 * Array to mark a settings that was touched by callback.
 * To be used inside tests.
 */
static bool simple_g12_touched[ARRAY_SIZE(simple_g12) - 1];


static int simple_g12_mark_if_properly_used(
		const char *key, size_t len,
		settings_read_cb read_cb, void *cb_arg,
		const char *group_name)
{
	return set_check_and_mark(key, len, read_cb, cb_arg, group_name,
			   simple_g12, simple_g12_touched);
}

static int simple_g12_cb_set_group1(const char *key, size_t len,
				    settings_read_cb read_cb, void *cb_arg)
{
	return simple_g12_mark_if_properly_used(key, len, read_cb, cb_arg,
						"group1");
}

static int simple_g12_cb_set_group2(const char *key, size_t len,
				    settings_read_cb read_cb, void *cb_arg)
{
	return simple_g12_mark_if_properly_used(key, len, read_cb, cb_arg,
						"group2");
}

static struct settings_handler simple_g12_h_group1 = {
	.name = "group1",
	.h_set = simple_g12_cb_set_group1
};

static struct settings_handler simple_g12_h_group2 = {
	.name = "group2",
	.h_set = simple_g12_cb_set_group2
};

static void test_simple_g12_pattern_loading(void)
{
	int rc;
	static const bool expected_zero[6] = {
		false, false, false, false, false, false
	};
	static const bool expected_group1_only[6] = {
		true, true, true, false, false, false
	};
	static const bool expected_group12_1[6] = {
		true, false, false, true, false, false
	};
	static const bool expected_group12_2[6] = {
		false, true, false, false, true, false
	};
	static const bool expected_group12_3[6] = {
		false, false, true, false, false, true
	};


	rc = settings_table_import(simple_g12);
	zassert_equal(0, rc, "Cannot import settings (%d)", rc);

	rc = settings_register(&simple_g12_h_group1);
	zassert_equal(0, rc, "Cannot register handler (%d)", rc);

	rc = settings_register(&simple_g12_h_group2);
		zassert_equal(0, rc, "Cannot register handler (%d)", rc);

	/* Clearing auxiliary variables */
	memset(simple_g12_touched, 0, sizeof(simple_g12_touched));

	/* Nothing should match this pattern */
	settings_load_selected("group/**");

	zassert_mem_equal(simple_g12_touched,
			  expected_zero,
			  sizeof(simple_g12_touched),
			  NULL);

	/* We expect all the elements in group1 to be called */
	settings_load_selected("group1/**");
	zassert_mem_equal(simple_g12_touched,
			  expected_group1_only,
			  sizeof(simple_g12_touched),
			  NULL);

	/* Element 1 from botch groups should be called */
	memset(simple_g12_touched, 0, sizeof(simple_g12_touched));
	settings_load_selected("*/one");
	zassert_mem_equal(simple_g12_touched,
			  expected_group12_1,
			  sizeof(simple_g12_touched),
			  NULL);

	/* Element 2 from botch groups should be called */
	memset(simple_g12_touched, 0, sizeof(simple_g12_touched));
	settings_load_selected("**/two");
	zassert_mem_equal(simple_g12_touched,
			  expected_group12_2,
			  sizeof(simple_g12_touched),
			  NULL);

	/* Element 3 from botch groups should be called */
	memset(simple_g12_touched, 0, sizeof(simple_g12_touched));
	settings_load_selected("*/three/**");
	zassert_mem_equal(simple_g12_touched,
			  expected_group12_3,
			  sizeof(simple_g12_touched),
			  NULL);

	/* This pattern should not match any of the existing elements */
	memset(simple_g12_touched, 0, sizeof(simple_g12_touched));
	settings_load_selected("*/one/*");
	zassert_mem_equal(simple_g12_touched,
			  expected_zero,
			  sizeof(simple_g12_touched),
			  NULL);
}


/* ****************************************************************************
 * single_group - single group but multiple elements to load
 */

/**
 * Sample settings data
 */
static const struct test_settings_entry single_group[] = {
	{"g/peer/0",        0x0000},
	{"g/peer/0/super",  0x0001},
	{"g/peer/0/other",  0x0002},
	{"g/peer/1",        0x0100},
	{"g/peer/1/super",  0x0101},
	{"g/peer/1/other",  0x0102},
	{"g/settings",      0xFFFE},
	{"g/configuration", 0xFFFF},
	{NULL, 0}
};

/**
 * Array to mark a settings that was touched by callback.
 * To be used inside tests.
 */
static bool single_group_touched[ARRAY_SIZE(single_group) - 1];


static int single_group_cb_set(const char *key, size_t len,
				    settings_read_cb read_cb, void *cb_arg)
{
	return set_check_and_mark(key, len, read_cb, cb_arg, "g",
				  single_group, single_group_touched);
}

static struct settings_handler single_group_h = {
	.name = "g",
	.h_set = single_group_cb_set
};


static void test_single_group_pattern_loading(void)
{
	int rc;
	static const bool expected_zero[8] = {
		false, false, false, false, false, false, false, false
	};
	static const bool expected_all_peers[8] = {
		true, true, true, true, true, true, false, false
	};
	static const bool expected_peers_single_level[8] = {
		true, false, false, true, false, false, false, false
	};
	static const bool expected_peers_super_only[8] = {
		false, true, false, false, true, false, false, false
	};
	static const bool expected_configuration[8] = {
		false, false, false, false, false, false, false, true
	};

	rc = settings_table_import(single_group);
	zassert_equal(0, rc, "Cannot import settings (%d)", rc);

	rc = settings_register(&single_group_h);
	zassert_equal(0, rc, "Cannot register handler (%d)", rc);

	/* Clearing auxiliary variables */
	memset(single_group_touched, 0, sizeof(single_group_touched));

	/* Nothing should match this pattern */
	settings_load_selected("g/none/**");

	zassert_mem_equal(single_group_touched,
			  expected_zero,
			  sizeof(single_group_touched),
			  NULL);

	/* Load all peers */
	settings_load_selected("g/peer/**");
	zassert_mem_equal(single_group_touched,
			  expected_all_peers,
			  sizeof(single_group_touched),
			  NULL);

	/* Load only single element after peer */
	memset(single_group_touched, 0, sizeof(single_group_touched));
	settings_load_selected("g/peer/*");
	zassert_mem_equal(single_group_touched,
			  expected_peers_single_level,
			  sizeof(single_group_touched),
			  NULL
			);

	/* Load "super" subelements of peers */
	memset(single_group_touched, 0, sizeof(single_group_touched));
	settings_load_selected("g/peer/*/super");
	zassert_mem_equal(single_group_touched,
			  expected_peers_super_only,
			  sizeof(single_group_touched),
			  NULL);

	/* Load "configuration" element */
	memset(single_group_touched, 0, sizeof(single_group_touched));
	settings_load_selected("g/configuration");
	zassert_mem_equal(single_group_touched,
			  expected_configuration,
			  sizeof(single_group_touched),
			  NULL);
}



ztest_test_suite(
	settings_load_patter_suite,
	ztest_unit_test_setup_teardown(test_simple_g12_pattern_loading,
				       setup, teardown),
	ztest_unit_test_setup_teardown(test_single_group_pattern_loading,
				       setup, teardown)
);


void settings_load_pattern(void)
{
	ztest_run_test_suite(settings_load_patter_suite);
}
