/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2020 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/net_buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/mesh.h>

#include "mesh.h"
#include "net.h"
#include "rpl.h"
#include "settings.h"

#define LOG_LEVEL CONFIG_BT_MESH_RPL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_rpl);

/* Replay Protection List information for persistent storage. */
struct rpl_val {
	uint32_t seq:24,
	      old_iv:1;
};

static struct bt_mesh_rpl replay_list[CONFIG_BT_MESH_CRPL];
static ATOMIC_DEFINE(store, CONFIG_BT_MESH_CRPL);

enum {
	PENDING_CLEAR,
	PENDING_RESET,
	RPL_FLAGS_COUNT,
};
static ATOMIC_DEFINE(rpl_flags, RPL_FLAGS_COUNT);

static inline int rpl_idx(const struct bt_mesh_rpl *rpl)
{
	return rpl - &replay_list[0];
}

static void clear_rpl(struct bt_mesh_rpl *rpl)
{
	int err;
	char path[18];

	if (!rpl->src) {
		return;
	}

	atomic_clear_bit(store, rpl_idx(rpl));

	snprintk(path, sizeof(path), "bt/mesh/RPL/%x", rpl->src);
	err = settings_delete(path);
	if (err) {
		LOG_ERR("Failed to clear RPL");
	} else {
		LOG_DBG("Cleared RPL");
	}
}

static void schedule_rpl_store(struct bt_mesh_rpl *entry, bool force)
{
	atomic_set_bit(store, rpl_idx(entry));

	if (force
#ifdef CONFIG_BT_MESH_RPL_STORE_TIMEOUT
	    || CONFIG_BT_MESH_RPL_STORE_TIMEOUT >= 0
#endif
	    ) {
		bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_RPL_PENDING);
	}
}

void bt_mesh_rpl_update(struct bt_mesh_rpl *rpl,
		struct bt_mesh_net_rx *rx)
{
	/* If this is the first message on the new IV index, we should reset it
	 * to zero to avoid invalid combinations of IV index and seg.
	 */
	if (rpl->old_iv && !rx->old_iv) {
		rpl->seg = 0;
	}

	rpl->src = rx->ctx.addr;
	rpl->seq = rx->seq;
	rpl->old_iv = rx->old_iv;

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		schedule_rpl_store(rpl, false);
	}
}

/* Check the Replay Protection List for a replay attempt. If non-NULL match
 * parameter is given the RPL slot is returned, but it is not immediately
 * updated. This is used to prevent storing data in RPL that has been rejected
 * by upper logic (access, transport commands) and for receiving the segmented messages.
 * If a NULL match is given the RPL is immediately updated (used for proxy configuration).
 */
bool bt_mesh_rpl_check(struct bt_mesh_net_rx *rx, struct bt_mesh_rpl **match, bool bridge)
{
	struct bt_mesh_rpl *rpl;
	int i;

	/* Don't bother checking messages from ourselves */
	if (rx->net_if == BT_MESH_NET_IF_LOCAL) {
		return false;
	}

	/* The RPL is used only for the local node or Subnet Bridge. */
	if (!rx->local_match && !bridge) {
		return false;
	}

	for (i = 0; i < ARRAY_SIZE(replay_list); i++) {
		rpl = &replay_list[i];

		/* Empty slot */
		if (!rpl->src) {
			goto match;
		}

		/* Existing slot for given address */
		if (rpl->src == rx->ctx.addr) {
			if (!rpl->old_iv &&
			    atomic_test_bit(rpl_flags, PENDING_RESET) &&
			    !atomic_test_bit(store, i)) {
				/* Until rpl reset is finished, entry with old_iv == false and
				 * without "store" bit set will be removed, therefore it can be
				 * reused. If such entry is reused, "store" bit will be set and
				 * the entry won't be removed.
				 */
				goto match;
			}

			if (rx->old_iv && !rpl->old_iv) {
				return true;
			}

			if ((!rx->old_iv && rpl->old_iv) ||
			    rpl->seq < rx->seq) {
				goto match;
			} else {
				return true;
			}
		}
	}

	LOG_ERR("RPL is full!");
	return true;

match:
	if (match) {
		*match = rpl;
	} else {
		bt_mesh_rpl_update(rpl, rx);
	}

	return false;
}

void bt_mesh_rpl_clear(void)
{
	LOG_DBG("");

	if (!IS_ENABLED(CONFIG_BT_SETTINGS)) {
		(void)memset(replay_list, 0, sizeof(replay_list));
		return;
	}

	atomic_set_bit(rpl_flags, PENDING_CLEAR);

	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_RPL_PENDING);
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
	/* Discard "old old" IV Index entries from RPL and flag
	 * any other ones (which are valid) as old.
	 */
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		int i;

		for (i = 0; i < ARRAY_SIZE(replay_list); i++) {
			struct bt_mesh_rpl *rpl = &replay_list[i];

			if (!rpl->src) {
				continue;
			}

			/* Entries with "store" bit set will be stored, other entries will be
			 * removed.
			 */
			atomic_set_bit_to(store, i, !rpl->old_iv);
			rpl->old_iv = !rpl->old_iv;
		}

		if (i != 0) {
			atomic_set_bit(rpl_flags, PENDING_RESET);
			bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_RPL_PENDING);
		}
	} else {
		int shift = 0;
		int last = 0;

		for (int i = 0; i < ARRAY_SIZE(replay_list); i++) {
			struct bt_mesh_rpl *rpl = &replay_list[i];

			if (rpl->src) {
				if (rpl->old_iv) {
					(void)memset(rpl, 0, sizeof(*rpl));

					shift++;
				} else {
					rpl->old_iv = true;

					if (shift > 0) {
						replay_list[i - shift] = *rpl;
					}
				}

				last = i;
			}
		}

		(void)memset(&replay_list[last - shift + 1], 0, sizeof(struct bt_mesh_rpl) * shift);
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
		LOG_ERR("Insufficient number of arguments");
		return -ENOENT;
	}

	src = strtol(name, NULL, 16);
	entry = bt_mesh_rpl_find(src);

	if (len_rd == 0) {
		LOG_DBG("val (null)");
		if (entry) {
			(void)memset(entry, 0, sizeof(*entry));
		} else {
			LOG_WRN("Unable to find RPL entry for 0x%04x", src);
		}

		return 0;
	}

	if (!entry) {
		entry = bt_mesh_rpl_alloc(src);
		if (!entry) {
			LOG_ERR("Unable to allocate RPL entry for 0x%04x", src);
			return -ENOMEM;
		}
	}

	err = bt_mesh_settings_set(read_cb, cb_arg, &rpl, sizeof(rpl));
	if (err) {
		LOG_ERR("Failed to set `net`");
		return err;
	}

	entry->seq = rpl.seq;
	entry->old_iv = rpl.old_iv;

	LOG_DBG("RPL entry for 0x%04x: Seq 0x%06x old_iv %u", entry->src, entry->seq,
		entry->old_iv);

	return 0;
}

BT_MESH_SETTINGS_DEFINE(rpl, "RPL", rpl_set);

static void store_rpl(struct bt_mesh_rpl *entry)
{
	struct rpl_val rpl = {0};
	char path[18];
	int err;

	if (!entry->src) {
		return;
	}

	LOG_DBG("src 0x%04x seq 0x%06x old_iv %u", entry->src, entry->seq, entry->old_iv);

	rpl.seq = entry->seq;
	rpl.old_iv = entry->old_iv;

	snprintk(path, sizeof(path), "bt/mesh/RPL/%x", entry->src);

	err = settings_save_one(path, &rpl, sizeof(rpl));
	if (err) {
		LOG_ERR("Failed to store RPL %s value", path);
	} else {
		LOG_DBG("Stored RPL %s value", path);
	}
}

void bt_mesh_rpl_pending_store(uint16_t addr)
{
	int shift = 0;
	int last = 0;
	bool clr;
	bool rst;

	if (!IS_ENABLED(CONFIG_BT_SETTINGS) ||
	    (!BT_MESH_ADDR_IS_UNICAST(addr) &&
	     addr != BT_MESH_ADDR_ALL_NODES)) {
		return;
	}

	if (addr == BT_MESH_ADDR_ALL_NODES) {
		bt_mesh_settings_store_cancel(BT_MESH_SETTINGS_RPL_PENDING);
	}

	clr = atomic_test_and_clear_bit(rpl_flags, PENDING_CLEAR);
	rst = atomic_test_bit(rpl_flags, PENDING_RESET);

	for (int i = 0; i < ARRAY_SIZE(replay_list); i++) {
		struct bt_mesh_rpl *rpl = &replay_list[i];

		if (addr != BT_MESH_ADDR_ALL_NODES && addr != rpl->src) {
			continue;
		}

		if (clr) {
			clear_rpl(rpl);
			shift++;
		} else if (atomic_test_and_clear_bit(store, i)) {
			if (shift > 0) {
				replay_list[i - shift] = *rpl;
			}

			store_rpl(&replay_list[i - shift]);
		} else if (rst) {
			clear_rpl(rpl);

			/* Check if this entry was re-used during removal. If so, shift it as well.
			 * Otherwise, increment shift counter.
			 */
			if (atomic_test_and_clear_bit(store, i)) {
				replay_list[i - shift] = *rpl;
				atomic_set_bit(store, i - shift);
			} else {
				shift++;
			}
		}

		last = i;

		if (addr != BT_MESH_ADDR_ALL_NODES) {
			break;
		}
	}

	atomic_clear_bit(rpl_flags, PENDING_RESET);

	if (addr == BT_MESH_ADDR_ALL_NODES) {
		(void)memset(&replay_list[last - shift + 1], 0, sizeof(struct bt_mesh_rpl) * shift);
	}
}
