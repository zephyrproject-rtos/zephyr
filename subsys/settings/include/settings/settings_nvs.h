/*
 * Copyright (c) 2019 Laczen
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SETTINGS_NVS_H_
#define __SETTINGS_NVS_H_

#include <zephyr/fs/nvs.h>
#include <zephyr/settings/settings.h>

#ifdef __cplusplus
extern "C" {
#endif

/* In the NVS backend, each setting is stored in two NVS entries:
 *	1. setting's name
 *	2. setting's value
 *
 * The NVS entry ID for the setting's value is determined implicitly based on
 * the ID of the NVS entry for the setting's name, once that is found. The
 * difference between name and value ID is constant and equal to
 * NVS_NAME_ID_OFFSET.
 *
 * Setting's name entries start from NVS_NAMECNT_ID + 1. The entry at
 * NVS_NAMECNT_ID is used to store the largest name ID in use.
 *
 * Deleted records will not be found, only the last record will be
 * read.
 */
#define NVS_NAMECNT_ID 0x8000
#define NVS_NAME_ID_OFFSET 0x4000

struct settings_nvs {
	struct settings_store cf_store;
	struct nvs_fs cf_nvs;
	uint16_t last_name_id;
	const char *flash_dev_name;
};

/* register nvs to be a source of settings */
int settings_nvs_src(struct settings_nvs *cf);

/* register nvs to be the destination of settings */
int settings_nvs_dst(struct settings_nvs *cf);

/* Initialize a nvs backend. */
int settings_nvs_backend_init(struct settings_nvs *cf);


#ifdef __cplusplus
}
#endif

#endif /* __SETTINGS_NVS_H_ */
