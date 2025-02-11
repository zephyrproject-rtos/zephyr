/*
 * Copyright (c) 2022 Laird Connectivity
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_MCUMGR_IMG_MGMT_CALLBACKS_
#define H_MCUMGR_IMG_MGMT_CALLBACKS_

#ifdef __cplusplus
extern "C" {
#endif

/* Dummy definitions, include zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt.h for actual definitions */
struct img_mgmt_upload_action;
struct img_mgmt_upload_req;

/**
 * @brief MCUmgr img_mgmt callback API
 * @defgroup mcumgr_callback_api_img_mgmt MCUmgr img_mgmt callback API
 * @ingroup mcumgr_callback_api
 * @{
 */

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
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
