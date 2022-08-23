/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Declares implementation-specific functions required by image management.  The default
 * stubs can be overridden with functions that are compatible with the host OS.
 */

#ifndef H_IMG_MGMT_IMPL_
#define H_IMG_MGMT_IMPL_

#include <stdbool.h>
#include <inttypes.h>
#include "img_mgmt/img_mgmt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Ensures the spare slot is fully erased.
 *
 * @param slot		The slot to erase.  In the typical use case, this is 1.
 *
 * @return 0 on success, MGMT_ERR_[...] code on failure.
 */
int img_mgmt_impl_erase_slot(int slot);

/**
 * @brief Marks the image in the specified slot as pending. On the next reboot,
 * the system will perform a boot of the specified image.
 *
 * @param slot		The slot to mark as pending.  In the typical use case, this is 1.
 * @param permanent	Whether the image should be used permanently or only tested once:
 *				0=run image once, then confirm or revert.
 *				1=run image forever.
 *
 * @return 0 on success, MGMT_ERR_[...] code on failure.
 */
int img_mgmt_impl_write_pending(int slot, bool permanent);

/**
 * @brief Marks the image in slot 0 as confirmed. The system will continue
 * booting into the image in slot 0 until told to boot from a different slot.
 *
 * @return 0 on success, MGMT_ERR_[...] code on failure.
 */
int img_mgmt_impl_write_confirmed(void);

/**
 * @brief Reads the specified chunk of data from an image slot.
 *
 * @param slot		The index of the slot to read from.
 * @param offset	The offset within the slot to read from.
 * @param dst		On success, the read data gets written here.
 * @param num_bytes	The number of bytes to read.
 *
 * @return 0 on success, MGMT_ERR_[...] code on failure.
 */
int img_mgmt_impl_read(int slot, unsigned int offset, void *dst, unsigned int num_bytes);

/**
 * @brief Writes the specified chunk of image data to slot 1.
 *
 * @param offset	The offset within slot 1 to write to.
 * @param data		The image data to write.
 * @param num_bytes	The number of bytes to read.
 * @param last		Whether this chunk is the end of the image:
 *				false=additional image chunks are forthcoming.
 *				true=last image chunk; flush unwritten data to disk.
 *
 * @return 0 on success, MGMT_ERR_[...] code on failure.
 */
int img_mgmt_impl_write_image_data(unsigned int offset, const void *data, unsigned int num_bytes,
				   bool last);

/**
 * @brief Indicates the type of swap operation that will occur on the next
 * reboot, if any, between provided slot and it's pair.
 * Querying any slots of the same pair will give the same result.
 *
 * @param image                 An slot number;
 * @return                      An IMG_MGMT_SWAP_TYPE_[...] code.
 */
int img_mgmt_impl_swap_type(int slot);

/**
 * Collects information about the specified image slot.
 *
 * @return Flags of the specified image slot
 */
uint8_t img_mgmt_state_flags(int query_slot);

/**
 * Erases image data at given offset
 *
 * @param offset	The offset within slot 1 to erase at.
 * @param num_bytes	The number of bytes to erase.
 *
 * @return 0 on success, MGMT_ERR_[...] code on failure.
 */
int img_mgmt_impl_erase_image_data(unsigned int off, unsigned int num_bytes);

/**
 * Erases a flash sector as image upload crosses a sector boundary.
 * Erasing the entire flash size at one time can take significant time,
 *   causing a bluetooth disconnect or significant battery sag.
 * Instead we will erase immediately prior to crossing a sector.
 * We could check for empty to increase efficiency, but instead we always erase
 *   for consistency and simplicity.
 *
 * @param off		Offset that is about to be written
 * @param len		Number of bytes to be written
 *
 * @return 0 if success ERROR_CODE if could not erase sector
 */
int img_mgmt_impl_erase_if_needed(uint32_t off, uint32_t len);

/**
 * Verifies an upload request and indicates the actions that should be taken
 * during processing of the request.  This is a "read only" function in the
 * sense that it doesn't write anything to flash and doesn't modify any global
 * variables.
 *
 * @param req		The upload request to inspect.
 * @param action	On success, gets populated with information about how to process the
 *			request.
 *
 * @return 0 if processing should occur;A MGMT_ERR code if an error response should be sent instead.
 */
int img_mgmt_impl_upload_inspect(const struct img_mgmt_upload_req *req,
				 struct img_mgmt_upload_action *action);

#define ERASED_VAL_32(x) (((x) << 24) | ((x) << 16) | ((x) << 8) | (x))
int img_mgmt_impl_erased_val(int slot, uint8_t *erased_val);

int img_mgmt_impl_log_upload_start(int status);

int img_mgmt_impl_log_upload_done(int status, const uint8_t *hashp);

int img_mgmt_impl_log_pending(int status, const uint8_t *hash);

int img_mgmt_impl_log_confirm(int status, const uint8_t *hash);

#ifdef __cplusplus
}
#endif

#endif
