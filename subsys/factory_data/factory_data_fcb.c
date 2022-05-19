/**
 * Copyright (c) 2022 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "factory_data_common.h"

#include <stdint.h>
#include <string.h>

#include <zephyr/factory_data/factory_data.h>
#include <zephyr/fs/fcb.h>
#include <zephyr/storage/flash_map.h>

static struct flash_sector factory_data_fcb_areas[CONFIG_FACTORY_DATA_FCB_NUM_AREAS + 1];

/*
 * For simplicity reasons, assigned here instead of the init functions. Possible because for now,
 * those fields are static during runtime.
 */
static struct fcb factory_data_fcb = {
	.f_magic = CONFIG_FACTORY_DATA_FCB_MAGIC,
	.f_version = 1,
	.f_sectors = factory_data_fcb_areas,
	.f_scratch_cnt = 0 /* Writing during manufacturing time only, no need for power-cut safe */
};

K_MUTEX_DEFINE(factory_data_lock);

static bool factory_data_subsys_initialized;

static int factory_data_init_fcb(void)
{
	int ret;
	struct flash_sector hw_flash_sector;
	uint32_t sector_cnt = ARRAY_SIZE(factory_data_fcb_areas) - 1;

	ret = flash_area_get_sectors(FACTORY_DATA_FLASH_PARTITION, &sector_cnt,
				     factory_data_fcb_areas);
	if (ret == -ENODEV) {
		return ret;
	} else if (ret != 0 && ret != -ENOMEM) {
		k_panic();
	}

	factory_data_fcb.f_sector_cnt = sector_cnt;

	ret = fcb_init(FACTORY_DATA_FLASH_PARTITION, &factory_data_fcb);
	if (ret) {
		return ret;
	}

	ret = flash_area_get_sectors(FACTORY_DATA_FLASH_PARTITION, &sector_cnt, &hw_flash_sector);
	if (ret == -ENODEV) {
		return ret;
	} else if (ret != 0 && ret != -ENOMEM) {
		k_panic();
	}

	if (hw_flash_sector.fs_size > UINT16_MAX) {
		return -EDOM;
	}

	return 0;
}

int factory_data_init(void)
{
	int ret;

	if (factory_data_subsys_initialized) {
		return 0;
	}

	ret = factory_data_init_fcb();
	if (ret) {
		return ret;
	}

	factory_data_subsys_initialized = true;

	return 0;
}

#if CONFIG_FACTORY_DATA_WRITE
static int factory_data_value_exists_callback(struct fcb_entry_ctx *const entry, void *const arg)
{
	const char *name = arg;
	uint8_t buf[CONFIG_FACTORY_DATA_NAME_LEN_MAX + 1];
	int ret;
	uint16_t max_read_len = MIN(entry->loc.fe_data_len, sizeof(buf));
	size_t name_len;

	ret = flash_area_read(entry->fap, FCB_ENTRY_FA_DATA_OFF(entry->loc), buf, max_read_len);
	if (ret) {
		return -EIO;
	}

	/* Missing strnlen, but input got written by us, guaranteed to have have a trailing '\0' */
	name_len = strlen((const char *)buf);
	if (name_len == 0) {
		/*  */
		__ASSERT(false, "Zero length names are not allowed");
		return -EIO;
	}

	if (name_len != strlen(name)) {
		return 0;
	}

	return memcmp(name, buf, name_len) == 0; /* positive return value on match */
}

static bool factory_data_value_exists(const char *const name)
{
	return fcb_walk(&factory_data_fcb, NULL, factory_data_value_exists_callback, (char *)name);
}

int factory_data_save_one(const char *const name, const void *const value, const size_t val_len)
{
	int ret;
	struct fcb_entry_ctx loc;
	char w_buf[16];
	size_t remaining;
	size_t w_size;
	size_t written;
	const uint8_t *byte_value = value;

	if (!factory_data_subsys_initialized) {
		return -ECANCELED;
	}

	if (sizeof(w_buf) < factory_data_fcb.f_align) {
		return -ENOMEM;
	}

	if (sizeof(w_buf) % factory_data_fcb.f_align) {
		return -ENOSYS;
	}

	remaining = strlen(name);
	if (remaining == 0) {
		return -EINVAL;
	}

	if (remaining > CONFIG_FACTORY_DATA_NAME_LEN_MAX) {
		return -ENAMETOOLONG;
	}

	if (val_len > CONFIG_FACTORY_DATA_VALUE_LEN_MAX) {
		return -EFBIG;
	}

	k_mutex_lock(&factory_data_lock, K_FOREVER);

	if (factory_data_value_exists(name)) {
		ret = -EEXIST;
		goto exit_unlock;
	}

	ret = fcb_append(&factory_data_fcb, factory_data_line_len_calc(name, val_len), &loc.loc);
	if (ret) {
		goto exit_unlock;
	}

	/* Write all of the name except for the remaining bytes which do not align nicely */
	w_size = remaining - remaining % factory_data_fcb.f_align;
	ret = flash_area_write(factory_data_fcb.fap, FCB_ENTRY_FA_DATA_OFF(loc.loc), name, w_size);
	if (ret) {
		goto exit_unlock;
	}
	written = w_size;

	/*
	 * Initialize write buffer with (maybe existing) last bytes of the name and the
	 * name-value-separating zero byte.
	 */
	remaining %= factory_data_fcb.f_align;
	memcpy(w_buf, name + w_size, remaining); /* no-op if remaining is zero */
	w_size = remaining;
	w_buf[w_size++] = '\0';

	remaining = val_len;
	/* Actually write (likely existing) last part of the name, separator byte and value bytes */
	do {
		size_t add = MIN(sizeof(w_buf) - w_size, remaining);

		memcpy(w_buf + w_size, byte_value, add);
		remaining -= add;
		w_size += add;
		byte_value += add;
		/* Pad writing buffer/size to match write alignment needs */
		if (w_size < sizeof(w_buf) && (w_size % factory_data_fcb.f_align) != 0) {
			add = factory_data_fcb.f_align - w_size % factory_data_fcb.f_align;
			memset(w_buf + w_size, factory_data_fcb.f_erase_value, add);
			w_size += add;
		}
		ret = flash_area_write(factory_data_fcb.fap,
				       FCB_ENTRY_FA_DATA_OFF(loc.loc) + written, w_buf, w_size);
		if (ret) {
			goto exit_unlock;
		}
		written += w_size;
		w_size = 0;
	} while (remaining);

	ret = fcb_append_finish(&factory_data_fcb, &loc.loc);
	if (ret != 0) {
		goto exit_unlock;
	}

exit_unlock:
	k_mutex_unlock(&factory_data_lock);

	return ret;
}

int factory_data_erase(void)
{
	int ret;
	const struct flash_area *fap;

	/* Already successfully initialized - can use the regular FCB facilities to erase */
	if (factory_data_subsys_initialized) {
		return fcb_clear(&factory_data_fcb);
	}

	/* Not yet initialized - maybe something is totally broken. Clear whole flash partition. */
	ret = flash_area_open(FACTORY_DATA_FLASH_PARTITION, &fap);
	if (ret) {
		return ret;
	}

	return flash_area_erase(fap, 0, fap->fa_size);
}
#endif /* CONFIG_FACTORY_DATA_WRITE */

struct factory_data_load_callback_ctx {
	factory_data_load_direct_cb user_cb;
	const void *user_ctx;
};

static int factory_data_load_callback(struct fcb_entry_ctx *const loc_ctx, void *const arg)
{
	struct factory_data_load_callback_ctx *ctx = arg;
	uint8_t buf[FACTORY_DATA_TOTAL_LEN_MAX];
	int ret;
	uint16_t fcb_entry_len = loc_ctx->loc.fe_data_len;
	size_t name_len;
	size_t value_len;

	if (fcb_entry_len > sizeof(buf)) {
		/*
		 * Could happen when max name and/or value length Kconfig values got lowered and
		 * existing, big data gets loaded
		 */
		return -ENOMEM;
	}

	ret = flash_area_read(loc_ctx->fap, FCB_ENTRY_FA_DATA_OFF(loc_ctx->loc), buf,
			      fcb_entry_len);
	if (ret) {
		return -EIO;
	}

	/* Missing strnlen, but input got written by us, guaranteed to have have a trailing '\0' */
	name_len = strlen((const char *)buf);
	if (name_len == 0) {
		/*  */
		__ASSERT(false, "Zero length names are not allowed");
		return -EIO;
	}

	value_len = fcb_entry_len - 1 /* terminating C string null byte */ - name_len;
	if (buf[name_len] != '\0') {
		return -EIO;
	}

	return ctx->user_cb((const char *)buf, &buf[name_len + 1], value_len, ctx->user_ctx);
}

int factory_data_load(const factory_data_load_direct_cb cb, const void *const param)
{
	struct factory_data_load_callback_ctx ctx = {
		.user_cb = cb,
		.user_ctx = param,
	};

	if (!factory_data_subsys_initialized) {
		return -ECANCELED;
	}

	return fcb_walk(&factory_data_fcb, NULL, factory_data_load_callback, &ctx);
}

struct factory_data_load_one_callback_context {
	const char *name;
	uint8_t *const out_buf;
	const size_t out_buf_size;
	bool found;
};

static int factory_data_load_one_callback(const char *name, const uint8_t *value, size_t len,
					  const void *param)
{
	struct factory_data_load_one_callback_context *ctx =
		(struct factory_data_load_one_callback_context *)param;
	const size_t read = MIN(len, ctx->out_buf_size);

	if (strcmp(name, ctx->name) != 0) {
		return 0;
	}

	ctx->found = true;
	memcpy(ctx->out_buf, value, read);

	return read;
}

ssize_t factory_data_load_one(const char *const name, uint8_t *const value, const size_t val_len)
{
	struct factory_data_load_one_callback_context ctx = {
		.name = name,
		.out_buf = value,
		.out_buf_size = val_len,
		.found = false,
	};
	int ret;

	if (!factory_data_subsys_initialized) {
		return -ECANCELED;
	}

	ret = factory_data_load(factory_data_load_one_callback, &ctx);
	if (ret < 0) {
		return ret;
	}
	if (ctx.found == false) {
		return -ENOENT;
	}

	return ret; /* number of bytes read */
}
