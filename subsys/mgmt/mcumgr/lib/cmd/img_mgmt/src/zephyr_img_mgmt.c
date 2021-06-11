/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#define LOG_MODULE_NAME mcumgr_flash_mgmt
#define LOG_LEVEL CONFIG_IMG_MANAGER_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <assert.h>
#include <drivers/flash.h>
#include <storage/flash_map.h>
#include <zephyr.h>
#include <soc.h>
#include <init.h>
#include <dfu/mcuboot.h>
#include <dfu/flash_img.h>
#include <mgmt/mgmt.h>
#include <img_mgmt/img_mgmt_impl.h>
#include <img_mgmt/img_mgmt.h>
#include <img_mgmt/image.h>
#include "img_mgmt_priv.h"

/**
 * Determines if the specified area of flash is completely unwritten.
 */
static int
zephyr_img_mgmt_flash_check_empty(uint8_t fa_id, bool *out_empty)
{
	const struct flash_area *fa;
	uint32_t data[16];
	off_t addr;
	off_t end;
	int bytes_to_read;
	int rc;
	int i;
	uint8_t erased_val;
	uint32_t erased_val_32;

	rc = flash_area_open(fa_id, &fa);
	if (rc != 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	assert(fa->fa_size % 4 == 0);

	erased_val = flash_area_erased_val(fa);
	erased_val_32 = ERASED_VAL_32(erased_val);

	end = fa->fa_size;
	for (addr = 0; addr < end; addr += sizeof(data)) {
		if (end - addr < sizeof(data)) {
			bytes_to_read = end - addr;
		} else {
			bytes_to_read = sizeof(data);
		}

		rc = flash_area_read(fa, addr, data, bytes_to_read);
		if (rc != 0) {
			flash_area_close(fa);
			return MGMT_ERR_EUNKNOWN;
		}

		for (i = 0; i < bytes_to_read / 4; i++) {
			if (data[i] != erased_val_32) {
				*out_empty = false;
				flash_area_close(fa);
				return 0;
			}
		}
	}

	*out_empty = true;
	flash_area_close(fa);
	return 0;
}

/**
 * Get flash_area ID for a image slot number.
 */
static uint8_t
zephyr_img_mgmt_flash_area_id(int slot)
{
	uint8_t fa_id;

	switch (slot) {
	case 0:
		fa_id = FLASH_AREA_ID(image_0);
		break;

	case 1:
		fa_id = FLASH_AREA_ID(image_1);
		break;

	default:
		assert(0);
		fa_id = FLASH_AREA_ID(image_1);
		break;
	}

	return fa_id;
}

static int
img_mgmt_find_best_area_id(void)
{
	struct image_version ver;
	int best = -1;
	int i;
	int rc;

	for (i = 0; i < 2; i++) {
		rc = img_mgmt_read_info(i, &ver, NULL, NULL);
		if (rc < 0) {
			continue;
		}
		if (rc == 0) {
			/* Image in slot is ok. */
			if (img_mgmt_slot_in_use(i)) {
				/* Slot is in use; can't use this. */
				continue;
			} else {
				/*
				 * Not active slot, but image is ok. Use it if there are
				 * no better candidates.
				 */
				best = i;
			}
			continue;
		}
		best = i;
		break;
	}
	if (best >= 0) {
		best = zephyr_img_mgmt_flash_area_id(best);
	}
	return best;
}

/**
 * Compares two image version numbers in a semver-compatible way.
 *
 * @param a	The first version to compare.
 * @param b	The second version to compare.
 *
 * @return	-1 if a < b
 * @return	0 if a = b
 * @return	1 if a > b
 */
static int
img_mgmt_vercmp(const struct image_version *a, const struct image_version *b)
{
	if (a->iv_major < b->iv_major) {
		return -1;
	} else if (a->iv_major > b->iv_major) {
		return 1;
	}

	if (a->iv_minor < b->iv_minor) {
		return -1;
	} else if (a->iv_minor > b->iv_minor) {
		return 1;
	}

	if (a->iv_revision < b->iv_revision) {
		return -1;
	} else if (a->iv_revision > b->iv_revision) {
		return 1;
	}

	/* Note: For semver compatibility, don't compare the 32-bit build num. */

	return 0;
}

int
img_mgmt_impl_erase_slot(void)
{
	bool empty;
	int rc;

	/* Select non-active slot */
	const int best_id = img_mgmt_find_best_area_id();

	rc = zephyr_img_mgmt_flash_check_empty(best_id, &empty);
	if (rc != 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	if (!empty) {
		rc = boot_erase_img_bank(best_id);
		if (rc != 0) {
			return MGMT_ERR_EUNKNOWN;
		}
	}

	return 0;
}

int
img_mgmt_impl_write_pending(int slot, bool permanent)
{
	int rc;

	if (slot != 1) {
		return MGMT_ERR_EINVAL;
	}

	rc = boot_request_upgrade(permanent);
	if (rc != 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	return 0;
}

int
img_mgmt_impl_write_confirmed(void)
{
	int rc;

	rc = boot_write_img_confirmed();
	if (rc != 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	return 0;
}

int
img_mgmt_impl_read(int slot, unsigned int offset, void *dst,
				   unsigned int num_bytes)
{
	const struct flash_area *fa;
	int rc;

	rc = flash_area_open(zephyr_img_mgmt_flash_area_id(slot), &fa);
	if (rc != 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	rc = flash_area_read(fa, offset, dst, num_bytes);
	flash_area_close(fa);

	if (rc != 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	return 0;
}

int
img_mgmt_impl_write_image_data(unsigned int offset, const void *data,
							   unsigned int num_bytes, bool last)
{
	int rc;
#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
	static struct flash_img_context *ctx;
#else
	static struct flash_img_context ctx_data;
#define ctx (&ctx_data)
#endif

#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
	if (offset != 0 && ctx == NULL) {
		return MGMT_ERR_EUNKNOWN;
	}
#endif

	if (offset == 0) {
#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
		if (ctx == NULL) {
			ctx = k_malloc(sizeof(*ctx));

			if (ctx == NULL) {
				return MGMT_ERR_ENOMEM;
			}
		}
#endif
		rc = flash_img_init_id(ctx, img_mgmt_find_best_area_id());

		if (rc != 0) {
			return MGMT_ERR_EUNKNOWN;
		}
	}

	if (offset != ctx->stream.bytes_written + ctx->stream.buf_bytes) {
		return MGMT_ERR_EUNKNOWN;
	}

	/* Cast away const. */
	rc = flash_img_buffered_write(ctx, (void *)data, num_bytes, last);
	if (rc != 0) {
		return MGMT_ERR_EUNKNOWN;
	}

#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
	if (last) {
		k_free(ctx);
		ctx = NULL;
	}
#endif

	return 0;
}

int
img_mgmt_impl_erase_image_data(unsigned int off, unsigned int num_bytes)
{
	const struct flash_area *fa;
	int rc;

	if (off != 0) {
		rc = MGMT_ERR_EINVAL;
		goto end;
	}

	rc = flash_area_open(img_mgmt_find_best_area_id(), &fa);
	if (rc != 0) {
		LOG_ERR("Can't bind to the flash area (err %d)", rc);
		rc = MGMT_ERR_EUNKNOWN;
		goto end;
	}

	/* align requested erase size to the erase-block-size */
	const struct device *dev = flash_area_get_device(fa);
	struct flash_pages_info page;
	off_t page_offset = fa->fa_off + num_bytes - 1;

	rc = flash_get_page_info_by_offs(dev, page_offset, &page);
	if (rc != 0) {
		LOG_ERR("bad offset (0x%lx)", (long)page_offset);
		rc = MGMT_ERR_EUNKNOWN;
		goto end_fa;
	}

	size_t erase_size = page.start_offset + page.size - fa->fa_off;

	rc = flash_area_erase(fa, 0, erase_size);

	if (rc != 0) {
		LOG_ERR("image slot erase of 0x%zx bytes failed (err %d)", erase_size,
				rc);
		rc = MGMT_ERR_EUNKNOWN;
		goto end_fa;
	}

	LOG_INF("Erased 0x%zx bytes of image slot", erase_size);

	/* erase the image trailer area if it was not erased */
	off = BOOT_TRAILER_IMG_STATUS_OFFS(fa);
	if (off >= erase_size) {
		rc = flash_get_page_info_by_offs(dev, fa->fa_off + off, &page);

		off = page.start_offset - fa->fa_off;
		erase_size = fa->fa_size - off;

		rc = flash_area_erase(fa, off, erase_size);
		if (rc != 0) {
			LOG_ERR("image slot trailer erase of 0x%zx bytes failed (err %d)",
					erase_size, rc);
			rc = MGMT_ERR_EUNKNOWN;
			goto end_fa;
		}

		LOG_INF("Erased 0x%zx bytes of image slot trailer", erase_size);
	}

	rc = 0;

end_fa:
	flash_area_close(fa);
end:
	return rc;
}

#if IMG_MGMT_LAZY_ERASE
int img_mgmt_impl_erase_if_needed(uint32_t off, uint32_t len)
{
	/* This is done internally to the flash_img API. */
	return 0;
}
#endif

int
img_mgmt_impl_swap_type(void)
{
	switch (mcuboot_swap_type()) {
	case BOOT_SWAP_TYPE_NONE:
		return IMG_MGMT_SWAP_TYPE_NONE;
	case BOOT_SWAP_TYPE_TEST:
		return IMG_MGMT_SWAP_TYPE_TEST;
	case BOOT_SWAP_TYPE_PERM:
		return IMG_MGMT_SWAP_TYPE_PERM;
	case BOOT_SWAP_TYPE_REVERT:
		return IMG_MGMT_SWAP_TYPE_REVERT;
	default:
		assert(0);
		return IMG_MGMT_SWAP_TYPE_NONE;
	}
}

/**
 * Verifies an upload request and indicates the actions that should be taken
 * during processing of the request.  This is a "read only" function in the
 * sense that it doesn't write anything to flash and doesn't modify any global
 * variables.
 *
 * @param req		The upload request to inspect.
 * @param action	On success, gets populated with information about how to process
 *			the request.
 *
 * @return 0 if processing should occur; A MGMT_ERR code if an error response should be sent
 *	   instead.
 */
int
img_mgmt_impl_upload_inspect(const struct img_mgmt_upload_req *req,
			     struct img_mgmt_upload_action *action, const char **errstr)
{
	const struct image_header *hdr;
	const struct flash_area *fa;
	struct image_version cur_ver;
	uint8_t rem_bytes;
	bool empty;
	int rc;

	memset(action, 0, sizeof(*action));

	if (req->off == -1) {
		/* Request did not include an `off` field. */
		*errstr = img_mgmt_err_str_hdr_malformed;
		return MGMT_ERR_EINVAL;
	}

	if (req->off == 0) {
		/* First upload chunk. */
		if (req->data_len < sizeof(struct image_header)) {
			/*
			 * Image header is the first thing in the image.
			 */
			*errstr = img_mgmt_err_str_hdr_malformed;
			return MGMT_ERR_EINVAL;
		}

		if (req->size == -1) {
			/* Request did not include a `len` field. */
			*errstr = img_mgmt_err_str_hdr_malformed;
			return MGMT_ERR_EINVAL;
		}
		action->size = req->size;

		hdr = (struct image_header *)req->img_data;
		if (hdr->ih_magic != IMAGE_MAGIC) {
			*errstr = img_mgmt_err_str_magic_mismatch;
			return MGMT_ERR_EINVAL;
		}

		if (req->data_sha_len > IMG_MGMT_DATA_SHA_LEN) {
			return MGMT_ERR_EINVAL;
		}

		/*
		 * If request includes proper data hash we can check whether there is
		 * upload in progress (interrupted due to e.g. link disconnection) with
		 * the same data hash so we can just resume it by simply including
		 * current upload offset in response.
		 */
		if ((req->data_sha_len > 0) && (g_img_mgmt_state.area_id != -1)) {
			if ((g_img_mgmt_state.data_sha_len == req->data_sha_len) &&
			    !memcmp(g_img_mgmt_state.data_sha, req->data_sha, req->data_sha_len)) {
				return 0;
			}
		}

		action->area_id = img_mgmt_find_best_area_id();
		if (action->area_id < 0) {
			/* No slot where to upload! */
			*errstr = img_mgmt_err_str_no_slot;
			return MGMT_ERR_ENOMEM;
		}


#if defined(CONFIG_IMG_MGMT_REJECT_DIRECT_XIP_MISMATCHED_SLOT)
		if (hdr->ih_flags & IMAGE_F_ROM_FIXED_ADDR) {
			rc = flash_area_open(action->area_id, &fa);
			if (rc) {
				*errstr = img_mgmt_err_str_flash_open_failed;
				return MGMT_ERR_EUNKNOWN;
			}

			if (fa->fa_off != hdr->ih_load_addr) {
				*errstr = img_mgmt_err_str_image_bad_flash_addr;
				flash_area_close(fa);
				return MGMT_ERR_EINVAL;
			}

			flash_area_close(fa);
		}
#endif


		if (req->upgrade) {
			/* User specified upgrade-only.  Make sure new image version is
			 * greater than that of the currently running image.
			 */
			rc = img_mgmt_my_version(&cur_ver);
			if (rc != 0) {
				return MGMT_ERR_EUNKNOWN;
			}

			if (img_mgmt_vercmp(&cur_ver, &hdr->ih_ver) >= 0) {
				*errstr = img_mgmt_err_str_downgrade;
				return MGMT_ERR_EBADSTATE;
			}
		}

#if IMG_MGMT_LAZY_ERASE
		(void) empty;
#else
		rc = zephyr_img_mgmt_flash_check_empty(action->area_id, &empty);
		if (rc) {
			return MGMT_ERR_EUNKNOWN;
		}

		action->erase = !empty;
#endif
	} else {
		/* Continuation of upload. */
		action->area_id = g_img_mgmt_state.area_id;
		action->size = g_img_mgmt_state.size;

		if (req->off != g_img_mgmt_state.off) {
			/*
			 * Invalid offset. Drop the data, and respond with the offset we're
			 * expecting data for.
			 */
			return 0;
		}
	}

	/* Calculate size of flash write. */
	action->write_bytes = req->data_len;
	if (req->off + req->data_len < action->size) {
		/*
		 * Respect flash write alignment if not in the last block
		 */
		rc = flash_area_open(action->area_id, &fa);
		if (rc) {
			*errstr = img_mgmt_err_str_flash_open_failed;
			return MGMT_ERR_EUNKNOWN;
		}

		rem_bytes = req->data_len % flash_area_align(fa);
		flash_area_close(fa);

		if (rem_bytes) {
			action->write_bytes -= rem_bytes;
		}
	}

	action->proceed = true;
	return 0;
}

int
img_mgmt_impl_erased_val(int slot, uint8_t *erased_val)
{
	const struct flash_area *fa;
	int rc;

	rc = flash_area_open(zephyr_img_mgmt_flash_area_id(slot), &fa);
	if (rc != 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	*erased_val = flash_area_erased_val(fa);
	flash_area_close(fa);

	return 0;
}
