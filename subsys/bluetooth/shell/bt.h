/** @file
 *  @brief Bluetooth shell functions
 *
 *  This is not to be included by the application.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BT_H
#define __BT_H

#include <zephyr/bluetooth/bluetooth.h>
#include <sys/types.h>

extern const struct shell *ctx_shell;
extern struct bt_conn *default_conn;

#if defined(CONFIG_BT_ISO)
extern struct bt_iso_chan iso_chan;
#endif /* CONFIG_BT_ISO */

#if defined(CONFIG_BT_EXT_ADV)
extern uint8_t selected_adv;
extern struct bt_le_ext_adv *adv_sets[CONFIG_BT_EXT_ADV_MAX_ADV_SET];
#if defined(CONFIG_BT_PER_ADV_SYNC)
extern struct bt_le_per_adv_sync *per_adv_syncs[CONFIG_BT_PER_ADV_SYNC_MAX];
#endif /* CONFIG_BT_PER_ADV_SYNC */
#endif /* CONFIG_BT_EXT_ADV */

#if defined(CONFIG_BT_AUDIO)
/* Must guard before including audio.h as audio.h uses Kconfigs guarded by
 * CONFIG_BT_AUDIO
 */
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>

struct named_lc3_preset {
	const char *name;
	struct bt_bap_lc3_preset preset;
};

#if defined(CONFIG_BT_BAP_UNICAST_CLIENT)

extern struct bt_bap_ep *snks[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
extern struct bt_bap_ep *srcs[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];
extern const struct named_lc3_preset *default_sink_preset;
extern const struct named_lc3_preset *default_source_preset;
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT */
#endif /* CONFIG_BT_AUDIO */

void conn_addr_str(struct bt_conn *conn, char *addr, size_t len);

#endif /* __BT_H */
