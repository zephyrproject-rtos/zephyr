/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __FLASH_IMG_H__
#define __FLASH_IMG_H__

#ifdef __cplusplus
extern "C" {
#endif

struct flash_img_context {
	u8_t buf[CONFIG_IMG_BLOCK_BUF_SIZE];
	struct device *dev;
	size_t bytes_written;
	u16_t buf_bytes;
};

/**
 * @brief Initialize context needed for writing the image to the flash.
 *
 * @param ctx context to be initialized
 * @param dev flash driver to used while writing the image
 */
void flash_img_init(struct flash_img_context *ctx, struct device *dev);

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
int flash_img_buffered_write(struct flash_img_context *ctx, u8_t *data,
		    size_t len, bool flush);

#ifdef __cplusplus
}
#endif

#endif	/* __FLASH_IMG_H__ */
