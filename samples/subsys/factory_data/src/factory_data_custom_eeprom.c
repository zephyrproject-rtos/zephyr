/*
 * Copyright (c) 2022 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/drivers/eeprom.h>
#include <zephyr/factory_data/factory_data.h>

#define sample_factory_data_magic 0xfac70123

static bool factory_data_subsys_initialized;

struct sample_factory_data_v1 {
	uint32_t magic;
	uint8_t version;
	uint8_t uuid[16];
	uint8_t mac_address[6];
	uint8_t value_max_len[CONFIG_FACTORY_DATA_VALUE_LEN_MAX];
} __packed;

struct sample_factory_data_field_description {
	const char *const name;
	const size_t offset;
	const size_t size;
};

struct sample_factory_data_field_description description[] = {
	{.name = "uuid",
	 .offset = offsetof(struct sample_factory_data_v1, uuid),
	 sizeof(((struct sample_factory_data_v1){0}).uuid)},
	{.name = "mac_address",
	 .offset = offsetof(struct sample_factory_data_v1, mac_address),
	 sizeof(((struct sample_factory_data_v1){0}).mac_address)},
	{.name = "value_max_len",
	 .offset = offsetof(struct sample_factory_data_v1, value_max_len),
	 sizeof(((struct sample_factory_data_v1){0}).value_max_len)}};

static const struct device *eeprom_dev = DEVICE_DT_GET(DT_ALIAS(eeprom_0));

/**
 * Return pointer to description of a named field.
 *
 * @param name Name of the value to look for
 *
 * @return Pointer to the field description, NULL when not found
 */
static struct sample_factory_data_field_description *find_description(const char *const name)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(description); i++) {
		if (strcmp(description[i].name, name) == 0) {
			return description + i;
		}
	}

	return NULL;
}

static int factory_data_init_partition(void);

int factory_data_init(void)
{
	int ret;
	uint32_t magic;
	uint8_t version;

	if (factory_data_subsys_initialized) {
		return 0;
	}

	if (!device_is_ready(eeprom_dev)) {
		return -EIO;
	}

	if (eeprom_get_size(eeprom_dev) < sizeof(struct sample_factory_data_v1)) {
		return -ENOMEM;
	}

	ret = eeprom_read(eeprom_dev, 0, &magic, sizeof(magic));
	if (ret) {
		return ret;
	}

	/* Check version if partition was initialized before, initialize (write header) otherwise */
	if (magic != sample_factory_data_magic) {
		return factory_data_init_partition();
	}

	ret = eeprom_read(eeprom_dev, sizeof(magic), &version, sizeof(version));
	if (ret) {
		return ret;
	}
	if (version != 1) {
		return -EIO;
	}

	factory_data_subsys_initialized = true;

	return 0;
}

#if CONFIG_FACTORY_DATA_WRITE
static int factory_data_init_partition(void)
{
	struct sample_factory_data_v1 initial_state = {.magic = sample_factory_data_magic,
						       .version = 1};

	if (!device_is_ready(eeprom_dev)) {
		return -EIO;
	}

	return eeprom_write(eeprom_dev, 0, &initial_state, sizeof(initial_state));
}

int factory_data_save_one(const char *name, const void *value, size_t val_len)
{
	struct sample_factory_data_field_description *d = find_description(name);
	int ret;

	if (!factory_data_subsys_initialized) {
		return -ECANCELED;
	}

	if (!d) {
		return -EINVAL;
	}

	if (val_len > d->size) {
		return -EFBIG;
	}

	ret = eeprom_write(eeprom_dev, d->offset, value, val_len);
	if (ret) {
		return ret;
	}

	return 0;
}

int factory_data_erase(void)
{
	return factory_data_init_partition();
}

#else /* CONFIG_FACTORY_DATA_WRITE */

static int factory_data_init_partition(void)
{
	return -EACCES;
}

#endif /* CONFIG_FACTORY_DATA_WRITE */

int factory_data_load(factory_data_load_direct_cb cb, const void *param)
{
	size_t i;
	uint8_t buf[FACTORY_DATA_TOTAL_LEN_MAX];

	if (!factory_data_subsys_initialized) {
		return -ECANCELED;
	}

	for (i = 0; i < ARRAY_SIZE(description); i++) {
		int ret;

		ret = eeprom_read(eeprom_dev, description[i].offset, buf, description[i].size);
		if (ret) {
			return ret;
		}

		ret = cb(description[i].name, buf, description[i].size, param);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

ssize_t factory_data_load_one(const char *name, uint8_t *value, size_t val_len)
{
	struct sample_factory_data_field_description *d = find_description(name);
	size_t num_bytes;
	int ret;

	if (!factory_data_subsys_initialized) {
		return -ECANCELED;
	}

	if (!d) {
		return -ENOENT;
	}

	num_bytes = MIN(d->size, val_len);

	ret = eeprom_read(eeprom_dev, d->offset, value, num_bytes);
	if (ret) {
		return ret;
	}

	return num_bytes;
}
