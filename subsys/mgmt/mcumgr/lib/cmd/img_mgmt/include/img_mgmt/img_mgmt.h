/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_IMG_MGMT_
#define H_IMG_MGMT_

#include <inttypes.h>
#include "img_mgmt_config.h"
#include "image.h"
#include "mgmt/mgmt.h"
#include <zcbor_common.h>

struct image_version;

#ifdef __cplusplus
extern "C" {
#endif

#define IMG_MGMT_HASH_STR	48
#define IMG_MGMT_HASH_LEN	32
#define IMG_MGMT_DATA_SHA_LEN	32 /* SHA256 */

/*
 * Image state flags
 */
#define IMG_MGMT_STATE_F_PENDING	0x01
#define IMG_MGMT_STATE_F_CONFIRMED	0x02
#define IMG_MGMT_STATE_F_ACTIVE		0x04
#define IMG_MGMT_STATE_F_PERMANENT	0x08

/* 255.255.65535.4294967295\0 */
#define IMG_MGMT_VER_MAX_STR_LEN	(sizeof("255.255.65535.4294967295"))

/*
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

/*
 * IMG_MGMT_ID_UPLOAD statuses.
 */
#define IMG_MGMT_ID_UPLOAD_STATUS_START		0
#define IMG_MGMT_ID_UPLOAD_STATUS_ONGOING	1
#define IMG_MGMT_ID_UPLOAD_STATUS_COMPLETE	2

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
} __packed;

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
} __packed;

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
#ifdef CONFIG_IMG_MGMT_VERBOSE_ERR
	/** "rsn" string to be sent as explanation for "rc" code */
	const char *rc_rsn;
#endif
} __packed;

/**
 * @brief Registers the image management command handler group.
 */
void img_mgmt_register_group(void);

/**
 * @brief Unregisters the image management command handler group.
 */
void img_mgmt_unregister_group(void);

/*
 * @brief Read info of an image give the slot number
 *
 * @param image_slot	 Image slot to read info from
 * @param image_version  Image version to be filled up
 * @param hash		   Ptr to the read image hash
 * @param flags		  Ptr to flags filled up from the image
 */
int img_mgmt_read_info(int image_slot, struct image_version *ver, uint8_t *hash, uint32_t *flags);

/**
 * @brief Get the current running image version
 *
 * @param image_version Given image version
 *
 * @return 0 on success, non-zero on failure
 */
int img_mgmt_my_version(struct image_version *ver);

/**
 * @brief Get image version in string from image_version
 *
 * @param image_version Structure filled with image version
 *					  information
 * @param dst		   Destination string created from the given
 *					  in image version
 *
 * @return Non-negative on success, negative value on error.
 */
int img_mgmt_ver_str(const struct image_version *ver, char *dst);

/**
 * @brief Check if the image slot is in use
 *
 * @param slot Slot to check if its in use
 *
 * @return 0 on success, non-zero on failure
 */
int img_mgmt_slot_in_use(int slot);

/**
 * @brief Check if the DFU status is pending
 *
 * @return 1 if there's pending DFU otherwise 0.
 */
int img_mgmt_state_any_pending(void);

/**
 * @brief Collects information about the specified image slot
 *
 * @param query_slot Slot to read state flags from
 *
 * @return return the state flags
 */
uint8_t img_mgmt_state_flags(int query_slot);

/**
 * @brief Sets the pending flag for the specified image slot.  That is, the system
 * will swap to the specified image on the next reboot.  If the permanent
 * argument is specified, the system doesn't require a confirm after the swap
 * occurs.
 *
 * @param slot	   Image slot to set pending
 * @param permanent  If set no confirm is required after image swap
 *
 * @return 0 on success, non-zero on failure
 */
int img_mgmt_state_set_pending(int slot, int permanent);

/**
 * Confirms the current image state.  Prevents a fallback from occurring on the
 * next reboot if the active image is currently being tested.
 *
 * @return 0 on success, non -zero on failure
 */
int img_mgmt_state_confirm(void);

/** @brief Generic callback function for events */
typedef void (*img_mgmt_dfu_cb)(void);

/** Callback function pointers */
struct img_mgmt_dfu_callbacks_t {
	img_mgmt_dfu_cb dfu_started_cb;
	img_mgmt_dfu_cb dfu_stopped_cb;
	img_mgmt_dfu_cb dfu_pending_cb;
	img_mgmt_dfu_cb dfu_confirmed_cb;
};

/** @typedef img_mgmt_upload_fn
 * @brief Application callback that is executed when an image upload request is
 * received.
 *
 * The callback's return code determines whether the upload request is accepted
 * or rejected.  If the callback returns 0, processing of the upload request
 * proceeds.  If the callback returns nonzero, the request is rejected with a
 * response containing an `rc` value equal to the return code.
 *
 * @param req		Image upload request structure
 * @param action	Image upload action structure
 *
 * @return	0 if the upload request should be accepted; nonzero to reject
 *		the request with the specified status.
 */
typedef int (*img_mgmt_upload_fn)(const struct img_mgmt_upload_req req,
				  const struct img_mgmt_upload_action action);

/**
 * @brief Configures a callback that gets called whenever a valid image upload
 * request is received.
 *
 * The callback's return code determines whether the upload request is accepted
 * or rejected.  If the callback returns 0, processing of the upload request
 * proceeds.  If the callback returns nonzero, the request is rejected with a
 * response containing an `rc` value equal to the return code.
 *
 * @param cb	The callback to execute on rx of an upload request.
 */
void img_mgmt_set_upload_cb(img_mgmt_upload_fn cb);
void img_mgmt_register_callbacks(const struct img_mgmt_dfu_callbacks_t *cb_struct);
void img_mgmt_dfu_stopped(void);
void img_mgmt_dfu_started(void);
void img_mgmt_dfu_pending(void);
void img_mgmt_dfu_confirmed(void);

/**
 * Compares two image version numbers in a semver-compatible way.
 *
 * @param a	The first version to compare.
 * @param b	The second version to compare.
 *
 * @return	-1 if a < b
 * @return	0 if a = b
 * @return	1 if a > b
 */
int img_mgmt_vercmp(const struct image_version *a, const struct image_version *b);

#ifdef CONFIG_IMG_MGMT_VERBOSE_ERR
#define IMG_MGMT_UPLOAD_ACTION_SET_RC_RSN(action, rsn) ((action)->rc_rsn = (rsn))
#define IMG_MGMT_UPLOAD_ACTION_RC_RSN(action) ((action)->rc_rsn)
int img_mgmt_error_rsp(struct mgmt_ctxt *ctxt, int rc, const char *rsn);
extern const char *img_mgmt_err_str_app_reject;
extern const char *img_mgmt_err_str_hdr_malformed;
extern const char *img_mgmt_err_str_magic_mismatch;
extern const char *img_mgmt_err_str_no_slot;
extern const char *img_mgmt_err_str_flash_open_failed;
extern const char *img_mgmt_err_str_flash_erase_failed;
extern const char *img_mgmt_err_str_flash_write_failed;
extern const char *img_mgmt_err_str_downgrade;
extern const char *img_mgmt_err_str_image_bad_flash_addr;
#else
#define IMG_MGMT_UPLOAD_ACTION_SET_RC_RSN(action, rsn)
#define IMG_MGMT_UPLOAD_ACTION_RC_RSN(action) NULL
#endif

#ifdef __cplusplus
}
#endif

#endif /* H_IMG_MGMT_ */
