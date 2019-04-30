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
 * @brief Assertion macro to simplify further tests
 *
 * This macro limits the number of characters required to generate tests below.
 */
#define assert_true_name_split(a, b, next) \
	zassert_true(settings_name_split(a, b, next), NULL)
/**
 * @brief Assertion macro to simplify further tests
 *
 * This macro limits the number of characters required to generate tests below.
 */
#define assert_false_name_split(a, b, next) \
	zassert_false(settings_name_split(a, b, next), NULL)

/**
 * @brief Test simple comparison of the first element
 *
 * test verifies if the simplest usage of the key comparison
 * function works as expected.
 */
static void test_settings_name_split(void)
{
	static const char sample_patch[] = "it/is/going/to/be/legendary";

	assert_true_name_split (sample_patch, "it",  NULL);
	assert_false_name_split(sample_patch, "its", NULL);
	assert_false_name_split(sample_patch, "i",   NULL);
	assert_false_name_split(sample_patch, "it/", NULL);
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

	assert_true_name_split(sample_patch, "it", NULL);
	assert_true_name_split(sample_patch, "it/is", NULL);
	assert_true_name_split(sample_patch, "it/is/going", &current_pos);
	zassert_is_null(current_pos, NULL);

	assert_false_name_split(sample_patch, "it/", NULL);
	assert_false_name_split(sample_patch, "it/is/", NULL);
	assert_false_name_split(sample_patch, "it/is/g", NULL);
	assert_false_name_split(sample_patch, "it/is/going/", NULL);

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
	assert_true_name_split(current_pos, "it", &current_pos);
	zassert_equal_ptr(current_pos, sample_patch + strlen("it/"),
			  "Current pos string: %s", current_pos);

	/* is */
	assert_false_name_split(current_pos, "it", NULL);
	assert_false_name_split(current_pos, "is/go", NULL);
	assert_true_name_split (current_pos, "is", &current_pos);
	zassert_equal_ptr(current_pos, sample_patch + strlen("it/is/"),
			  "Current pos string: %s", current_pos);

	/* going */
	assert_false_name_split(current_pos, "it", NULL);
	assert_false_name_split(current_pos, "is", NULL);
	assert_true_name_split (current_pos, "going", &current_pos);
	zassert_is_null(current_pos, NULL);
}


/**
 * @brief Assertion macro to simplify further tests
 *
 * This macro limits the number of characters required to generate tests below.
 */
#define assert_true_name_cmp(a, b) \
	zassert_true(settings_name_cmp(a, b), NULL)
/**
 * @brief Assertion macro to simplify further tests
 *
 * This macro limits the number of characters required to generate tests below.
 */
#define assert_false_name_cmp(a, b) \
	zassert_false(settings_name_cmp(a, b), NULL)

static void test_settings_name_cmp_doc(void)
{
	assert_true_name_cmp("my_key/other/stuff", "my_key/*/stuff");
	assert_true_name_cmp("my_key/other/stuff", "*/other/stuff");
	assert_true_name_cmp("my_key/other/stuff", "my_key/other/*");

	assert_true_name_cmp("my_key/other/stuff", "**");
	assert_true_name_cmp("my_key/other/stuff", "**/other/stuff");
	assert_true_name_cmp("my_key/other/stuff", "**/stuff");
	assert_true_name_cmp("my_key/other/stuff", "my_key/**");

	/* Check if there is exactly one element in given name: */
	assert_true_name_cmp ("element", "*");
	assert_false_name_cmp("element/other", "*");
	assert_false_name_cmp("", "*");

	/* Check if there is at least one element in given name: */
	assert_true_name_cmp ("element", "*/**");
	assert_true_name_cmp ("element/other", "*/**");
	assert_false_name_cmp("", "*/**");
}

static void test_settings_name_cmp(void)
{
	static const char sample_patch[] = "it/is/going/to/be/legendary";

	assert_true_name_cmp (sample_patch, "it/*/going/to/be/*");
	assert_false_name_cmp(sample_patch, "it/*/to/be/*");
	assert_true_name_cmp (sample_patch, "**");
	assert_true_name_cmp (sample_patch, "**/legendary");
	assert_false_name_cmp(sample_patch, "**/legend");

}

static void test_settings_name_cmp_single(void)
{
	assert_true_name_cmp ("single", "*");
	assert_false_name_cmp("dual/element", "*");

	assert_true_name_cmp ("single", "*/**");
	assert_true_name_cmp ("single/double", "*/**");
	assert_false_name_cmp("", "*/**");
	assert_true_name_cmp ("", "**");
}

static void test_settings_name_cmp_match_empty(void)
{
	assert_false_name_cmp("it/is/going", "it/*/is/going");
	assert_true_name_cmp ("it/is/going", "it/**/is/going");
}

static void test_settings_name_cmp_partial(void)
{
	assert_true_name_cmp ("prefix_0", "prefix_*");
	assert_false_name_cmp("prefix_", "prefix_*");

	assert_true_name_cmp ("prefix_1", "prefix_*");
	assert_false_name_cmp("prefix_1/a", "prefix_*");
	assert_true_name_cmp ("prefix_1/a", "prefix_*/**");
}

static void test_settings_name_cmp_final_double(void)
{
	assert_true_name_cmp("a/b", "**");
	assert_true_name_cmp("a/b", "a/**");
	assert_true_name_cmp("a/b", "a/b/**");
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
			 ztest_unit_test(test_settings_name_cmp_partial),
			 ztest_unit_test(test_settings_name_cmp_final_double));
	ztest_run_test_suite(test_settings);
}
