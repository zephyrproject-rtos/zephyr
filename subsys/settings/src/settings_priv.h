/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SETTINGS_PRIV_H_
#define __SETTINGS_PRIV_H_

#ifdef __cplusplus
extern "C" {
#endif

int settings_cli_register(void);
int settings_nmgr_register(void);

struct mgmt_cbuf;
int settings_cbor_line(struct mgmt_cbuf *cb, char *name, int nlen, char *value,
		       int vlen);

int settings_line_parse(char *buf, char **namep, char **valp);
int settings_line_make(char *dst, int dlen, const char *name, const char *val);
int settings_line_make2(char *dst, int dlen, const char *name,
			const char *value);

/*
 * API for config storage.
 */
typedef void (*load_cb)(char *name, char *val, void *cb_arg);
struct settings_store_itf {
	int (*csi_load)(struct settings_store *cs, load_cb cb, void *cb_arg);
	int (*csi_save_start)(struct settings_store *cs);
	int (*csi_save)(struct settings_store *cs, const char *name,
			const char *value);
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
