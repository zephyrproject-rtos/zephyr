/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_AUDIO_MEDIA_PROXY_INTERNAL_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_AUDIO_MEDIA_PROXY_INTERNAL_H_

/** @brief Internal APIs for Bluetooth Media Control */

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/media_proxy.h>
#include <zephyr/sys/util_macro.h>

#define MPL_NO_TRACK_ID 0

/* Debug output of 48 bit Object ID value */
/* (Zephyr does not yet support debug output of more than 32 bit values.) */
/* Takes a text and a 64-bit integer as input */
#define LOG_DBG_OBJ_ID(text, id64) \
	do { \
		if (IS_ENABLED(CONFIG_BT_MCS_LOG_LEVEL_DBG)) { \
			char t[BT_OTS_OBJ_ID_STR_LEN]; \
			(void)bt_ots_obj_id_to_str(id64, t, sizeof(t)); \
			LOG_DBG(text "0x%s", t); \
		} \
	} while (0)


/* SYNCHRONOUS (CALL/RETURN) API FOR CONTROLLERS */

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_AUDIO_MEDIA_PROXY_INTERNAL_H_ */
