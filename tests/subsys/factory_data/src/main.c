/*
 * Copyright (c) 2022 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <factory_data/factory_data.h>
#include <storage/flash_map.h>
#include <ztest.h>
#include <ztest_error_hook.h>

static int test_factory_data_pre_init_errors_data_load_dummy(const char *name, const uint8_t *value,
							     size_t len, const void *param)
{
	return -ENOSYS;
};

static void test_factory_data_pre_init(void)
{
	const char *const value = "value";
	uint8_t buf[16];

	zassert_equal(-ECANCELED, factory_data_save_one("name", value, strlen(value)),
		      "Failing because not initialized");
	zassert_equal(-ECANCELED,
		      factory_data_load(test_factory_data_pre_init_errors_data_load_dummy, NULL),
		      "Failing because not initialized");
	zassert_equal(-ECANCELED, factory_data_load_one("name", buf, sizeof(buf)),
		      "Failing because not initialized");

	zassert_ok(factory_data_erase(), "Must work even when not initialized");
}

static void test_factory_data_init(void)
{
	zassert_ok(factory_data_init(), "First init must work");
	zassert_ok(factory_data_init(), "2nd initialization must work too");
	zassert_ok(factory_data_init(), "Actually, every initialization must work");
}

static void test_factory_data_erase(void)
{
	const char *const value_to_set = "value";
	uint8_t value_read_back[strlen(value_to_set)];

	zassert_ok(factory_data_save_one("erase-me", value_to_set, strlen(value_to_set)),
		   "Saving must work");
	zassert_equal((ssize_t)strlen(value_to_set),
		      factory_data_load_one("erase-me", value_read_back, sizeof(value_read_back)),
		      "Read back to prove proper storing");
	zassert_ok(factory_data_erase(), "Erase must succeed");
	zassert_equal(-ENOENT,
		      factory_data_load_one("erase-me", value_read_back, sizeof(value_read_back)),
		      "Entry must be gone");
}

static void test_factory_data_save_one_invalid_name(void)
{
	const char *const value_to_set = "value";
	uint8_t value_read_back[strlen(value_to_set)];

	zassert_equal(-ENOENT, factory_data_load_one("", value_read_back, sizeof(value_read_back)),
		      "Must not exist");

	/* Explicity set, then ensure still not existing */
	zassert_equal(-EINVAL, factory_data_save_one("", value_to_set, strlen(value_to_set)),
		      "Empty name is not allowed");
	zassert_equal(-ENOENT, factory_data_load_one("", value_read_back, sizeof(value_read_back)),
		      "Must not exist");
}

static void test_factory_data_save_one_name_smallest(void)
{
	const char *const value_to_set = "1char";
	uint8_t value_read_back[strlen(value_to_set)];

	zassert_ok(factory_data_save_one("s", value_to_set, strlen(value_to_set)),
		   "Single char name");
	zassert_equal((ssize_t)strlen(value_to_set),
		      factory_data_load_one("s", value_read_back, sizeof(value_read_back)),
		      "Must exist");
	zassert_mem_equal(value_to_set, value_read_back, strlen(value_to_set),
			  "Expecting proper restore");
}

static void test_factory_data_save_one_name_max_size(void)
{
	const char *const value_to_set = "longest";
	uint8_t value_read_back[strlen(value_to_set)];
	char name[CONFIG_FACTORY_DATA_NAME_LEN_MAX + 1] = {0};

	memset(name, 'M', CONFIG_FACTORY_DATA_NAME_LEN_MAX);

	zassert_ok(factory_data_save_one(name, value_to_set, strlen(value_to_set)),
		   "Max sized name must be allowed");
	zassert_equal((ssize_t)strlen(value_to_set),
		      factory_data_load_one(name, value_read_back, sizeof(value_read_back)),
		      "Must exist");
	zassert_mem_equal(value_to_set, value_read_back, strlen(value_to_set),
			  "Expecting proper restore");
}

static void test_factory_data_save_one_name_oversize(void)
{
	const char *const value_to_set = "value";
	char name[CONFIG_FACTORY_DATA_NAME_LEN_MAX + 2] = {0};

	memset(name, 'N', CONFIG_FACTORY_DATA_NAME_LEN_MAX + 1);
	zassert_equal(-ENAMETOOLONG,
		      factory_data_save_one(name, value_to_set, strlen(value_to_set)),
		      "Name exceeding max name length must be rejected");
}

/* Can not actually be set from shell, but still nice to have */
static void test_factory_data_save_one_name_with_spaces(void)
{
	const char *const value_to_set = "value";

	zassert_ok(factory_data_save_one("name with spaces", value_to_set, strlen(value_to_set)),
		   "name with spaces");
}

static void test_factory_data_save_one_value_empty(void)
{
	const char *const value_to_set = "";
	uint8_t value_read_back[10];

	zassert_ok(factory_data_save_one("value_empty", value_to_set, strlen(value_to_set)),
		   "Simple save must work");
	zassert_equal(
		0, factory_data_load_one("value_empty", value_read_back, sizeof(value_read_back)),
		"Must exist and be of size 0");
}

static void test_factory_data_save_one_value_regular(void)
{
	const char *const value_to_set = "value";
	uint8_t value_read_back[strlen(value_to_set)];

	zassert_ok(factory_data_save_one("value_regular", value_to_set, strlen(value_to_set)),
		   "Simple save must work");
	zassert_equal(
		(ssize_t)strlen(value_to_set),
		factory_data_load_one("value_regular", value_read_back, sizeof(value_read_back)),
		"Must exist");
	zassert_mem_equal(value_to_set, value_read_back, strlen(value_to_set),
			  "Expecting proper restore");
}

static void test_factory_data_save_one_value_max_length(void)
{
	char value_to_set[CONFIG_FACTORY_DATA_VALUE_LEN_MAX];
	uint8_t value_read_back[CONFIG_FACTORY_DATA_VALUE_LEN_MAX + 10];

	memset(value_to_set, 'X', sizeof(value_to_set));

	zassert_ok(factory_data_save_one("value_huge", value_to_set, sizeof(value_to_set)),
		   "Huge values must be persistable");
	zassert_equal((ssize_t)sizeof(value_to_set),
		      factory_data_load_one("value_huge", value_read_back, sizeof(value_read_back)),
		      "Must exist");
	zassert_mem_equal(value_to_set, value_read_back, sizeof(value_to_set),
			  "Expecting proper restore");
}

static void test_factory_data_save_one_value_oversize(void)
{
	char value_to_set[CONFIG_FACTORY_DATA_VALUE_LEN_MAX + 1];
	uint8_t value_read_back[sizeof(value_to_set)];

	memset(value_to_set, 0xAA, sizeof(value_to_set));

	zassert_equal(-EFBIG,
		      factory_data_save_one("value_too_big", value_to_set, sizeof(value_to_set)),
		      "Values exceeding max size must be rejected");
	zassert_equal(
		-ENOENT,
		factory_data_load_one("value_too_big", value_read_back, sizeof(value_read_back)),
		"Must not exist");
}

static void test_factory_data_save_one_reject_already_set_names(void)
{
	const char *const value_to_set = "value";

	zassert_ok(factory_data_save_one("unique_only_once", value_to_set, strlen(value_to_set)),
		   "First write allowed");
	zassert_equal(-EEXIST,
		      factory_data_save_one("unique_only_once", value_to_set, strlen(value_to_set)),
		      "2nd write to same variable not allowed");
}

struct test_factory_data_load_values_seen {
	bool smallest_name;
	bool max_sized_name;
	bool name_with_spaces;
	bool value_empty;
	bool value_regular;
	bool value_huge;
	bool unique_only_once;
};
static int test_factory_data_load_callback(const char *name, const uint8_t *value, size_t len,
					   const void *param)
{
	struct test_factory_data_load_values_seen *seen =
		(struct test_factory_data_load_values_seen *)param;
	char max_sized_name[CONFIG_FACTORY_DATA_NAME_LEN_MAX + 1] = {0};

	memset(max_sized_name, 'M', CONFIG_FACTORY_DATA_NAME_LEN_MAX);

	if (!strcmp(name, "s")) {
		seen->smallest_name = true;
	} else if (!strcmp(name, max_sized_name)) {
		seen->max_sized_name = true;
	} else if (!strcmp(name, "name with spaces")) {
		seen->name_with_spaces = true;
	} else if (!strcmp(name, "value_empty")) {
		seen->value_empty = true;
	} else if (!strcmp(name, "value_regular")) {
		seen->value_regular = true;
	} else if (!strcmp(name, "value_huge")) {
		seen->value_huge = true;
	} else if (!strcmp(name, "unique_only_once")) {
		seen->unique_only_once = true;
	} else {
		/* unknown entry */
		ztest_test_fail();
	}

	return 0;
}

static void test_factory_data_load(void)
{
	struct test_factory_data_load_values_seen seen;

	zassert_ok(factory_data_load(test_factory_data_load_callback, &seen), "Loading must work");
	zassert_true(seen.smallest_name, "'s' must be stored");
	zassert_true(seen.max_sized_name, "'NNNNNNNNNNvv...' must be stored");
	zassert_true(seen.name_with_spaces, "'name with spaces' must be stored");
	zassert_true(seen.value_empty, "'value_empty' must be stored");
	zassert_true(seen.value_regular, "'value_regular' must be stored");
	zassert_true(seen.value_huge, "'value_huge' must be stored");
	zassert_true(seen.unique_only_once, "'unique_only_once' must be stored");
}

/* Running those tests often will kill the flash soon! */
static void erase_flash(void)
{
	int ret;
	const struct flash_area *fap;

	ret = flash_area_open(FACTORY_DATA_FLASH_PARTITION, &fap);
	zassert_ok(ret, "Flash area open must work");

	ret = flash_area_erase(fap, 0, fap->fa_size);
	zassert_ok(ret, "Flash area erase must work");
}

void test_main(void)
{
	/*
	 * To reduce flash wear, erase flash just once per run, not before each tests.
	 * Tests are assumed to be executed in order.
	 */
	erase_flash();

	ztest_test_suite(factory_data, ztest_unit_test(test_factory_data_pre_init),
			 ztest_unit_test(test_factory_data_init),
			 ztest_unit_test(test_factory_data_erase),
			 ztest_unit_test(test_factory_data_save_one_invalid_name),
			 ztest_unit_test(test_factory_data_save_one_name_smallest),
			 ztest_unit_test(test_factory_data_save_one_name_max_size),
			 ztest_unit_test(test_factory_data_save_one_name_oversize),
			 ztest_unit_test(test_factory_data_save_one_name_with_spaces),
			 ztest_unit_test(test_factory_data_save_one_value_empty),
			 ztest_unit_test(test_factory_data_save_one_value_regular),
			 ztest_unit_test(test_factory_data_save_one_value_max_length),
			 ztest_unit_test(test_factory_data_save_one_value_oversize),
			 ztest_unit_test(test_factory_data_save_one_reject_already_set_names),
			 ztest_unit_test(test_factory_data_load));

	ztest_run_test_suite(factory_data);
}
