/** @file
 *  @brief Internal APIs for Bluetooth Hearing Access Profile.
 */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct bt_hap_harc;

struct hap_ha_binaural {
	struct bt_conn *conn;
	struct bt_vocs *vocs;
	struct bt_vcp_vol_ctlr *vol_ctlr;
	const struct bt_csip_set_coordinator_set_member *csip_member;
	struct bt_hap_harc *pair;
};

struct hap_ha_monaural {
	struct bt_conn *conn;
	struct bt_has *has;
	struct bt_vcp_vol_ctlr *vol_ctlr;
};

struct hap_ha_banded {
	struct bt_conn *conn;
	struct bt_vocs *vocs_left;
	struct bt_vocs *vocs_right;
	struct bt_vcp_vol_ctlr *vol_ctlr;
};
