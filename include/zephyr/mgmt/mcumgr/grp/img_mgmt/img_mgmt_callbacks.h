/*
 * Copyright (c) 2022 Laird Connectivity
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_MCUMGR_IMG_MGMT_CALLBACKS_
#define H_MCUMGR_IMG_MGMT_CALLBACKS_

#include <stdbool.h>
#include <stdint.h>
#include <zcbor_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dummy definitions, include zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt.h for actual definitions */
struct img_mgmt_upload_action;
struct img_mgmt_upload_req;

/**
 * @brief MCUmgr Image Management Callbacks API
 * @defgroup mcumgr_callback_api_img_mgmt Image Management Callbacks
 * @ingroup mcumgr_img_mgmt
 * @ingroup mcumgr_callback_api
 * @{
 */

/**
 * MGMT event opcodes for image management group.
 */
enum img_mgmt_group_events {
	/** Callback when a client sends a file upload chunk, data is @ref img_mgmt_upload_check. */
	MGMT_EVT_OP_IMG_MGMT_DFU_CHUNK			= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_IMG, 0),

	/** Callback when a DFU operation is stopped. */
	MGMT_EVT_OP_IMG_MGMT_DFU_STOPPED		= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_IMG, 1),

	/** Callback when a DFU operation is started. */
	MGMT_EVT_OP_IMG_MGMT_DFU_STARTED		= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_IMG, 2),

	/** Callback when a DFU operation has finished being transferred. */
	MGMT_EVT_OP_IMG_MGMT_DFU_PENDING		= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_IMG, 3),

	/** Callback when an image has been confirmed. data is @ref img_mgmt_image_confirmed. */
	MGMT_EVT_OP_IMG_MGMT_DFU_CONFIRMED		= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_IMG, 4),

	/** Callback when an image write command has finished writing to flash. */
	MGMT_EVT_OP_IMG_MGMT_DFU_CHUNK_WRITE_COMPLETE	= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_IMG, 5),

	/**
	 * Callback when an image slot's state is encoded for a response, data is
	 * @ref img_mgmt_state_slot_encode.
	 */
	MGMT_EVT_OP_IMG_MGMT_IMAGE_SLOT_STATE		= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_IMG, 6),

	/**
	 * Callback when a slot list command outputs fields for an image, data is
	 * @ref img_mgmt_slot_info_image.
	 */
	MGMT_EVT_OP_IMG_MGMT_SLOT_INFO_IMAGE		= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_IMG, 7),

	/**
	 * Callback when a slot list command outputs fields for a slot of an image, data is
	 * @ref img_mgmt_slot_info_slot.
	 */
	MGMT_EVT_OP_IMG_MGMT_SLOT_INFO_SLOT		= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_IMG, 8),

	/** Used to enable all img_mgmt_group events. */
	MGMT_EVT_OP_IMG_MGMT_ALL			= MGMT_DEF_EVT_OP_ALL(MGMT_EVT_GRP_IMG),
};

/**
 * Structure provided in the #MGMT_EVT_OP_IMG_MGMT_DFU_CHUNK notification callback: This callback
 * function is used to notify the application about a pending firmware upload packet from a client
 * and authorise or deny it. Upload will be allowed so long as all notification handlers return
 * #MGMT_ERR_EOK, if one returns an error then the upload will be denied.
 */
struct img_mgmt_upload_check {
	/** Action to take */
	struct img_mgmt_upload_action *action;

	/** Upload request information */
	struct img_mgmt_upload_req *req;
};

/**
 * Structure provided in the #MGMT_EVT_OP_IMG_MGMT_DFU_CONFIRMED notification callback: This
 * callback function is used to notify the application about an image confirmation being executed
 * successfully.
 */
struct img_mgmt_image_confirmed {
	/** Image number which has been confirmed */
	const uint8_t image;
};

/**
 * Structure provided in the #MGMT_EVT_OP_IMG_MGMT_IMAGE_SLOT_STATE notification callback: This
 * callback function is used to allow applications or modules append custom fields to the image
 * slot state response.
 */
struct img_mgmt_state_slot_encode {
	bool *ok;
	zcbor_state_t *zse;
	const uint32_t slot;
	const char *version;
	const uint8_t *hash;
	const int flags;
};

/**
 * Structure provided in the #MGMT_EVT_OP_IMG_MGMT_SLOT_INFO_IMAGE notification callback: This
 * callback function is called once per image when the slot info command is used, it can be used
 * to return additional information/fields in the response.
 */
struct img_mgmt_slot_info_image {
	/** The image that is currently being enumerated. */
	const uint8_t image;

	/**
	 * The zcbor encoder which is currently being used to output information, additional fields
	 * can be added using this.
	 */
	zcbor_state_t *zse;
};

/**
 * Structure provided in the #MGMT_EVT_OP_IMG_MGMT_SLOT_INFO_SLOT notification callback: This
 * callback function is called once per slot per image when the slot info command is used, it can
 * be used to return additional information/fields in the response.
 */
struct img_mgmt_slot_info_slot {
	/** The image that is currently being enumerated. */
	const uint8_t image;

	/** The slot that is currently being enumerated. */
	const uint8_t slot;

	/** Flash area of the slot that is current being enumerated. */
	const struct flash_area *fa;

	/**
	 * The zcbor encoder which is currently being used to output information, additional fields
	 * can be added using this.
	 */
	zcbor_state_t *zse;
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
