/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DFU_FLASH_IMG_H_
#define ZEPHYR_INCLUDE_DFU_FLASH_IMG_H_

#include <storage/stream_flash.h>

#ifdef __cplusplus
extern "C" {
#endif

struct flash_img_context {
	uint8_t buf[CONFIG_IMG_BLOCK_BUF_SIZE];
	const struct flash_area *flash_area;
	struct stream_flash_ctx stream;
};

/**
 * @brief Initialize context needed for writing the image to the flash.
 *
 * @param ctx     context to be initialized
 * @param area_id flash area id of partition where the image should be written
 *
 * @return  0 on success, negative errno code on fail
 */
int flash_img_init_id(struct flash_img_context *ctx, uint8_t area_id);

/**
 * @brief Initialize context needed for writing the image to the flash.
 *
 * @param ctx context to be initialized
 *
 * @return  0 on success, negative errno code on fail
 */
int flash_img_init(struct flash_img_context *ctx);

/**
 * @brief Read number of bytes of the image written to the flash.
 *
 * @param ctx context
 *
 * @return Number of bytes written to the image flash.
 */
size_t flash_img_bytes_written(struct flash_img_context *ctx);

/**
 * @brief  Process input buffers to be written to the image slot 1. flash
 * memory in single blocks. Will store remainder between calls.
 *
 * A final call to this function with flush set to true
 * will write out the remaining block buffer to flash. Since flash is written to
 * in blocks, the contents of flash from the last byte written up to the next
 * multiple of CONFIG_IMG_BLOCK_BUF_SIZE is padded with 0xff.
 *
 * @param ctx context
 * @param data data to write
 * @param len Number of bytes to write
 * @param flush when true this forces any buffered
 * data to be written to flash
 *
 * @return  0 on success, negative errno code on fail
 */
int flash_img_buffered_write(struct flash_img_context *ctx, const uint8_t *data,
		    size_t len, bool flush);

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_DFU_FLASH_IMG_H_ */
