/*
 * Copyright (c) 2018-2021 mcumgr authors
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_IMG_MGMT_PRIV_
#define H_IMG_MGMT_PRIV_

#include <stdbool.h>
#include <inttypes.h>

#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_MCUBOOT_BOOTLOADER_USES_SHA512
#define IMAGE_TLV_SHA		IMAGE_TLV_SHA512
#define IMAGE_SHA_LEN		64
#else
#define IMAGE_TLV_SHA		IMAGE_TLV_SHA256
#define IMAGE_SHA_LEN		32
#endif

/**
 * @brief Ensures the spare slot (slot 1) is fully erased.
 *
 * @param slot		A slot to erase.
 *
 * @return 0 on success, MGMT_ERR_[...] code on failure.
 */
int img_mgmt_erase_slot(int slot);

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
int img_mgmt_write_pending(int slot, bool permanent);

/**
 * @brief Marks the image in slot 0 as confirmed. The system will continue
 * booting into the image in slot 0 until told to boot from a different slot.
 *
 * @return 0 on success, MGMT_ERR_[...] code on failure.
 */
int img_mgmt_write_confirmed(void);

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
int img_mgmt_read(int slot, unsigned int offset, void *dst, unsigned int num_bytes);

/**
 * @brief Writes the specified chunk of image data to slot 1.
 *
 * @param offset	The offset within slot 1 to write to.
 * @param data		The image data to write.
 * @param num_bytes	The number of bytes to write.
 * @param last		Whether this chunk is the end of the image:
 *				false=additional image chunks are forthcoming.
 *				true=last image chunk; flush unwritten data to disk.
 *
 * @return 0 on success, MGMT_ERR_[...] code on failure.
 */
int img_mgmt_write_image_data(unsigned int offset, const void *data, unsigned int num_bytes,
			      bool last);

/**
 * @brief Indicates the type of swap operation that will occur on the next
 * reboot, if any, between provided slot and it's pair.
 * Querying any slots of the same pair will give the same result.
 *
 * @param slot                  An slot number;
 *
 * @return                      An IMG_MGMT_SWAP_TYPE_[...] code.
 */
int img_mgmt_swap_type(int slot);

/**
 * @brief Returns image that the given slot belongs to.
 *
 * @param slot			A slot number.
 *
 * @return 0 based image number.
 */
static inline int img_mgmt_slot_to_image(int slot)
{
	__ASSERT(slot >= 0 && slot < (CONFIG_MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER << 1),
		 "Impossible slot number");

	return (slot >> 1);
}

/**
 * @brief Get slot number of alternate (inactive) image pair
 *
 * @param slot			A slot number.
 *
 * @return Number of other slot in pair
 */
static inline int img_mgmt_get_opposite_slot(int slot)
{
	__ASSERT(slot >= 0 && slot < (CONFIG_MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER << 1),
		 "Impossible slot number");

	return (slot ^ 1);
}

enum img_mgmt_next_boot_type {
	/** The normal boot to active or non-active slot */
	NEXT_BOOT_TYPE_NORMAL	=	0,
	/** The test/non-permanent boot to non-active slot */
	NEXT_BOOT_TYPE_TEST	=	1,
	/** Next boot will be revert to already confirmed slot; this
	 * type of next boot means that active slot is not confirmed
	 * yet as it has been marked for test in previous boot.
	 */
	NEXT_BOOT_TYPE_REVERT	=	2
};

/**
 * @brief Get next boot slot number for a given image.
 *
 * @param image			An image number.
 * @param type			Type of next boot
 *
 * @return Number of slot, from pair of slots assigned to image, that will
 * boot on next reset. User needs to compare this slot against active slot
 * to check whether application image will change for the next boot.
 * @return -1 in case when next boot slot can not be established.
 */
int img_mgmt_get_next_boot_slot(int image, enum img_mgmt_next_boot_type *type);

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
int img_mgmt_erase_image_data(unsigned int off, unsigned int num_bytes);

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
int img_mgmt_erase_if_needed(uint32_t off, uint32_t len);

/**
 * Verifies an upload request and indicates the actions that should be taken
 * during processing of the request.  This is a "read only" function in the
 * sense that it doesn't write anything to flash and doesn't modify any global
 * variables.
 *
 * @param req		The upload request to inspect.
 * @param action	On success, gets populated with information about how to
 *                      process the request.
 *
 * @return 0 if processing should occur;
 *           A MGMT_ERR code if an error response should be sent instead.
 */
int img_mgmt_upload_inspect(const struct img_mgmt_upload_req *req,
			    struct img_mgmt_upload_action *action);

/**
 * @brief	Takes the image management lock (if enabled) to prevent other
 *		threads interfering with an ongoing operation.
 */
void img_mgmt_take_lock(void);

/**
 * @brief	Releases the held image management lock (if enabled) to allow
 *		other threads to use image management operations.
 */
void img_mgmt_release_lock(void);

#define ERASED_VAL_32(x) (((x) << 24) | ((x) << 16) | ((x) << 8) | (x))
int img_mgmt_erased_val(int slot, uint8_t *erased_val);

int img_mgmt_find_by_hash(uint8_t *find, struct image_version *ver);
int img_mgmt_find_by_ver(struct image_version *find, uint8_t *hash);
int img_mgmt_state_read(struct smp_streamer *ctxt);
int img_mgmt_state_write(struct smp_streamer *njb);
int img_mgmt_flash_area_id(int slot);

#ifdef __cplusplus
}
#endif

#endif
