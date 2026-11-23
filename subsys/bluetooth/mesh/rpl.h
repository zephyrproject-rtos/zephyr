/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2020 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct bt_mesh_rpl {
	uint64_t src:15,
		 old_iv:1,
		 seq:24,
		 /** Sequence authentication value for the previous segmented
		  *  message received from this address.
		  *
		  *  This value is used to manage the parallel RPL of the
		  *  SeqAuth values in transport.
		  */
		 seg:24;
};

typedef void (*bt_mesh_rpl_func_t)(struct bt_mesh_rpl *rpl,
					void *user_data);

void bt_mesh_rpl_reset(void);
bool bt_mesh_rpl_check(struct bt_mesh_net_rx *rx, struct bt_mesh_rpl **match, bool bridge);
void bt_mesh_rpl_clear(void);
void bt_mesh_rpl_update(struct bt_mesh_rpl *rpl,
			struct bt_mesh_net_rx *rx);
/*
 * A storage backend using the pending store scheduler schedules
 * BT_MESH_SETTINGS_RPL_PENDING for entry updates and
 * BT_MESH_SETTINGS_RPL_RESET_PENDING when an IV Index update or recovery
 * invalidates the list. Both run this function, which must cancel both flags.
 */
void bt_mesh_rpl_pending_store_all_nodes(void);
