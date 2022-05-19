/*
 * Copyright (c) 2022 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/factory_data/factory_data.h>
#include <zephyr/kernel.h>
#include <zephyr/storage/flash_map.h>

/* Copied from bootutil_public.h */
#ifndef ALIGN_UP
#define ALIGN_UP(num, align) (((num) + ((align)-1)) & ~((align)-1))
#endif

#define FACTORY_DATA_LV_HEADER_SIZE 8

struct factory_data_lv {
	uint32_t f_magic;
	/**< Magic value, should not be 0xFFFFFFFF.
	 * It is xored with inversion of f_erase_value and placed in
	 * the beginning of FCB flash sector. FCB uses this when determining
	 * whether sector contains valid data or not.
	 * Giving it value of 0xFFFFFFFF means leaving bytes of the filed
	 * in "erased" state.
	 */
	uint8_t f_version; /**<  Current version number of the data */

	const struct flash_area *fap;
};

#define ALIGNMENT_SAFETY_MARGIN 4 /* XXX: enough? */

struct factory_data_lv_entry {
	uint16_t length; /* Value length in bytes. 0xFFFF is reserved to mark unused flash */
	uint8_t value[FACTORY_DATA_TOTAL_LEN_MAX + ALIGNMENT_SAFETY_MARGIN];
};

struct lv_area_header {
	uint32_t fd_magic;
	uint8_t fd_ver;
	uint8_t _pad1;
	uint8_t _pad2;
	uint8_t _pad3;
};
BUILD_ASSERT(sizeof(struct lv_area_header) == FACTORY_DATA_LV_HEADER_SIZE,
	     "Unexpected flash area header size");
/*
 * For simplicity reasons, assigned here instead of the init functions. Possible because for now,
 * both fields are static during runtime.
 */
static struct factory_data_lv factory_data_lv = {
	.f_magic = CONFIG_FACTORY_DATA_LV_MAGIC,
	.f_version = 1 /* the one and only version supported so far */,
};

K_MUTEX_DEFINE(factory_data_lock);

static bool factory_data_subsys_initialized;

/**
 * Initialize erased sector for use.
 *
 * @retval -EIO on error
 * @retval 0 success
 */
static int factory_data_step_lv_hdr_init(struct factory_data_lv *lv);

/**
 * Checks whether LV area contains data or not.
 *
 * @retval <0 in error.
 * @retval 0 if sector is unused;
 * @retval 1 if sector has data.
 */
static int factory_data_step_lv_hdr_read(struct factory_data_lv *lv);

/**
 * Step through a LV entry, one at a time.
 *
 * @param fa              Flash area to work on
 * @param offset[in, out] Offset in flash area current (in) and next entry (updated by the routine)
 * @param entry[out]      Return buffer
 *
 * @retval -ERRNO if error
 * @retval 0 if OK
 */
static int factory_data_step_lv(const struct flash_area *const fa, off_t *offset,
				struct factory_data_lv_entry *entry);

/**
 * Offset for next entry. Not adjusted for write sizes.
 *
 * @param fa
 * @retval >0 when free offset found
 * @retval -ENOSPC when no more space is available
 * @retval any other negative number represents an error
 */
static off_t factory_data_lv_first_free_offset(const struct flash_area *const fa);

/**
 * @brief Structure for transferring complete information about FCB entry
 * location within flash memory.
 */
struct lv_entry_ctx {
	struct factory_data_lv_entry entry; /**< LV entry info */
	const struct flash_area *fap;
	/**< Flash area where the entry is placed */
};

/**
 * TL walk callback function type.
 *
 * Type of function which is expected to be called while walking over TL
 * entries thanks to a @ref lv_walk call.
 *
 * If lv_walk_cb wants to stop the walk, it should return non-zero value.
 *
 * @param[in] entry LV entry
 * @param[in] offset Offset (of the entry) within the factory data partition
 * @param[in,out] arg callback context, transferred from @ref lv_walk.
 *
 * @return 0 continue walking, non-zero stop walking.
 */
typedef int (*lv_walk_cb)(const struct factory_data_lv_entry *entry, off_t offset, void *arg);

/**
 * Walk over all entries in the LV area
 *
 * @param[in] fa         TL instance structure.
 * @param[in] cb         pointer to the function which gets called for every
 *                       entry. If cb wants to stop the walk, it should return
 *                       non-zero value.
 * @param[in,out] ctx    callback context, transferred to the callback
 *                       implementation.
 *
 * @return 0 on success, negative on failure (or transferred form callback
 *         return-value), positive transferred from (last) callback return-value
 */
static int lv_walk(const struct flash_area *const fa, lv_walk_cb cb, void *cb_arg);

static int factory_data_step_lv_hdr_init(struct factory_data_lv *lv)
{
	int ret;
	const struct lv_area_header area_header = {
		.fd_magic = lv->f_magic,
		.fd_ver = lv->f_version,
		._pad1 = flash_area_erased_val(lv->fap),
		._pad2 = flash_area_erased_val(lv->fap),
		._pad3 = flash_area_erased_val(lv->fap),
	};

	ret = flash_area_write(lv->fap, 0, &area_header, sizeof(area_header));
	if (ret) {
		return -EIO;
	}

	return 0;
}

static int factory_data_step_lv_hdr_read(struct factory_data_lv *lv)
{
	struct lv_area_header area_header;
	int ret;

	ret = flash_area_read(lv->fap, 0, &area_header, sizeof(area_header));
	if (ret) {
		return -EIO;
	}

	if ((area_header.fd_magic & 0xFF) == flash_area_erased_val(lv->fap) &&
	    (area_header.fd_magic >> 8 & 0xFF) == flash_area_erased_val(lv->fap) &&
	    (area_header.fd_magic >> 16 & 0xFF) == flash_area_erased_val(lv->fap) &&
	    (area_header.fd_magic >> 24 & 0xFF) == flash_area_erased_val(lv->fap)) {
		return 0;
	}

	if (area_header.fd_ver != 1) {
		return -EILSEQ;
	}

	if (area_header.fd_magic != lv->f_magic) {
		return -ENOMSG;
	}

	return 1;
}

static int factory_data_step_lv(const struct flash_area *const fa, off_t *offset,
				struct factory_data_lv_entry *entry)
{
	int ret;
	size_t read_size = ALIGN_UP(2, flash_area_align(fa));
	off_t offset_aligned = ALIGN_UP(*offset, flash_area_align(fa));

	/* Read out the length */
	ret = flash_area_read(fa, offset_aligned, entry, read_size);
	if (ret) {
		return ret;
	}

	if ((entry->length & 0xFF) == flash_area_erased_val(fa) &&
	    (entry->length >> 8 & 0xFF) == flash_area_erased_val(fa)) {
		return -ENOENT;
	}

	/* Use header information to read data */
	read_size = ALIGN_UP(entry->length, flash_area_align(fa));

	if (read_size > sizeof(entry->value)) {
		return -ENOMEM;
	}

	ret = flash_area_read(fa, offset_aligned + sizeof(entry->length), &entry->value, read_size);
	if (ret) {
		return ret;
	}

	*offset = offset_aligned + sizeof(entry->length) + entry->length;

	return 0;
}

static int factory_data_lv_first_free_offset_cb(const struct factory_data_lv_entry *const entry,
						const off_t offset, void *arg)
{
	off_t *first_free_offset = (off_t *)arg;

	*first_free_offset = offset;

	return 0;
}

static off_t factory_data_lv_first_free_offset(const struct flash_area *const fa)
{
	int ret;
	off_t first_free_offset = FACTORY_DATA_LV_HEADER_SIZE;

	ret = lv_walk(fa, factory_data_lv_first_free_offset_cb, &first_free_offset);
	if (ret) {
		return ret;
	}

	return first_free_offset;
}

static int lv_walk(const struct flash_area *const fa, lv_walk_cb cb, void *ctx)
{
	int ret;
	off_t offset = FACTORY_DATA_LV_HEADER_SIZE;
	struct factory_data_lv_entry entry; /* XXX: Should this be caller provided? */

	/*
	 * Walk until no more entries left or callback stops iteration, both of which are a success
	 * cases.
	 */
	do {
		ret = factory_data_step_lv(fa, &offset, &entry);
		/* Exit when no more data is available */
		if (ret == -ENOENT) {
			return 0;
		}
		/* Bail out on error */
		if (ret != 0) {
			return ret;
		}
		ret = cb(&entry, offset, ctx);
	} while (ret == 0);

	return ret;
}

int factory_data_init_lv(void)
{
	int ret;

	/* Initialize area */
	ret = flash_area_open(FACTORY_DATA_FLASH_PARTITION, &factory_data_lv.fap);
	if (ret) {
		return ret;
	}

	ret = factory_data_step_lv_hdr_read(&factory_data_lv);
	if (ret < 0) {
		/* Bail out on error */
		return ret;
	}
	if (ret == 0) {
		/* Need to initialize area with LV header */
		ret = factory_data_step_lv_hdr_init(&factory_data_lv);
		if (ret) {
			return ret;
		}
	} else {
		/* Already previously initialized */
		__ASSERT(ret == 1, "Expecting LV flash area to contain data");
	}

	return ret;
}

int factory_data_init(void)
{
	int ret;

	k_mutex_lock(&factory_data_lock, K_FOREVER);

	if (factory_data_subsys_initialized) {
		return 0;
	}

	ret = factory_data_init_lv();
	if (ret) {
		goto exit_unlock;
	}

	factory_data_subsys_initialized = true;

exit_unlock:
	k_mutex_unlock(&factory_data_lock);
	return ret;
}

/*XXX: Almost the same as in factory_data_fcb.c */
static int factory_data_value_exists_callback(const struct factory_data_lv_entry *entry,
					      off_t offset, void *arg)
{
	const char *name = arg;
	size_t name_len;

	name_len = strnlen((const char *)entry->value, CONFIG_FACTORY_DATA_NAME_LEN_MAX);
	if (name_len == 0) {
		/*  */
		__ASSERT(false, "Zero length names are not allowed");
		return -EIO;
	}

	/* positive return value on match, will stop lv_walk */
	return memcmp(name, entry->value, name_len) == 0;
}

static bool factory_data_value_exists(const char *const name)
{
	volatile int i =
		lv_walk(factory_data_lv.fap, factory_data_value_exists_callback, (char *)name);
	return i;
}

#if CONFIG_FACTORY_DATA_WRITE
/* XXX: "unpacking" is same as in factory_data_fcb.c */
int factory_data_save_one(const char *const name, const void *const value, const size_t val_len)
{
	int ret;
	struct factory_data_lv_entry entry = {0}; /* Should this be provided by the caller? */
	off_t free_offset_aligned;
	size_t name_len;
	size_t actual_write_length;

	k_mutex_lock(&factory_data_lock, K_FOREVER);

	if (!factory_data_subsys_initialized) {
		return -ECANCELED;
	}

	/* Doing those checks before locking and init check? */
	name_len = strlen(name);
	if (name_len == 0) {
		return -EINVAL;
	}

	if (name_len > CONFIG_FACTORY_DATA_NAME_LEN_MAX) {
		return -ENAMETOOLONG;
	}

	if (val_len > CONFIG_FACTORY_DATA_VALUE_LEN_MAX) {
		return -EFBIG;
	}

	if (factory_data_value_exists(name)) {
		ret = -EEXIST;
		goto exit_unlock;
	}

	entry.length = name_len + 1 + val_len;
	memcpy(entry.value, name, name_len);
	entry.value[name_len] = '\0';
	memcpy(entry.value + name_len + 1, value, val_len);

	free_offset_aligned = ALIGN_UP(factory_data_lv_first_free_offset(factory_data_lv.fap),
				       flash_area_align(factory_data_lv.fap));
	actual_write_length = ALIGN_UP(sizeof(entry.length) + entry.length,
				       flash_area_align(factory_data_lv.fap));
	if (free_offset_aligned < 0) {
		return -EIO;
	}
	ret = flash_area_write(factory_data_lv.fap, free_offset_aligned, &entry,
			       actual_write_length);
	if (ret) {
		ret = -EIO;
		goto exit_unlock;
	}

exit_unlock:
	k_mutex_unlock(&factory_data_lock);

	return ret;
}

int factory_data_erase(void)
{
	int ret;

	ret = flash_area_open(FACTORY_DATA_FLASH_PARTITION, &factory_data_lv.fap);
	if (ret) {
		return ret;
	}

	ret = flash_area_erase(factory_data_lv.fap, 0, factory_data_lv.fap->fa_size);
	if (ret) {
		return ret;
	}

	/* Was already successfully initialized - re-run the relevant initialization code */
	if (factory_data_subsys_initialized) {
		return factory_data_init_lv();
	}

	return 0;
}
#endif /* CONFIG_FACTORY_DATA_WRITE */

/* XXX: Same as in factory_data_fcb.c */
struct factory_data_load_callback_ctx {
	factory_data_load_direct_cb user_cb;
	const void *user_ctx;
};

/* XXX: "unpacking" is same as in factory_data_fcb.c */
static int factory_data_load_callback(const struct factory_data_lv_entry *const entry, off_t offset,
				      void *const arg)
{
	struct factory_data_load_callback_ctx *ctx = arg;
	size_t name_len;
	size_t value_len;

	name_len = strnlen((const char *)entry->value, CONFIG_FACTORY_DATA_NAME_LEN_MAX);
	if (name_len == 0) {
		/*  */
		__ASSERT(false, "Zero length names are not allowed");
		return -EIO;
	}

	value_len = entry->length - 1 /* terminating C string null byte */ - name_len;
	if (entry->value[name_len] != '\0') {
		return -EIO;
	}

	return ctx->user_cb((const char *)entry->value, &entry->value[name_len + 1], value_len,
			    ctx->user_ctx);
}

/* XXX: Very similar to factory_data_load in factory_data_fcb.c */
int factory_data_load(const factory_data_load_direct_cb cb, const void *const param)
{
	struct factory_data_load_callback_ctx ctx = {
		.user_cb = cb,
		.user_ctx = param,
	};

	if (!factory_data_subsys_initialized) {
		return -ECANCELED;
	}

	return lv_walk(factory_data_lv.fap, factory_data_load_callback, &ctx);
}

/* XXX: Same as in factory_data_fcb.c! */
struct factory_data_load_one_callback_context {
	const char *name;	/**< [int] */
	uint8_t *const out_buf; /**< [out] */
	size_t out_buf_size;	/**< [in,out] */
	bool found;		/**< [out] */
};

/* XXX: Same as in factory_data_fcb.c! */
static int factory_data_load_one_callback(const char *name, const uint8_t *value, size_t len,
					  const void *param)
{
	struct factory_data_load_one_callback_context *ctx =
		(struct factory_data_load_one_callback_context *)param;

	if (strcmp(name, ctx->name) != 0) {
		return 0;
	}

	ctx->out_buf_size = MIN(len, ctx->out_buf_size);
	ctx->found = true;
	memcpy(ctx->out_buf, value, ctx->out_buf_size);

	return ctx->out_buf_size; /* number of read bytes - this stops the walking */
}

/* XXX: Same as in factory_data_fcb.c! */
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

	return ctx.out_buf_size; /* number of bytes read */
}
