/*
 * Copyright (c) 2019 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <subsys/settings/src/settings.c>

/* *********************************************
 *  Dummy storage implementation
 */
void settings_store_init(void)
{
	/* Nothing to do */
}

int settings_line_val_read(off_t val_off, off_t off, char *out, size_t len_req,
			   size_t *len_read, void *cb_arg)
{
	return 0;
}

size_t settings_line_val_get_len(off_t val_off, void *read_cb_ctx)
{
	return 0;
}

/* End of dummy storage
 * *******************************************
 */

/**
 * @brief Test simple comparison of the first element
 *
 * test verifies if the simplest usage of the key comparison
 * function works as expected.
 */
static void test_settings_name_split(void)
{
	static const char sample_patch[] = "it/is/going/to/be/legendary";

	zassert_true (settings_name_split(sample_patch, "it",  NULL), NULL);
	zassert_false(settings_name_split(sample_patch, "its", NULL), NULL);
	zassert_false(settings_name_split(sample_patch, "i",   NULL), NULL);
	zassert_false(settings_name_split(sample_patch, "it/", NULL), NULL);
}

/**
 * @brief Test the usage of patch part as a key
 *
 * Test verifies if we can safely use a complete patch with backslash
 * inside as a key to compare.
 */
static void test_settings_name_split_keypatch(void)
{
	static const char sample_patch[] = "it/is/going";
	const char *current_pos = sample_patch;

	zassert_true(settings_name_split(sample_patch, "it", NULL), NULL);
	zassert_true(settings_name_split(sample_patch, "it/is", NULL), NULL);
	zassert_true(settings_name_split(sample_patch,
					"it/is/going",
					&current_pos), NULL);
	zassert_is_null(current_pos, NULL);

	zassert_false(settings_name_split(sample_patch, "it/", NULL), NULL);
	zassert_false(settings_name_split(sample_patch, "it/is/", NULL), NULL);
	zassert_false(settings_name_split(sample_patch, "it/is/g", NULL), NULL);
	zassert_false(settings_name_split(sample_patch, "it/is/going/", NULL), NULL);

}

/**
 * @brief Test if the tree can be properly traversed
 *
 * Test verifies if the tree can be traversed from the first to the last
 * key inside the patch.
 * The current position has to be properly returned by the tested comparison
 * function.
 */
static void test_settings_tree_traverse(void)
{
	static const char sample_patch[] = "it/is/going";
	const char *current_pos = sample_patch;

	/* it */
	zassert_true (settings_name_split(current_pos, "it", &current_pos), NULL);
	zassert_equal_ptr(current_pos, sample_patch + strlen("it/"),
			  "Current pos string: %s", current_pos);

	/* is */
	zassert_false(settings_name_split(current_pos, "it", NULL), NULL);
	zassert_false(settings_name_split(current_pos, "is/go", NULL), NULL);
	zassert_true (settings_name_split(current_pos, "is", &current_pos), NULL);
	zassert_equal_ptr(current_pos, sample_patch + strlen("it/is/"),
			  "Current pos string: %s", current_pos);

	/* going */
	zassert_false(settings_name_split(current_pos, "it", NULL), NULL);
	zassert_false(settings_name_split(current_pos, "is", NULL), NULL);
	zassert_true (settings_name_split(current_pos, "going", &current_pos), NULL);
	zassert_is_null(current_pos, NULL);
}



static void test_settings_name_cmp_doc(void)
{
	zassert_true(settings_name_cmp("my_key/other/stuff", "my_key/*/stuff"), NULL);
	zassert_true(settings_name_cmp("my_key/other/stuff", "*/other/stuff"),  NULL);
	zassert_true(settings_name_cmp("my_key/other/stuff", "my_key/other/*"), NULL);

	zassert_true(settings_name_cmp("my_key/other/stuff", "**"), NULL);
	zassert_true(settings_name_cmp("my_key/other/stuff", "**/other/stuff"), NULL);
	zassert_true(settings_name_cmp("my_key/other/stuff", "**/stuff"), NULL);
	zassert_true(settings_name_cmp("my_key/other/stuff", "my_key/**"), NULL);

	/* Check if there is exactly one element in given name: */
	zassert_true (settings_name_cmp("element", "*"), NULL);
	zassert_false(settings_name_cmp("element/other", "*"), NULL);
	zassert_false(settings_name_cmp("", "*"), NULL);

	/* Check if there is at least one element in given name: */
	zassert_true (settings_name_cmp("element", "*/**"), NULL);
	zassert_true (settings_name_cmp("element/other", "*/**"), NULL);
	zassert_false(settings_name_cmp("", "*/**"), NULL);
}


static void test_settings_name_cmp(void)
{
	static const char sample_patch[] = "it/is/going/to/be/legendary";

	zassert_true (settings_name_cmp(sample_patch, "it/*/going/to/be/*"), NULL);
	zassert_false(settings_name_cmp(sample_patch, "it/*/to/be/*"), NULL);
	zassert_true (settings_name_cmp(sample_patch, "**"), NULL);
	zassert_true (settings_name_cmp(sample_patch, "**/legendary"), NULL);
	zassert_false(settings_name_cmp(sample_patch, "**/legend"), NULL);

}

static void test_settings_name_cmp_single(void)
{
	zassert_true (settings_name_cmp("single", "*"), NULL);
	zassert_false(settings_name_cmp("dual/element", "*"), NULL);

	zassert_true (settings_name_cmp("single", "*/**"), NULL);
	zassert_true (settings_name_cmp("single/double", "*/**"), NULL);
	zassert_false(settings_name_cmp("", "*/**"), NULL);
	zassert_true (settings_name_cmp("", "**"), NULL);
}

static void test_settings_name_cmp_match_empty(void)
{
	zassert_false(settings_name_cmp("it/is/going", "it/*/is/going"), NULL);
	zassert_true (settings_name_cmp("it/is/going", "it/**/is/going"), NULL);
}

static void test_settings_name_cmp_partial(void)
{
	zassert_true (settings_name_cmp("prefix_0", "prefix_*"), NULL);
	zassert_false(settings_name_cmp("prefix_", "prefix_*"), NULL);

	zassert_true (settings_name_cmp("prefix_1", "prefix_*"), NULL);
	zassert_false(settings_name_cmp("prefix_1/a", "prefix_*"), NULL);
	zassert_true (settings_name_cmp("prefix_1/a", "prefix_*/**"), NULL);
}


void test_main(void)
{
	ztest_test_suite(test_settings,
			 ztest_unit_test(test_settings_name_split),
			 ztest_unit_test(test_settings_name_split_keypatch),
			 ztest_unit_test(test_settings_tree_traverse),
			 ztest_unit_test(test_settings_name_cmp_doc),
			 ztest_unit_test(test_settings_name_cmp),
			 ztest_unit_test(test_settings_name_cmp_single),
			 ztest_unit_test(test_settings_name_cmp_match_empty),
			 ztest_unit_test(test_settings_name_cmp_partial));
	ztest_run_test_suite(test_settings);
}
