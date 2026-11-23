/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the Bluetooth Mesh BLOB flash stream API.
 * @ingroup bt_mesh_blob_io_flash
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_BLOB_IO_FLASH_H__
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_BLOB_IO_FLASH_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup bt_mesh_blob_io_flash Bluetooth Mesh BLOB flash stream
 * @ingroup bt_mesh
 * @{
 */

/** BLOB flash stream. */
struct bt_mesh_blob_io_flash {
	/** Flash area ID to write the BLOB to. */
	uint8_t area_id;
	/** Active stream mode. */
	enum bt_mesh_blob_io_mode mode;
	/** Offset into the flash area to place the BLOB at (in bytes). */
	off_t offset;


	/* Internal flash area pointer. */
	const struct flash_area *area;
	/* BLOB stream. */
	struct bt_mesh_blob_io io;

#if defined(CONFIG_BT_MESH_BLOB_IO_FLASH_BLOCK_CACHE)
	/* Block cache used when CONFIG_BT_MESH_BLOB_IO_FLASH_BLOCK_CACHE is set.
	 * Received chunks are accumulated here by block-relative offset, and the
	 * whole block is programmed write-block-aligned, exactly once, by the
	 * wr() call that completes the block. This prevents re-programming a
	 * write block that is shared by two adjacent chunks, which would corrupt
	 * the ECC syndrome on flash that allows only one program per erase.
	 */
	/* Staging buffer for the current block. */
	uint8_t block_buf[CONFIG_BT_MESH_BLOB_BLOCK_SIZE_MAX];
	/* Number of valid bytes received into block_buf for the current block;
	 * reaches the block size when the block is complete and gets flushed.
	 */
	size_t received;
	/* Sticky error: set when a block flush fails, so that no later wr() can
	 * re-program a write block. Cleared only in io_open (NOT block_start,
	 * which does not always erase); a failed flush latches for the rest of
	 * the stream and the whole transfer fails.
	 */
	bool block_err;
#endif /* CONFIG_BT_MESH_BLOB_IO_FLASH_BLOCK_CACHE */
};

/** @brief Initialize a flash stream.
 *
 *  @param flash   Flash stream.
 *  @param area_id Flash partition identifier. See @ref flash_area_open.
 *  @param offset  Offset into the flash area, in bytes.
 *
 *  @return 0 on success or (negative) error code otherwise.
 */
int bt_mesh_blob_io_flash_init(struct bt_mesh_blob_io_flash *flash,
			       uint8_t area_id, off_t offset);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_BLOB_IO_FLASH_H__ */
