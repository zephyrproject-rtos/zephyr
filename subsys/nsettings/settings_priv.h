/*
 * Cipyright (c) 2018 Laczen
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SETTINGS_PRIV_H_
#define __SETTINGS_PRIV_H_

#include <sys/types.h>
#include <errno.h>
#include <nsettings/settings.h>

#ifdef __cplusplus
extern "C" {
#endif

struct settings_handler *settings_parse_and_lookup(char *name, int *name_argc,
						   char *name_argv[]);

/*
 * API for config storage.
 */

struct settings_store_itf {
	int (*csi_load)(struct settings_store *cs);
	int (*csi_save_start)(struct settings_store *cs);
	int (*csi_save)(struct settings_store *cs, const char *name,
			const char *value, size_t val_len);
	int (*csi_save_end)(struct settings_store *cs);
};

void settings_src_register(struct settings_store *cs);
void settings_dst_register(struct settings_store *cs);

extern sys_slist_t settings_load_srcs;
extern sys_slist_t settings_handlers;
extern struct settings_store *settings_save_dst;

#ifdef __cplusplus
}
#endif

#endif /* __SETTINGS_PRIV_H_ */
