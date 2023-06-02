/*
 * Copyright (c) 2018-2021 mcumgr authors
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_IMG_MGMT_
#define H_IMG_MGMT_

#include <inttypes.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <bootutil/image.h>
#include <zcbor_common.h>

/**
 * @brief MCUmgr img_mgmt API
 * @defgroup mcumgr_img_mgmt MCUmgr img_mgmt API
 * @ingroup mcumgr
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#define IMG_MGMT_DATA_SHA_LEN	32 /* SHA256 */

/**
 * Image state flags
 */
#define IMG_MGMT_STATE_F_PENDING	0x01
#define IMG_MGMT_STATE_F_CONFIRMED	0x02
#define IMG_MGMT_STATE_F_ACTIVE		0x04
#define IMG_MGMT_STATE_F_PERMANENT	0x08

/* 255.255.65535.4294967295\0 */
#define IMG_MGMT_VER_MAX_STR_LEN	(sizeof("255.255.65535.4294967295"))

/**
 * Swap Types for image management state machine
 */
#define IMG_MGMT_SWAP_TYPE_NONE		0
#define IMG_MGMT_SWAP_TYPE_TEST		1
#define IMG_MGMT_SWAP_TYPE_PERM		2
#define IMG_MGMT_SWAP_TYPE_REVERT	3
#define IMG_MGMT_SWAP_TYPE_UNKNOWN	255

/**
 * Command IDs for image management group.
 */
#define IMG_MGMT_ID_STATE	0
#define IMG_MGMT_ID_UPLOAD	1
#define IMG_MGMT_ID_FILE	2
#define IMG_MGMT_ID_CORELIST	3
#define IMG_MGMT_ID_CORELOAD	4
#define IMG_MGMT_ID_ERASE	5

/**
 * Command result codes for image management group.
 */
enum img_mgmt_err_code_t {
	/** No error, this is implied if there is no ret value in the response */
	IMG_MGMT_ERR_OK = 0,

	/** Unknown error occurred. */
	IMG_MGMT_ERR_UNKNOWN,

	/** Failed to query flash area configuration. */
	IMG_MGMT_ERR_FLASH_CONFIG_QUERY_FAIL,

	/** There is no image in the slot. */
	IMG_MGMT_ERR_NO_IMAGE,

	/** The image in the slot has no TLVs (tag, length, value). */
	IMG_MGMT_ERR_NO_TLVS,

	/** The image in the slot has an invalid TLV type and/or length. */
	IMG_MGMT_ERR_INVALID_TLV,

	/** The image in the slot has multiple hash TLVs, which is invalid. */
	IMG_MGMT_ERR_TLV_MULTIPLE_HASHES_FOUND,

	/** The image in the slot has an invalid TLV size. */
	IMG_MGMT_ERR_TLV_INVALID_SIZE,

	/** The image in the slot does not have a hash TLV, which is required.  */
	IMG_MGMT_ERR_HASH_NOT_FOUND,

	/** There is no free slot to place the image. */
	IMG_MGMT_ERR_NO_FREE_SLOT,

	/** Flash area opening failed. */
	IMG_MGMT_ERR_FLASH_OPEN_FAILED,

	/** Flash area reading failed. */
	IMG_MGMT_ERR_FLASH_READ_FAILED,

	/** Flash area writing failed. */
	IMG_MGMT_ERR_FLASH_WRITE_FAILED,

	/** Flash area erase failed. */
	IMG_MGMT_ERR_FLASH_ERASE_FAILED,

	/** The provided slot is not valid. */
	IMG_MGMT_ERR_INVALID_SLOT,

	/** Insufficient heap memory (malloc failed). */
	IMG_MGMT_ERR_NO_FREE_MEMORY,

	/** The flash context is already set. */
	IMG_MGMT_ERR_FLASH_CONTEXT_ALREADY_SET,

	/** The flash context is not set. */
	IMG_MGMT_ERR_FLASH_CONTEXT_NOT_SET,

	/** The device for the flash area is NULL. */
	IMG_MGMT_ERR_FLASH_AREA_DEVICE_NULL,

	/** The offset for a page number is invalid. */
	IMG_MGMT_ERR_INVALID_PAGE_OFFSET,

	/** The offset parameter was not provided and is required. */
	IMG_MGMT_ERR_INVALID_OFFSET,

	/** The length parameter was not provided and is required. */
	IMG_MGMT_ERR_INVALID_LENGTH,

	/** The image length is smaller than the size of an image header. */
	IMG_MGMT_ERR_INVALID_IMAGE_HEADER,

	/** The image header magic value does not match the expected value. */
	IMG_MGMT_ERR_INVALID_IMAGE_HEADER_MAGIC,

	/** The hash parameter provided is not valid. */
	IMG_MGMT_ERR_INVALID_HASH,

	/** The image load address does not match the address of the flash area. */
	IMG_MGMT_ERR_INVALID_FLASH_ADDRESS,

	/** Failed to get version of currently running application. */
	IMG_MGMT_ERR_VERSION_GET_FAILED,

	/** The currently running application is newer than the version being uploaded. */
	IMG_MGMT_ERR_CURRENT_VERSION_IS_NEWER,

	/** There is already an image operating pending. */
	IMG_MGMT_ERR_IMAGE_ALREADY_PENDING,

	/** The image vector table is invalid. */
	IMG_MGMT_ERR_INVALID_IMAGE_VECTOR_TABLE,

	/** The image it too large to fit. */
	IMG_MGMT_ERR_INVALID_IMAGE_TOO_LARGE,

	/** The amount of data sent is larger than the provided image size. */
	IMG_MGMT_ERR_INVALID_IMAGE_DATA_OVERRUN,

	/** Confirmation of image has been denied */
	IMG_MGMT_ERR_IMAGE_CONFIRMATION_DENIED,

	/** Setting test to active slot is not allowed */
	IMG_MGMT_ERR_IMAGE_SETTING_TEST_TO_ACTIVE_DENIED,
};

/**
 * IMG_MGMT_ID_UPLOAD statuses.
 */
enum img_mgmt_id_upload_t {
	IMG_MGMT_ID_UPLOAD_STATUS_START		= 0,
	IMG_MGMT_ID_UPLOAD_STATUS_ONGOING,
	IMG_MGMT_ID_UPLOAD_STATUS_COMPLETE,
};

extern int boot_current_slot;
extern struct img_mgmt_state g_img_mgmt_state;

/** Represents an individual upload request. */
struct img_mgmt_upload_req {
	uint32_t image;	/* 0 by default */
	size_t off;	/* SIZE_MAX if unspecified */
	size_t size;	/* SIZE_MAX if unspecified */
	struct zcbor_string img_data;
	struct zcbor_string data_sha;
	bool upgrade;			/* Only allow greater version numbers. */
};

/** Global state for upload in progress. */
struct img_mgmt_state {
	/** Flash area being written; -1 if no upload in progress. */
	int area_id;
	/** Flash offset of next chunk. */
	size_t off;
	/** Total size of image data. */
	size_t size;
	/** Hash of image data; used for resumption of a partial upload. */
	uint8_t data_sha_len;
	uint8_t data_sha[IMG_MGMT_DATA_SHA_LEN];
};

/** Describes what to do during processing of an upload request. */
struct img_mgmt_upload_action {
	/** The total size of the image. */
	unsigned long long size;
	/** The number of image bytes to write to flash. */
	int write_bytes;
	/** The flash area to write to. */
	int area_id;
	/** Whether to process the request; false if offset is wrong. */
	bool proceed;
	/** Whether to erase the destination flash area. */
	bool erase;
#ifdef CONFIG_MCUMGR_GRP_IMG_VERBOSE_ERR
	/** "rsn" string to be sent as explanation for "rc" code */
	const char *rc_rsn;
#endif
};

/*
 * @brief Read info of an image at the specified slot number
 *
 * @param image_slot	image slot number
 * @param ver		output buffer for image version
 * @param hash		output buffer for image hash
 * @param flags		output buffer for image flags
 *
 * @return 0 on success, non-zero on failure.
 */
int img_mgmt_read_info(int image_slot, struct image_version *ver, uint8_t *hash, uint32_t *flags);

/**
 * @brief Get the image version of the currently running application.
 *
 * @param ver		output buffer for an image version information object.
 *
 * @return 0 on success, non-zero on failure.
 */
int img_mgmt_my_version(struct image_version *ver);

/**
 * @brief Format version string from struct image_version
 *
 * @param ver		pointer to image_version object
 * @param dst		output buffer for image version string
 *
 * @return Non-negative on success, negative value on error.
 */
int img_mgmt_ver_str(const struct image_version *ver, char *dst);

/**
 * @brief Get active, running application slot number for an image
 *
 * @param image		image number to get active slot for.
 *
 * @return Non-negative slot number
 */
int img_mgmt_active_slot(int image);

/**
 * @brief Get active image number
 *
 * Gets 0 based number for running application.
 *
 * @return Non-negative image number.
 */
int img_mgmt_active_image(void);

/**
 * @brief Check if the image slot is in use.
 *
 * The check is based on MCUboot flags, not image contents. This means that
 * slot with image in it, but no bootable flags set, is considered empty.
 * Active slot is always in use.
 *
 * @param slot		slot number
 *
 * @return 0 if slot is not used, non-0 otherwise.
 */
int img_mgmt_slot_in_use(int slot);

/**
 * @brief Check if any slot is in MCUboot pending state.
 *
 * Function returns 1 if slot 0 or slot 1 is in MCUboot pending state,
 * which means that it has been either marked for test or confirmed.
 *
 * @return 1 if there's pending DFU otherwise 0.
 */
int img_mgmt_state_any_pending(void);

/**
 * @brief Returns state flags set to slot.
 *
 * Flags are translated from MCUboot image state flags.
 * Returned value is zero if no flags are set or a combination of:
 *  IMG_MGMT_STATE_F_PENDING
 *  IMG_MGMT_STATE_F_CONFIRMED
 *  IMG_MGMT_STATE_F_ACTIVE
 *  IMG_MGMT_STATE_F_PERMANENT
 *
 * @param query_slot	slot number
 *
 * @return return the state flags.
 *
 */
uint8_t img_mgmt_state_flags(int query_slot);

/**
 * @brief Sets the pending flag for the specified image slot.
 *
 * Sets specified image slot to be used as active slot during next boot,
 * either for test or permanently. Non-permanent image will be reverted
 * unless image confirms itself during next boot.
 *
 * @param slot		slot number
 * @param permanent	permanent or test only
 *
 * @return 0 on success, non-zero on failure
 */
int img_mgmt_state_set_pending(int slot, int permanent);

/**
 * @brief Confirms the current image state.
 *
 * Prevents a fallback from occurring on the next reboot if the active image
 * is currently being tested.
 *
 * @return 0 on success, non-zero on failure
 */
int img_mgmt_state_confirm(void);

/**
 * Compares two image version numbers in a semver-compatible way.
 *
 * @param a	The first version to compare
 * @param b	The second version to compare
 *
 * @return	-1 if a < b
 * @return	0 if a = b
 * @return	1 if a > b
 */
int img_mgmt_vercmp(const struct image_version *a, const struct image_version *b);

#if IS_ENABLED(CONFIG_MCUMGR_GRP_IMG_MUTEX)
/*
 * @brief	Will reset the image management state back to default (no ongoing upload),
 *		requires that CONFIG_MCUMGR_GRP_IMG_MUTEX be enabled to allow for mutex
 *		locking of the image management state object.
 */
void img_mgmt_reset_upload(void);
#endif

#ifdef CONFIG_MCUMGR_GRP_IMG_VERBOSE_ERR
#define IMG_MGMT_UPLOAD_ACTION_SET_RC_RSN(action, rsn) ((action)->rc_rsn = (rsn))
#define IMG_MGMT_UPLOAD_ACTION_RC_RSN(action) ((action)->rc_rsn)
int img_mgmt_error_rsp(struct smp_streamer *ctxt, int rc, const char *rsn);
extern const char *img_mgmt_err_str_app_reject;
extern const char *img_mgmt_err_str_hdr_malformed;
extern const char *img_mgmt_err_str_magic_mismatch;
extern const char *img_mgmt_err_str_no_slot;
extern const char *img_mgmt_err_str_flash_open_failed;
extern const char *img_mgmt_err_str_flash_erase_failed;
extern const char *img_mgmt_err_str_flash_write_failed;
extern const char *img_mgmt_err_str_downgrade;
extern const char *img_mgmt_err_str_image_bad_flash_addr;
extern const char *img_mgmt_err_str_image_too_large;
extern const char *img_mgmt_err_str_data_overrun;
#else
#define IMG_MGMT_UPLOAD_ACTION_SET_RC_RSN(action, rsn)
#define IMG_MGMT_UPLOAD_ACTION_RC_RSN(action) NULL
#endif

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* H_IMG_MGMT_ */
