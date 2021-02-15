/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2020 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/atomic.h>
#include <sys/util.h>
#include <sys/byteorder.h>

#include <net/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_RPL)
#define LOG_MODULE_NAME bt_mesh_rpl
#include "common/log.h"

#include "mesh.h"
#include "adv.h"
#include "net.h"
#include "rpl.h"
#include "settings.h"

/* Replay Protection List information for persistent storage. */
struct rpl_val {
	uint32_t seq:24,
	      old_iv:1;
};

static struct bt_mesh_rpl replay_list[CONFIG_BT_MESH_CRPL];

static void schedule_rpl_store(struct bt_mesh_rpl *entry)
{
#ifdef CONFIG_BT_SETTINGS
	entry->store = true;
#endif
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_RPL_PENDING);
}

static void schedule_rpl_clear(void)
{
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_RPL_PENDING);
}

void bt_mesh_rpl_update(struct bt_mesh_rpl *rpl,
		struct bt_mesh_net_rx *rx)
{
	rpl->src = rx->ctx.addr;
	rpl->seq = rx->seq;
	rpl->old_iv = rx->old_iv;

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		schedule_rpl_store(rpl);
	}
}

/* Check the Replay Protection List for a replay attempt. If non-NULL match
 * parameter is given the RPL slot is returned but it is not immediately
 * updated (needed for segmented messages), whereas if a NULL match is given
 * the RPL is immediately updated (used for unsegmented messages).
 */
bool bt_mesh_rpl_check(struct bt_mesh_net_rx *rx,
		struct bt_mesh_rpl **match)
{
	int i;

	/* Don't bother checking messages from ourselves */
	if (rx->net_if == BT_MESH_NET_IF_LOCAL) {
		return false;
	}

	/* The RPL is used only for the local node */
	if (!rx->local_match) {
		return false;
	}

	for (i = 0; i < ARRAY_SIZE(replay_list); i++) {
		struct bt_mesh_rpl *rpl = &replay_list[i];

		/* Empty slot */
		if (!rpl->src) {
			if (match) {
				*match = rpl;
			} else {
				bt_mesh_rpl_update(rpl, rx);
			}

			return false;
		}

		/* Existing slot for given address */
		if (rpl->src == rx->ctx.addr) {
			if (rx->old_iv && !rpl->old_iv) {
				return true;
			}

			if ((!rx->old_iv && rpl->old_iv) ||
			    rpl->seq < rx->seq) {
				if (match) {
					*match = rpl;
				} else {
					bt_mesh_rpl_update(rpl, rx);
				}

				return false;
			} else {
				return true;
			}
		}
	}

	BT_ERR("RPL is full!");
	return true;
}

void bt_mesh_rpl_clear(void)
{
	BT_DBG("");

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		schedule_rpl_clear();
	} else {
		(void)memset(replay_list, 0, sizeof(replay_list));
	}
}

static struct bt_mesh_rpl *bt_mesh_rpl_find(uint16_t src)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(replay_list); i++) {
		if (replay_list[i].src == src) {
			return &replay_list[i];
		}
	}

	return NULL;
}

static struct bt_mesh_rpl *bt_mesh_rpl_alloc(uint16_t src)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(replay_list); i++) {
		if (!replay_list[i].src) {
			replay_list[i].src = src;
			return &replay_list[i];
		}
	}

	return NULL;
}

void bt_mesh_rpl_reset(void)
{
	int i;

	/* Discard "old old" IV Index entries from RPL and flag
	 * any other ones (which are valid) as old.
	 */
	for (i = 0; i < ARRAY_SIZE(replay_list); i++) {
		struct bt_mesh_rpl *rpl = &replay_list[i];

		if (rpl->src) {
			if (rpl->old_iv) {
				(void)memset(rpl, 0, sizeof(*rpl));
			} else {
				rpl->old_iv = true;
			}

			if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
				schedule_rpl_store(rpl);
			}
		}
	}
}

static int rpl_set(const char *name, size_t len_rd,
		   settings_read_cb read_cb, void *cb_arg)
{
	struct bt_mesh_rpl *entry;
	struct rpl_val rpl;
	int err;
	uint16_t src;

	if (!name) {
		BT_ERR("Insufficient number of arguments");
		return -ENOENT;
	}

	src = strtol(name, NULL, 16);
	entry = bt_mesh_rpl_find(src);

	if (len_rd == 0) {
		BT_DBG("val (null)");
		if (entry) {
			(void)memset(entry, 0, sizeof(*entry));
		} else {
			BT_WARN("Unable to find RPL entry for 0x%04x", src);
		}

		return 0;
	}

	if (!entry) {
		entry = bt_mesh_rpl_alloc(src);
		if (!entry) {
			BT_ERR("Unable to allocate RPL entry for 0x%04x", src);
			return -ENOMEM;
		}
	}

	err = bt_mesh_settings_set(read_cb, cb_arg, &rpl, sizeof(rpl));
	if (err) {
		BT_ERR("Failed to set `net`");
		return err;
	}

	entry->seq = rpl.seq;
	entry->old_iv = rpl.old_iv;

	BT_DBG("RPL entry for 0x%04x: Seq 0x%06x old_iv %u", entry->src,
	       entry->seq, entry->old_iv);

	return 0;
}

BT_MESH_SETTINGS_DEFINE(rpl, "RPL", rpl_set);

static void store_rpl(struct bt_mesh_rpl *entry)
{
	struct rpl_val rpl;
	char path[18];
	int err;

	BT_DBG("src 0x%04x seq 0x%06x old_iv %u", entry->src, entry->seq,
	       entry->old_iv);

	rpl.seq = entry->seq;
	rpl.old_iv = entry->old_iv;

	snprintk(path, sizeof(path), "bt/mesh/RPL/%x", entry->src);

	err = settings_save_one(path, &rpl, sizeof(rpl));
	if (err) {
		BT_ERR("Failed to store RPL %s value", log_strdup(path));
	} else {
		BT_DBG("Stored RPL %s value", log_strdup(path));
	}
}

static void clear_rpl(struct bt_mesh_rpl *rpl)
{
	int err;
	char path[18];

	if (!rpl->src) {
		return;
	}

	snprintk(path, sizeof(path), "bt/mesh/RPL/%x", rpl->src);
	err = settings_delete(path);
	if (err) {
		BT_ERR("Failed to clear RPL");
	} else {
		BT_DBG("Cleared RPL");
	}

	(void)memset(rpl, 0, sizeof(*rpl));
}

static void store_pending_rpl(struct bt_mesh_rpl *rpl)
{
	BT_DBG("");

#ifdef CONFIG_BT_SETTINGS
	if (!rpl->store) {
		return;
	}

	rpl->store = false;
#endif
	store_rpl(rpl);
}

void bt_mesh_rpl_pending_store(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(replay_list); i++) {
		if (atomic_test_bit(bt_mesh.flags, BT_MESH_VALID)) {
			store_pending_rpl(&replay_list[i]);
		} else {
			clear_rpl(&replay_list[i]);
		}
	}
}
