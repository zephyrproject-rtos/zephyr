/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_AUDIO_MEDIA_PROXY_INTERNAL_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_AUDIO_MEDIA_PROXY_INTERNAL_H_

/** @brief Internal APIs for Bluetooth Media Control */

#include <stdint.h>

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

/** @brief Callbacks to a controller, from the media proxy */
struct media_proxy_sctrl_cbs {
	void (*player_name)(const char *name);

	void (*icon_url)(const char *url);

	void (*track_changed)(void);

	void (*track_title)(const char *title);

	void (*track_duration)(int32_t duration);

	void (*track_position)(int32_t position);

	void (*playback_speed)(int8_t speed);

	void (*seeking_speed)(int8_t speed);
#ifdef CONFIG_BT_OTS
	void (*current_track_id)(uint64_t id);

	void (*next_track_id)(uint64_t id);

	void (*current_group_id)(uint64_t id);

	void (*parent_group_id)(uint64_t id);
#endif /* CONFIG_BT_OTS */

	void (*playing_order)(uint8_t order);

	void (*media_state)(uint8_t state);

	void (*command)(const struct mpl_cmd_ntf *cmd_ntf);

	void (*commands_supported)(uint32_t opcodes);

#ifdef CONFIG_BT_OTS
	void (*search)(uint8_t result_code);

	void (*search_results_id)(uint64_t id);
#endif /* CONFIG_BT_OTS */
};

/** @brief Register a controller with the media proxy */
int media_proxy_sctrl_register(struct media_proxy_sctrl_cbs *sctrl_cbs);

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_AUDIO_MEDIA_PROXY_INTERNAL_H_ */
