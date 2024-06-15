/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2016-2017 Linaro Limited
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

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>

#include "bootutil/bootutil_public.h"
#include <zephyr/dfu/mcuboot.h>

#include "mcuboot_priv.h"

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

/* Get active partition. zephyr,code-partition chosen node must be defined */
#define ACTIVE_SLOT_FLASH_AREA_ID DT_FIXED_PARTITION_ID(DT_CHOSEN(zephyr_code_partition))

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

static int boot_read_v1_header(uint8_t area_id,
			       struct mcuboot_v1_raw_header *v1_raw)
{
	const struct flash_area *fa;
	int rc;

	rc = flash_area_open(area_id, &fa);
	if (rc) {
		return rc;
	}

	/*
	 * Read and sanity-check the raw header.
	 */
	rc = flash_area_read(fa, 0, v1_raw, sizeof(*v1_raw));
	flash_area_close(fa);
	if (rc) {
		return rc;
	}

	v1_raw->header_magic = sys_le32_to_cpu(v1_raw->header_magic);
	v1_raw->image_load_address =
		sys_le32_to_cpu(v1_raw->image_load_address);
	v1_raw->header_size = sys_le16_to_cpu(v1_raw->header_size);
	v1_raw->image_size = sys_le32_to_cpu(v1_raw->image_size);
	v1_raw->image_flags = sys_le32_to_cpu(v1_raw->image_flags);
	v1_raw->version.revision =
		sys_le16_to_cpu(v1_raw->version.revision);
	v1_raw->version.build_num =
		sys_le32_to_cpu(v1_raw->version.build_num);

	/*
	 * Sanity checks.
	 *
	 * Larger values in header_size than BOOT_HEADER_SIZE_V1 are
	 * possible, e.g. if Zephyr was linked with
	 * CONFIG_ROM_START_OFFSET > BOOT_HEADER_SIZE_V1.
	 */
	if ((v1_raw->header_magic != BOOT_HEADER_MAGIC_V1) ||
	    (v1_raw->header_size < BOOT_HEADER_SIZE_V1)) {
		return -EIO;
	}

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

	rc = flash_area_open(FLASH_AREA_IMAGE_PRIMARY, &fa);
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
