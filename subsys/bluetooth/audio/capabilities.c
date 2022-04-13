/*  Bluetooth Audio Capabilities */

/*
 * Copyright (c) 2021 Intel Corporation
 * Copyright (c) 2022 Nordic Semiconductor ASA
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
#include <bluetooth/audio/audio.h>
#include <bluetooth/audio/capabilities.h>

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
				    enum bt_audio_pac_type type,
				    const struct bt_codec *codec,
				    struct bt_audio_stream **stream,
				    struct bt_codec_qos_pref *const pref)
{
	struct bt_audio_capability *cap;
	sys_slist_t *lst;

	if (type == BT_AUDIO_SINK) {
		lst = &snks;
	} else if (type == BT_AUDIO_SOURCE) {
		lst = &srcs;
	} else {
		BT_ERR("Invalid endpoint type: %u", type);
		return -EINVAL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(lst, cap, node) {
		/* Skip if capabilities don't match */
		if (codec->id != cap->codec->id) {
			continue;
		}

		if (cap->ops == NULL || cap->ops->config == NULL) {
			return -EACCES;
		}

		*stream = cap->ops->config(conn, (struct bt_audio_ep *)ep,
					   type, cap, (struct bt_codec *)codec);

		if (*stream == NULL) {
			return -ENOMEM;
		}

		pref->unframed_supported =
			cap->pref.framing == BT_AUDIO_CAPABILITY_UNFRAMED_SUPPORTED;
		pref->phy = cap->pref.phy;
		pref->rtn = cap->pref.rtn;
		pref->latency = cap->pref.latency;
		pref->pd_min = cap->pref.pd_min;
		pref->pd_max = cap->pref.pd_max;
		pref->pref_pd_min = cap->pref.pref_pd_min;
		pref->pref_pd_max = cap->pref.pref_pd_max;

		(*stream)->user_data = cap;

		return 0;
	}

	BT_ERR("No capability for type %u and codec ID %u", type, codec->id);

	return -EOPNOTSUPP;
}

static int unicast_server_reconfig_cb(struct bt_audio_stream *stream,
				      uint8_t type,
				      const struct bt_codec *codec,
				      struct bt_codec_qos_pref *const pref)
{
	struct bt_audio_capability *cap;
	sys_slist_t *lst;

	if (type == BT_AUDIO_SINK) {
		lst = &snks;
	} else if (type == BT_AUDIO_SOURCE) {
		lst = &srcs;
	} else {
		BT_ERR("Invalid endpoint type: %u", type);
		return -EINVAL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(lst, cap, node) {
		int err;

		if (codec->id != cap->codec->id) {
			continue;
		}

		if (cap->ops == NULL || cap->ops->reconfig == NULL) {
			return -EACCES;
		}

		pref->unframed_supported =
			cap->pref.framing == BT_AUDIO_CAPABILITY_UNFRAMED_SUPPORTED;
		pref->phy = cap->pref.phy;
		pref->rtn = cap->pref.rtn;
		pref->latency = cap->pref.latency;
		pref->pd_min = cap->pref.pd_min;
		pref->pd_max = cap->pref.pd_max;
		pref->pref_pd_min = cap->pref.pref_pd_min;
		pref->pref_pd_max = cap->pref.pref_pd_max;

		err = cap->ops->reconfig(stream, cap,
					  (struct bt_codec *)codec);
		if (err != 0) {
			return err;
		}

		stream->user_data = cap;

		return 0;
	}

	BT_ERR("No capability for type %u and codec ID %u", type, codec->id);

	return -EOPNOTSUPP;
}

static int unicast_server_qos_cb(struct bt_audio_stream *stream,
				 const struct bt_codec_qos *qos)
{
	struct bt_audio_capability *cap = stream->user_data;

	if (cap->ops == NULL || cap->ops->qos == NULL) {
		return -EACCES;
	}

	return cap->ops->qos(stream, (struct bt_codec_qos *)qos);
}

static int unicast_server_enable_cb(struct bt_audio_stream *stream,
				    const struct bt_codec_data *meta,
				    size_t meta_count)
{
	struct bt_audio_capability *cap = stream->user_data;

	if (cap->ops == NULL || cap->ops->enable == NULL) {
		return -EACCES;
	}

	return cap->ops->enable(stream, (struct bt_codec_data *)meta,
				meta_count);
}

static int unicast_server_start_cb(struct bt_audio_stream *stream)
{
	struct bt_audio_capability *cap = stream->user_data;

	if (cap->ops == NULL || cap->ops->start == NULL) {
		return -EACCES;
	}

	return cap->ops->start(stream);
}

static int unicast_server_metadata_cb(struct bt_audio_stream *stream,
				      const struct bt_codec_data *meta,
				      size_t meta_count)
{
	struct bt_audio_capability *cap = stream->user_data;

	if (cap->ops == NULL || cap->ops->metadata == NULL) {
		return -EACCES;
	}

	return cap->ops->metadata(stream, (struct bt_codec_data *)meta,
				  meta_count);
}

static int unicast_server_disable_cb(struct bt_audio_stream *stream)
{
	struct bt_audio_capability *cap = stream->user_data;

	if (cap->ops == NULL || cap->ops->disable == NULL) {
		return -EACCES;
	}

	return cap->ops->disable(stream);
}

static int unicast_server_stop_cb(struct bt_audio_stream *stream)
{
	struct bt_audio_capability *cap = stream->user_data;

	if (cap->ops == NULL || cap->ops->stop == NULL) {
		return -EACCES;
	}

	return cap->ops->stop(stream);
}

static int unicast_server_release_cb(struct bt_audio_stream *stream)
{
	struct bt_audio_capability *cap = stream->user_data;

	if (cap->ops == NULL || cap->ops->release == NULL) {
		return -EACCES;
	}

	return cap->ops->release(stream);
}

static int publish_capability_cb(struct bt_conn *conn, uint8_t type,
				 uint8_t index, struct bt_codec *codec)
{
	struct bt_audio_capability *cap;
	sys_slist_t *lst;
	uint8_t i;

	if (type == BT_AUDIO_SINK) {
		lst = &snks;
	} else if (type == BT_AUDIO_SOURCE) {
		lst = &srcs;
	} else {
		BT_ERR("Invalid endpoint type: %u", type);
		return -EINVAL;
	}

	i = 0;
	SYS_SLIST_FOR_EACH_CONTAINER(lst, cap, node) {
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
			       enum bt_audio_pac_type type,
			       enum bt_audio_location *location)
{
	if (0) {
#if defined(CONFIG_BT_PAC_SNK_LOC)
	} else if (type == BT_AUDIO_SINK) {
		*location = sink_location;
#endif /* CONFIG_BT_PAC_SNK_LOC */
#if defined(CONFIG_BT_PAC_SRC_LOC)
	} else if (type == BT_AUDIO_SOURCE) {
		*location = source_location;
#endif /* CONFIG_BT_PAC_SRC_LOC */
	} else {
		BT_ERR("Invalid endpoint type: %u", type);

		return -EINVAL;
	}

	return 0;
}

#if defined(CONFIG_BT_PAC_SNK_LOC_WRITEABLE) || defined(CONFIG_BT_PAC_SRC_LOC_WRITEABLE)
static int write_location_cb(struct bt_conn *conn, enum bt_audio_pac_type type,
			     enum bt_audio_location location)
{
	return bt_audio_capability_set_location(type, location);
}
#endif /* CONFIG_BT_PAC_SNK_LOC_WRITEABLE || CONFIG_BT_PAC_SRC_LOC_WRITEABLE */
#endif /* CONFIG_BT_PAC_SNK_LOC || CONFIG_BT_PAC_SRC_LOC */

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
	.publish_capability = publish_capability_cb,
#if defined(CONFIG_BT_PAC_SNK_LOC) || defined(CONFIG_BT_PAC_SRC_LOC)
	.publish_location = publish_location_cb,
#if defined(CONFIG_BT_PAC_SNK_LOC_WRITEABLE) || defined(CONFIG_BT_PAC_SRC_LOC_WRITEABLE)
	.write_location = write_location_cb
#endif /* CONFIG_BT_PAC_SNK_LOC_WRITEABLE || CONFIG_BT_PAC_SRC_LOC_WRITEABLE */
#endif /* CONFIG_BT_PAC_SNK_LOC || CONFIG_BT_PAC_SRC_LOC */
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

#if defined(CONFIG_BT_PAC_SNK_LOC) || defined(CONFIG_BT_PAC_SRC_LOC)
int bt_audio_capability_set_location(enum bt_audio_pac_type type,
				     enum bt_audio_location location)
{
	int err;

	if (0) {
#if defined(CONFIG_BT_PAC_SNK_LOC)
	} else if (type == BT_AUDIO_SINK) {
		sink_location = location;
#endif /* CONFIG_BT_PAC_SNK_LOC */
#if defined(CONFIG_BT_PAC_SRC_LOC)
	} else if (type == BT_AUDIO_SOURCE) {
		source_location = location;
#endif /* CONFIG_BT_PAC_SRC_LOC */
	} else {
		BT_ERR("Invalid endpoint type: %u", type);

		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_UNICAST_SERVER)) {
		err = bt_audio_unicast_server_location_changed(type);
		if (err) {
			BT_DBG("Location for type %d wasn't notified: %d",
			       type, err);
		}
	}

	return 0;
}
#endif /* CONFIG_BT_PAC_SNK_LOC || CONFIG_BT_PAC_SRC_LOC */
