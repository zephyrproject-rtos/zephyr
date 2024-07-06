/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/bluetooth/mesh.h>

#include "mesh/blob.h"

#define SLOT1_PARTITION		slot1_partition
#define SLOT1_PARTITION_ID	FIXED_PARTITION_ID(SLOT1_PARTITION)
#define SLOT1_PARTITION_SIZE	FIXED_PARTITION_SIZE(SLOT1_PARTITION)
/* Chunk size is set to value that is not multiple of 4, to verify that chunks are written correctly
 * even if they are not aligned with word length used in flash
 */
#define CHUNK_SIZE		65

static struct bt_mesh_blob_io_flash blob_flash_stream;

static size_t chunk_size(const struct bt_mesh_blob_block *block,
				uint16_t chunk_idx)
{
	if ((chunk_idx == block->chunk_count - 1) &&
	    (block->size % CHUNK_SIZE)) {
		return block->size % CHUNK_SIZE;
	}

	return CHUNK_SIZE;
}

static uint8_t block_size_to_log(size_t size)
{
	uint8_t block_size_log = 0;

	while (size > 1) {
		size = size / 2;
		block_size_log++;
	}

	return block_size_log;
}

ZTEST_SUITE(blob_io_flash, NULL, NULL, NULL, NULL, NULL);

ZTEST(blob_io_flash, test_chunk_read)
{
	const struct flash_area *fa = NULL;
	struct bt_mesh_blob_xfer xfer;
	struct bt_mesh_blob_block block = { 0 };
	struct bt_mesh_blob_chunk chunk = { 0 };
	size_t remaining = SLOT1_PARTITION_SIZE;
	k_off_t block_idx = 0;
	uint16_t chunk_idx = 0;
	uint8_t chunk_data[CHUNK_SIZE];
	uint8_t test_data[SLOT1_PARTITION_SIZE];
	uint8_t ctrl_data[SLOT1_PARTITION_SIZE];
	size_t tests_data_offset = 0;
	int i, err;

	/* Fill test data with pattern */
	for (i = 0; i < SLOT1_PARTITION_SIZE; i++) {
		test_data[i] = i % 0xFF;
	}

	err = flash_area_open(SLOT1_PARTITION_ID, &fa);
	zassert_equal(err, 0, "Preparing test data failed with err=%d", err);

	err = flash_area_flatten(fa, 0, ARRAY_SIZE(ctrl_data));
	zassert_equal(err, 0, "Preparing test data failed with err=%d", err);

	err = flash_area_write(fa, 0, test_data, ARRAY_SIZE(ctrl_data));
	zassert_equal(err, 0, "Preparing test data failed with err=%d", err);

	err = flash_area_read(fa, 0, ctrl_data, ARRAY_SIZE(ctrl_data));
	zassert_equal(err, 0, "Preparing test data failed with err=%d", err);

	zassert_mem_equal(ctrl_data, test_data, ARRAY_SIZE(ctrl_data),
			  "Incorrect data written into flash");

	memset(ctrl_data, 0, SLOT1_PARTITION_SIZE);

	flash_area_close(fa);

	err = bt_mesh_blob_io_flash_init(&blob_flash_stream,
					 SLOT1_PARTITION_ID, 0);
	zassert_equal(err, 0, "BLOB I/O init failed with err=%d", err);

	err = blob_flash_stream.io.open(&blob_flash_stream.io, &xfer, BT_MESH_BLOB_READ);
	zassert_equal(err, 0, "BLOB I/O open failed with err=%d", err);

	chunk.data = chunk_data;

	/* Simulate reading whole partition divided into blocks and chunk of maximum sizes */
	while (remaining > 0) {
		block.chunk_count =
			DIV_ROUND_UP(CONFIG_BT_MESH_BLOB_BLOCK_SIZE_MAX,
					 CHUNK_SIZE);
		block.size = remaining > CONFIG_BT_MESH_BLOB_BLOCK_SIZE_MAX
				     ? CONFIG_BT_MESH_BLOB_BLOCK_SIZE_MAX
				     : remaining;

		/* BLOB Client should do nothing to flash area as it's in read mode */
		err = blob_flash_stream.io.block_start(&blob_flash_stream.io, &xfer, &block);
		zassert_equal(err, 0, "BLOB I/O open failed with err=%d", err);

		/* `block_start` in write mode will erase flash pages that can fit block.
		 * Assert that at least block size of data was not erased in read mode
		 */
		flash_area_read(blob_flash_stream.area, block.offset, ctrl_data, block.size);
		zassert_mem_equal(ctrl_data, &test_data[block.offset], block.size,
				  "Flash data was altered by `block_start` in read mode");

		memset(ctrl_data, 0, SLOT1_PARTITION_SIZE);

		block.offset = block_idx * (1 << block_size_to_log(block.size));

		for (i = 0; i < block.chunk_count; i++) {
			chunk.size = chunk_size(&block, chunk_idx);
			chunk.offset = CHUNK_SIZE * chunk_idx;

			err = blob_flash_stream.io.rd(&blob_flash_stream.io, &xfer, &block, &chunk);
			zassert_equal(err, 0, "BLOB I/O read failed with err=%d off=%d len=%d",
				      err, block.offset + chunk.offset, chunk.size);

			zassert_mem_equal(&chunk_data, &test_data[tests_data_offset], chunk.size,
					  "Incorrect data written into flash");
			chunk_idx++;

			remaining -= chunk.size;
			tests_data_offset += chunk.size;
		}
		block_idx++;
		chunk_idx = 0;
	}

	/* We read whole sector as BLOB. Try to increment every offset by one and read,
	 * which should attempt to read outside flash area
	 */
	chunk.offset++;
	err = blob_flash_stream.io.rd(&blob_flash_stream.io, &xfer, &block, &chunk);
	zassert_false(err == 0, "Read outside flash area successful");

	chunk.offset--;
	block.offset++;
	err = blob_flash_stream.io.rd(&blob_flash_stream.io, &xfer, &block, &chunk);
	zassert_false(err == 0, "Read outside flash area successful");

	block.offset--;
	blob_flash_stream.offset++;
	err = blob_flash_stream.io.rd(&blob_flash_stream.io, &xfer, &block, &chunk);
	zassert_false(err == 0, "Read outside flash area successful");

	blob_flash_stream.io.close(&blob_flash_stream.io, &xfer);
}

ZTEST(blob_io_flash, test_chunk_write)
{
	struct bt_mesh_blob_xfer xfer;
	struct bt_mesh_blob_block block = { 0 };
	struct bt_mesh_blob_chunk chunk = { 0 };
	size_t remaining = SLOT1_PARTITION_SIZE;
	k_off_t block_idx = 0;
	uint16_t chunk_idx = 0;
	uint8_t chunk_data[CHUNK_SIZE];
	/* 3 is maximum length of padding at the end of written chunk */
	uint8_t chunk_ctrl_data[CHUNK_SIZE + 3];
	uint8_t end_padding_len;
	uint8_t test_data[SLOT1_PARTITION_SIZE];
	uint8_t erased_block_data[CONFIG_BT_MESH_BLOB_BLOCK_SIZE_MAX];
	uint8_t ctrl_data[SLOT1_PARTITION_SIZE];
	size_t tests_data_offset = 0;
	int i, j, err;

	/* Fill test data with pattern */
	for (i = 0; i < SLOT1_PARTITION_SIZE; i++) {
		test_data[i] = i % 0xFF;
	}

	err = bt_mesh_blob_io_flash_init(&blob_flash_stream,
					 SLOT1_PARTITION_ID, 0);
	zassert_equal(err, 0, "BLOB I/O init failed with err=%d", err);

	err = blob_flash_stream.io.open(&blob_flash_stream.io, &xfer, BT_MESH_BLOB_WRITE);
	zassert_equal(err, 0, "BLOB I/O open failed with err=%d", err);

	chunk.data = chunk_data;

	memset(erased_block_data, flash_area_erased_val(blob_flash_stream.area),
	       CONFIG_BT_MESH_BLOB_BLOCK_SIZE_MAX);
	/* Simulate writing whole partition divided into blocks and chunk of maximum sizes */
	while (remaining > 0) {
		block.chunk_count =
			DIV_ROUND_UP(CONFIG_BT_MESH_BLOB_BLOCK_SIZE_MAX,
					 CHUNK_SIZE);
		block.size = remaining > CONFIG_BT_MESH_BLOB_BLOCK_SIZE_MAX
				     ? CONFIG_BT_MESH_BLOB_BLOCK_SIZE_MAX
				     : remaining;
		block.offset = block_idx * (1 << block_size_to_log(block.size));

		err = blob_flash_stream.io.block_start(&blob_flash_stream.io, &xfer, &block);
		zassert_equal(err, 0, "BLOB I/O open failed with err=%d", err);

		flash_area_read(blob_flash_stream.area, block.offset,
				ctrl_data, block.size);

		zassert_mem_equal(ctrl_data, erased_block_data, block.size,
				  "Flash data was not erased by `block_start` in write mode");

		memset(ctrl_data, 0, SLOT1_PARTITION_SIZE);

		for (i = 0; i < block.chunk_count; i++) {
			chunk.size = chunk_size(&block, chunk_idx);
			chunk.offset = CHUNK_SIZE * chunk_idx;

			memcpy(chunk.data,
			       &test_data[chunk.offset + block.offset],
			       chunk.size);

			err = blob_flash_stream.io.wr(&blob_flash_stream.io, &xfer, &block, &chunk);
			zassert_equal(err, 0, "BLOB I/O write failed with err=%d", err);

			/* To calculate end padding length we must calculate size of whole buffer
			 * and subtract start offset length and chunk size
			 */
			end_padding_len =
				ROUND_UP((block.offset + chunk.offset) %
					 flash_area_align(blob_flash_stream.area) +
					 chunk.size, flash_area_align(blob_flash_stream.area)) -
				(block.offset + chunk.offset) %
				flash_area_align(blob_flash_stream.area) - chunk.size;

			flash_area_read(blob_flash_stream.area, block.offset + chunk.offset,
					chunk_ctrl_data, chunk.size + end_padding_len);

			zassert_mem_equal(chunk_ctrl_data, chunk_data, chunk.size,
					  "Incorrect data written into flash");

			/* Assert that nothing was written into end padding */
			for (j = 1; j <= end_padding_len; j++) {
				zassert_equal(chunk_ctrl_data[chunk.size + j],
					      flash_area_erased_val(blob_flash_stream.area));
			}
			chunk_idx++;

			remaining -= chunk.size;
			tests_data_offset += chunk.size;
		}

		block_idx++;
		chunk_idx = 0;
	}

	flash_area_read(blob_flash_stream.area, 0, ctrl_data, SLOT1_PARTITION_SIZE);
	zassert_mem_equal(ctrl_data, test_data, SLOT1_PARTITION_SIZE,
			  "Incorrect chunks written into flash");

	/* We wrote whole sector as BLOB. Try to increment every offset by one and write,
	 * which should attempt to write outside flash area
	 */
	chunk.offset++;
	err = blob_flash_stream.io.wr(&blob_flash_stream.io, &xfer, &block, &chunk);
	zassert_false(err == 0, "Write outside flash area successful");

	chunk.offset--;
	block.offset++;
	err = blob_flash_stream.io.wr(&blob_flash_stream.io, &xfer, &block, &chunk);
	zassert_false(err == 0, "Write outside flash area successful");

	block.offset--;
	blob_flash_stream.offset++;
	err = blob_flash_stream.io.wr(&blob_flash_stream.io, &xfer, &block, &chunk);
	zassert_false(err == 0, "Write outside flash area successful");

	blob_flash_stream.io.close(&blob_flash_stream.io, &xfer);
}
