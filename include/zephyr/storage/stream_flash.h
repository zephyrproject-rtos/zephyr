/*
 * Copyright (c) 2017, 2020 Nordic Semiconductor ASA
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for stream writes to flash
 */

#ifndef ZEPHYR_INCLUDE_STORAGE_STREAM_FLASH_H_
#define ZEPHYR_INCLUDE_STORAGE_STREAM_FLASH_H_

/**
 * @brief Abstraction over stream writes to flash
 *
 * @defgroup stream_flash Stream to flash interface
 * @since 2.3
 * @version 0.1.0
 * @ingroup storage_apis
 * @{
 */

#include <stdbool.h>
#include <zephyr/drivers/flash.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef stream_flash_callback_t
 *
 * @brief Signature for callback invoked after flash write completes.
 *
 * @details Functions of this type are invoked with a buffer containing
 * data read back from the flash after a flash write has completed.
 * This enables verifying that the data has been correctly stored (for
 * instance by using a SHA function). The write buffer 'buf' provided in
 * stream_flash_init is used as a read buffer for this purpose.
 *
 * @param buf Pointer to the data read.
 * @param len The length of the data read.
 * @param offset The offset the data was read from.
 */
typedef int (*stream_flash_callback_t)(uint8_t *buf, size_t len, size_t offset);

/**
 * @brief Structure for stream flash context
 *
 * Users should treat these structures as opaque values and only interact
 * with them through the below API.
 */
struct stream_flash_ctx {
	uint8_t *buf; /* Write buffer */
	size_t buf_len; /* Length of write buffer */
	size_t buf_bytes; /* Number of bytes currently stored in write buf */
	const struct device *fdev; /* Flash device */
	size_t bytes_written; /* Number of bytes written to flash */
	size_t offset; /* Offset from base of flash device to write area */
	size_t available; /* Available bytes in write area */
	stream_flash_callback_t callback; /* Callback invoked after write op */
#ifdef CONFIG_STREAM_FLASH_ERASE
	off_t last_erased_page_start_offset; /* Last erased offset */
#endif
};

/**
 * @brief Initialize context needed for stream writes to flash.
 *
 * @param ctx context to be initialized
 * @param fdev Flash device to operate on
 * @param buf Write buffer
 * @param buf_len Length of write buffer. Can not be larger than the page size.
 *                Must be multiple of the flash device write-block-size.
 * @param offset Offset within flash device to start writing to
 * @param size Number of bytes available for performing buffered write.
 *             If this is '0', the size will be set to the total size
 *             of the flash device minus the offset.
 * @param cb Callback to be invoked on completed flash write operations.
 *
 * @return non-negative on success, negative errno code on fail
 */
int stream_flash_init(struct stream_flash_ctx *ctx, const struct device *fdev,
		      uint8_t *buf, size_t buf_len, size_t offset, size_t size,
		      stream_flash_callback_t cb);
/**
 * @brief Read number of bytes written to the flash.
 *
 * @note api-tags: pre-kernel-ok isr-ok
 *
 * @param ctx context
 *
 * @return Number of payload bytes written to flash.
 */
size_t stream_flash_bytes_written(struct stream_flash_ctx *ctx);

/**
 * @brief  Process input buffers to be written to flash device in single blocks.
 * Will store remainder between calls.
 *
 * A final call to this function with flush set to true
 * will write out the remaining block buffer to flash.
 *
 * @param ctx context
 * @param data data to write
 * @param len Number of bytes to write
 * @param flush when true this forces any buffered data to be written to flash
 *        A flush write should be the last write operation in a sequence of
 *        write operations for given context (although this is not mandatory
 *        if the total data size is a multiple of the buffer size).
 *
 * @return non-negative on success, negative errno code on fail
 */
int stream_flash_buffered_write(struct stream_flash_ctx *ctx, const uint8_t *data,
				size_t len, bool flush);

/**
 * @brief Erase the flash page to which a given offset belongs.
 *
 * This function erases a flash page to which an offset belongs if this page
 * is not the page previously erased by the provided ctx
 * (ctx->last_erased_page_start_offset).
 *
 * @param ctx context
 * @param off offset from the base address of the flash device
 *
 * @return non-negative on success, negative errno code on fail
 */
int stream_flash_erase_page(struct stream_flash_ctx *ctx, off_t off);

/**
 * @brief Load persistent stream write progress stored with key
 *        @p settings_key .
 *
 * This function should be called directly after @ref stream_flash_init to
 * load previous stream write progress before writing any data. If the loaded
 * progress has fewer bytes written than @p ctx then it will be ignored.
 *
 * @param ctx context
 * @param settings_key key to use with the settings module for loading
 *                     the stream write progress
 *
 * @return non-negative on success, negative errno code on fail
 */
int stream_flash_progress_load(struct stream_flash_ctx *ctx,
			       const char *settings_key);

/**
 * @brief Save persistent stream write progress using key @p settings_key .
 *
 * @param ctx context
 * @param settings_key key to use with the settings module for storing
 *                     the stream write progress
 *
 * @return non-negative on success, negative errno code on fail
 */
int stream_flash_progress_save(struct stream_flash_ctx *ctx,
			       const char *settings_key);

/**
 * @brief Clear persistent stream write progress stored with key
 *        @p settings_key .
 *
 * @param ctx context
 * @param settings_key key previously used for storing the stream write progress
 *
 * @return non-negative on success, negative errno code on fail
 */
int stream_flash_progress_clear(struct stream_flash_ctx *ctx,
				const char *settings_key);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_STORAGE_STREAM_FLASH_H_ */
