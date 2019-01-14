/*
 * Copyright (c) 2018 Laczen
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SETTINGS_NVS_H_
#define __SETTINGS_NVS_H_

#include <nvs/nvs.h>
#include "nsettings/settings.h"

#ifdef __cplusplus
extern "C" {
#endif

struct settings_nvs {
	struct settings_store cf_store;
	struct nvs_fs cf_nvs;
	u16_t last_name_id;
};

#define NVS_NAMECNT_ID 0x8000
#define NVS_NAME_ID_OFFSET 0x4000

extern int settings_nvs_src(struct settings_nvs *cf);
extern int settings_nvs_dst(struct settings_nvs *cf);

#ifdef __cplusplus
}
#endif

#endif /* __SETTINGS_NVS_H_ */
