/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <string.h>

#include <zephyr/settings/settings.h>
#include "settings_priv.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(settings, CONFIG_SETTINGS_LOG_LEVEL);

struct settings_io_cb_s {
	int (*read_cb)(void *ctx, off_t off, char *buf, size_t *len);
	int (*write_cb)(void *ctx, off_t off, char const *buf, size_t len);
	size_t (*get_len_cb)(void *ctx);
	uint8_t rwbs;
} static settings_io_cb;

int settings_line_write(const char *name, const char *value, size_t val_len,
			off_t w_loc, void *cb_arg)
{
	size_t w_size, rem, add;

	bool done;
	char w_buf[32]; /* write buff, must be aligned either to minimal */
			/* base64 encoding size and write-block-size */
	int rc;
	uint8_t wbs = settings_io_cb.rwbs;
#ifdef CONFIG_SETTINGS_ENCODE_LEN
	uint16_t len_field;
#endif

	rem = strlen(name);

#ifdef CONFIG_SETTINGS_ENCODE_LEN
	len_field = settings_line_len_calc(name, val_len);
	memcpy(w_buf, &len_field, sizeof(len_field));
	w_size = 0;


	add = sizeof(len_field) % wbs;
	if (add) {
		w_size = wbs - add;
		if (rem < w_size) {
			w_size = rem;
		}

		memcpy(w_buf + sizeof(len_field), name, w_size);
		name += w_size;
		rem -= w_size;
	}

	w_size += sizeof(len_field);
	if (w_size % wbs == 0) {
		rc = settings_io_cb.write_cb(cb_arg, w_loc, w_buf, w_size);
		if (rc) {
			return -EIO;
		}
	}
	/* The Alternative to condition above mean that `rem == 0` as `name` */
	/* must have been consumed					     */
#endif
	w_size = rem - rem % wbs;
	rem %= wbs;

	rc = settings_io_cb.write_cb(cb_arg, w_loc, name, w_size);
	w_loc += w_size;
	name += w_size;
	w_size = rem;

	if (rem) {
		memcpy(w_buf, name, rem);
	}

	w_buf[rem] = '=';
	w_size++;

	rem = val_len;
	done = false;

	while (1) {
		while (w_size < sizeof(w_buf)) {
			if (rem) {
				add = MIN(rem, sizeof(w_buf) - w_size);
				memcpy(&w_buf[w_size], value, add);
				value += add;
				rem -= add;
				w_size += add;
			} else {
				add = (w_size) % wbs;
				if (add) {
					add = wbs - add;
					memset(&w_buf[w_size], '\0',
					       add);
					w_size += add;
				}
				done = true;
				break;
			}
		}

		rc = settings_io_cb.write_cb(cb_arg, w_loc, w_buf, w_size);
		if (rc) {
			return -EIO;
		}

		if (done) {
			break;
		}
		w_loc += w_size;
		w_size = 0;
	}

	return 0;
}

#ifdef CONFIG_SETTINGS_ENCODE_LEN
int settings_next_line_ctx(struct line_entry_ctx *entry_ctx)
{
	size_t len_read;
	uint16_t readout;
	int rc;

	entry_ctx->seek += entry_ctx->len; /* to begin of next line */

	entry_ctx->len = 0; /* ask read handler to ignore len */

	rc = settings_line_raw_read(0, (char *)&readout, sizeof(readout),
			   &len_read, entry_ctx);
	if (rc == 0) {
		if (len_read != sizeof(readout)) {
			if (len_read != 0) {
				rc = -ESPIPE;
			}
		} else {
			entry_ctx->seek += sizeof(readout);
			entry_ctx->len = readout;
		}
	}

	return rc;
}
#endif

int settings_line_len_calc(const char *name, size_t val_len)
{
	/* <name>=<value> */
	return strlen(name) + 1 + val_len;
}


/**
 * Read RAW settings line entry data until a char from the storage.
 *
 * @param seek offset from the line beginning.
 * @param[out] out buffer for name
 * @param[in] len_req size of <p>out</p> buffer
 * @param[out] len_read length of read name
 * @param[in] until_char pointer on char value until which all line data will
 * be read. If Null entire data will be read.
 * @param[in] cb_arg settings line storage context expected by the
 * <p>read_cb</p> implementation
 *
 * @retval 0 on success,
 * -ERCODE on storage errors
 */
static int settings_line_raw_read_until(off_t seek, char *out, size_t len_req,
				 size_t *len_read, char const *until_char,
				 void *cb_arg)
{
	size_t rem_size, len;
	char temp_buf[32]; /* buffer for fit read-block-size requirements */
	size_t exp_size, read_size;
	uint8_t rbs = settings_io_cb.rwbs;
	off_t off;
	int rc = -EINVAL;

	if (len_req == 0) {
		return -EINVAL;
	}

	rem_size = len_req;

	while (rem_size) {
		off = seek / rbs * rbs;

		read_size = sizeof(temp_buf);
		exp_size = read_size;

		rc = settings_io_cb.read_cb(cb_arg, off, temp_buf, &read_size);
		if (rc) {
			return -EIO;
		}

		off = seek - off;
		len = read_size - off;
		len = MIN(rem_size, len);

		if (until_char != NULL) {
			char *pend;
			pend = memchr(&temp_buf[off], *until_char, len);
			if (pend != NULL) {
				len = pend - &temp_buf[off];
				rc = 1; /* will cause loop expiration */
			}
		}

		memcpy(out, &temp_buf[off], len);

		rem_size -= len;

		if (exp_size > read_size || rc) {
			break;
		}

		out += len;
		seek += len;
	}

	*len_read = len_req - rem_size;

	if (until_char != NULL) {
		return (rc) ? 0 : 1;
	}

	return 0;
}

int settings_line_raw_read(off_t seek, char *out, size_t len_req,
			   size_t *len_read, void *cb_arg)
{
	return settings_line_raw_read_until(seek, out, len_req, len_read,
					    NULL, cb_arg);
}

/* off from value begin */
int settings_line_val_read(off_t val_off, off_t off, char *out, size_t len_req,
			   size_t *len_read, void *cb_arg)
{
	return settings_line_raw_read(val_off + off, out, len_req, len_read,
				      cb_arg);
}

size_t settings_line_val_get_len(off_t val_off, void *read_cb_ctx)
{
	size_t len;

	len = settings_io_cb.get_len_cb(read_cb_ctx);

	return len - val_off;
}

/**
 * @param line_loc offset of the settings line, expect that it is aligned to rbs physically.
 * @param seek offset form the line beginning.
 * @retval 0 : read proper name
 * 1 : when read unproper name
 * -ERCODE for storage errors
 */
int settings_line_name_read(char *out, size_t len_req, size_t *len_read,
			    void *cb_arg)
{
	char const until_char = '=';

	return settings_line_raw_read_until(0, out, len_req, len_read,
					    &until_char, cb_arg);
}


int settings_line_entry_copy(void *dst_ctx, off_t dst_off, void *src_ctx,
			     off_t src_off, size_t len)
{
	int rc = -EINVAL;
	char buf[16];
	size_t chunk_size;

	while (len) {
		chunk_size = MIN(len, sizeof(buf));

		rc = settings_io_cb.read_cb(src_ctx, src_off, buf, &chunk_size);
		if (rc) {
			break;
		}

		size_t write_size = chunk_size;

		if (chunk_size % settings_io_cb.rwbs) {
			write_size += settings_io_cb.rwbs -
				      chunk_size % settings_io_cb.rwbs;
		}

		rc = settings_io_cb.write_cb(dst_ctx, dst_off, buf, write_size);

		if (rc) {
			break;
		}

		src_off += chunk_size;
		dst_off += chunk_size;
		len -= chunk_size;
	}

	return rc;
}

void settings_line_io_init(int (*read_cb)(void *ctx, off_t off, char *buf,
					  size_t *len),
			  int (*write_cb)(void *ctx, off_t off, char const *buf,
					  size_t len),
			  size_t (*get_len_cb)(void *ctx),
			  uint8_t io_rwbs)
{
	settings_io_cb.read_cb = read_cb;
	settings_io_cb.write_cb = write_cb;
	settings_io_cb.get_len_cb = get_len_cb;
	settings_io_cb.rwbs = io_rwbs;
}


/* val_off - offset of value-string within line entries */
static int settings_line_cmp(char const *val, size_t val_len,
			     void *val_read_cb_ctx, off_t val_off)
{
	size_t len_read, exp_len;
	size_t rem;
	char buf[16];
	int rc = -EINVAL;
	off_t off = 0;

	if (val_len == 0) {
		return -EINVAL;
	}

	for (rem = val_len; rem > 0; rem -= len_read) {
		len_read = exp_len = MIN(sizeof(buf), rem);
		rc = settings_line_val_read(val_off, off, buf, len_read,
					    &len_read, val_read_cb_ctx);
		if (rc) {
			break;
		}

		if (len_read != exp_len) {
			rc = 1;
			break;
		}

		rc = memcmp(val, buf, len_read);
		if (rc) {
			break;
		}
		val += len_read;
		off += len_read;
	}

	return rc;
}

int settings_line_dup_check_cb(const char *name, void *val_read_cb_ctx,
				off_t off, void *cb_arg)
{
	struct settings_line_dup_check_arg *cdca;
	size_t len_read;

	cdca = (struct settings_line_dup_check_arg *)cb_arg;
	if (strcmp(name, cdca->name)) {
		return 0;
	}

	len_read = settings_line_val_get_len(off, val_read_cb_ctx);
	if (len_read != cdca->val_len) {
		cdca->is_dup = 0;
	} else if (len_read == 0) {
		cdca->is_dup = 1;
	} else {
		if (!settings_line_cmp(cdca->val, cdca->val_len,
				       val_read_cb_ctx, off)) {
			cdca->is_dup = 1;
		} else {
			cdca->is_dup = 0;
		}
	}
	return 0;
}

static ssize_t settings_line_read_cb(void *cb_arg, void *data, size_t len)
{
	struct settings_line_read_value_cb_ctx *value_context = cb_arg;
	size_t len_read;
	int rc;

	rc = settings_line_val_read(value_context->off, 0, data, len,
				    &len_read,
				    value_context->read_cb_ctx);

	if (rc == 0) {
		return len_read;
	}

	return -1;
}

int settings_line_load_cb(const char *name, void *val_read_cb_ctx, off_t off,
			  void *cb_arg)
{
	size_t len;
	struct settings_line_read_value_cb_ctx value_ctx;
	struct settings_load_arg *arg = cb_arg;
	value_ctx.read_cb_ctx = val_read_cb_ctx;
	value_ctx.off = off;
	len = settings_line_val_get_len(off, val_read_cb_ctx);

	return settings_call_set_handler(name, len, settings_line_read_cb,
					 &value_ctx, arg);
}
