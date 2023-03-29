/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @defgroup bt_mesh_blob_io_flash Bluetooth mesh BLOB flash stream
 * @{
 * @brief
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_BLOB_IO_FLASH_H__
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_BLOB_IO_FLASH_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_BLOB_IO_FLASH_H__ */

/** @} */
