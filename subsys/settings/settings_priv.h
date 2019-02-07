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
#include <settings/settings.h>

#ifdef __cplusplus
extern "C" {
#endif

extern sys_slist_t settings_load_srcs;
extern sys_slist_t settings_handlers;
extern struct settings_store *settings_save_dst;

#ifdef __cplusplus
}
#endif

#endif /* __SETTINGS_PRIV_H_ */
