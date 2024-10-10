/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SETTINGS_ZMS_H_
#define __SETTINGS_ZMS_H_

#include <zephyr/fs/zms.h>
#include <zephyr/settings/settings.h>

#ifdef __cplusplus
extern "C" {
#endif

/* In the ZMS backend, each setting is stored in two ZMS entries:
 *	1. setting's name
 *	2. setting's value
 *
 * The ZMS entry ID for the setting's value is determined implicitly based on
 * the ID of the ZMS entry for the setting's name, once that is found. The
 * difference between name and value ID is constant and equal to
 * ZMS_NAME_ID_OFFSET.
 *
 * Setting's name entries start from ZMS_NAMECNT_ID + 1.
 * The entry with ID == ZMS_NAMECNT_ID is used to store the largest name ID in use.
 *
 * Deleted records will not be found, only the last record will be read.
 */
#define ZMS_NAMECNT_ID     0x80000000
#define ZMS_NAME_ID_OFFSET 0x40000000

struct settings_zms {
	struct settings_store cf_store;
	struct zms_fs cf_zms;
	uint32_t last_name_id;
	const struct device *flash_dev;
#if CONFIG_SETTINGS_ZMS_NAME_CACHE
	struct {
		uint32_t name_hash;
		uint32_t name_id;
	} cache[CONFIG_SETTINGS_ZMS_NAME_CACHE_SIZE];

	uint32_t cache_next;
	uint32_t cache_total;
	bool loaded;
#endif
};

/* register zms to be a source of settings */
int settings_zms_src(struct settings_zms *cf);

/* register zms to be the destination of settings */
int settings_zms_dst(struct settings_zms *cf);

/* Initialize a zms backend. */
int settings_zms_backend_init(struct settings_zms *cf);

#ifdef __cplusplus
}
#endif

#endif /* __SETTINGS_ZMS_H_ */
