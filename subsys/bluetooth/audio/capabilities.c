/*  Bluetooth Audio Capabilities */

/*
 * Copyright (c) 2021 Intel Corporation
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/device.h>
#include <zephyr/init.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/capabilities.h>

#include "pacs_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_AUDIO_DEBUG_CAPABILITIES)
#define LOG_MODULE_NAME bt_audio_capability
#include "common/log.h"

static sys_slist_t snks;
static sys_slist_t srcs;

IF_ENABLED(CONFIG_BT_PAC_SNK, (static enum bt_audio_context sink_available_contexts;))
IF_ENABLED(CONFIG_BT_PAC_SRC, (static enum bt_audio_context source_available_contexts;));

static int publish_capability_cb(struct bt_conn *conn, uint8_t dir,
				 uint8_t index, struct bt_codec *codec)
{
	struct bt_audio_capability *cap;
	sys_slist_t *lst;
	uint8_t i;

	if (dir == BT_AUDIO_DIR_SINK) {
		lst = &snks;
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		lst = &srcs;
	} else {
		BT_ERR("Invalid endpoint dir: %u", dir);
		return -EINVAL;
	}

	i = 0;
	SYS_SLIST_FOR_EACH_CONTAINER(lst, cap, _node) {
		if (i != index) {
			i++;
			continue;
		}

		(void)memcpy(codec, cap->codec, sizeof(*codec));

		return 0;
	}

	return -ENOENT;
}

#if defined(CONFIG_BT_PAC_SNK_LOC) || defined(CONFIG_BT_PAC_SRC_LOC)

#if defined(CONFIG_BT_PAC_SNK_LOC)
static enum bt_audio_location sink_location;
#endif /* CONFIG_BT_PAC_SNK_LOC */
#if defined(CONFIG_BT_PAC_SRC_LOC)
static enum bt_audio_location source_location;
#endif /* CONFIG_BT_PAC_SRC_LOC */

static int publish_location_cb(struct bt_conn *conn,
			       enum bt_audio_dir dir,
			       enum bt_audio_location *location)
{
	if (0) {
#if defined(CONFIG_BT_PAC_SNK_LOC)
	} else if (dir == BT_AUDIO_DIR_SINK) {
		*location = sink_location;
#endif /* CONFIG_BT_PAC_SNK_LOC */
#if defined(CONFIG_BT_PAC_SRC_LOC)
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		*location = source_location;
#endif /* CONFIG_BT_PAC_SRC_LOC */
	} else {
		BT_ERR("Invalid endpoint dir: %u", dir);

		return -EINVAL;
	}

	return 0;
}

#if defined(CONFIG_BT_PAC_SNK_LOC_WRITEABLE) || defined(CONFIG_BT_PAC_SRC_LOC_WRITEABLE)
static int write_location_cb(struct bt_conn *conn, enum bt_audio_dir dir,
			     enum bt_audio_location location)
{
	return bt_audio_capability_set_location(dir, location);
}
#endif /* CONFIG_BT_PAC_SNK_LOC_WRITEABLE || CONFIG_BT_PAC_SRC_LOC_WRITEABLE */
#endif /* CONFIG_BT_PAC_SNK_LOC || CONFIG_BT_PAC_SRC_LOC */
static int get_available_contexts(enum bt_audio_dir dir,
				  enum bt_audio_context *contexts);

static int get_available_contexts_cb(struct bt_conn *conn, enum bt_audio_dir dir,
				     enum bt_audio_context *contexts)
{
	return get_available_contexts(dir, contexts);
}

static struct bt_audio_pacs_cb pacs_cb = {
	.publish_capability = publish_capability_cb,
	.get_available_contexts = get_available_contexts_cb,
#if defined(CONFIG_BT_PAC_SNK_LOC) || defined(CONFIG_BT_PAC_SRC_LOC)
	.publish_location = publish_location_cb,
#if defined(CONFIG_BT_PAC_SNK_LOC_WRITEABLE) || defined(CONFIG_BT_PAC_SRC_LOC_WRITEABLE)
	.write_location = write_location_cb
#endif /* CONFIG_BT_PAC_SNK_LOC_WRITEABLE || CONFIG_BT_PAC_SRC_LOC_WRITEABLE */
#endif /* CONFIG_BT_PAC_SNK_LOC || CONFIG_BT_PAC_SRC_LOC */
};

sys_slist_t *bt_audio_capability_get(enum bt_audio_dir dir)
{
	switch (dir) {
	case BT_AUDIO_DIR_SINK:
		return &snks;
	case BT_AUDIO_DIR_SOURCE:
		return &srcs;
	}

	return NULL;
}

/* Register Audio Capability */
int bt_audio_capability_register(enum bt_audio_dir dir, struct bt_audio_capability *cap)
{
	static bool pacs_cb_registered;
	sys_slist_t *lst;

	if (!cap || !cap->codec) {
		return -EINVAL;
	}

	lst = bt_audio_capability_get(dir);
	if (!lst) {
		return -EINVAL;
	}

	BT_DBG("cap %p dir 0x%02x codec 0x%02x codec cid 0x%04x "
	       "codec vid 0x%04x", cap, dir, cap->codec->id,
	       cap->codec->cid, cap->codec->vid);

	if (!pacs_cb_registered) {
		int err;

		err = bt_audio_pacs_register_cb(&pacs_cb);
		if (err != 0) {
			BT_DBG("Failed to register PACS callbacks: %d",
			       err);
			return err;
		}

		pacs_cb_registered = true;
	}

	sys_slist_append(lst, &cap->_node);

#if defined(CONFIG_BT_PACS)
	bt_pacs_capabilities_changed(dir);
#endif /* CONFIG_BT_PACS */

	return 0;
}

/* Unregister Audio Capability */
int bt_audio_capability_unregister(enum bt_audio_dir dir, struct bt_audio_capability *cap)
{
	sys_slist_t *lst;

	if (!cap) {
		return -EINVAL;
	}

	lst = bt_audio_capability_get(dir);
	if (!lst) {
		return -EINVAL;
	}

	BT_DBG("cap %p dir 0x%02x", cap, dir);

	if (!sys_slist_find_and_remove(lst, &cap->_node)) {
		return -ENOENT;
	}

#if defined(CONFIG_BT_PACS)
	bt_pacs_capabilities_changed(dir);
#endif /* CONFIG_BT_PACS */

	return 0;
}

#if defined(CONFIG_BT_PAC_SNK_LOC) || defined(CONFIG_BT_PAC_SRC_LOC)
int bt_audio_capability_set_location(enum bt_audio_dir dir,
				     enum bt_audio_location location)
{
	if (0) {
#if defined(CONFIG_BT_PAC_SNK_LOC)
	} else if (dir == BT_AUDIO_DIR_SINK) {
		sink_location = location;
#endif /* CONFIG_BT_PAC_SNK_LOC */
#if defined(CONFIG_BT_PAC_SRC_LOC)
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		source_location = location;
#endif /* CONFIG_BT_PAC_SRC_LOC */
	} else {
		BT_ERR("Invalid endpoint dir: %u", dir);

		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_PAC_SNK_LOC) ||
	    IS_ENABLED(CONFIG_BT_PAC_SRC_LOC)) {
		int err;

		err = bt_audio_pacs_location_changed(dir);
		if (err) {
			BT_DBG("Location for dir %d wasn't notified: %d",
			       dir, err);

			return err;
		}
	}

	return 0;
}
#endif /* CONFIG_BT_PAC_SNK_LOC || CONFIG_BT_PAC_SRC_LOC */

static int set_available_contexts(enum bt_audio_dir dir,
				  enum bt_audio_context contexts)
{
	IF_ENABLED(CONFIG_BT_PAC_SNK, (
		if (dir == BT_AUDIO_DIR_SINK) {
			const enum bt_audio_context supported = CONFIG_BT_PACS_SNK_CONTEXT;

			if (contexts & ~supported) {
				return -ENOTSUP;
			}

			sink_available_contexts = contexts;
			return 0;
		}
	));

	IF_ENABLED(CONFIG_BT_PAC_SRC, (
		if (dir == BT_AUDIO_DIR_SOURCE) {
			const enum bt_audio_context supported = CONFIG_BT_PACS_SRC_CONTEXT;

			if (contexts & ~supported) {
				return -ENOTSUP;
			}

			source_available_contexts = contexts;
			return 0;
		}
	));

	BT_ERR("Invalid endpoint dir: %u", dir);

	return -EINVAL;
}

static int get_available_contexts(enum bt_audio_dir dir,
				  enum bt_audio_context *contexts)
{
	IF_ENABLED(CONFIG_BT_PAC_SNK, (
		if (dir == BT_AUDIO_DIR_SINK) {
			*contexts = sink_available_contexts;
			return 0;
		}
	));

	IF_ENABLED(CONFIG_BT_PAC_SRC, (
		if (dir == BT_AUDIO_DIR_SOURCE) {
			*contexts = source_available_contexts;
			return 0;
		}
	));

	BT_ASSERT_PRINT_MSG("Invalid endpoint dir: %u", dir);

	return -EINVAL;
}

int bt_audio_capability_set_available_contexts(enum bt_audio_dir dir,
					       enum bt_audio_context contexts)
{
	int err;

	err = set_available_contexts(dir, contexts);
	if (err) {
		return err;
	}

	if (IS_ENABLED(CONFIG_BT_PACS)) {
		err = bt_pacs_available_contexts_changed();
		if (err) {
			BT_DBG("Available contexts weren't notified: %d", err);
			return err;
		}
	}

	return 0;
}

enum bt_audio_context bt_audio_capability_get_available_contexts(enum bt_audio_dir dir)
{
	enum bt_audio_context contexts;
	int err;

	err = get_available_contexts(dir, &contexts);
	if (err < 0) {
		return BT_AUDIO_CONTEXT_TYPE_PROHIBITED;
	}

	return contexts;
}
