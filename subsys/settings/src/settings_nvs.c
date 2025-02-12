/*
 * Copyright (c) 2019 Laczen
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/settings/settings.h>
#include "settings/settings_nvs.h"
#include <zephyr/sys/crc.h>
#include "settings_priv.h"
#include <zephyr/storage/flash_map.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(settings, CONFIG_SETTINGS_LOG_LEVEL);

#if DT_HAS_CHOSEN(zephyr_settings_partition)
#define SETTINGS_PARTITION DT_FIXED_PARTITION_ID(DT_CHOSEN(zephyr_settings_partition))
#else
#define SETTINGS_PARTITION FIXED_PARTITION_ID(storage_partition)
#endif

struct settings_nvs_read_fn_arg {
	struct nvs_fs *fs;
	uint16_t id;
};

static int settings_nvs_load(struct settings_store *cs,
			     const struct settings_load_arg *arg);
static int settings_nvs_save(struct settings_store *cs, const char *name,
			     const char *value, size_t val_len);
static void *settings_nvs_storage_get(struct settings_store *cs);

static struct settings_store_itf settings_nvs_itf = {
	.csi_load = settings_nvs_load,
	.csi_save = settings_nvs_save,
	.csi_storage_get = settings_nvs_storage_get
};

static ssize_t settings_nvs_read_fn(void *back_end, void *data, size_t len)
{
	struct settings_nvs_read_fn_arg *rd_fn_arg;
	ssize_t rc;

	rd_fn_arg = (struct settings_nvs_read_fn_arg *)back_end;

	rc = nvs_read(rd_fn_arg->fs, rd_fn_arg->id, data, len);
	if (rc > (ssize_t)len) {
		/* nvs_read signals that not all bytes were read
		 * align read len to what was requested
		 */
		rc = len;
	}
	return rc;
}

int settings_nvs_src(struct settings_nvs *cf)
{
	cf->cf_store.cs_itf = &settings_nvs_itf;
	settings_src_register(&cf->cf_store);

	return 0;
}

int settings_nvs_dst(struct settings_nvs *cf)
{
	cf->cf_store.cs_itf = &settings_nvs_itf;
	settings_dst_register(&cf->cf_store);

	return 0;
}

#if CONFIG_SETTINGS_NVS_NAME_CACHE
#define SETTINGS_NVS_CACHE_OVFL(cf) ((cf)->cache_total > ARRAY_SIZE((cf)->cache))

static void settings_nvs_cache_add(struct settings_nvs *cf, const char *name,
				   uint16_t name_id)
{
	uint16_t name_hash = crc16_ccitt(0xffff, name, strlen(name));

	cf->cache[cf->cache_next].name_hash = name_hash;
	cf->cache[cf->cache_next++].name_id = name_id;

	cf->cache_next %= CONFIG_SETTINGS_NVS_NAME_CACHE_SIZE;
}

static uint16_t settings_nvs_cache_match(struct settings_nvs *cf, const char *name,
					 char *rdname, size_t len)
{
	uint16_t name_hash = crc16_ccitt(0xffff, name, strlen(name));
	int rc;

	for (int i = 0; i < CONFIG_SETTINGS_NVS_NAME_CACHE_SIZE; i++) {
		if (cf->cache[i].name_hash != name_hash) {
			continue;
		}

		if (cf->cache[i].name_id <= NVS_NAMECNT_ID) {
			continue;
		}

		rc = nvs_read(&cf->cf_nvs, cf->cache[i].name_id, rdname, len);
		if (rc < 0) {
			continue;
		}

		rdname[rc] = '\0';

		if (strcmp(name, rdname)) {
			continue;
		}

		return cf->cache[i].name_id;
	}

	return NVS_NAMECNT_ID;
}
#endif /* CONFIG_SETTINGS_NVS_NAME_CACHE */

static int settings_nvs_load(struct settings_store *cs,
			     const struct settings_load_arg *arg)
{
	int ret = 0;
	struct settings_nvs *cf = CONTAINER_OF(cs, struct settings_nvs, cf_store);
	struct settings_nvs_read_fn_arg read_fn_arg;
	char name[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN + 1];
	char buf;
	ssize_t rc1, rc2;
	uint16_t name_id = NVS_NAMECNT_ID;

#if CONFIG_SETTINGS_NVS_NAME_CACHE
	uint16_t cached = 0;

	cf->loaded = false;
#endif

	name_id = cf->last_name_id + 1;

	while (1) {

		name_id--;
		if (name_id == NVS_NAMECNT_ID) {
#if CONFIG_SETTINGS_NVS_NAME_CACHE
			cf->loaded = true;
			cf->cache_total = cached;
#endif
			break;
		}

		/* In the NVS backend, each setting item is stored in two NVS
		 * entries one for the setting's name and one with the
		 * setting's value.
		 */
		rc1 = nvs_read(&cf->cf_nvs, name_id, &name, sizeof(name));
		rc2 = nvs_read(&cf->cf_nvs, name_id + NVS_NAME_ID_OFFSET,
			       &buf, sizeof(buf));

		if ((rc1 <= 0) && (rc2 <= 0)) {
			/* Settings largest ID in use is invalid due to
			 * reset, power failure or partition overflow.
			 * Decrement it and check the next ID in subsequent
			 * iteration.
			 */
			if (name_id == cf->last_name_id) {
				cf->last_name_id--;
				nvs_write(&cf->cf_nvs, NVS_NAMECNT_ID,
					  &cf->last_name_id, sizeof(uint16_t));
			}

			continue;
		}

		if ((rc1 <= 0) || (rc2 <= 0)) {
			/* Settings item is not stored correctly in the NVS.
			 * NVS entry for its name or value is either missing
			 * or deleted. Clean dirty entries to make space for
			 * future settings item.
			 */
			nvs_delete(&cf->cf_nvs, name_id);
			nvs_delete(&cf->cf_nvs, name_id + NVS_NAME_ID_OFFSET);

			if (name_id == cf->last_name_id) {
				cf->last_name_id--;
				nvs_write(&cf->cf_nvs, NVS_NAMECNT_ID,
					  &cf->last_name_id, sizeof(uint16_t));
			}

			continue;
		}

		/* Found a name, this might not include a trailing \0 */
		name[rc1] = '\0';
		read_fn_arg.fs = &cf->cf_nvs;
		read_fn_arg.id = name_id + NVS_NAME_ID_OFFSET;

#if CONFIG_SETTINGS_NVS_NAME_CACHE
		settings_nvs_cache_add(cf, name, name_id);
		cached++;
#endif

		ret = settings_call_set_handler(
			name, rc2,
			settings_nvs_read_fn, &read_fn_arg,
			(void *)arg);
		if (ret) {
			break;
		}
	}
	return ret;
}

static int settings_nvs_save(struct settings_store *cs, const char *name,
			     const char *value, size_t val_len)
{
	struct settings_nvs *cf = CONTAINER_OF(cs, struct settings_nvs, cf_store);
	char rdname[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN + 1];
	uint16_t name_id, write_name_id;
	bool delete, write_name;
	int rc = 0;

	if (!name) {
		return -EINVAL;
	}

	/* Find out if we are doing a delete */
	delete = ((value == NULL) || (val_len == 0));

#if CONFIG_SETTINGS_NVS_NAME_CACHE
	bool name_in_cache = false;

	name_id = settings_nvs_cache_match(cf, name, rdname, sizeof(rdname));
	if (name_id != NVS_NAMECNT_ID) {
		write_name_id = name_id;
		write_name = false;
		name_in_cache = true;
		goto found;
	}
#endif

	name_id = cf->last_name_id + 1;
	write_name_id = cf->last_name_id + 1;
	write_name = true;

#if CONFIG_SETTINGS_NVS_NAME_CACHE
	/* We can skip reading NVS if we know that the cache wasn't overflowed. */
	if (cf->loaded && !SETTINGS_NVS_CACHE_OVFL(cf)) {
		goto found;
	}
#endif

	while (1) {
		name_id--;
		if (name_id == NVS_NAMECNT_ID) {
			break;
		}

		rc = nvs_read(&cf->cf_nvs, name_id, &rdname, sizeof(rdname));

		if (rc < 0) {
			/* Error or entry not found */
			if (rc == -ENOENT) {
				write_name_id = name_id;
			}
			continue;
		}

		rdname[rc] = '\0';

		if (strcmp(name, rdname)) {
			continue;
		}

		if (!delete) {
			write_name_id = name_id;
			write_name = false;
		}

		goto found;
	}

found:
	if (delete) {
		if (name_id == NVS_NAMECNT_ID) {
			return 0;
		}

		rc = nvs_delete(&cf->cf_nvs, name_id);
		if (rc >= 0) {
			rc = nvs_delete(&cf->cf_nvs, name_id +
					NVS_NAME_ID_OFFSET);
		}

		if (rc < 0) {
			return rc;
		}

		if (name_id == cf->last_name_id) {
			cf->last_name_id--;
			rc = nvs_write(&cf->cf_nvs, NVS_NAMECNT_ID,
				       &cf->last_name_id, sizeof(uint16_t));
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
	if (write_name_id == NVS_NAMECNT_ID + NVS_NAME_ID_OFFSET) {
		return -ENOMEM;
	}

	/* update the last_name_id and write to flash if required*/
	if (write_name_id > cf->last_name_id) {
		cf->last_name_id = write_name_id;
		rc = nvs_write(&cf->cf_nvs, NVS_NAMECNT_ID, &cf->last_name_id,
			       sizeof(uint16_t));
		if (rc < 0) {
			return rc;
		}
	}

	/* write the value */
	rc = nvs_write(&cf->cf_nvs, write_name_id + NVS_NAME_ID_OFFSET,
		       value, val_len);
	if (rc < 0) {
		return rc;
	}

	/* write the name if required */
	if (write_name) {
		rc = nvs_write(&cf->cf_nvs, write_name_id, name, strlen(name));
		if (rc < 0) {
			return rc;
		}
	}

#if CONFIG_SETTINGS_NVS_NAME_CACHE
	if (!name_in_cache) {
		settings_nvs_cache_add(cf, name, write_name_id);
		if (cf->loaded && !SETTINGS_NVS_CACHE_OVFL(cf)) {
			cf->cache_total++;
		}
	}
#endif

	return 0;
}

/* Initialize the nvs backend. */
int settings_nvs_backend_init(struct settings_nvs *cf)
{
	int rc;
	uint16_t last_name_id;

	cf->cf_nvs.flash_device = cf->flash_dev;
	if (cf->cf_nvs.flash_device == NULL) {
		return -ENODEV;
	}

	rc = nvs_mount(&cf->cf_nvs);
	if (rc) {
		return rc;
	}

	rc = nvs_read(&cf->cf_nvs, NVS_NAMECNT_ID, &last_name_id,
		      sizeof(last_name_id));
	if (rc < 0) {
		cf->last_name_id = NVS_NAMECNT_ID;
	} else {
		cf->last_name_id = last_name_id;
	}

	LOG_DBG("Initialized");
	return 0;
}

int settings_backend_init(void)
{
	static struct settings_nvs default_settings_nvs;
	int rc;
	uint16_t cnt = 0;
	size_t nvs_sector_size, nvs_size = 0;
	const struct flash_area *fa;
	struct flash_sector hw_flash_sector;
	uint32_t sector_cnt = 1;

	rc = flash_area_open(SETTINGS_PARTITION, &fa);
	if (rc) {
		return rc;
	}

	rc = flash_area_get_sectors(SETTINGS_PARTITION, &sector_cnt,
				    &hw_flash_sector);
	if (rc != 0 && rc != -ENOMEM) {
		return rc;
	}

	nvs_sector_size = CONFIG_SETTINGS_NVS_SECTOR_SIZE_MULT *
			  hw_flash_sector.fs_size;

	if (nvs_sector_size > UINT16_MAX) {
		return -EDOM;
	}

	while (cnt < CONFIG_SETTINGS_NVS_SECTOR_COUNT) {
		nvs_size += nvs_sector_size;
		if (nvs_size > fa->fa_size) {
			break;
		}
		cnt++;
	}

	/* define the nvs file system using the page_info */
	default_settings_nvs.cf_nvs.sector_size = nvs_sector_size;
	default_settings_nvs.cf_nvs.sector_count = cnt;
	default_settings_nvs.cf_nvs.offset = fa->fa_off;
	default_settings_nvs.flash_dev = fa->fa_dev;

	rc = settings_nvs_backend_init(&default_settings_nvs);
	if (rc) {
		return rc;
	}

	rc = settings_nvs_src(&default_settings_nvs);

	if (rc) {
		return rc;
	}

	rc = settings_nvs_dst(&default_settings_nvs);

	return rc;
}

static void *settings_nvs_storage_get(struct settings_store *cs)
{
	struct settings_nvs *cf = CONTAINER_OF(cs, struct settings_nvs, cf_store);

	return &cf->cf_nvs;
}
