/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/check.h>
#include <bluetooth/gatt.h>
#include <bluetooth/audio/tbs.h>
#include <bluetooth/audio/cap.h>
#include "cap_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_CAP_INITIATOR)
#define LOG_MODULE_NAME bt_cap_initiator
#include "common/log.h"

static const struct bt_cap_initiator_cb *cap_cb;

int bt_cap_initiator_register_cb(const struct bt_cap_initiator_cb *cb)
{
	return -ENOSYS;
}

#if defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)

int bt_cap_initiator_broadcast_audio_start(const struct bt_cap_broadcast_audio_start_param *param,
					   struct bt_audio_broadcast_source **source)
{
	return -ENOSYS;
}

int bt_cap_initiator_broadcast_audio_update(struct bt_audio_broadcast_source *broadcast_source,
					    uint8_t meta_count,
					    const struct bt_codec_data *meta)
{
	return -ENOSYS;
}

int bt_cap_initiator_broadcast_audio_stop(struct bt_audio_broadcast_source *broadcast_source)
{
	return -ENOSYS;
}

#endif /* CONFIG_BT_AUDIO_BROADCAST_SOURCE */

#if defined(CONFIG_BT_AUDIO_UNICAST_CLIENT)

int bt_cap_initiator_unicast_discover(struct bt_conn *conn)
{
	return -ENOSYS;
}

int bt_cap_initiator_unicast_audio_start(const struct bt_cap_unicast_audio_start_param *param,
					 struct bt_audio_unicast_group **unicast_group)
{
	return -ENOSYS;
}

int bt_cap_initiator_unicast_audio_update(struct bt_audio_unicast_group *unicast_group,
					  uint8_t meta_count,
					  const struct bt_codec_data *meta)
{
	return -ENOSYS;
}

int bt_cap_initiator_unicast_audio_stop(struct bt_audio_unicast_group *unicast_group)
{
	return -ENOSYS;
}

#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT */

#if defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE) && defined(CONFIG_BT_AUDIO_UNICAST_CLIENT)

int bt_cap_initiator_unicast_to_broadcast(
	const struct bt_cap_unicast_to_broadcast_param *param,
	struct bt_audio_broadcast_source **source)
{
	return -ENOSYS;
}

int bt_cap_initiator_broadcast_to_unicast(const struct bt_cap_broadcast_to_unicast_param *param,
					  struct bt_audio_unicast_group **unicast_group)
{
	return -ENOSYS;
}

#endif /* CONFIG_BT_AUDIO_BROADCAST_SOURCE && CONFIG_BT_AUDIO_UNICAST_CLIENT */
