/* @file
 * @brief Object Transfer client internal header file
 *
 * For use with the Object Transfer Service Client (OTC)
 *
 * Copyright (c) 2020 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_HOST_AUDIO_OTC_INTERNAL_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_HOST_AUDIO_OTC_INTERNAL_H_

/* TODO: Temporarily here - clean up, and move alongside the Object Transfer Service */

#include "otc.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Translate OTS OLCP result code to "normal" error
 *
 * Replace the OTS SUCCESS value with the value 0.
 *
 * The OLCP result code has the value "1" for success.
 * Elsewhere in the le-audio host stack, "0" is used for no error.
 *
 * TODO: Update to also convert the non-SUCCESS values
 * TODO: The macro is called OLCP, but uses OACP success value
 * OTS does not use the value "0".
 */
#define OLCP_RESULT_TO_ERROR(RESULT) \
	(((RESULT) == BT_OTS_OACP_RES_SUCCESS) ? 0 : RESULT)


#define OTS_FEATURE_LEN        (uint32_t)(2 * sizeof(uint32_t))
#define OTS_SIZE_LEN           (uint32_t)(2 * sizeof(uint32_t))
#define OTS_PROPERTIES_LEN     (uint32_t)(sizeof(uint32_t))
#define OTS_TYPE_MAX_LEN       (uint32_t)(sizeof(struct bt_ots_obj_type))

#define OTS_MAX_OBJECT_META_SIZE       170

/** PSM for Object Transfer Service. */
#define OTS_L2CAP_PSM                  0x0025
/**Start of the usable range of Object IDs (values 0 to 0xFF are reserved)*/
#define OTS_ID_RANGE_START             0x000000000100
/** Type UUID of the directory listing object. */
#define OTS_DIR_LISTING_TYPE           0x2acb
/** ID of the directory listing object. */
#define OTS_DIR_LISTING_ID             0x000000000000
/** Name of the directory listing object. */
#define OTS_DIR_LISTING_NAME           "Directory_listing"

/** Maximum size of the directory listing object. */
#define BT_OTS_DIR_LISTING_MAX_SIZE \
	(OTS_MAX_OBJECT_META_SIZE * BT_OTS_MAX_OBJ_CNT)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HOST_AUDIO_OTC_INTERNAL_H_ */
