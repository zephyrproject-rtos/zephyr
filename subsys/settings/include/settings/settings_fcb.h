/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SETTINGS_FCB_H_
#define __SETTINGS_FCB_H_

#include <zephyr/fs/fcb.h>
#include <zephyr/settings/settings.h>

#ifdef __cplusplus
extern "C" {
#endif

struct settings_fcb {
	struct settings_store cf_store;
	struct fcb cf_fcb;
};

extern int settings_fcb_src(struct settings_fcb *cf);
extern int settings_fcb_dst(struct settings_fcb *cf);
void settings_mount_fcb_backend(struct settings_fcb *cf);

#ifdef __cplusplus
}
#endif

#endif /* __SETTINGS_FCB_H_ */
