/*
 * Copyright (c) 2026 Clovis Corde
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file delta.h
 * @brief Delta Firmware Update API
 */
#ifndef ZEPHYR_INCLUDE_MGMT_DELTA_H_
#define ZEPHYR_INCLUDE_MGMT_DELTA_H_

#include <zephyr/kernel.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/dfu/flash_img.h>
#include <zephyr/mgmt/delta/delta_algorithm.h>

/**
 * @brief Delta image slot type.
 *
 * Identifies which flash partition to use for source/target operations.
 */
enum delta_image_slot {
	/** Primary slot containing the current firmware */
	DELTA_IMAGE_SLOT_0 = PARTITION_ID(slot0_partition),
	/** Secondary slot for the new firmware */
	DELTA_IMAGE_SLOT_1 = PARTITION_ID(slot1_partition),
};

/**
 * @brief Patch storage backend type.
 *
 * Identifies where the delta patch is stored.
 */
enum delta_patch_storage {
	/** Patch stored in flash memory */
	DELTA_PATCH_STORAGE_FLASH,
	/** Patch stored in RAM */
	DELTA_PATCH_STORAGE_RAM,
};

/**
 * @brief Structure to apply delta using flash memory.
 */
struct delta_flash_ctx {
	/** Flash area descriptor for patch storage */
	const struct flash_area *flash_area;
	/** Flash image write context */
	struct flash_img_context img_ctx;
};

/**
 * @brief Structure to apply delta using RAM memory.
 */
struct delta_ram_ctx {
	/** Buffer to store patch data */
	uint8_t *patch;
	/** Size of the patch buffer in bytes */
	size_t patch_size;
};

/**
 * @brief Offset tracking for delta operations.
 *
 * Tracks the current read/write positions in the source firmware,
 * target firmware, and patch data.
 */
struct delta_offset {
	/** Current offset in source firmware. May seek backward when ctrl[2] is negative. */
	uint64_t source;
	/** Current offset in target firmware */
	uint64_t target;
	/** Current offset in patch data */
	uint64_t patch;
};

/**
 * @brief Delta memory context.
 *
 * Contains all state needed for delta memory operations.
 * Must be zero-initialized before passing to delta_apply_patch_init().
 */
struct delta_memory {
	/** Offset tracking */
	struct delta_offset offset;
	/** Flash slot for source firmware reads */
	enum delta_image_slot slot;
	/** The storage backend used for the patch */
	enum delta_patch_storage patch_storage;
	/** Storage backend context (flash or RAM, mutually exclusive) */
	union {
		/** Flash memory context */
		struct delta_flash_ctx flash;
		/** RAM buffer context */
		struct delta_ram_ctx ram;
	};
};

/**
 * @brief Delta API context structure.
 *
 * This structure holds all the state needed to perform
 * a delta firmware update operation.
 */
struct delta_ctx {
	/** Backend algorithm implementation */
	const struct delta_backend_api *backend;
	/** Backend-specific configuration data */
	void *backend_data;
	/** Memory context */
	struct delta_memory memory;
	/** Size of the target firmware in bytes */
	int64_t target_size;
};

/**
 * @brief Read data from the source firmware (slot 0).
 *
 * Reads @p size bytes at the current source offset into @p buf.
 *
 * @param mem Pointer to delta memory context.
 * @param buf Buffer to read data into.
 * @param size Number of bytes to read.
 *
 * @retval 0 Success.
 * @retval -errno Negative error code on failure.
 */
int delta_mem_read_source(struct delta_memory *mem, uint8_t *buf, size_t size);

/**
 * @brief Read data from the patch storage (flash or RAM).
 *
 * Reads @p size bytes at the current patch offset into @p buf.
 * The storage backend is selected by @c mem->patch_storage.
 *
 * @param mem Pointer to delta memory context.
 * @param buf Buffer to read data into.
 * @param size Number of bytes to read.
 *
 * @retval 0 Success.
 * @retval -errno Negative error code on failure.
 */
int delta_mem_read_patch(struct delta_memory *mem, uint8_t *buf, size_t size);

/**
 * @brief Write data to the target firmware (slot 1).
 *
 * @param mem Pointer to delta memory context.
 * @param buf Buffer containing data to write.
 * @param size Number of bytes to write.
 * @param flush If true, flush buffered data to flash.
 *
 * @retval 0 Success.
 * @retval -errno Negative error code on failure.
 */
int delta_mem_write(struct delta_memory *mem, uint8_t *buf, size_t size, bool flush);

/**
 * @brief Update the current read/write positions.
 *
 * @param mem Pointer to delta memory context.
 * @param source_offset New offset in source firmware.
 * @param patch_offset New offset in patch data.
 * @param target_offset New offset in target firmware.
 *
 * @retval 0 Success.
 */
int delta_mem_seek(struct delta_memory *mem, size_t source_offset, size_t patch_offset,
		   size_t target_offset);

#endif /* ZEPHYR_INCLUDE_MGMT_DELTA_H_ */
