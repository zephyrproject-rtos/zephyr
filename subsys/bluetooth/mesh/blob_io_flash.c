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

#define WRITE_BLOCK_SIZE DT_PROP(DT_INST(0, soc_nv_flash), write_block_size)

#define FLASH_IO(_io) CONTAINER_OF(_io, struct bt_mesh_blob_io_flash, io)

static int test_flash_area(uint8_t area_id)
{
	const struct flash_area *area;
	uint8_t align;
	int err;

	err = flash_area_open(area_id, &area);
	if (err) {
		return err;
	}

	align = flash_area_align(area);
	flash_area_close(area);

	if (align > WRITE_BLOCK_SIZE) {
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
	/* If there are no devices requiring erase, then there is nothing to do */
	if (IS_ENABLED(CONFIG_BT_MESH_BLOB_IO_FLASH_WITH_ERASE)) {
		const struct device *fdev = flash_area_get_device(fa);

		if (!fdev) {
			return -ENODEV;
		}

		/* We have a mix of devices in system */
		if (IS_ENABLED(CONFIG_BT_MESH_BLOB_IO_FLASH_WITHOUT_ERASE)) {
			const struct flash_parameters *fparam = flash_get_parameters(fdev);

			/* If device has no erase requirement then do nothing */
			if (!(flash_params_get_erase_cap(fparam) & FLASH_ERASE_C_EXPLICIT)) {
				return 0;
			}
		}

		if (IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)) {
			struct flash_pages_info page;
			int err;

			err = flash_get_page_info_by_offs(fdev, start, &page);
			if (err) {
				return err;
			}

			size = page.size * DIV_ROUND_UP(size, page.size);
			start = page.start_offset;
		}
		return flash_area_erase(fa, start, size);
	}

	return 0;
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

	if (IS_ENABLED(CONFIG_SOC_FLASH_NRF_RRAM)) {
		return flash_area_write(flash->area,
					flash->offset + block->offset + chunk->offset,
					chunk->data, chunk->size);
	}

	uint8_t buf[ROUND_UP(BLOB_RX_CHUNK_SIZE, WRITE_BLOCK_SIZE)];
	off_t area_offset = flash->offset + block->offset + chunk->offset;
	int i = 0;

	/* Write block align the chunk data */
	memset(&buf[i], 0xff, area_offset % WRITE_BLOCK_SIZE);
	i += area_offset % WRITE_BLOCK_SIZE;

	memcpy(&buf[i], chunk->data, chunk->size);
	i += chunk->size;

	memset(&buf[i], 0xff, ROUND_UP(i, WRITE_BLOCK_SIZE) - i);
	i = ROUND_UP(i, WRITE_BLOCK_SIZE);

	return flash_area_write(flash->area,
				ROUND_DOWN(area_offset, WRITE_BLOCK_SIZE),
				buf, i);
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
