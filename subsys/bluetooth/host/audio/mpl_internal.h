/**
 * @file
 * @brief Internal header for the Media Control Service (MCS) and
 * Client (MCC) and for the media player (MPL).
 *
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MPL_INTERNAL_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MPL_INTERNAL_

#include "mpl.h"

#define MPL_NO_TRACK_ID 0

/* Debug output of 48 bit Object ID value */
/* (Zephyr does not yet support debug output of more than 32 bit values.) */
/* Takes a text and a 64-bit integer as input */
#define BT_DBG_OBJ_ID(text, id64) \
	do { \
		if (IS_ENABLED(CONFIG_BT_DEBUG_MCS)) { \
			char t[BT_OTS_OBJ_ID_STR_LEN]; \
			(void)bt_ots_obj_id_to_str(id64, t, sizeof(t)); \
			BT_DBG(text "0x%s", log_strdup(t)); \
		} \
	} while (0)


#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MPL_INTERNAL_*/
