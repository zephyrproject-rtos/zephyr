/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/dfu/flash_img.h>
#include <zephyr/logging/log.h>
#include <bootutil/bootutil_public.h>
#include <assert.h>

#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/image.h>

#include <mgmt/mcumgr/grp/img_mgmt/img_mgmt_impl.h>
#include <mgmt/mcumgr/grp/img_mgmt/img_mgmt_priv.h>

LOG_MODULE_DECLARE(mcumgr_img_grp, CONFIG_MCUMGR_GRP_IMG_LOG_LEVEL);

#define SLOT0_PARTITION		slot0_partition
#define SLOT1_PARTITION		slot1_partition
#define SLOT2_PARTITION		slot2_partition
#define SLOT3_PARTITION		slot3_partition

BUILD_ASSERT(CONFIG_MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER == 1 ||
	     (CONFIG_MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER == 2 &&
	      FIXED_PARTITION_EXISTS(SLOT2_PARTITION) &&
	      FIXED_PARTITION_EXISTS(SLOT3_PARTITION)),
	     "Missing partitions?");

#if defined(CONFIG_MCUMGR_GRP_IMG_DIRECT_UPLOAD) &&		\
	!(CONFIG_MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER > 1)
/* In case when direct upload is enabled, slot2 and slot3 are optional
 * as long as there is support for one application image only.
 */
#define ADD_SLOT_2_CONDITION FIXED_PARTITION_EXISTS(SLOT2_PARTITION)
#define ADD_SLOT_3_CONDITION FIXED_PARTITION_EXISTS(SLOT3_PARTITION)
#elif (CONFIG_MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER > 1)
/* For more than one application image slot2 and slot3 are required. */
#define ADD_SLOT_2_CONDITION 1
#define ADD_SLOT_3_CONDITION 1
#else
/* If neither in direct upload mode nor more than one application image
 * is supported, then slot2 and slot3 support is useless.
 */
#define ADD_SLOT_2_CONDITION 0
#define ADD_SLOT_3_CONDITION 0
#endif

static int
img_mgmt_slot_to_image(int slot)
{
	switch (slot) {
	case 0:
	case 1:
		return 0;
#if ADD_SLOT_2_CONDITION
	case 2:
		return 1;
#endif
#if ADD_SLOT_3_CONDITION
	case 3:
		return 1;
#endif
	default:
		assert(0);
	}
	return 0;
}

/**
 * Determines if the specified area of flash is completely unwritten.
 *
 * @param	fa	pointer to flash area to scan
 *
 * @return	0 when not empty, 1 when empty, negative errno code on error.
 */
static int img_mgmt_flash_check_empty_inner(const struct flash_area *fa)
{
	uint32_t data[16];
	off_t addr;
	off_t end;
	int bytes_to_read;
	int rc;
	int i;
	uint8_t erased_val;
	uint32_t erased_val_32;

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
		if (rc < 0) {
			LOG_ERR("Failed to read data from flash area: %d", rc);
			return rc;
		}

		for (i = 0; i < bytes_to_read / 4; i++) {
			if (data[i] != erased_val_32) {
				return 0;
			}
		}
	}

	return 1;
}

#ifndef CONFIG_IMG_ERASE_PROGRESSIVELY
/* Check if area is empty
 *
 * @param	fa_id	ID of flash area to scan.
 *
 * @return	0 when not empty, 1 when empty, negative errno code on error.
 */
static int img_mgmt_flash_check_empty(uint8_t fa_id)
{
	const struct flash_area *fa;
	int rc;

	rc = flash_area_open(fa_id, &fa);
	if (rc == 0) {
		rc = img_mgmt_flash_check_empty_inner(fa);

		flash_area_close(fa);
	} else {
		LOG_ERR("Failed to open flash area ID %u: %d", fa_id, rc);
	}

	return rc;
}
#endif

/**
 * Get flash_area ID for a image number; actually the slots are images
 * for Zephyr, as slot 0 of image 0 is image_0, slot 0 of image 1 is
 * image_2 and so on. The function treats slot numbers as absolute
 * slot number starting at 0.
 */
static int
img_mgmt_flash_area_id(int slot)
{
	uint8_t fa_id;

	switch (slot) {
	case 0:
		fa_id = FIXED_PARTITION_ID(SLOT0_PARTITION);
		break;

	case 1:
		fa_id = FIXED_PARTITION_ID(SLOT1_PARTITION);
		break;

#if ADD_SLOT_2_CONDITION
	case 2:
		fa_id = FIXED_PARTITION_ID(SLOT2_PARTITION);
		break;
#endif

#if ADD_SLOT_3_CONDITION
	case 3:
		fa_id = FIXED_PARTITION_ID(SLOT3_PARTITION);
		break;
#endif

	default:
		fa_id = -1;
		break;
	}

	return fa_id;
}

#if CONFIG_MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER == 1
/**
 * In normal operation this function will select between first two slot
 * (in reality it just checks whether second slot can be used), ignoring the
 * slot parameter.
 * When CONFIG_MCUMGR_GRP_IMG_DIRECT_UPLOAD is defined it will check if given
 * slot is available, and allowed, for DFU; providing 0 as a parameter means
 * find any unused and non-active available (auto-select); any other positive
 * value is direct (slot + 1) to be used; if checks are positive, then area
 * ID is returned, -1 is returned otherwise.
 * Note that auto-selection is performed only between two two first slots.
 */
static int
img_mgmt_get_unused_slot_area_id(int slot)
{
#if defined(CONFIG_MCUMGR_GRP_IMG_DIRECT_UPLOAD)
	slot--;
	if (slot < -1) {
		return -1;
	} else if (slot == -1) {
#endif
		/*
		 * Auto select slot; note that this is performed only between two first
		 * slots, at this point, which will require fix when Direct-XIP, which
		 * may support more slots, gets support within Zephyr.
		 */
		for (slot = 0; slot < 2; slot++) {
			if (img_mgmt_slot_in_use(slot) == 0) {
				int area_id = img_mgmt_flash_area_id(slot);

				if (area_id >= 0) {
					return area_id;
				}
			}
		}
		return -1;
#if defined(CONFIG_MCUMGR_GRP_IMG_DIRECT_UPLOAD)
	}
	/*
	 * Direct selection; the first two slots are checked for being available
	 * and unused; the all other slots are just checked for availability.
	 */
	if (slot < 2) {
		slot = img_mgmt_slot_in_use(slot) == 0 ? slot : -1;
	}

	/* Return area ID for the slot or -1 */
	return slot != -1  ? img_mgmt_flash_area_id(slot) : -1;
#endif
}

#elif CONFIG_MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER == 2
static int
img_mgmt_get_unused_slot_area_id(int image)
{
	int area_id = -1;

	if (image == 0 || image == -1) {
		if (img_mgmt_slot_in_use(1) == 0) {
			area_id = img_mgmt_flash_area_id(1);
		}
	} else if (image == 1) {
		area_id = img_mgmt_flash_area_id(3);
	}

	return area_id;
}
#else
#error "Unsupported number of images"
#endif

int
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
img_mgmt_erase_slot(int slot)
{
	const struct flash_area *fa;
	int rc;
	int area_id = img_mgmt_flash_area_id(slot);

	if (area_id < 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	rc = flash_area_open(area_id, &fa);

	if (rc < 0) {
		LOG_ERR("Failed to open flash area ID %u: %d", area_id, rc);
		return MGMT_ERR_EUNKNOWN;
	}

	rc = img_mgmt_flash_check_empty_inner(fa);

	if (rc == 0) {
		rc = flash_area_erase(fa, 0, fa->fa_size);

		if (rc != 0) {
			LOG_ERR("Failed to erase flash area: %d", rc);
		}
	}

	flash_area_close(fa);

	return (rc >= 0 ? MGMT_ERR_EOK : MGMT_ERR_EUNKNOWN);
}

int
img_mgmt_write_pending(int slot, bool permanent)
{
	int rc;

	if (slot != 1 && !(CONFIG_MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER == 2 && slot == 3)) {
		return MGMT_ERR_EINVAL;
	}

	rc = boot_request_upgrade_multi(img_mgmt_slot_to_image(slot), permanent);
	if (rc != 0) {
		LOG_ERR("Failed to write pending flag for slot %d: %d", slot, rc);
		return MGMT_ERR_EUNKNOWN;
	}

	return 0;
}

int
img_mgmt_write_confirmed(void)
{
	int rc;

	rc = boot_write_img_confirmed();
	if (rc != 0) {
		LOG_ERR("Failed to write confirmed flag: %d", rc);
		return MGMT_ERR_EUNKNOWN;
	}

	return 0;
}

int
img_mgmt_read(int slot, unsigned int offset, void *dst, unsigned int num_bytes)
{
	const struct flash_area *fa;
	int rc;
	int area_id = img_mgmt_flash_area_id(slot);

	if (area_id < 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	rc = flash_area_open(area_id, &fa);
	if (rc != 0) {
		LOG_ERR("Failed to open flash area ID %u: %d", area_id, rc);
		return MGMT_ERR_EUNKNOWN;
	}

	rc = flash_area_read(fa, offset, dst, num_bytes);
	flash_area_close(fa);

	if (rc != 0) {
		LOG_ERR("Failed to read data from flash: %d", rc);
		return MGMT_ERR_EUNKNOWN;
	}

	return 0;
}

#if defined(CONFIG_MCUMGR_GRP_IMG_USE_HEAP_FOR_FLASH_IMG_CONTEXT)
int
img_mgmt_write_image_data(unsigned int offset, const void *data, unsigned int num_bytes, bool last)
{
	/* Even if CONFIG_HEAP_MEM_POOL_SIZE will be able to match size of the structure,
	 * keep in mind that when application will put the heap under pressure, obtaining
	 * of a flash image context may not be possible, so plan bigger heap size or
	 * make sure to limit application pressure on heap when DFU is expected.
	 */
	BUILD_ASSERT(CONFIG_HEAP_MEM_POOL_SIZE >= (sizeof(struct flash_img_context)),
		     "Not enough heap mem for flash_img_context.");

	int rc = MGMT_ERR_EOK;
	static struct flash_img_context *ctx;

	if (offset != 0 && ctx == NULL) {
		return MGMT_ERR_EUNKNOWN;
	}

	if (offset == 0) {
		if (ctx != NULL) {
			return MGMT_ERR_EUNKNOWN;
		}
		ctx = k_malloc(sizeof(struct flash_img_context));

		if (ctx == NULL) {
			return MGMT_ERR_ENOMEM;
		}

		if (flash_img_init_id(ctx, g_img_mgmt_state.area_id) != 0) {
			rc = MGMT_ERR_EUNKNOWN;
			goto out;
		}
	}

	if (flash_img_buffered_write(ctx, data, num_bytes, last) != 0) {
		rc = MGMT_ERR_EUNKNOWN;
		goto out;
	}

out:
	if (last || rc != MGMT_ERR_EOK) {
		k_free(ctx);
		ctx = NULL;
	}

	return rc;
}

#else
int
img_mgmt_write_image_data(unsigned int offset, const void *data, unsigned int num_bytes, bool last)
{
	static struct flash_img_context ctx;

	if (offset == 0) {
		if (flash_img_init_id(&ctx, g_img_mgmt_state.area_id) != 0) {
			return MGMT_ERR_EUNKNOWN;
		}
	}

	if (flash_img_buffered_write(&ctx, data, num_bytes, last) != 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	return MGMT_ERR_EOK;
}
#endif

int
img_mgmt_erase_image_data(unsigned int off, unsigned int num_bytes)
{
	const struct flash_area *fa;
	int rc;

	if (off != 0) {
		rc = MGMT_ERR_EINVAL;
		goto end;
	}

	rc = flash_area_open(g_img_mgmt_state.area_id, &fa);
	if (rc != 0) {
		LOG_ERR("Can't bind to the flash area (err %d)", rc);
		rc = MGMT_ERR_EUNKNOWN;
		goto end;
	}

	/* align requested erase size to the erase-block-size */
	const struct device *dev = flash_area_get_device(fa);

	if (dev == NULL) {
		rc = MGMT_ERR_EUNKNOWN;
		goto end_fa;
	}
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

#ifdef CONFIG_MCUBOOT_IMG_MANAGER
	/* Right now MCUmgr supports only mcuboot images.
	 * Above compilation swich might help to recognize mcuboot related
	 * code when supports for anothe bootloader will be introduced.
	 */

	/* erase the image trailer area if it was not erased */
	off = boot_get_trailer_status_offset(fa->fa_size);
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
#endif
	rc = 0;

end_fa:
	flash_area_close(fa);
end:
	return rc;
}

int
img_mgmt_swap_type(int slot)
{
	int image = img_mgmt_slot_to_image(slot);

	switch (mcuboot_swap_type_multi(image)) {
	case BOOT_SWAP_TYPE_NONE:
		return IMG_MGMT_SWAP_TYPE_NONE;
	case BOOT_SWAP_TYPE_TEST:
		return IMG_MGMT_SWAP_TYPE_TEST;
	case BOOT_SWAP_TYPE_PERM:
		return IMG_MGMT_SWAP_TYPE_PERM;
	case BOOT_SWAP_TYPE_REVERT:
		return IMG_MGMT_SWAP_TYPE_REVERT;
	default:
		return IMG_MGMT_SWAP_TYPE_UNKNOWN;
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
img_mgmt_upload_inspect(const struct img_mgmt_upload_req *req,
			struct img_mgmt_upload_action *action)
{
	const struct image_header *hdr;
	struct image_version cur_ver;
	int rc;

	memset(action, 0, sizeof(*action));

	if (req->off == SIZE_MAX) {
		/* Request did not include an `off` field. */
		IMG_MGMT_UPLOAD_ACTION_SET_RC_RSN(action, img_mgmt_err_str_hdr_malformed);
		return MGMT_ERR_EINVAL;
	}

	if (req->off == 0) {
		/* First upload chunk. */
		if (req->img_data.len < sizeof(struct image_header)) {
			/*  Image header is the first thing in the image */
			IMG_MGMT_UPLOAD_ACTION_SET_RC_RSN(action, img_mgmt_err_str_hdr_malformed);
			return MGMT_ERR_EINVAL;
		}

		if (req->size == SIZE_MAX) {
			/* Request did not include a `len` field. */
			IMG_MGMT_UPLOAD_ACTION_SET_RC_RSN(action, img_mgmt_err_str_hdr_malformed);
			return MGMT_ERR_EINVAL;
		}
		action->size = req->size;

		hdr = (struct image_header *)req->img_data.value;
		if (hdr->ih_magic != IMAGE_MAGIC) {
			IMG_MGMT_UPLOAD_ACTION_SET_RC_RSN(action, img_mgmt_err_str_magic_mismatch);
			return MGMT_ERR_EINVAL;
		}

		if (req->data_sha.len > IMG_MGMT_DATA_SHA_LEN) {
			return MGMT_ERR_EINVAL;
		}

		/*
		 * If request includes proper data hash we can check whether there is
		 * upload in progress (interrupted due to e.g. link disconnection) with
		 * the same data hash so we can just resume it by simply including
		 * current upload offset in response.
		 */
		if ((req->data_sha.len > 0) && (g_img_mgmt_state.area_id != -1)) {
			if ((g_img_mgmt_state.data_sha_len == req->data_sha.len) &&
			    !memcmp(g_img_mgmt_state.data_sha, req->data_sha.value,
				    req->data_sha.len)) {
				return 0;
			}
		}

		action->area_id = img_mgmt_get_unused_slot_area_id(req->image);
		if (action->area_id < 0) {
			/* No slot where to upload! */
			IMG_MGMT_UPLOAD_ACTION_SET_RC_RSN(action, img_mgmt_err_str_no_slot);
			return MGMT_ERR_ENOENT;
		}

#if defined(CONFIG_MCUMGR_GRP_IMG_REJECT_DIRECT_XIP_MISMATCHED_SLOT)
		if (hdr->ih_flags & IMAGE_F_ROM_FIXED_ADDR) {
			const struct flash_area *fa;

			rc = flash_area_open(action->area_id, &fa);
			if (rc) {
				IMG_MGMT_UPLOAD_ACTION_SET_RC_RSN(action,
					img_mgmt_err_str_flash_open_failed);
				LOG_ERR("Failed to open flash area ID %u: %d", action->area_id,
					rc);
				return MGMT_ERR_EUNKNOWN;
			}

			if (fa->fa_off != hdr->ih_load_addr) {
				IMG_MGMT_UPLOAD_ACTION_SET_RC_RSN(action,
					img_mgmt_err_str_image_bad_flash_addr);
				flash_area_close(fa);
				return MGMT_ERR_EINVAL;
			}

			flash_area_close(fa);
		}
#endif


		if (req->upgrade) {
			/* User specified upgrade-only. Make sure new image version is
			 * greater than that of the currently running image.
			 */
			rc = img_mgmt_my_version(&cur_ver);
			if (rc != 0) {
				return MGMT_ERR_EUNKNOWN;
			}

			if (img_mgmt_vercmp(&cur_ver, &hdr->ih_ver) >= 0) {
				IMG_MGMT_UPLOAD_ACTION_SET_RC_RSN(action,
					img_mgmt_err_str_downgrade);
				return MGMT_ERR_EBADSTATE;
			}
		}

#ifndef CONFIG_IMG_ERASE_PROGRESSIVELY
		rc = img_mgmt_flash_check_empty(action->area_id);
		if (rc < 0) {
			return MGMT_ERR_EUNKNOWN;
		}

		action->erase = (rc == 0);
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

	action->write_bytes = req->img_data.len;
	action->proceed = true;
	IMG_MGMT_UPLOAD_ACTION_SET_RC_RSN(action, NULL);

	return 0;
}

int
img_mgmt_erased_val(int slot, uint8_t *erased_val)
{
	const struct flash_area *fa;
	int rc;
	int area_id = img_mgmt_flash_area_id(slot);

	if (area_id < 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	rc = flash_area_open(area_id, &fa);
	if (rc != 0) {
		LOG_ERR("Failed to open flash area ID %u: %d", area_id, rc);
		return MGMT_ERR_EUNKNOWN;
	}

	*erased_val = flash_area_erased_val(fa);
	flash_area_close(fa);

	return 0;
}
