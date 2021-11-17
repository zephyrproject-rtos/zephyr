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
 * OTS does not use the value "0".
 */
#define OLCP_RESULT_TO_ERROR(RESULT) \
	(((RESULT) == BT_OTS_OACP_RES_SUCCESS) ? 0 : RESULT)


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HOST_AUDIO_OTC_INTERNAL_H_ */
