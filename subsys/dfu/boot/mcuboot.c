/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2016-2017 Linaro Limited
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>

#include "bootutil/bootutil_public.h"
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/dfu/dfu_boot.h>

#if defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_RAM_LOAD) || \
	defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_RAM_LOAD_WITH_REVERT)
/* For RAM LOAD mode, the active image must be fetched from the bootloader */
#include <bootutil/boot_status.h>
#include <zephyr/retention/blinfo.h>

#define SLOT0_PARTITION		slot0_partition
#define SLOT1_PARTITION		slot1_partition
#define SLOT2_PARTITION		slot2_partition
#define SLOT3_PARTITION		slot3_partition
#define SLOT4_PARTITION		slot4_partition
#define SLOT5_PARTITION		slot5_partition
#define SLOT6_PARTITION		slot6_partition
#define SLOT7_PARTITION		slot7_partition
#define SLOT8_PARTITION		slot8_partition
#define SLOT9_PARTITION		slot9_partition
#define SLOT10_PARTITION	slot10_partition
#define SLOT11_PARTITION	slot11_partition
#define SLOT12_PARTITION	slot12_partition
#define SLOT13_PARTITION	slot13_partition
#define SLOT14_PARTITION	slot14_partition
#define SLOT15_PARTITION	slot15_partition
#endif

#define ERASED_VAL_32(x) (((x) << 24) | ((x) << 16) | ((x) << 8) | (x))

#ifdef CONFIG_MCUBOOT_BOOTLOADER_USES_SHA512
#define IMAGE_TLV_SHA		IMAGE_TLV_SHA512
#else
#define IMAGE_TLV_SHA		IMAGE_TLV_SHA256
#endif

#include "mcuboot_priv.h"

LOG_MODULE_REGISTER(mcuboot_dfu, CONFIG_IMG_MANAGER_LOG_LEVEL);

BUILD_ASSERT(sizeof(struct image_header) == IMAGE_HEADER_SIZE,
	     "struct image_header not required size");

/*
 * Helpers for image headers and trailers, as defined by mcuboot.
 */

/*
 * Strict defines: the definitions in the following block contain
 * values which are MCUboot implementation requirements.
 */

/* Header: */
#define BOOT_HEADER_MAGIC_V1 0x96f3b83d
#define BOOT_HEADER_SIZE_V1 32

enum IMAGE_INDEXES {
	IMAGE_INDEX_INVALID = -1,
	IMAGE_INDEX_0,
	IMAGE_INDEX_1,
	IMAGE_INDEX_2,
	IMAGE_INDEX_3,
	IMAGE_INDEX_4,
	IMAGE_INDEX_5,
	IMAGE_INDEX_6,
	IMAGE_INDEX_7,
};

#if defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_RAM_LOAD) || \
	defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_RAM_LOAD_WITH_REVERT)
/* For RAM LOAD mode, the active image must be fetched from the bootloader */
#define ACTIVE_SLOT_FLASH_AREA_ID boot_fetch_active_slot()
#define INVALID_SLOT_ID 255
#else
/* Get active partition. zephyr,code-partition chosen node must be defined */
#define ACTIVE_SLOT_FLASH_AREA_ID DT_PARTITION_ID(DT_CHOSEN(zephyr_code_partition))
#endif

/*
 * Raw (on-flash) representation of the v1 image header.
 */
struct mcuboot_v1_raw_header {
	uint32_t header_magic;
	uint32_t image_load_address;
	uint16_t header_size;
	uint16_t pad;
	uint32_t image_size;
	uint32_t image_flags;
	struct {
		uint8_t major;
		uint8_t minor;
		uint16_t revision;
		uint32_t build_num;
	} version;
	uint32_t pad2;
} __packed;

/*
 * End of strict defines
 */

#if defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_RAM_LOAD) || \
	defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_RAM_LOAD_WITH_REVERT)
uint8_t boot_fetch_active_slot(void)
{
	int rc;
	uint8_t slot;

	rc = blinfo_lookup(BLINFO_RUNNING_SLOT, &slot, sizeof(slot));

	if (rc <= 0) {
		LOG_ERR("Failed to fetch active slot: %d", rc);
		return INVALID_SLOT_ID;
	}

	LOG_DBG("Active slot: %d", slot);

	rc = dfu_boot_get_flash_area_id(slot);
	if (rc < 0) {
		return INVALID_SLOT_ID;
	}

	return (uint8_t)rc;
}
#else  /* CONFIG_MCUBOOT_BOOTLOADER_MODE_RAM_LOAD ||
	* CONFIG_MCUBOOT_BOOTLOADER_MODE_RAM_LOAD_WITH_REVERT
	*/

uint8_t boot_fetch_active_slot(void)
{
	return ACTIVE_SLOT_FLASH_AREA_ID;
}
#endif /* CONFIG_MCUBOOT_BOOTLOADER_MODE_RAM_LOAD ||
	* CONFIG_MCUBOOT_BOOTLOADER_MODE_RAM_LOAD_WITH_REVERT
	*/

#if defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_SWAP_USING_OFFSET)
size_t boot_get_image_start_offset(uint8_t area_id)
{
	size_t off = 0;
	int image = IMAGE_INDEX_INVALID;

	if (area_id == PARTITION_ID(slot1_partition)) {
		image = IMAGE_INDEX_0;
#if PARTITION_EXISTS(slot3_partition)
	} else if (area_id == PARTITION_ID(slot3_partition)) {
		image = IMAGE_INDEX_1;
#endif
#if PARTITION_EXISTS(slot5_partition)
	} else if (area_id == PARTITION_ID(slot5_partition)) {
		image = IMAGE_INDEX_2;
#endif
#if PARTITION_EXISTS(slot7_partition)
	} else if (area_id == PARTITION_ID(slot7_partition)) {
		image = IMAGE_INDEX_3;
#endif
#if PARTITION_EXISTS(slot9_partition)
	} else if (area_id == PARTITION_ID(slot9_partition)) {
		image = IMAGE_INDEX_4;
#endif
#if PARTITION_EXISTS(slot11_partition)
	} else if (area_id == PARTITION_ID(slot11_partition)) {
		image = IMAGE_INDEX_5;
#endif
#if PARTITION_EXISTS(slot13_partition)
	} else if (area_id == PARTITION_ID(slot13_partition)) {
		image = IMAGE_INDEX_6;
#endif
#if PARTITION_EXISTS(slot15_partition)
	} else if (area_id == PARTITION_ID(slot15_partition)) {
		image = IMAGE_INDEX_7;
#endif

	}

	if (image != IMAGE_INDEX_INVALID) {
		/* Need to check status from primary slot to get correct offset for secondary
		 * slot image header
		 */
		const struct flash_area *fa;
		uint32_t num_sectors = SWAP_USING_OFFSET_SECTOR_UPDATE_BEGIN;
		struct flash_sector sector_data;
		int rc;

		rc = flash_area_open(area_id, &fa);
		if (rc) {
			LOG_ERR("Flash open area %u failed: %d", area_id, rc);
			goto done;
		}

		if (mcuboot_swap_type_multi(image) != BOOT_SWAP_TYPE_REVERT) {
			/* For swap using offset mode, the image starts in the second sector of
			 * the upgrade slot, so apply the offset when this is needed, do this by
			 * getting information on first sector only, this is expected to return an
			 * error as there are more slots, so allow the not enough memory error
			 */
			rc = flash_area_get_sectors(area_id, &num_sectors, &sector_data);
			if ((rc != 0 && rc != -ENOMEM) ||
				num_sectors != SWAP_USING_OFFSET_SECTOR_UPDATE_BEGIN) {
				LOG_ERR("Failed to get sector details: %d", rc);
			} else {
				off = sector_data.fs_size;
			}
		}

		flash_area_close(fa);
	}

done:
	LOG_DBG("Start offset for area %u: 0x%x", area_id, off);
	return off;
}
#endif /* CONFIG_MCUBOOT_BOOTLOADER_MODE_SWAP_USING_OFFSET */

static int boot_read_v1_header(uint8_t area_id,
			       struct mcuboot_v1_raw_header *v1_raw)
{
	const struct flash_area *fa;
	int rc;
	size_t off = boot_get_image_start_offset(area_id);

	rc = flash_area_open(area_id, &fa);
	if (rc) {
		return rc;
	}

	/*
	 * Read and validty-check the raw header.
	 */
	rc = flash_area_read(fa, off, v1_raw, sizeof(*v1_raw));
	flash_area_close(fa);
	if (rc) {
		return rc;
	}

	v1_raw->header_magic = sys_le32_to_cpu(v1_raw->header_magic);
	v1_raw->header_size = sys_le16_to_cpu(v1_raw->header_size);

	/*
	 * Validity checks.
	 *
	 * Larger values in header_size than BOOT_HEADER_SIZE_V1 are
	 * possible, e.g. if Zephyr was linked with
	 * CONFIG_ROM_START_OFFSET > BOOT_HEADER_SIZE_V1.
	 */
	if ((v1_raw->header_magic != BOOT_HEADER_MAGIC_V1) ||
	    (v1_raw->header_size < BOOT_HEADER_SIZE_V1)) {
		return -EIO;
	}

	v1_raw->image_load_address =
		sys_le32_to_cpu(v1_raw->image_load_address);
	v1_raw->header_size = sys_le16_to_cpu(v1_raw->header_size);
	v1_raw->image_size = sys_le32_to_cpu(v1_raw->image_size);
	v1_raw->image_flags = sys_le32_to_cpu(v1_raw->image_flags);
	v1_raw->version.revision =
		sys_le16_to_cpu(v1_raw->version.revision);
	v1_raw->version.build_num =
		sys_le32_to_cpu(v1_raw->version.build_num);

	return 0;
}

int boot_read_bank_header(uint8_t area_id,
			  struct mcuboot_img_header *header,
			  size_t header_size)
{
	int rc;
	struct mcuboot_v1_raw_header v1_raw;
	struct mcuboot_img_sem_ver *sem_ver;
	size_t v1_min_size = (sizeof(uint32_t) +
			      sizeof(struct mcuboot_img_header_v1));

	/*
	 * Only version 1 image headers are supported.
	 */
	if (header_size < v1_min_size) {
		return -ENOMEM;
	}
	rc = boot_read_v1_header(area_id, &v1_raw);
	if (rc) {
		return rc;
	}

	/*
	 * Copy just the fields we care about into the return parameter.
	 *
	 * - header_magic:       skip (only used to check format)
	 * - image_load_address: skip (only matters for PIC code)
	 * - header_size:        skip (only used to check format)
	 * - image_size:         include
	 * - image_flags:        skip (all unsupported or not relevant)
	 * - version:            include
	 */
	header->mcuboot_version = 1U;
	header->h.v1.image_size = v1_raw.image_size;
	sem_ver = &header->h.v1.sem_ver;
	sem_ver->major = v1_raw.version.major;
	sem_ver->minor = v1_raw.version.minor;
	sem_ver->revision = v1_raw.version.revision;
	sem_ver->build_num = v1_raw.version.build_num;
	return 0;
}

#if defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP_WITH_REVERT)
int mcuboot_read_directxip_state(int slot)
{
	struct boot_swap_state bss;
	int fa_id = dfu_boot_get_flash_area_id(slot);
	const struct flash_area *fa;
	int rc;

	__ASSERT(fa_id >= 0, "Could not map slot to area ID");

	rc = flash_area_open(fa_id, &fa);
	if (rc < 0) {
		return rc;
	}

	rc = boot_read_swap_state(fa, &bss);
	flash_area_close(fa);

	if (rc != 0) {
		LOG_ERR("Failed to read state of slot %d with error %d", slot, rc);
		return -EIO;
	}

	if (bss.magic == BOOT_MAGIC_GOOD) {
		if (bss.image_ok == BOOT_FLAG_SET) {
			return MCUBOOT_DIRECT_XIP_BOOT_FOREVER;
		} else if (bss.copy_done == BOOT_FLAG_SET) {
			return MCUBOOT_DIRECT_XIP_BOOT_REVERT;
		}
		return MCUBOOT_DIRECT_XIP_BOOT_ONCE;
	}

	return MCUBOOT_DIRECT_XIP_BOOT_UNSET;
}
#endif /* CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP_WITH_REVERT */

int mcuboot_swap_type_multi(int image_index)
{
	return boot_swap_type_multi(image_index);
}

int mcuboot_swap_type(void)
{
#ifdef FLASH_AREA_IMAGE_SECONDARY
	return boot_swap_type();
#else
	return BOOT_SWAP_TYPE_NONE;
#endif

}

int boot_request_upgrade(int permanent)
{
#ifdef FLASH_AREA_IMAGE_SECONDARY
	int rc;

	rc = boot_set_pending(permanent);
	if (rc) {
		return -EFAULT;
	}
#endif /* FLASH_AREA_IMAGE_SECONDARY */
	return 0;
}

int boot_request_upgrade_multi(int image_index, int permanent)
{
	int rc;

	rc = boot_set_pending_multi(image_index, permanent);
	if (rc) {
		return -EFAULT;
	}
	return 0;
}

bool boot_is_img_confirmed(void)
{
	struct boot_swap_state state;
	const struct flash_area *fa;
	int rc;

	rc = flash_area_open(ACTIVE_SLOT_FLASH_AREA_ID, &fa);
	if (rc) {
		return false;
	}

	rc = boot_read_swap_state(fa, &state);
	if (rc != 0) {
		return false;
	}

	if (state.magic == BOOT_MAGIC_UNSET) {
		/* This is initial/preprogramed image.
		 * Such image can neither be reverted nor physically confirmed.
		 * Treat this image as confirmed which ensures consistency
		 * with `boot_write_img_confirmed...()` procedures.
		 */
		return true;
	}

	return state.image_ok == BOOT_FLAG_SET;
}

int boot_write_img_confirmed(void)
{
	const struct flash_area *fa;
	int rc = 0;

	if (flash_area_open(ACTIVE_SLOT_FLASH_AREA_ID, &fa) != 0) {
		return -EIO;
	}

	rc = boot_set_next(fa, true, true);

	flash_area_close(fa);

	return rc;
}

int boot_write_img_confirmed_multi(int image_index)
{
	int rc;

	rc = boot_set_confirmed_multi(image_index);
	if (rc) {
		return -EIO;
	}

	return 0;
}

int boot_erase_img_bank(uint8_t area_id)
{
	const struct flash_area *fa;
	int rc;

	rc = flash_area_open(area_id, &fa);
	if (rc) {
		return rc;
	}

	rc = flash_area_flatten(fa, 0, fa->fa_size);

	flash_area_close(fa);

	return rc;
}

ssize_t boot_get_trailer_status_offset(size_t area_size)
{
	return (ssize_t)area_size - BOOT_MAGIC_SZ - BOOT_MAX_ALIGN * 2;
}

ssize_t boot_get_area_trailer_status_offset(uint8_t area_id)
{
	int rc;
	const struct flash_area *fa;
	ssize_t offset;

	rc = flash_area_open(area_id, &fa);
	if (rc) {
		return rc;
	}

	offset = boot_get_trailer_status_offset(fa->fa_size);

	flash_area_close(fa);

	if (offset < 0) {
		return -EFAULT;
	}

	return offset;
}

/**
 * Finds the TLVs in the specified image slot, if any.
 */
static int mcuboot_find_tlvs(int slot, size_t *start_off, size_t *end_off, uint16_t magic)
{
	struct image_tlv_info tlv_info;
	int rc;

	rc = dfu_boot_read(slot, *start_off, &tlv_info, sizeof(tlv_info));
	if (rc != 0) {
		return rc;
	}

	if (tlv_info.it_magic != magic) {
		/* No TLVs. */
		return -ENOENT;
	}

	*start_off += sizeof(tlv_info);
	*end_off = *start_off + tlv_info.it_tlv_tot;

	return 0;
}

/**
 * Helper to populate dfu_boot_img_info from an image_header
 */
static void populate_img_info_from_header(const struct image_header *hdr,
							struct dfu_boot_img_info *info)
{
	BUILD_ASSERT(sizeof(struct image_version) == sizeof(struct dfu_boot_img_version),
		"image_version must match dfu_boot_img_version size");
	memcpy(&info->version, &hdr->ih_ver, sizeof(struct dfu_boot_img_version));

	info->img_size = hdr->ih_img_size;
	info->hdr_size = hdr->ih_hdr_size;
	info->load_addr = hdr->ih_load_addr;

	info->flags = 0;
	if (hdr->ih_flags & IMAGE_F_NON_BOOTABLE) {
		info->flags |= DFU_BOOT_IMG_F_NON_BOOTABLE;
	}
	if (hdr->ih_flags & IMAGE_F_ROM_FIXED) {
		info->flags |= DFU_BOOT_IMG_F_ROM_FIXED;
	}

	info->valid = true;
}

int dfu_boot_read_img_info(int slot, struct dfu_boot_img_info *info)
{
	struct image_header hdr;
	struct image_tlv tlv;
	size_t data_off;
	size_t data_end;
	bool hash_found;
	int rc;

	if (info == NULL) {
		return -EINVAL;
	}

	memset(info, 0, sizeof(*info));

	rc = dfu_boot_read(slot,
			   boot_get_image_start_offset(dfu_boot_get_flash_area_id(slot)),
			   &hdr, sizeof(hdr));

	if (rc != 0) {
		return rc;
	}

	if (hdr.ih_magic != IMAGE_MAGIC) {
		return -ENOENT;
	}

	/* Use shared helper to populate basic info */
	populate_img_info_from_header(&hdr, info);

	/* Read the image's TLVs. We first try to find the protected TLVs, if the protected
	 * TLV does not exist, we try to find non-protected TLV which also contains the hash
	 * TLV. All images are required to have a hash TLV.  If the hash is missing, the image
	 * is considered invalid.
	 */
	data_off = hdr.ih_hdr_size + hdr.ih_img_size +
		   boot_get_image_start_offset(dfu_boot_get_flash_area_id(slot));

	rc = mcuboot_find_tlvs(slot, &data_off, &data_end, IMAGE_TLV_PROT_INFO_MAGIC);
	if (!rc) {
		/* The data offset should start after the header bytes after the end of
		 * the protected TLV, if one exists.
		 */
		data_off = data_end - sizeof(struct image_tlv_info);
	}

	rc = mcuboot_find_tlvs(slot, &data_off, &data_end, IMAGE_TLV_INFO_MAGIC);
	if (rc != 0) {
		return rc;
	}

	hash_found = false;
	while (data_off + sizeof(tlv) <= data_end) {
		rc = dfu_boot_read(slot, data_off, &tlv, sizeof(tlv));
		if (rc != 0) {
			return rc;
		}
		if (tlv.it_type == 0xff && tlv.it_len == 0xffff) {
			return -EINVAL;
		}
		if (tlv.it_type != IMAGE_TLV_SHA || tlv.it_len != DFU_BOOT_IMG_SHA_LEN) {
			/* Non-hash TLV.  Skip it. */
			data_off += sizeof(tlv) + tlv.it_len;
			continue;
		}

		if (hash_found) {
			/* More than one hash. */
			return -EINVAL;
		}

		data_off += sizeof(tlv);
		if (data_off + DFU_BOOT_IMG_SHA_LEN > data_end) {
			return -EINVAL;
		}

		rc = dfu_boot_read(slot, data_off, info->hash, DFU_BOOT_IMG_SHA_LEN);
		if (rc != 0) {
			return rc;
		}

		info->hash_len = DFU_BOOT_IMG_SHA_LEN;
		hash_found = true;
		data_off += DFU_BOOT_IMG_SHA_LEN;
	}

	if (!hash_found) {
		return -ENOENT;
	}

	return 0;
}

int dfu_boot_validate_header(const void *data, size_t len, struct dfu_boot_img_info *info)
{
	const struct image_header *hdr = (const struct image_header *)data;

	if (data == NULL || len < sizeof(struct image_header)) {
		return -EINVAL;
	}

	if (hdr->ih_magic != IMAGE_MAGIC) {
		return -EINVAL;
	}

	if (info != NULL) {
		memset(info, 0, sizeof(*info));
		populate_img_info_from_header(hdr, info);
	}

	return 0;
}

int dfu_boot_get_swap_type(int image_index)
{
	switch (boot_swap_type_multi(image_index)) {
	case BOOT_SWAP_TYPE_NONE:
		return DFU_BOOT_SWAP_TYPE_NONE;
	case BOOT_SWAP_TYPE_TEST:
		return DFU_BOOT_SWAP_TYPE_TEST;
	case BOOT_SWAP_TYPE_PERM:
		return DFU_BOOT_SWAP_TYPE_PERM;
	case BOOT_SWAP_TYPE_REVERT:
		return DFU_BOOT_SWAP_TYPE_REVERT;
	default:
		return DFU_BOOT_SWAP_TYPE_UNKNOWN;
	}
}

static int slot_to_image(int slot)
{
	__ASSERT(slot >= 0 && slot < (CONFIG_UPDATEABLE_IMAGE_NUMBER << 1),
		 "Impossible slot number");

	return (slot >> 1);
}

int dfu_boot_set_next(int slot, bool active, bool confirm)
{
	int rc;
	const struct flash_area *fa;

	if (flash_area_open(dfu_boot_get_flash_area_id(slot), &fa) != 0) {
		return -EIO;
	}

	rc = boot_set_next(fa, active, confirm);

	flash_area_close(fa);

	return rc;
}

int dfu_boot_set_pending(int slot, bool permanent)
{
	return boot_set_pending_multi(slot_to_image(slot), permanent);
}

int dfu_boot_confirm(void)
{
	return boot_write_img_confirmed();
}

size_t dfu_boot_get_image_start_offset(int slot)
{
	return boot_get_image_start_offset(dfu_boot_get_flash_area_id(slot));
}

size_t dfu_boot_get_trailer_status_offset(size_t area_size)
{
	return boot_get_trailer_status_offset(area_size);
}
