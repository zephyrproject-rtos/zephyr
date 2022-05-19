/*
 * Copyright (c) 2022 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/eeprom.h>

#include <factory_data/factory_data.h>
#include <ztest.h>
#include <ztest_error_hook.h>

static int test_factory_data_pre_init_errors_data_load_dummy(const char *name, const uint8_t *value,
							     size_t len, const void *param)
{
	return -ENOSYS;
};

static void test_factory_data_pre_init_errors(void)
{
	const char *const value = "value";
	uint8_t buf[16];

	zassert_equal(-ECANCELED, factory_data_save_one("uuid", value, strlen(value)),
		      "Failing because not initialized");
	zassert_equal(-ECANCELED,
		      factory_data_load(test_factory_data_pre_init_errors_data_load_dummy, NULL),
		      "Failing because not initialized");
	zassert_equal(-ECANCELED, factory_data_load_one("uuid", buf, sizeof(buf)),
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
	const uint8_t value_to_set[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
	uint8_t value_read_back[6];
	const uint8_t value_after_erasure[6] = {0};

	zassert_ok(factory_data_save_one("mac_address", value_to_set, sizeof(value_to_set)),
		   "Saving must work");
	zassert_equal(
		6, factory_data_load_one("mac_address", value_read_back, sizeof(value_read_back)),
		"Read back to prove proper storing");
	zassert_ok(factory_data_erase(), "Erase must succeed");

	/* EEPROM specific: Loading an erased value returns all zeros */
	zassert_equal(
		6, factory_data_load_one("mac_address", value_read_back, sizeof(value_read_back)),
		"Entry can still be loaded");
	zassert_mem_equal(value_after_erasure, value_read_back, sizeof(value_read_back),
			  "All zero");
}

static void test_factory_data_save_one_name_invalid(void)
{
	const char *const value_to_set = "value";
	uint8_t value_read_back[strlen(value_to_set)];

	zassert_equal(-ENOENT,
		      factory_data_load_one("invalid", value_read_back, sizeof(value_read_back)),
		      "Must not exist");

	/* Explicity set, then ensure still not existing */
	zassert_equal(-EINVAL, factory_data_save_one("invalid", value_to_set, strlen(value_to_set)),
		      "Empty name is not allowed");
	zassert_equal(-ENOENT,
		      factory_data_load_one("invalid", value_read_back, sizeof(value_read_back)),
		      "Must not exist");
}

static void test_factory_data_save_one_value_empty(void)
{
	const char *const value_to_set = "";
	uint8_t value_read_back[16];

	zassert_ok(factory_data_save_one("uuid", value_to_set, strlen(value_to_set)),
		   "Simple save must work");

	/* EEPROM specific: Always return fixed length buffer */
	zassert_equal(16, factory_data_load_one("uuid", value_read_back, sizeof(value_read_back)),
		      "Must exist and be of size 16 - not testing value because undefined");
}

static void test_factory_data_save_one_value_regular(void)
{
	const uint8_t value_to_set[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
	uint8_t value_read_back[sizeof(value_to_set)];

	zassert_ok(factory_data_save_one("mac_address", value_to_set, sizeof(value_to_set)),
		   "Simple save must work");
	zassert_equal(
		sizeof(value_to_set),
		factory_data_load_one("mac_address", value_read_back, sizeof(value_read_back)),
		"Must exist");
	zassert_mem_equal(value_to_set, value_read_back, sizeof(value_to_set),
			  "Expecting proper restore");
}

static void test_factory_data_save_one_value_max_length(void)
{
	char value_to_set[CONFIG_FACTORY_DATA_VALUE_LEN_MAX];
	uint8_t value_read_back[CONFIG_FACTORY_DATA_VALUE_LEN_MAX + 10];

	memset(value_to_set, 'X', sizeof(value_to_set));

	zassert_ok(factory_data_save_one("value_max_len", value_to_set, sizeof(value_to_set)),
		   "Max sized values must be persistable");
	zassert_equal(
		(ssize_t)sizeof(value_to_set),
		factory_data_load_one("value_max_len", value_read_back, sizeof(value_read_back)),
		"Must exist");
	zassert_mem_equal(value_to_set, value_read_back, sizeof(value_to_set),
			  "Expecting proper restore");
}

static void test_factory_data_save_one_value_oversize(void)
{
	char value_to_set[CONFIG_FACTORY_DATA_VALUE_LEN_MAX + 1] = {};

	zassert_equal(-EFBIG,
		      factory_data_save_one("value_max_len", value_to_set, sizeof(value_to_set)),
		      "Values exceeding max size must be rejected");
}

struct test_factory_data_load_values_seen {
	bool uuid;
	bool mac_address;
	bool value_max_len;
};

static int test_factory_data_load_callback(const char *name, const uint8_t *value, size_t len,
					   const void *param)
{
	struct test_factory_data_load_values_seen *seen =
		(struct test_factory_data_load_values_seen *)param;
	char value_max_len[CONFIG_FACTORY_DATA_NAME_LEN_MAX + 1] = {0};

	memset(value_max_len, 'M', CONFIG_FACTORY_DATA_NAME_LEN_MAX);

	if (!strcmp(name, "uuid")) {
		seen->uuid = true;
	} else if (!strcmp(name, "mac_address")) {
		seen->mac_address = true;
	} else if (!strcmp(name, "value_max_len")) {
		seen->value_max_len = true;
	} else {
		/* unknown entry */
		ztest_test_fail();
	}

	return 0;
}

static void test_factory_data_load(void)
{
	struct test_factory_data_load_values_seen seen = {0};

	zassert_ok(factory_data_load(test_factory_data_load_callback, &seen), "Loading must work");
	zassert_true(seen.uuid, "'uuid' must be stored");
	zassert_true(seen.mac_address, "'mac_address' must be stored");
	zassert_true(seen.value_max_len, "'value_max_len' must be stored");
}

void test_main(void)
{
	ztest_test_suite(factory_data, ztest_unit_test(test_factory_data_pre_init_errors),
			 ztest_unit_test(test_factory_data_init),
			 ztest_unit_test(test_factory_data_erase),
			 ztest_unit_test(test_factory_data_save_one_name_invalid),
			 ztest_unit_test(test_factory_data_save_one_value_empty),
			 ztest_unit_test(test_factory_data_save_one_value_regular),
			 ztest_unit_test(test_factory_data_save_one_value_max_length),
			 ztest_unit_test(test_factory_data_save_one_value_oversize),
			 ztest_unit_test(test_factory_data_load));

	ztest_run_test_suite(factory_data);
}
