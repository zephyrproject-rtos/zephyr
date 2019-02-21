/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <string.h>
#include <misc/__assert.h>

#include "settings/settings.h"
#include "settings_priv.h"

#ifdef CONFIG_SETTINGS_USE_BASE64
#include "base64.h"
#endif

#include "misc/printk.h"

int settings_line_parse(char *buf, char **namep, char **valp)
{
	char *cp;
	enum {
		FIND_NAME,
		FIND_NAME_END,
		FIND_VAL,
		FIND_VAL_END
		} state = FIND_NAME;

	*valp = NULL;
	for (cp = buf; *cp != '\0'; cp++) {
		switch (state) {
		case FIND_NAME:
			if (!isspace((unsigned char)*cp)) {
				*namep = cp;
				state = FIND_NAME_END;
			}
			break;
		case FIND_NAME_END:
			if (*cp == '=') {
				*cp = '\0';
				state = FIND_VAL;
			} else if (isspace((unsigned char)*cp)) {
				*cp = '\0';
			}
			break;
		case FIND_VAL:
			if (!isspace((unsigned char)*cp)) {
				*valp = cp;
				state = FIND_VAL_END;
			}
			break;
		case FIND_VAL_END:
			if (isspace((unsigned char)*cp)) {
				*cp = '\0';
			}
			break;
		}
	}

	if (state == FIND_VAL_END || state == FIND_VAL) {
		return 0;
	} else {
		return -1;
	}
}

int settings_line_make(char *dst, int dlen, const char *name, const char *value)
{
	int nlen;
	int vlen;
	int off;

	nlen = strlen(name);

	if (value) {
		vlen = strlen(value);
	} else {
		vlen = 0;
	}

	if (nlen + vlen + 2 > dlen) {
		return -1;
	}

	memcpy(dst, name, nlen);
	off = nlen;
	dst[off++] = '=';

	memcpy(dst + off, value, vlen);
	off += vlen;
	dst[off] = '\0';

	return off;
}

struct settings_io_cb_s {
	int (*read_cb)(void *ctx, off_t off, char *buf, size_t *len);
	int (*write_cb)(void *ctx, off_t off, char const *buf, size_t len);
	size_t (*get_len_cb)(void *ctx);
	u8_t rwbs;
} settings_io_cb;

#define MAX_ENC_BLOCK_SIZE 4

int settings_line_write(const char *name, const char *value, size_t val_len,
			off_t w_loc, void *cb_arg)
{
	size_t w_size, rem, add;

#ifdef CONFIG_SETTINGS_USE_BASE64
	/* minimal buffer for encoding base64 + EOL*/
	char enc_buf[MAX_ENC_BLOCK_SIZE + 1];

	char *p_enc = enc_buf;
	size_t enc_len = 0;
#endif

	bool done;
	char w_buf[16]; /* write buff, must be aligned either to minimal */
			/* base64 encoding size and write-block-size */
	int rc;
	u8_t wbs = settings_io_cb.rwbs;
#ifdef CONFIG_SETTINGS_ENCODE_LEN
	u16_t len_field;
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
#ifdef CONFIG_SETTINGS_USE_BASE64
			if (enc_len) {
				add = MIN(enc_len, sizeof(w_buf) - w_size);
				memcpy(&w_buf[w_size], p_enc, add);
				enc_len -= add;
				w_size += add;
				p_enc += add;
			} else {
#endif
				if (rem) {
#ifdef CONFIG_SETTINGS_USE_BASE64
					add = MIN(rem, MAX_ENC_BLOCK_SIZE/4*3);
					rc = base64_encode(enc_buf, sizeof(enc_buf), &enc_len, value, add);
					if (rc) {
						return -EINVAL;
					}
					value += add;
					rem -= add;
					p_enc = enc_buf;
#else
					add = MIN(rem, sizeof(w_buf) - w_size);
					memcpy(&w_buf[w_size], value, add);
					value += add;
					rem -= add;
					w_size += add;
#endif
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
#ifdef CONFIG_SETTINGS_USE_BASE64
			}
#endif
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
	u16_t readout;
	int rc;

	entry_ctx->seek += entry_ctx->len; /* to begin of nex line */

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
	int len;

#ifdef CONFIG_SETTINGS_USE_BASE64
	/* <enc(value)> */
	len = val_len/3*4 + ((val_len%3) ? 4 : 0);
#else
	/* <evalue> */
	len = val_len;
#endif
	/* <name>=<enc(value)> */
	len += strlen(name) + 1;

	return len;
}


/**
 * Read RAW settings line entry data until a char from the storage.
 *
 * @param seek offset form the line beginning.
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
	char temp_buf[16]; /* buffer for fit read-block-size requirements */
	size_t exp_size, read_size;
	u8_t rbs = settings_io_cb.rwbs;
	off_t off;
	int rc;

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

#ifdef CONFIG_SETTINGS_USE_BASE64
/* off from value begin */
int settings_line_val_read(off_t val_off, off_t off, char *out, size_t len_req,
			   size_t *len_read, void *cb_arg)
{
	char enc_buf[16 + 1];
	char dec_buf[sizeof(enc_buf)/4 * 3 + 1];
	size_t rem_size, read_size, exp_size, clen, olen;
	off_t seek_begin, off_begin;
	int rc;


	rem_size = len_req;

	while (rem_size) {
		seek_begin = off / 3 * 4;
		off_begin = seek_begin / 4 * 3;

		read_size = rem_size / 3 * 4;
		read_size += (rem_size % 3 != 0 || off_begin != off) ? 4 : 0;

		read_size = MIN(read_size, sizeof(enc_buf) - 1);
		exp_size = read_size;

		rc = settings_line_raw_read(val_off + seek_begin, enc_buf,
					    read_size, &read_size, cb_arg);
		if (rc) {
			return rc;
		}

		enc_buf[read_size] = 0; /* breaking guaranted */
		read_size = strlen(enc_buf);

		if (read_size == 0 || read_size % 4) {
			/* unexpected use case - a NULL value or an encoding */
			/* problem */
			return -EINVAL;
		}

		rc = base64_decode(dec_buf, sizeof(dec_buf), &olen, enc_buf,
				   read_size);
		dec_buf[olen] = 0;

		clen = MIN(olen + off_begin - off, rem_size);

		memcpy(out, &dec_buf[off - off_begin], clen);
		rem_size -= clen;

		if (exp_size > read_size || olen < read_size/4*3) {
			break;
		}

		out += clen;
		off += clen;
	}

	*len_read = len_req - rem_size;

	return 0;
}
#else

/* off from value begin */
int settings_line_val_read(off_t val_off, off_t off, char *out, size_t len_req,
			   size_t *len_read, void *cb_arg)
{
	return settings_line_raw_read(val_off + off, out, len_req, len_read,
				      cb_arg);
}
#endif

size_t settings_line_val_get_len(off_t val_off, void *read_cb_ctx)
{
	size_t len;

	len = settings_io_cb.get_len_cb(read_cb_ctx);
#ifdef CONFIG_SETTINGS_USE_BASE64
	u8_t raw[2];
	int rc;
	size_t len_base64 = len - val_off;

	/* don't care about lack of alignmet to 4 B */
	/* entire value redout call will return error anyway */
	if (len_base64 >= 4) {
		/* read last 2 B of base64 */
		rc = settings_line_raw_read(len - 2, raw, 2, &len, read_cb_ctx);
		if (rc || len != 2) {
			/* very unexpected error */
			__ASSERT(rc == 0, "Failed to read the storage.\n");
			return 0;
		}

		len = (len_base64 / 4) * 3;

		/* '=' is the padding of Base64 */
		if (raw[0] == '=') {
			len -= 2;
		} else if (raw[1] == '=') {
			len--;
		}

		return len;
	} else {
		return 0;
	}
#else
	return len - val_off;
#endif
}

/**
 * @param line_loc offset of the settings line, expect that it is aligned to rbs physically.
 * @param seek offset form the line begining.
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


int settings_entry_copy(void *dst_ctx, off_t dst_off, void *src_ctx,
			off_t src_off, size_t len)
{
	int rc;
	char buf[16];
	size_t chunk_size;

	while (len) {
		chunk_size = MIN(len, sizeof(buf));

		rc = settings_io_cb.read_cb(src_ctx, src_off, buf, &chunk_size);
		if (rc) {
			break;
		}

		rc = settings_io_cb.write_cb(dst_ctx, dst_off, buf, chunk_size);
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
			  u8_t io_rwbs)
{
	settings_io_cb.read_cb = read_cb;
	settings_io_cb.write_cb = write_cb;
	settings_io_cb.get_len_cb = get_len_cb;
	settings_io_cb.rwbs = io_rwbs;
}
