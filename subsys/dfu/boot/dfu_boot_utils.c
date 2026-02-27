/*
 * Copyright (c) 2018-2021 mcumgr authors
 * Copyright (c) 2022-2025 Nordic Semiconductor ASA
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/dfu/dfu_boot.h>
#include <zephyr/logging/log.h>

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

#define ERASED_VAL_32(x) (((x) << 24) | ((x) << 16) | ((x) << 8) | (x))

/**
 * Callback function type for flash area operations
 *
 * @param fa    Pointer to the opened flash area
 * @param ctx   User context passed through from the caller
 *
 * @return 0 on success, negative errno code on failure
 */
typedef int (*flash_area_op_cb_t)(const struct flash_area *fa, void *ctx);

/* Context structure for flash operation */
struct read_ctx {
	size_t offset;
	void *dst;
	size_t len;
};

/**
 * Execute an operation on a flash area with automatic open/close handling
 *
 * @param area_id Flash area ID
 * @param cb    Callback function to execute with the opened flash area
 * @param ctx   User context to pass to the callback
 *
 * @return 0 on success, negative errno code on failure
 */
static int with_flash_area(int area_id, flash_area_op_cb_t cb, void *ctx)
{
	const struct flash_area *fa;
	int rc;

	rc = flash_area_open(area_id, &fa);
	if (rc != 0) {
		return -EIO;
	}

	rc = cb(fa, ctx);

	flash_area_close(fa);

	return rc;
}

/**
 * Determines if the specified area of flash is completely unwritten.
 *
 * @param fa    Pointer to flash area to scan
 *
 * @return 0 when not empty, 1 when empty, negative errno code on error.
 */
static int flash_check_empty(const struct flash_area *fa)
{
	uint32_t data[16];
	off_t end;
	int bytes_to_read;
	int rc;
	uint8_t erased_val;
	uint32_t erased_val_32;

	__ASSERT_NO_MSG(fa->fa_size % 4 == 0);

	erased_val = flash_area_erased_val(fa);
	erased_val_32 = ERASED_VAL_32(erased_val);

	end = fa->fa_size;
	for (off_t addr = 0; addr < end; addr += sizeof(data)) {
		if (end - addr < sizeof(data)) {
			bytes_to_read = end - addr;
		} else {
			bytes_to_read = sizeof(data);
		}

		rc = flash_area_read(fa, addr, data, bytes_to_read);
		if (rc < 0) {
			return rc;
		}

		for (int i = 0; i < bytes_to_read / 4; i++) {
			if (data[i] != erased_val_32) {
				return 0;
			}
		}
	}

	return 1;
}

static int check_empty_op(const struct flash_area *fa, void *ctx)
{
	ARG_UNUSED(ctx);

	return flash_check_empty(fa);
}

static int erase_op(const struct flash_area *fa, void *ctx)
{
	ARG_UNUSED(ctx);
	int rc;

	rc = flash_check_empty(fa);
	if (rc == 0) {
		/* Not empty, erase it */
		rc = flash_area_flatten(fa, 0, fa->fa_size);
		if (rc != 0) {
			return rc;
		}
	} else {
		if (rc == 1) {
			/* Already erased */
			rc = 0;
		}
	}

	return rc;
}

static int read_op(const struct flash_area *fa, void *ctx)
{
	struct read_ctx *read = ctx;
	int rc;

	rc = flash_area_read(fa, read->offset, read->dst, read->len);
	if (rc != 0) {
		return -EIO;
	}

	return 0;
}

static int get_erased_val_op(const struct flash_area *fa, void *ctx)
{
	uint8_t *erased_val = ctx;

	*erased_val = flash_area_erased_val(fa);

	return 0;
}

int dfu_boot_get_flash_area_id(int slot)
{
	switch (slot) {
	case 0:
		return PARTITION_ID(SLOT0_PARTITION);
	case 1:
		return PARTITION_ID(SLOT1_PARTITION);
#if !defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_FIRMWARE_UPDATER)
#if PARTITION_EXISTS(SLOT2_PARTITION)
	case 2:
		return PARTITION_ID(SLOT2_PARTITION);
#endif
#if PARTITION_EXISTS(SLOT3_PARTITION)
	case 3:
		return PARTITION_ID(SLOT3_PARTITION);
#endif
#if PARTITION_EXISTS(SLOT4_PARTITION)
	case 4:
		return PARTITION_ID(SLOT4_PARTITION);
#endif
#if PARTITION_EXISTS(SLOT5_PARTITION)
	case 5:
		return PARTITION_ID(SLOT5_PARTITION);
#endif
#if PARTITION_EXISTS(SLOT6_PARTITION)
	case 6:
		return PARTITION_ID(SLOT6_PARTITION);
#endif
#if PARTITION_EXISTS(SLOT7_PARTITION)
	case 7:
		return PARTITION_ID(SLOT7_PARTITION);
#endif
#if PARTITION_EXISTS(SLOT8_PARTITION)
	case 8:
		return PARTITION_ID(SLOT8_PARTITION);
#endif
#if PARTITION_EXISTS(SLOT9_PARTITION)
	case 9:
		return PARTITION_ID(SLOT9_PARTITION);
#endif
#if PARTITION_EXISTS(SLOT10_PARTITION)
	case 10:
		return PARTITION_ID(SLOT10_PARTITION);
#endif
#if PARTITION_EXISTS(SLOT11_PARTITION)
	case 11:
		return PARTITION_ID(SLOT11_PARTITION);
#endif
#if PARTITION_EXISTS(SLOT12_PARTITION)
	case 12:
		return PARTITION_ID(SLOT12_PARTITION);
#endif
#if PARTITION_EXISTS(SLOT13_PARTITION)
	case 13:
		return PARTITION_ID(SLOT13_PARTITION);
#endif
#if PARTITION_EXISTS(SLOT14_PARTITION)
	case 14:
		return PARTITION_ID(SLOT14_PARTITION);
#endif
#if PARTITION_EXISTS(SLOT15_PARTITION)
	case 15:
		return PARTITION_ID(SLOT15_PARTITION);
#endif
#endif /* !defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_FIRMWARE_UPDATER) */
	default:
		return -EINVAL;
	}
}

int dfu_boot_read(int slot, size_t offset, void *dst, size_t len)
{
	struct read_ctx ctx = {
		.offset = offset,
		.dst = dst,
		.len = len,
	};

	return with_flash_area(dfu_boot_get_flash_area_id(slot), read_op, &ctx);
}

int dfu_boot_get_erased_val(int slot, uint8_t *erased_val)
{
	return with_flash_area(dfu_boot_get_flash_area_id(slot), get_erased_val_op, erased_val);
}

int dfu_boot_flash_area_check_empty(int area_id)
{
	return with_flash_area(area_id, check_empty_op, NULL);
}

int dfu_boot_erase_slot(int slot)
{
	return with_flash_area(dfu_boot_get_flash_area_id(slot), erase_op, NULL);
}
