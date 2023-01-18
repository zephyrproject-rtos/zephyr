/* audio_iso.c - Audio ISO handling */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "audio_iso.h"
#include "audio_internal.h"
#include "endpoint.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_audio_iso, CONFIG_BT_AUDIO_ISO_LOG_LEVEL);

/* TODO: Optimize the ISO_POOL_SIZE */
#define ISO_POOL_SIZE CONFIG_BT_ISO_MAX_CHAN

static struct bt_audio_iso iso_pool[ISO_POOL_SIZE];

struct bt_audio_iso *bt_audio_iso_new(void)
{
	struct bt_audio_iso *iso = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(iso_pool); i++) {
		if (atomic_cas(&iso_pool[i].ref, 0, 1)) {
			iso = &iso_pool[i];
			break;
		}
	}

	if (!iso) {
		return NULL;
	}

	(void)memset(iso, 0, offsetof(struct bt_audio_iso, ref));

	return iso;
}

struct bt_audio_iso *bt_audio_iso_ref(struct bt_audio_iso *iso)
{
	atomic_val_t old;

	__ASSERT_NO_MSG(iso != NULL);

	/* Reference counter must be checked to avoid incrementing ref from
	 * zero, then we should return NULL instead.
	 * Loop on clear-and-set in case someone has modified the reference
	 * count since the read, and start over again when that happens.
	 */
	do {
		old = atomic_get(&iso->ref);

		if (!old) {
			return NULL;
		}
	} while (!atomic_cas(&iso->ref, old, old + 1));

	return iso;
}

void bt_audio_iso_unref(struct bt_audio_iso *iso)
{
	atomic_val_t old;

	__ASSERT_NO_MSG(iso != NULL);

	old = atomic_dec(&iso->ref);

	__ASSERT(old > 0, "iso reference counter is 0");
}

void bt_audio_iso_foreach(bt_audio_iso_func_t func, void *user_data)
{
	for (size_t i = 0; i < ARRAY_SIZE(iso_pool); i++) {
		struct bt_audio_iso *iso = bt_audio_iso_ref(&iso_pool[i]);
		bool iter;

		if (!iso) {
			continue;
		}

		iter = func(iso, user_data);
		bt_audio_iso_unref(iso);

		if (!iter) {
			return;
		}
	}
}

struct bt_audio_iso_find_param {
	struct bt_audio_iso *iso;
	bt_audio_iso_func_t func;
	void *user_data;
};

static bool bt_audio_iso_find_cb(struct bt_audio_iso *iso, void *user_data)
{
	struct bt_audio_iso_find_param *param = user_data;
	bool found;

	found = param->func(iso, param->user_data);
	if (found) {
		param->iso = bt_audio_iso_ref(iso);
	}

	return !found;
}

struct bt_audio_iso *bt_audio_iso_find(bt_audio_iso_func_t func,
				       void *user_data)
{
	struct bt_audio_iso_find_param param = {
		.iso = NULL,
		.func = func,
		.user_data = user_data,
	};

	bt_audio_iso_foreach(bt_audio_iso_find_cb, &param);

	return param.iso;
}

void bt_audio_iso_init(struct bt_audio_iso *iso, struct bt_iso_chan_ops *ops)
{
	iso->chan.ops = ops;
	iso->chan.qos = &iso->qos;

	/* Setup points for both Tx and Rx
	 * This is due to the limitation in the ISO API where pointers like
	 * the `qos->tx` shall be initialized before the CIS is connected if
	 * ever want to use it for TX, and ditto for RX. They cannot be
	 * initialized after the CIS has been connected
	 */
	iso->chan.qos->rx = &iso->rx.qos;
	iso->chan.qos->rx->path = &iso->rx.path;
	iso->chan.qos->rx->path->cc = iso->rx.cc;

	iso->chan.qos->tx = &iso->tx.qos;
	iso->chan.qos->tx->path = &iso->tx.path;
	iso->chan.qos->tx->path->cc = iso->tx.cc;
}

void bt_audio_iso_bind_ep(struct bt_audio_iso *iso, struct bt_audio_ep *ep)
{
	__ASSERT_NO_MSG(ep != NULL);
	__ASSERT_NO_MSG(iso != NULL);
	__ASSERT(ep->iso == NULL, "ep %p bound with iso %p already", ep, ep->iso);
	__ASSERT(ep->dir == BT_AUDIO_DIR_SINK || ep->dir == BT_AUDIO_DIR_SOURCE,
		 "invalid dir: %u", ep->dir);

	LOG_DBG("iso %p ep %p dir %s", iso, ep, bt_audio_dir_str(ep->dir));

	if (IS_ENABLED(CONFIG_BT_AUDIO_UNICAST_CLIENT) &&
	    bt_audio_ep_is_unicast_client(ep)) {
		/* For the unicast client, the direction and tx/rx is reversed */
		if (ep->dir == BT_AUDIO_DIR_SOURCE) {
			__ASSERT(iso->rx.ep == NULL,
				"iso %p bound with ep %p", iso, iso->rx.ep);
			iso->rx.ep = ep;
		} else {
			__ASSERT(iso->tx.ep == NULL,
				"iso %p bound with ep %p", iso, iso->tx.ep);
			iso->tx.ep = ep;
		}
	} else {
		if (ep->dir == BT_AUDIO_DIR_SINK) {
			__ASSERT(iso->rx.ep == NULL,
				"iso %p bound with ep %p", iso, iso->rx.ep);
			iso->rx.ep = ep;
		} else {
			__ASSERT(iso->tx.ep == NULL,
				"iso %p bound with ep %p", iso, iso->tx.ep);
			iso->tx.ep = ep;
		}
	}

	ep->iso = bt_audio_iso_ref(iso);
}

void bt_audio_iso_unbind_ep(struct bt_audio_iso *iso, struct bt_audio_ep *ep)
{
	__ASSERT_NO_MSG(ep != NULL);
	__ASSERT_NO_MSG(iso != NULL);
	__ASSERT(ep->iso == iso, "ep %p not bound with iso %p, was bound to %p",
		 ep, iso, ep->iso);
	__ASSERT(ep->dir == BT_AUDIO_DIR_SINK || ep->dir == BT_AUDIO_DIR_SOURCE,
		 "Invalid dir: %u", ep->dir);

	LOG_DBG("iso %p ep %p dir %s", iso, ep, bt_audio_dir_str(ep->dir));

	if (IS_ENABLED(CONFIG_BT_AUDIO_UNICAST_CLIENT) &&
	    bt_audio_ep_is_unicast_client(ep)) {
		/* For the unicast client, the direction and tx/rx is reversed */
		if (ep->dir == BT_AUDIO_DIR_SOURCE) {
			__ASSERT(iso->rx.ep == ep,
				 "iso %p not bound with ep %p", iso, ep);
			iso->rx.ep = NULL;
		} else {
			__ASSERT(iso->tx.ep == ep,
				 "iso %p not bound with ep %p", iso, ep);
			iso->tx.ep = NULL;
		}
	} else {
		if (ep->dir == BT_AUDIO_DIR_SINK) {
			__ASSERT(iso->rx.ep == ep,
				 "iso %p not bound with ep %p", iso, ep);
			iso->rx.ep = NULL;
		} else  {
			__ASSERT(iso->tx.ep == ep,
				 "iso %p not bound with ep %p", iso, ep);
			iso->tx.ep = NULL;
		}
	}

	bt_audio_iso_unref(ep->iso);
	ep->iso = NULL;
}

struct bt_audio_ep *bt_audio_iso_get_ep(bool unicast_client,
					struct bt_audio_iso *iso,
					enum bt_audio_dir dir)
{
	__ASSERT(dir == BT_AUDIO_DIR_SINK || dir == BT_AUDIO_DIR_SOURCE,
		 "invalid dir: %u", dir);

	LOG_DBG("iso %p dir %s", iso, bt_audio_dir_str(dir));

	/* TODO FIX FOR CLIENT */
	if (IS_ENABLED(CONFIG_BT_AUDIO_UNICAST_CLIENT) && unicast_client) {
		/* For the unicast client, the direction and tx/rx is reversed */
		if (dir == BT_AUDIO_DIR_SOURCE) {
			return iso->rx.ep;
		} else {
			return iso->tx.ep;
		}
	}

	if (dir == BT_AUDIO_DIR_SINK) {
		return iso->rx.ep;
	} else {
		return iso->tx.ep;
	}
}

#if defined(CONFIG_BT_AUDIO_UNICAST_CLIENT)
void bt_audio_iso_bind_stream(struct bt_audio_iso *audio_iso,
			      struct bt_audio_stream *stream)
{
	struct bt_audio_iso_dir *audio_iso_ep;

	__ASSERT_NO_MSG(stream != NULL);
	__ASSERT_NO_MSG(audio_iso != NULL);
	__ASSERT(stream->audio_iso == NULL,
		 "stream %p bound with audio_iso %p already",
		 stream, stream->audio_iso);

	LOG_DBG("audio_iso %p stream %p dir %s",
		audio_iso, stream, bt_audio_dir_str(stream->dir));

	/* For the unicast client, the direction and tx/rx is reversed */
	if (stream->dir == BT_AUDIO_DIR_SOURCE) {
		audio_iso_ep = &audio_iso->rx;
	} else {
		audio_iso_ep = &audio_iso->tx;
	}

	__ASSERT(audio_iso_ep->stream == NULL,
		 "audio_iso %p bound with stream %p",
		 audio_iso, audio_iso_ep->stream);
	audio_iso_ep->stream = stream;

	stream->audio_iso = bt_audio_iso_ref(audio_iso);
}

void bt_audio_iso_unbind_stream(struct bt_audio_iso *audio_iso,
				struct bt_audio_stream *stream)
{
	struct bt_audio_iso_dir *audio_iso_ep;

	__ASSERT_NO_MSG(stream != NULL);
	__ASSERT_NO_MSG(audio_iso != NULL);
	__ASSERT(stream->audio_iso != NULL,
		 "stream %p not bound with an audio_iso",
		 stream);

	LOG_DBG("audio_iso %p stream %p dir %s",
		audio_iso, stream, bt_audio_dir_str(stream->dir));

	/* For the unicast client, the direction and tx/rx is reversed */
	if (stream->dir == BT_AUDIO_DIR_SOURCE) {
		audio_iso_ep = &audio_iso->rx;
	} else {
		audio_iso_ep = &audio_iso->tx;
	}

	__ASSERT(audio_iso_ep->stream == stream,
		 "audio_iso %p (%p) not bound with stream %p (%p)",
		 audio_iso, audio_iso_ep->stream, stream, stream->audio_iso);
	audio_iso_ep->stream = NULL;

	bt_audio_iso_unref(audio_iso);
	stream->audio_iso = NULL;
}

struct bt_audio_stream *bt_audio_iso_get_stream(struct bt_audio_iso *iso,
						enum bt_audio_dir dir)
{
	__ASSERT(dir == BT_AUDIO_DIR_SINK || dir == BT_AUDIO_DIR_SOURCE,
		 "invalid dir: %u", dir);

	LOG_DBG("iso %p dir %s", iso, bt_audio_dir_str(dir));

	/* For the unicast client, the direction and tx/rx is reversed */
	if (dir == BT_AUDIO_DIR_SOURCE) {
		return iso->rx.stream;
	} else {
		return iso->tx.stream;
	}
}
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT */
