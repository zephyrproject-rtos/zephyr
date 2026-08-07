/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/retention/retention.h>
#include <zephyr/settings/settings.h>
#include "settings_priv.h"

LOG_MODULE_DECLARE(settings, CONFIG_SETTINGS_LOG_LEVEL);

#if !DT_HAS_CHOSEN(zephyr_settings_partition)
#error "Missing zephyr,settings-partition chosen node"
#elif !DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_settings_partition), zephyr_retention)
#error "zephyr,settings-partition must be a zephyr,retention node"
#endif

/*
 * Retention storage stores each setting in the following format:
 *   uint16_t length_name
 *   uint16_t length_value
 *   uint8_t name[...]
 *   uint8_t value[...]
 *
 * Each setting is placed sequentially into the retention memory area, it is assumed that the
 * checksum feature is used to ensure data validity upon loading settings from the retained
 * memory though this is optional.
 *
 * Upon saving settings, the whole retention area is cleared first, then settings are written
 * one-by-one, it is only supported to save/load all settings in one go.
 */

/** Retention settings context object */
struct settings_retention {
	/** Settings storage */
	struct settings_store cf_store;

	/** Retention device */
	const struct device *cf_retention;

	/** Last write position when setting was saved */
	uint32_t last_write_pos;
};

/** Length of name and value object, used when reading/saving settings */
struct settings_retention_lengths {
	/** Length of name */
	uint16_t length_name;

	/** Length of value */
	uint16_t length_value;

	/* Name and value byte arrays follow past this point... */
};

BUILD_ASSERT(sizeof(struct settings_retention_lengths) == sizeof(uint16_t) + sizeof(uint16_t));

/** Used with read callback */
struct settings_retention_read_arg {
	/** Retention device */
	const struct device *cf_retention;

	/** Offset to read from */
	uint32_t offset;
};

static int settings_retention_load(struct settings_store *cs, const struct settings_load_arg *arg);
static int settings_retention_save(struct settings_store *cs, const char *name, const char *value,
				   size_t val_len);
static void *settings_retention_storage_get(struct settings_store *cs);
static int settings_retention_save_start(struct settings_store *cs);

static const struct device *storage_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_settings_partition));

static const struct settings_store_itf settings_retention_itf = {
	.csi_load = settings_retention_load,
	.csi_save = settings_retention_save,
	.csi_storage_get = settings_retention_storage_get,
	.csi_save_start = settings_retention_save_start,
};

static int settings_retention_src(struct settings_retention *cf)
{
	if (!retention_is_valid(cf->cf_retention)) {
		return -EIO;
	}

	cf->cf_store.cs_itf = &settings_retention_itf;
	settings_src_register(&cf->cf_store);

	return 0;
}

static int settings_retention_dst(struct settings_retention *cf)
{
	cf->cf_store.cs_itf = &settings_retention_itf;
	settings_dst_register(&cf->cf_store);

	return 0;
}

static int settings_retention_read_value(void *cb_arg, void *data, size_t len)
{
	int rc;
	struct settings_retention_read_arg *ctx = cb_arg;

	rc = retention_read(ctx->cf_retention, ctx->offset, data, len);

	if (rc != 0) {
		return rc;
	}

	return len;
}

static int settings_retention_load(struct settings_store *cs, const struct settings_load_arg *arg)
{
	int rc;
	uint32_t pos = 0;
	struct settings_retention *cf = CONTAINER_OF(cs, struct settings_retention, cf_store);
	uint32_t max_pos = retention_size(cf->cf_retention);
	struct settings_retention_read_arg read_arg = {
		.cf_retention = cf->cf_retention,
	};

	while (pos < max_pos) {
		struct settings_retention_lengths lengths;
		char name[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN + 1];

		if ((pos + sizeof(lengths)) >= max_pos) {
			return -EIO;
		}

		/* Read lengths and check validity */
		rc = retention_read(cf->cf_retention, pos, (uint8_t *)&lengths, sizeof(lengths));

		if (rc != 0) {
			return rc;
		}

		if ((lengths.length_name == 0 && lengths.length_value == 0) ||
		    (lengths.length_name == USHRT_MAX && lengths.length_value == USHRT_MAX)) {
			/* Empty data, finished loading */
			LOG_DBG("Finished loading retentions settings, size: 0x%x", pos);
			break;
		} else if (lengths.length_name > SETTINGS_MAX_NAME_LEN) {
			LOG_ERR("Invalid name length: %d, max supported: %d",
				lengths.length_name, SETTINGS_MAX_NAME_LEN);
			return -EIO;
		} else if (lengths.length_value > SETTINGS_MAX_VAL_LEN) {
			LOG_ERR("Invalid value length: %d, max supported: %d",
				lengths.length_name, SETTINGS_MAX_VAL_LEN);
			return -EIO;
		} else if ((lengths.length_name + lengths.length_value + pos) > max_pos) {
			LOG_ERR("Data length goes beyond retention area: 0x%x, max size: 0x%x",
				(lengths.length_name + lengths.length_value + pos), max_pos);
			return -EIO;
		}

		/* Read values */
		pos += sizeof(lengths);
		rc = retention_read(cf->cf_retention, pos, name, lengths.length_name);

		if (rc != 0) {
			return rc;
		}

		name[lengths.length_name] = '\0';
		pos += lengths.length_name;
		read_arg.offset = pos;

		rc = settings_call_set_handler(name, lengths.length_value,
					       &settings_retention_read_value, &read_arg, arg);

		if (rc != 0) {
			return rc;
		}

		pos += lengths.length_value;
	}

	return 0;
}

static int settings_retention_save(struct settings_store *cs, const char *name, const char *value,
				   size_t val_len)
{
	struct settings_retention *cf = CONTAINER_OF(cs, struct settings_retention, cf_store);
	struct settings_retention_lengths lengths;
	uint32_t off = cf->last_write_pos;
	int rc = -EINVAL;

	if (name == NULL || (value == NULL && val_len > 0)) {
		return -EINVAL;
	}

	lengths.length_name = (uint16_t)strlen(name);
	lengths.length_value = (uint16_t)val_len;

	if (lengths.length_name == 0) {
		return -EINVAL;
	} else if ((cf->last_write_pos + sizeof(lengths) + lengths.length_name + val_len) >
		   retention_size(cf->cf_retention)) {
		return -E2BIG;
	}

	/* Write data before writing length header to ensure that if something happens before one
	 * is written then the data is not wrongly seen as valid upon reading, as would be the
	 * case if it was partially written
	 */
	off += sizeof(lengths);
	rc = retention_write(cf->cf_retention, off, name, lengths.length_name);

	if (rc != 0) {
		return rc;
	}

	off += lengths.length_name;
	rc = retention_write(cf->cf_retention, off, value, val_len);

	if (rc != 0) {
		goto tidy;
	}

	rc = retention_write(cf->cf_retention, cf->last_write_pos, (uint8_t *)&lengths,
			     sizeof(lengths));

	if (rc != 0) {
		goto tidy;
	}

	off += val_len;
	cf->last_write_pos = off;

tidy:
	if (rc != 0) {
		/* Attempt to clear data header that was partially written */
		uint8_t empty_data[sizeof(lengths)] = { 0x00 };
		uint8_t l = sizeof(lengths) + lengths.length_name + val_len;
		uint8_t i = 0;

		while (i < l) {
			uint8_t write_len = (i + sizeof(empty_data)) > l ? (l - i) :
					    sizeof(empty_data);

			rc = retention_write(cf->cf_retention, (cf->last_write_pos + i),
					     empty_data, write_len);

			if (rc != 0) {
				break;
			}

			i += write_len;
		}
	}

	return rc;
}

static int settings_retention_save_start(struct settings_store *cs)
{
	struct settings_retention *cf = CONTAINER_OF(cs, struct settings_retention, cf_store);

	cf->last_write_pos = 0;

	return retention_clear(cf->cf_retention);
}

int settings_backend_init(void)
{
	int rc;
	static struct settings_retention config_init_settings_retention;

	if (!device_is_ready(storage_dev)) {
		return -ENOENT;
	}

	config_init_settings_retention.cf_retention = storage_dev;
	config_init_settings_retention.last_write_pos = 0;

	rc = settings_retention_src(&config_init_settings_retention);

	if (rc != 0 && rc != -EIO) {
		return rc;
	}

	rc = settings_retention_dst(&config_init_settings_retention);

	return rc;
}

static void *settings_retention_storage_get(struct settings_store *cs)
{
	struct settings_retention *cf = CONTAINER_OF(cs, struct settings_retention, cf_store);

	return &cf->cf_retention;
}
