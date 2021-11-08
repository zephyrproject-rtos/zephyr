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

#if defined(CONFIG_BT_AUDIO_UNICAST_SERVER) && defined(CONFIG_BT_ASCS)
/* TODO: The unicast server callbacks uses `const` for many of the pointers,
 * wheras the capabilities callbacks do no. The latter should be updated to use
 * `const` where possible.
 */
static int unicast_server_config_cb(struct bt_conn *conn,
				    const struct bt_audio_ep *ep,
				    const struct bt_audio_capability *cap,
				    const struct bt_codec *codec,
				    struct bt_audio_stream **stream)
{
	if (cap->ops == NULL || cap->ops->config == NULL) {
		return -EACCES;
	}

	*stream = cap->ops->config(conn,
				   (struct bt_audio_ep *)ep,
				   (struct bt_audio_capability *)cap,
				   (struct bt_codec *)codec);

	if (*stream == NULL) {
		return -ENOMEM;
	}

	return 0;
}

static int unicast_server_reconfig_cb(struct bt_audio_stream *stream,
				      const struct bt_audio_capability *cap,
				      const struct bt_codec *codec)
{
	if (cap->ops == NULL || cap->ops->reconfig == NULL) {
		return -EACCES;
	}

	return cap->ops->reconfig(stream,
				  (struct bt_audio_capability *)cap,
				  (struct bt_codec *)codec);
}

static int unicast_server_qos_cb(struct bt_audio_stream *stream,
				 const struct bt_codec_qos *qos)
{
	if (stream->cap->ops == NULL || stream->cap->ops->qos == NULL) {
		return -EACCES;
	}

	return stream->cap->ops->qos(stream, (struct bt_codec_qos *)qos);
}

static int unicast_server_enable_cb(struct bt_audio_stream *stream,
				    uint8_t meta_count,
				    const struct bt_codec_data *meta)
{
	if (stream->cap->ops == NULL || stream->cap->ops->enable == NULL) {
		return -EACCES;
	}

	return stream->cap->ops->enable(stream, meta_count,
					(struct bt_codec_data *)meta);
}

static int unicast_server_start_cb(struct bt_audio_stream *stream)
{
	if (stream->cap->ops == NULL || stream->cap->ops->start == NULL) {
		return -EACCES;
	}

	return stream->cap->ops->start(stream);
}

static int unicast_server_metadata_cb(struct bt_audio_stream *stream,
				      uint8_t meta_count,
				      const struct bt_codec_data *meta)
{
	if (stream->cap->ops == NULL || stream->cap->ops->metadata == NULL) {
		return -EACCES;
	}

	return stream->cap->ops->metadata(stream, meta_count,
					  (struct bt_codec_data *)meta);
}

static int unicast_server_disable_cb(struct bt_audio_stream *stream)
{
	if (stream->cap->ops == NULL || stream->cap->ops->disable == NULL) {
		return -EACCES;
	}

	return stream->cap->ops->disable(stream);
}

static int unicast_server_stop_cb(struct bt_audio_stream *stream)
{
	if (stream->cap->ops == NULL || stream->cap->ops->stop == NULL) {
		return -EACCES;
	}

	return stream->cap->ops->stop(stream);
}

static int unicast_server_release_cb(struct bt_audio_stream *stream)
{
	if (stream->cap->ops == NULL || stream->cap->ops->release == NULL) {
		return -EACCES;
	}

	return stream->cap->ops->release(stream);
}

static struct bt_audio_unicast_server_cb unicast_server_cb = {
	.config = unicast_server_config_cb,
	.reconfig = unicast_server_reconfig_cb,
	.qos = unicast_server_qos_cb,
	.enable = unicast_server_enable_cb,
	.start = unicast_server_start_cb,
	.metadata = unicast_server_metadata_cb,
	.disable = unicast_server_disable_cb,
	.stop = unicast_server_stop_cb,
	.release = unicast_server_release_cb,
};
#endif /* CONFIG_BT_AUDIO_UNICAST_SERVER && CONFIG_BT_ASCS */

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

#if defined(CONFIG_BT_AUDIO_UNICAST_SERVER) && defined(CONFIG_BT_ASCS)
	/* Using the capabilities instead of the unicast server directly will
	 * require the capabilities to register the callbacks, which not only
	 * will forward the unicast server callbacks, but also ensure that the
	 * unicast server callbacks are not registered elsewhere
	 */
	static bool unicast_server_cb_registered;

	if (!unicast_server_cb_registered) {
		int err;

		err = bt_audio_unicast_server_register_cb(&unicast_server_cb);
		if (err != 0) {
			BT_DBG("Failed to register unicast server callbacks: %d",
			       err);
			return err;
		}

		unicast_server_cb_registered = true;
	}
#endif /* CONFIG_BT_AUDIO_UNICAST_SERVER && CONFIG_BT_ASCS */

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

#if defined(CONFIG_BT_AUDIO_UNICAST_SERVER) && defined(CONFIG_BT_ASCS)
	/* If we are removing the last audio capability as the unicast
	 * server, we unregister the callbacks.
	 */
	if (sys_slist_is_empty(&snks) && sys_slist_is_empty(&srcs)) {
		int err;

		err = bt_audio_unicast_server_unregister_cb(&unicast_server_cb);
		if (err != 0) {
			BT_DBG("Failed to register unicast server callbacks: %d",
			       err);
			return err;
		}
	}
#endif /* CONFIG_BT_AUDIO_UNICAST_SERVER && CONFIG_BT_ASCS */

#if defined(CONFIG_BT_PACS)
	bt_pacs_remove_capability(cap->type);
#endif /* CONFIG_BT_PACS */

	return 0;
}
