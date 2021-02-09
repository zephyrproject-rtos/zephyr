/*  Bluetooth Audio Capabilities */

/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/byteorder.h>

#include <device.h>
#include <init.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/audio.h>

#include "capabilities.h"
#include "pacs_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_AUDIO_DEBUG_CAPABILITIES)
#define LOG_MODULE_NAME bt_audio_capability
#include "common/log.h"

static sys_slist_t snks;
static sys_slist_t srcs;

sys_slist_t *bt_audio_capability_get(uint8_t type)
{
	switch (type) {
	case BT_AUDIO_SINK:
		return &snks;
	case BT_AUDIO_SOURCE:
		return &srcs;
	}

	return NULL;
}

/* Register Audio Capability */
int bt_audio_capability_register(struct bt_audio_capability *cap)
{
	sys_slist_t *lst;

	if (!cap || !cap->codec) {
		return -EINVAL;
	}

	lst = bt_audio_capability_get(cap->type);
	if (!lst) {
		return -EINVAL;
	}

	BT_DBG("cap %p type 0x%02x codec 0x%02x codec cid 0x%04x "
	       "codec vid 0x%04x", cap, cap->type, cap->codec->id,
	       cap->codec->cid, cap->codec->vid);

	sys_slist_append(lst, &cap->node);

#if defined(CONFIG_BT_PACS)
	bt_pacs_add_capability(cap->type);
#endif /* CONFIG_BT_PACS */

	return 0;
}

/* Unregister Audio Capability */
int bt_audio_capability_unregister(struct bt_audio_capability *cap)
{
	sys_slist_t *lst;

	if (!cap) {
		return -EINVAL;
	}

	lst = bt_audio_capability_get(cap->type);
	if (!lst) {
		return -EINVAL;
	}

	BT_DBG("cap %p type 0x%02x", cap, cap->type);

	if (!sys_slist_find_and_remove(lst, &cap->node)) {
		return -ENOENT;
	}

#if defined(CONFIG_BT_PACS)
	bt_pacs_remove_capability(cap->type);
#endif /* CONFIG_BT_PACS */

	return 0;
}
