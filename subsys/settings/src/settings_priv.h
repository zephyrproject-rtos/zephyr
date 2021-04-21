/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SETTINGS_PRIV_H_
#define __SETTINGS_PRIV_H_

#include <sys/types.h>
#include <sys/slist.h>
#include <errno.h>
#include <settings/settings.h>

#ifdef __cplusplus
extern "C" {
#endif

int settings_cli_register(void);
int settings_nmgr_register(void);

struct mgmt_cbuf;
int settings_cbor_line(struct mgmt_cbuf *cb, char *name, int nlen, char *value,
		       int vlen);

void settings_line_io_init(int (*read_cb)(void *ctx, off_t off, char *buf,
					  size_t *len),
			   int (*write_cb)(void *ctx, off_t off,
					   char const *buf, size_t len),
			   size_t (*get_len_cb)(void *ctx),
			   uint8_t io_rwbs);

int settings_line_write(const char *name, const char *value, size_t val_len,
			off_t w_loc, void *cb_arg);

/* Get len of record without alignment to write-block-size */
int settings_line_len_calc(const char *name, size_t val_len);

int settings_line_dup_check_cb(const char *name, void *val_read_cb_ctx,
				off_t off, void *cb_arg);

int settings_line_load_cb(const char *name, void *val_read_cb_ctx,
			   off_t off, void *cb_arg);

typedef int (*line_load_cb)(const char *name, void *val_read_cb_ctx,
			     off_t off, void *cb_arg);

struct settings_line_read_value_cb_ctx {
	void *read_cb_ctx;
	off_t off;
};

struct settings_line_dup_check_arg {
	const char *name;
	const char *val;
	size_t val_len;
	int is_dup;
};

#ifdef CONFIG_SETTINGS_ENCODE_LEN
/* in storage line contex */
struct line_entry_ctx {
	void *stor_ctx;
	off_t seek; /* offset of id-value pair within the file */
	size_t len; /* len of line without len value */
};

int settings_next_line_ctx(struct line_entry_ctx *entry_ctx);
#endif

/**
 * Read RAW settings line entry data from the storage.
 *
 * @param seek offset form the line beginning.
 * @param[out] out buffer for name
 * @param[in] len_req size of <p>out</p> buffer
 * @param[out] len_read length of read name
 * @param[in] cb_arg settings line storage context expected by the
 * <p>read_cb</p> implementatio
 *
 * @retval 0 on success,
 * -ERCODE on storage errors
 */
int settings_line_raw_read(off_t seek, char *out, size_t len_req,
			   size_t *len_read, void *cb_arg);

/*
 * @param val_off offset of the value-string.
 * @param off from val_off (so within the value-string)
 */
int settings_line_val_read(off_t val_off, off_t off, char *out, size_t len_req,
			   size_t *len_read, void *cb_arg);

/**
 * Read the settings line entry name from the storage.
 *
 * @param[out] out buffer for name
 * @param[in] len_req size of <p>out</p> buffer
 * @param[out] len_read length of read name
 * @param[in] cb_arg settings line storage context expected by the
 * <p>read_cb</p> implementation
 *
 * @retval 0 on read proper name,
 * 1 on when read improper name,
 * -ERCODE on storage errors
 */
int settings_line_name_read(char *out, size_t len_req, size_t *len_read,
			    void *cb_arg);

size_t settings_line_val_get_len(off_t val_off, void *read_cb_ctx);

int settings_line_entry_copy(void *dst_ctx, off_t dst_off, void *src_ctx,
			off_t src_off, size_t len);

void settings_line_io_init(int (*read_cb)(void *ctx, off_t off, char *buf,
					  size_t *len),
			  int (*write_cb)(void *ctx, off_t off, char const *buf,
					  size_t len),
			  size_t (*get_len_cb)(void *ctx),
			  uint8_t io_rwbs);


extern sys_slist_t settings_load_srcs;
extern sys_slist_t settings_handlers;
extern struct settings_store *settings_save_dst;

#ifdef __cplusplus
}
#endif

#endif /* __SETTINGS_PRIV_H_ */
