/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/mesh.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/drivers/flash.h>
#include <assert.h>
#include "blob.h"
#include "net.h"
#include "transport.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_blob_io_flash);

#define FLASH_IO(_io) CONTAINER_OF(_io, struct bt_mesh_blob_io_flash, io)

static int test_flash_area(uint8_t area_id)
{
	const struct flash_parameters *fparam;
	const struct flash_area *area;
	const struct device *fdev;
	uint8_t align;
	int err;

	err = flash_area_open(area_id, &area);
	if (err) {
		return err;
	}

	align = flash_area_align(area);
	fdev = flash_area_get_device(area);
	fparam = flash_get_parameters(fdev);

	flash_area_close(area);

	if (!fdev) {
		return -ENODEV;
	}

	if ((flash_params_get_erase_cap(fparam) & FLASH_ERASE_C_EXPLICIT) &&
	    CONFIG_BT_MESH_BLOB_IO_FLASH_WRITE_BLOCK_SIZE_MAX % align) {
		LOG_ERR("CONFIG_BT_MESH_BLOB_IO_FLASH_WRITE_BLOCK_SIZE_MAX must be set to a\n"
			"multiple of the write block size for the flash deviced used.");
		return -EINVAL;
	}

	return 0;
}

static int io_open(const struct bt_mesh_blob_io *io,
		   const struct bt_mesh_blob_xfer *xfer,
		   enum bt_mesh_blob_io_mode mode)
{
	struct bt_mesh_blob_io_flash *flash = FLASH_IO(io);

	flash->mode = mode;

	return flash_area_open(flash->area_id, &flash->area);
}

static void io_close(const struct bt_mesh_blob_io *io,
		     const struct bt_mesh_blob_xfer *xfer)
{
	struct bt_mesh_blob_io_flash *flash = FLASH_IO(io);

	flash_area_close(flash->area);
}

static inline int erase_device_block(const struct flash_area *fa, off_t start, size_t size)
{
	const struct device *fdev = flash_area_get_device(fa);
	struct flash_pages_info page;
	int err;

	if (!fdev) {
		return -ENODEV;
	}

	const struct flash_parameters *fparam = flash_get_parameters(fdev);

	/* If device has no erase requirement then do nothing */
	if (!(flash_params_get_erase_cap(fparam) & FLASH_ERASE_C_EXPLICIT)) {
		return 0;
	}

	err = flash_get_page_info_by_offs(fdev, start, &page);
	if (err) {
		return err;
	}

	if (start != page.start_offset) {
		/* Only need to erase when starting the first block on the page. */
		return 0;
	}

	/* Align to page boundary. */
	size = page.size * DIV_ROUND_UP(size, page.size);

	return flash_area_erase(fa, start, size);
}

static int block_start(const struct bt_mesh_blob_io *io,
		       const struct bt_mesh_blob_xfer *xfer,
		       const struct bt_mesh_blob_block *block)
{
	struct bt_mesh_blob_io_flash *flash = FLASH_IO(io);

	if (flash->mode == BT_MESH_BLOB_READ) {
		return 0;
	}

	return erase_device_block(flash->area, flash->offset + block->offset, block->size);
}

static int rd_chunk(const struct bt_mesh_blob_io *io,
		    const struct bt_mesh_blob_xfer *xfer,
		    const struct bt_mesh_blob_block *block,
		    const struct bt_mesh_blob_chunk *chunk)
{
	struct bt_mesh_blob_io_flash *flash = FLASH_IO(io);

	return flash_area_read(flash->area,
			       flash->offset + block->offset + chunk->offset,
			       chunk->data, chunk->size);
}

static int wr_chunk(const struct bt_mesh_blob_io *io,
		    const struct bt_mesh_blob_xfer *xfer,
		    const struct bt_mesh_blob_block *block,
		    const struct bt_mesh_blob_chunk *chunk)
{
	struct bt_mesh_blob_io_flash *flash = FLASH_IO(io);
	const struct device *fdev = flash_area_get_device(flash->area);

	if (!fdev) {
		return -ENODEV;
	}

	const struct flash_parameters *fparam = flash_get_parameters(fdev);

	/*
	 * If device has no erase requirement then write directly.
	 * This is required since trick with padding using the erase value will
	 * not work in this case.
	 */
	if (!(flash_params_get_erase_cap(fparam) & FLASH_ERASE_C_EXPLICIT)) {
		return flash_area_write(flash->area,
					flash->offset + block->offset + chunk->offset,
					chunk->data, chunk->size);
	}

	/*
	 * Allocate one additional write block for the case where a chunk will need
	 * an extra write block on both sides to fit.
	 */
	uint8_t buf[ROUND_UP(BLOB_RX_CHUNK_SIZE, CONFIG_BT_MESH_BLOB_IO_FLASH_WRITE_BLOCK_SIZE_MAX)
		    + CONFIG_BT_MESH_BLOB_IO_FLASH_WRITE_BLOCK_SIZE_MAX];
	uint32_t write_block_size = flash_area_align(flash->area);
	off_t area_offset = flash->offset + block->offset + chunk->offset;
	int start_pad = area_offset % write_block_size;

	/*
	 * Fill buffer with erase value, to make sure only the part of the
	 * buffer with chunk data will overwrite flash.
	 * (Because chunks can arrive in random order, this is required unless
	 *  the entire block is cached in RAM).
	 */
	memset(buf, fparam->erase_value, sizeof(buf));

	memcpy(&buf[start_pad], chunk->data, chunk->size);

	return flash_area_write(flash->area,
				ROUND_DOWN(area_offset, write_block_size),
				buf,
				ROUND_UP(start_pad + chunk->size, write_block_size));
}

int bt_mesh_blob_io_flash_init(struct bt_mesh_blob_io_flash *flash,
			       uint8_t area_id, off_t offset)
{
	int err;

	err = test_flash_area(area_id);
	if (err) {
		return err;
	}

	flash->area_id = area_id;
	flash->offset = offset;
	flash->io.open = io_open;
	flash->io.close = io_close;
	flash->io.block_start = block_start;
	flash->io.block_end = NULL;
	flash->io.rd = rd_chunk;
	flash->io.wr = wr_chunk;

	return 0;
}
