/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include "settings/settings_zms.h"
#include "settings_priv.h"

#include <zephyr/settings/settings.h>
#include <zephyr/sys/hash_function.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(settings, CONFIG_SETTINGS_LOG_LEVEL);

#if DT_HAS_CHOSEN(zephyr_settings_partition)
#define SETTINGS_PARTITION DT_FIXED_PARTITION_ID(DT_CHOSEN(zephyr_settings_partition))
#else
#define SETTINGS_PARTITION FIXED_PARTITION_ID(storage_partition)
#endif

struct settings_zms_read_fn_arg {
	struct zms_fs *fs;
	uint32_t id;
};

static int settings_zms_load(struct settings_store *cs, const struct settings_load_arg *arg);
static int settings_zms_save(struct settings_store *cs, const char *name, const char *value,
			     size_t val_len);
static void *settings_zms_storage_get(struct settings_store *cs);

static struct settings_store_itf settings_zms_itf = {.csi_load = settings_zms_load,
						     .csi_save = settings_zms_save,
						     .csi_storage_get = settings_zms_storage_get};

static ssize_t settings_zms_read_fn(void *back_end, void *data, size_t len)
{
	struct settings_zms_read_fn_arg *rd_fn_arg;

	rd_fn_arg = (struct settings_zms_read_fn_arg *)back_end;

	return zms_read(rd_fn_arg->fs, rd_fn_arg->id, data, len);
}

int settings_zms_src(struct settings_zms *cf)
{
	cf->cf_store.cs_itf = &settings_zms_itf;
	settings_src_register(&cf->cf_store);

	return 0;
}

int settings_zms_dst(struct settings_zms *cf)
{
	cf->cf_store.cs_itf = &settings_zms_itf;
	settings_dst_register(&cf->cf_store);

	return 0;
}

#if CONFIG_SETTINGS_ZMS_NAME_CACHE
#define SETTINGS_ZMS_CACHE_OVFL(cf) ((cf)->cache_total > ARRAY_SIZE((cf)->cache))

static void settings_zms_cache_add(struct settings_zms *cf, const char *name, uint32_t name_id)
{
	uint32_t name_hash = sys_hash32(name, strlen(name));

	cf->cache[cf->cache_next].name_hash = name_hash;
	cf->cache[cf->cache_next++].name_id = name_id;

	cf->cache_next %= CONFIG_SETTINGS_ZMS_NAME_CACHE_SIZE;
}

static uint32_t settings_zms_cache_match(struct settings_zms *cf, const char *name, char *rdname,
					 size_t len)
{
	uint32_t name_hash = sys_hash32(name, strlen(name));
	int rc;

	for (int i = 0; i < CONFIG_SETTINGS_ZMS_NAME_CACHE_SIZE; i++) {
		if (cf->cache[i].name_hash != name_hash) {
			continue;
		}

		if (cf->cache[i].name_id <= ZMS_NAMECNT_ID) {
			continue;
		}

		rc = zms_read(&cf->cf_zms, cf->cache[i].name_id, rdname, len);
		if (rc < 0) {
			continue;
		}

		rdname[rc] = '\0';

		if (strcmp(name, rdname)) {
			continue;
		}

		return cf->cache[i].name_id;
	}

	return ZMS_NAMECNT_ID;
}
#endif /* CONFIG_SETTINGS_ZMS_NAME_CACHE */

static int settings_zms_load(struct settings_store *cs, const struct settings_load_arg *arg)
{
	int ret = 0;
	struct settings_zms *cf = CONTAINER_OF(cs, struct settings_zms, cf_store);
	struct settings_zms_read_fn_arg read_fn_arg;
	char name[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN + 1];
	ssize_t rc1, rc2;
	uint32_t name_id = ZMS_NAMECNT_ID;

#if CONFIG_SETTINGS_ZMS_NAME_CACHE
	uint32_t cached = 0;

	cf->loaded = false;
#endif

	name_id = cf->last_name_id + 1;

	while (1) {

		name_id--;
		if (name_id == ZMS_NAMECNT_ID) {
#if CONFIG_SETTINGS_ZMS_NAME_CACHE
			cf->loaded = true;
			cf->cache_total = cached;
#endif
			break;
		}

		/* In the ZMS backend, each setting item is stored in two ZMS
		 * entries one for the setting's name and one with the
		 * setting's value.
		 */
		rc1 = zms_read(&cf->cf_zms, name_id, &name, sizeof(name));
		/* get the length of data and verify that it exists */
		rc2 = zms_get_data_length(&cf->cf_zms, name_id + ZMS_NAME_ID_OFFSET);

		if ((rc1 <= 0) && (rc2 <= 0)) {
			/* Settings largest ID in use is invalid due to
			 * reset, power failure or partition overflow.
			 * Decrement it and check the next ID in subsequent
			 * iteration.
			 */
			if (name_id == cf->last_name_id) {
				cf->last_name_id--;
				zms_write(&cf->cf_zms, ZMS_NAMECNT_ID, &cf->last_name_id,
					  sizeof(uint32_t));
			}

			continue;
		}

		if ((rc1 <= 0) || (rc2 <= 0)) {
			/* Settings item is not stored correctly in the ZMS.
			 * ZMS entry for its name or value is either missing
			 * or deleted. Clean dirty entries to make space for
			 * future settings item.
			 */
			zms_delete(&cf->cf_zms, name_id);
			zms_delete(&cf->cf_zms, name_id + ZMS_NAME_ID_OFFSET);

			if (name_id == cf->last_name_id) {
				cf->last_name_id--;
				zms_write(&cf->cf_zms, ZMS_NAMECNT_ID, &cf->last_name_id,
					  sizeof(uint32_t));
			}

			continue;
		}

		/* Found a name, this might not include a trailing \0 */
		name[rc1] = '\0';
		read_fn_arg.fs = &cf->cf_zms;
		read_fn_arg.id = name_id + ZMS_NAME_ID_OFFSET;

#if CONFIG_SETTINGS_ZMS_NAME_CACHE
		settings_zms_cache_add(cf, name, name_id);
		cached++;
#endif

		ret = settings_call_set_handler(name, rc2, settings_zms_read_fn, &read_fn_arg,
						(void *)arg);
		if (ret) {
			break;
		}
	}
	return ret;
}

static int settings_zms_save(struct settings_store *cs, const char *name, const char *value,
			     size_t val_len)
{
	struct settings_zms *cf = CONTAINER_OF(cs, struct settings_zms, cf_store);
	char rdname[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN + 1];
	uint32_t name_id, write_name_id;
	bool delete, write_name;
	int rc = 0;

	if (!name) {
		return -EINVAL;
	}

	/* Find out if we are doing a delete */
	delete = ((value == NULL) || (val_len == 0));

#if CONFIG_SETTINGS_ZMS_NAME_CACHE
	bool name_in_cache = false;

	name_id = settings_zms_cache_match(cf, name, rdname, sizeof(rdname));
	if (name_id != ZMS_NAMECNT_ID) {
		write_name_id = name_id;
		write_name = false;
		name_in_cache = true;
		goto found;
	}
#endif

	/* No entry with "name" is in cache, let's find if it exists in the storage */
	name_id = cf->last_name_id + 1;
	write_name_id = cf->last_name_id + 1;
	write_name = true;

#if CONFIG_SETTINGS_ZMS_NAME_CACHE
	/* We can skip reading ZMS if we know that the cache wasn't overflowed. */
	if (cf->loaded && !SETTINGS_ZMS_CACHE_OVFL(cf)) {
		goto found;
	}
#endif

	/* Let's find if we already have an ID within storage */
	while (1) {
		name_id--;
		if (name_id == ZMS_NAMECNT_ID) {
			break;
		}

		rc = zms_read(&cf->cf_zms, name_id, &rdname, sizeof(rdname));

		if (rc < 0) {
			/* Error or entry not found */
			if (rc == -ENOENT) {
				/* This is a free ID let's keep it */
				write_name_id = name_id;
			}
			continue;
		}

		rdname[rc] = '\0';

		if (strcmp(name, rdname)) {
			/* ID exists but the name is different, that's not the ID
			 * we are looking for.
			 */
			continue;
		}

		/* At this step we found the ID that corresponds to name */
		if (!delete) {
			write_name_id = name_id;
			write_name = false;
		}

		goto found;
	}

found:
	if (delete) {
		if (name_id == ZMS_NAMECNT_ID) {
			return 0;
		}

		rc = zms_delete(&cf->cf_zms, name_id);
		if (rc >= 0) {
			rc = zms_delete(&cf->cf_zms, name_id + ZMS_NAME_ID_OFFSET);
		}

		if (rc < 0) {
			return rc;
		}

		if (name_id == cf->last_name_id) {
			cf->last_name_id--;
			rc = zms_write(&cf->cf_zms, ZMS_NAMECNT_ID, &cf->last_name_id,
				       sizeof(uint32_t));
			if (rc < 0) {
				/* Error: can't to store
				 * the largest name ID in use.
				 */
				return rc;
			}
		}

		return 0;
	}

	/* No free IDs left. */
	if (write_name_id == ZMS_NAMECNT_ID + ZMS_NAME_ID_OFFSET - 1) {
		return -ENOMEM;
	}

	/* update the last_name_id and write to flash if required*/
	if (write_name_id > cf->last_name_id) {
		cf->last_name_id = write_name_id;
		rc = zms_write(&cf->cf_zms, ZMS_NAMECNT_ID, &cf->last_name_id, sizeof(uint32_t));
		if (rc < 0) {
			return rc;
		}
	}

	/* write the value */
	rc = zms_write(&cf->cf_zms, write_name_id + ZMS_NAME_ID_OFFSET, value, val_len);
	if (rc < 0) {
		return rc;
	}

	/* write the name if required */
	if (write_name) {
		rc = zms_write(&cf->cf_zms, write_name_id, name, strlen(name));
		if (rc < 0) {
			return rc;
		}
	}

#if CONFIG_SETTINGS_ZMS_NAME_CACHE
	if (!name_in_cache) {
		settings_zms_cache_add(cf, name, write_name_id);
		if (cf->loaded && !SETTINGS_ZMS_CACHE_OVFL(cf)) {
			cf->cache_total++;
		}
	}
#endif

	return 0;
}

/* Initialize the zms backend. */
int settings_zms_backend_init(struct settings_zms *cf)
{
	int rc;
	uint32_t last_name_id;

	cf->cf_zms.flash_device = cf->flash_dev;
	if (cf->cf_zms.flash_device == NULL) {
		return -ENODEV;
	}

	rc = zms_mount(&cf->cf_zms);
	if (rc) {
		return rc;
	}

	rc = zms_read(&cf->cf_zms, ZMS_NAMECNT_ID, &last_name_id, sizeof(last_name_id));
	if (rc < 0) {
		cf->last_name_id = ZMS_NAMECNT_ID;
	} else {
		cf->last_name_id = last_name_id;
	}

	LOG_DBG("Initialized");
	return 0;
}

int settings_backend_init(void)
{
	static struct settings_zms default_settings_zms;
	int rc;
	uint32_t cnt = 0;
	size_t zms_sector_size, zms_size = 0;
	const struct flash_area *fa;
	struct flash_sector hw_flash_sector;
	uint32_t sector_cnt = 1;

	rc = flash_area_open(SETTINGS_PARTITION, &fa);
	if (rc) {
		return rc;
	}

	rc = flash_area_get_sectors(SETTINGS_PARTITION, &sector_cnt, &hw_flash_sector);
	if (rc != 0 && rc != -ENOMEM) {
		return rc;
	}

	zms_sector_size = CONFIG_SETTINGS_ZMS_SECTOR_SIZE_MULT * hw_flash_sector.fs_size;

	if (zms_sector_size > UINT32_MAX) {
		return -EDOM;
	}

	while (cnt < CONFIG_SETTINGS_ZMS_SECTOR_COUNT) {
		zms_size += zms_sector_size;
		if (zms_size > fa->fa_size) {
			break;
		}
		cnt++;
	}

	/* define the zms file system using the page_info */
	default_settings_zms.cf_zms.sector_size = zms_sector_size;
	default_settings_zms.cf_zms.sector_count = cnt;
	default_settings_zms.cf_zms.offset = fa->fa_off;
	default_settings_zms.flash_dev = fa->fa_dev;

	rc = settings_zms_backend_init(&default_settings_zms);
	if (rc) {
		return rc;
	}

	rc = settings_zms_src(&default_settings_zms);

	if (rc) {
		return rc;
	}

	rc = settings_zms_dst(&default_settings_zms);

	return rc;
}

static void *settings_zms_storage_get(struct settings_store *cs)
{
	struct settings_zms *cf = CONTAINER_OF(cs, struct settings_zms, cf_store);

	return &cf->cf_zms;
}
