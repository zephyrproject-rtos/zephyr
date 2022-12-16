/*
 * Copyright (c) 2022 Marcin Niestroj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/settings/settings.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/ztest.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#define FACTORY_VALUE		0x01020304
#define BOTH_IN_FACTORY_VALUE	0x25262728
#define BOTH_IN_RUN_VALUE	0x35363738
#define BOTH_IN_RUN_VALUE_2	0x45464748
#define RUN_VALUE		0x8a8b8c8d
#define RUN_VALUE_2		0x9a9b9c9d

struct settings_test_values {
	uint32_t factory;
	uint32_t both;
	uint32_t run;
};

Z_SETTINGS_DT_STORE_DECLARE(DT_NODELABEL(settings_factory));
Z_SETTINGS_DT_STORE_DECLARE(DT_NODELABEL(settings_run));

static const struct settings_store_static *factory_static =
	Z_SETTINGS_DT_STORE_GET(DT_NODELABEL(settings_factory));
static const struct settings_store_static *run_static =
	Z_SETTINGS_DT_STORE_GET(DT_NODELABEL(settings_run));

static struct settings_test_values test_values;

static uint32_t *settings_name_to_value(struct settings_test_values *test_values,
					const char *name)
{
	struct {
		const char *name;
		uint32_t *value;
	} map[] = {
		{ "factory", &test_values->factory },
		{ "both", &test_values->both },
		{ "run", &test_values->run },
	};

	for (size_t i = 0; i < ARRAY_SIZE(map); i++) {
		if (!strcmp(name, map[i].name)) {
			return map[i].value;
		}
	}

	return NULL;
}

struct store_load_data {
	const char *name;
	uint32_t expected;
	bool loaded;
};

static int store_load_direct(const char *name, size_t len,
			     settings_read_cb read_cb,
			     void *cb_arg, void *param)
{
	struct store_load_data *data = param;
	uint32_t loaded;
	int ret;

	ret = read_cb(cb_arg, &loaded, sizeof(loaded));
	zassert_equal(ret, sizeof(loaded), "invalid length of read value");

	zassert_equal(data->expected, loaded,
		      "Test value %s (0x%x) and load value (0x%x) are not equal",
		      (unsigned int) data->expected, (unsigned int) loaded);

	data->loaded = true;

	return 0;
}

static void store_check_expected(struct settings_store *store, const char *name,
				 uint32_t expected)
{
	struct store_load_data data = {
		.name = name,
		.expected = expected,
	};
	struct settings_load_arg arg = {
		.subtree = name,
		.cb = store_load_direct,
		.param = &data,
	};
	int err;

	LOG_INF("Check expected %-12s 0x%08x", name, (unsigned int) expected);

	err = store->cs_itf->csi_load(store, &arg);
	zassert_equal(err, 0, "Failed to directly load settings");

	zassert_true(data.loaded, "Expected %s value not loaded", name);
}

static void store_check_deleted(struct settings_store *store, const char *name)
{
	struct store_load_data data = {
		.name = name,
	};
	struct settings_load_arg arg = {
		.subtree = name,
		.cb = store_load_direct,
		.param = &data,
	};
	int err;

	LOG_INF("Check deleted  %s", name);

	err = store->cs_itf->csi_load(store, &arg);
	zassert_equal(err, 0, "Failed to directly load settings");

	zassert_false(data.loaded, "%s was not expected (should be deleted)",
		      name);
}

static int handler_get(const char *name, char *val, int val_len_max)
{
	uint32_t *value_p = settings_name_to_value(&test_values, name);

	zassert_not_equal(value_p, NULL, "value '%s' not found", name);
	zassert_true(val_len_max >= sizeof(*value_p), "invalid length of value get");

	memcpy(val, value_p, sizeof(*value_p));

	return sizeof(*value_p);
}

static int handler_set(const char *name, size_t len, settings_read_cb read_cb,
		       void *cb_arg)
{
	uint32_t *value_p = settings_name_to_value(&test_values, name);
	ssize_t ret;

	zassert_not_equal(value_p, NULL, "value '%s' not found", name);
	zassert_equal(len, sizeof(*value_p), "invalid length of value set");

	ret = read_cb(cb_arg, value_p, sizeof(*value_p));
	zassert_equal(ret, sizeof(*value_p), "invalid length of read value");

	return 0;
}

static int handler_export(int (*export)(const char *name, const void *val,
					size_t val_len))
{
	struct {
		const char *name;
		uint32_t *value;
	} map[] = {
		{ "factory", &test_values.factory },
		{ "both", &test_values.both },
		{ "run", &test_values.run },
	};
	int err;

	for (size_t i = 0; i < ARRAY_SIZE(map); i++) {
		err = export(map[i].name, map[i].value, sizeof(*map[i].value));
		zassert_equal(err, 0, "failed to export value '%s'", map[i].name);
	}

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(dt_init_test, "init",
			       handler_get, handler_set,
			       NULL, handler_export);

static int partition_erase(const char *name, int id)
{
	const struct flash_area *fap;
	int err;

	LOG_INF("Erasing %s", name);

	err = flash_area_open(id, &fap);
	if (err) {
		return err;
	}

	err = flash_area_erase(fap, 0, fap->fa_size);
	if (err) {
		LOG_ERR("Failed to erase %s: %d", name, err);
	}

	flash_area_close(fap);

	return err;
}

static int partitions_init(const struct device *dev)
{
#if FIXED_PARTITION_EXISTS(storage_partition)
	partition_erase("storage", FIXED_PARTITION_ID(storage_partition));
#endif
#if FIXED_PARTITION_EXISTS(settings_factory_partition)
	partition_erase("settings_factory", FIXED_PARTITION_ID(settings_factory_partition));
#endif
#if FIXED_PARTITION_EXISTS(settings_run_partition)
	partition_erase("settings_run", FIXED_PARTITION_ID(settings_run_partition));
#endif

	return 0;
}

SYS_INIT(partitions_init, POST_KERNEL, 95);

static void save_one_store(struct settings_store *store, const char *name,
			   uint32_t value)
{
	int err;

	err = store->cs_itf->csi_save(store, name, (void *)&value,
				      sizeof(value));

	zassert_equal(err, 0, "failed to save setting %s: %d",
		      name, err);
}

static void *dt_init_setup(void)
{
	struct settings_store *factory = factory_static->store;
	struct settings_store *run = run_static->store;
	int err;

	err = settings_subsys_init();
	zassert_true(err == 0, "subsys init failed");

	save_one_store(factory, "init/factory", FACTORY_VALUE);
	save_one_store(factory, "init/both", BOTH_IN_FACTORY_VALUE);

	save_one_store(run, "init/run", RUN_VALUE);
	save_one_store(run, "init/both", BOTH_IN_RUN_VALUE);

	return NULL;
}

ZTEST(dt_init, test_01_initial_load)
{
	struct settings_store *factory = factory_static->store;
	struct settings_store *run = run_static->store;
	int err;

	err = settings_load();
	zassert_true(err == 0, "can't load settings");

	store_check_expected(factory, "init/factory", FACTORY_VALUE);
	store_check_expected(factory, "init/both", BOTH_IN_FACTORY_VALUE);

	store_check_expected(run, "init/run", RUN_VALUE);
	store_check_expected(run, "init/both", BOTH_IN_RUN_VALUE);

	zassert_equal(test_values.factory, FACTORY_VALUE, "Value is not as expected");
	zassert_equal(test_values.both, BOTH_IN_RUN_VALUE, "Value is not as expected");
	zassert_equal(test_values.run, RUN_VALUE, "Value is not as expected");
}

static void save_one(const char *name, uint32_t value)
{
	int err;

	err = settings_save_one(name, &value, sizeof(value));
	zassert_equal(err, 0, "failed to save setting %s: %d",
		      name, err);
}

ZTEST(dt_init, test_02_save_and_load)
{
	struct settings_store *factory = factory_static->store;
	struct settings_store *run = run_static->store;
	int err;

	save_one("init/run", RUN_VALUE_2);
	save_one("init/both", BOTH_IN_RUN_VALUE_2);

	err = settings_load();
	zassert_true(err == 0, "can't load settings");

	store_check_expected(factory, "init/factory", FACTORY_VALUE);
	store_check_expected(factory, "init/both", BOTH_IN_FACTORY_VALUE);

	store_check_expected(run, "init/run", RUN_VALUE_2);
	store_check_expected(run, "init/both", BOTH_IN_RUN_VALUE_2);

	zassert_equal(test_values.factory, FACTORY_VALUE, "Value is not as expected");
	zassert_equal(test_values.both, BOTH_IN_RUN_VALUE_2, "Value is not as expected");
	zassert_equal(test_values.run, RUN_VALUE_2, "Value is not as expected");
}

ZTEST(dt_init, test_03_delete_and_load)
{
	struct settings_store *factory = factory_static->store;
	struct settings_store *run = run_static->store;
	int err;

	(void)settings_delete("init/run");
	(void)settings_delete("init/both");

	memset(&test_values, 0x0, sizeof(test_values));

	err = settings_load();
	zassert_true(err == 0, "can't load settings");

	store_check_expected(factory, "init/factory", FACTORY_VALUE);
	store_check_expected(factory, "init/both", BOTH_IN_FACTORY_VALUE);

	store_check_deleted(run, "init/run");
	store_check_deleted(run, "init/both");

	zassert_equal(test_values.factory, FACTORY_VALUE, "Value is not as expected");
	zassert_equal(test_values.both, BOTH_IN_FACTORY_VALUE, "Value is not as expected");
	zassert_equal(test_values.run, 0x0, "init/run is not as expected");
}

ZTEST_SUITE(dt_init, NULL, dt_init_setup, NULL, NULL, NULL);
