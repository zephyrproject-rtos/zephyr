/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CAP_COMMANDER)

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/micp.h>
#include <zephyr/bluetooth/audio/vcp.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include "common.h"
#include "bap_common.h"

#define SEM_TIMEOUT K_SECONDS(5)

extern enum bst_result_t bst_result;

static struct bt_conn *connected_conns[CONFIG_BT_MAX_CONN];
static volatile size_t connected_conn_cnt;

static struct k_sem sem_disconnected;
static struct k_sem sem_cas_discovered;
static struct k_sem sem_vcs_discovered;
static struct k_sem sem_mics_discovered;
CREATE_FLAG(flag_mtu_exchanged);
CREATE_FLAG(flag_volume_changed);
CREATE_FLAG(flag_volume_mute_changed);
CREATE_FLAG(flag_volume_offset_changed);
CREATE_FLAG(flag_microphone_mute_changed);
CREATE_FLAG(flag_microphone_gain_changed);

static void cap_discovery_complete_cb(struct bt_conn *conn, int err,
				      const struct bt_csip_set_coordinator_csis_inst *csis_inst)
{
	if (err != 0) {
		FAIL("Discover failed on %p: %d\n", (void *)conn, err);

		return;
	}

	if (IS_ENABLED(CONFIG_BT_CAP_ACCEPTOR_SET_MEMBER)) {
		if (csis_inst == NULL) {
			FAIL("Failed to discover CAS CSIS");

			return;
		}

		printk("Found CAS on %p with CSIS %p\n", (void *)conn, csis_inst);
	} else {
		printk("Found CAS on %p\n", (void *)conn);
	}

	k_sem_give(&sem_cas_discovered);
}

#if defined(CONFIG_BT_VCP_VOL_CTLR)
static void cap_volume_changed_cb(struct bt_conn *conn, int err)
{
	if (err != 0) {
		FAIL("Failed to change volume for conn %p: %d\n", conn, err);
		return;
	}

	SET_FLAG(flag_volume_changed);
}

static void cap_volume_mute_changed_cb(struct bt_conn *conn, int err)
{
	if (err != 0) {
		FAIL("Failed to change volume mute for conn %p: %d\n", conn, err);
		return;
	}

	SET_FLAG(flag_volume_mute_changed);
}

#if defined(CONFIG_BT_VCP_VOL_CTLR_VOCS)
static void cap_volume_offset_changed_cb(struct bt_conn *conn, int err)
{
	if (err != 0) {
		FAIL("Failed to change volume offset for conn %p: %d\n", conn, err);
		return;
	}

	SET_FLAG(flag_volume_offset_changed);
}
#endif /* CONFIG_BT_VCP_VOL_CTLR_VOCS */
#endif /* CONFIG_BT_VCP_VOL_CTLR */

#if defined(CONFIG_BT_MICP_MIC_CTLR)
static void cap_microphone_mute_changed_cb(struct bt_conn *conn, int err)
{
	if (err != 0) {
		FAIL("Failed to change microphone mute for conn %p: %d\n", conn, err);
		return;
	}

	SET_FLAG(flag_microphone_mute_changed);
}

#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
static void cap_microphone_gain_changed_cb(struct bt_conn *conn, int err)
{
	if (err != 0) {
		FAIL("Failed to change microphone gain for conn %p: %d\n", conn, err);
		return;
	}

	SET_FLAG(flag_microphone_gain_changed);
}
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */
#endif /* CONFIG_BT_MICP_MIC_CTLR */

static struct bt_cap_commander_cb cap_cb = {
	.discovery_complete = cap_discovery_complete_cb,
#if defined(CONFIG_BT_VCP_VOL_CTLR)
	.volume_changed = cap_volume_changed_cb,
	.volume_mute_changed = cap_volume_mute_changed_cb,
#if defined(CONFIG_BT_VCP_VOL_CTLR_VOCS)
	.volume_offset_changed = cap_volume_offset_changed_cb,
#endif /* CONFIG_BT_VCP_VOL_CTLR_VOCS */
#endif /* CONFIG_BT_VCP_VOL_CTLR */
#if defined(CONFIG_BT_MICP_MIC_CTLR)
	.microphone_mute_changed = cap_microphone_mute_changed_cb,
#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
	.microphone_gain_changed = cap_microphone_gain_changed_cb,
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */
#endif /* CONFIG_BT_MICP_MIC_CTLR */
};

static void cap_vcp_discover_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err, uint8_t vocs_count,
				uint8_t aics_count)
{
	if (err != 0) {
		FAIL("Failed to discover VCS: %d\n", err);

		return;
	}

	printk("VCS for %p found with %u VOCS and %u AICS\n", vol_ctlr, vocs_count, aics_count);
	k_sem_give(&sem_vcs_discovered);
}

static void cap_vcp_state_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err, uint8_t volume,
			     uint8_t mute)
{
	if (err != 0) {
		FAIL("VCP state cb err (%d)\n", err);
		return;
	}

	printk("State for %p: volume %u, mute %u\n", vol_ctlr, volume, mute);
}

static struct bt_vcp_vol_ctlr_cb vcp_cb = {
	.discover = cap_vcp_discover_cb,
	.state = cap_vcp_state_cb,
};

static void cap_micp_discover_cb(struct bt_micp_mic_ctlr *mic_ctlr, int err, uint8_t aics_count)
{
	if (err != 0) {
		FAIL("Failed to discover MICS: %d\n", err);

		return;
	}

	printk("MICS for %p found with %u AICS\n", mic_ctlr, aics_count);
	k_sem_give(&sem_mics_discovered);
}

static struct bt_micp_mic_ctlr_cb micp_cb = {
	.discover = cap_micp_discover_cb,
};

static void att_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	printk("MTU exchanged\n");
	SET_FLAG(flag_mtu_exchanged);
}

static struct bt_gatt_cb gatt_callbacks = {
	.att_mtu_updated = att_mtu_updated,
};

static void cap_disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	k_sem_give(&sem_disconnected);
}

static void init(size_t acceptor_cnt)
{
	static struct bt_conn_cb conn_cb = {
		.disconnected = cap_disconnected_cb,
	};
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	bt_gatt_cb_register(&gatt_callbacks);
	bt_conn_cb_register(&conn_cb);

	err = bt_cap_commander_register_cb(&cap_cb);
	if (err != 0) {
		FAIL("Failed to register CAP callbacks (err %d)\n", err);
		return;
	}

	err = bt_vcp_vol_ctlr_cb_register(&vcp_cb);
	if (err != 0) {
		FAIL("Failed to register VCP callbacks (err %d)\n", err);
		return;
	}

	err = bt_micp_mic_ctlr_cb_register(&micp_cb);
	if (err != 0) {
		FAIL("Failed to register MICP callbacks (err %d)\n", err);
		return;
	}

	k_sem_init(&sem_disconnected, 0, acceptor_cnt);
	k_sem_init(&sem_cas_discovered, 0, acceptor_cnt);
	k_sem_init(&sem_vcs_discovered, 0, acceptor_cnt);
	k_sem_init(&sem_mics_discovered, 0, acceptor_cnt);
}

static void cap_device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			     struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	struct bt_conn *conn;
	int err;

	/* We're only interested in connectable events */
	if (type != BT_HCI_ADV_IND && type != BT_HCI_ADV_DIRECT_IND) {
		return;
	}

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, addr);
	if (conn != NULL) {
		/* Already connected to this device */
		bt_conn_unref(conn);
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device found: %s (RSSI %d)\n", addr_str, rssi);

	/* connect only to devices in close proximity */
	if (rssi < -70) {
		FAIL("RSSI too low");
		return;
	}

	printk("Stopping scan\n");
	if (bt_le_scan_stop()) {
		FAIL("Could not stop scan");
		return;
	}

	err = bt_conn_le_create(
		addr, BT_CONN_LE_CREATE_CONN,
		BT_LE_CONN_PARAM(BT_GAP_INIT_CONN_INT_MIN, BT_GAP_INIT_CONN_INT_MIN, 0, 400),
		&connected_conns[connected_conn_cnt]);
	if (err) {
		FAIL("Could not connect to peer: %d", err);
	}
}

static void scan_and_connect(void)
{
	int err;

	UNSET_FLAG(flag_connected);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, cap_device_found);
	if (err != 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
	WAIT_FOR_FLAG(flag_connected);
	connected_conn_cnt++;
}

static void disconnect_acl(size_t acceptor_cnt)
{
	k_sem_reset(&sem_disconnected);

	for (size_t i = 0U; i < acceptor_cnt; i++) {
		struct bt_conn *conn = connected_conns[i];
		int err;

		printk("Disconnecting %p\n", (void *)conn);

		err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (err != 0) {
			FAIL("Failed to disconnect %p (err %d)\n", (void *)conn, err);
			return;
		}
	}

	for (size_t i = 0U; i < acceptor_cnt; i++) {
		const int err = k_sem_take(&sem_disconnected, SEM_TIMEOUT);

		if (err != 0) {
			const struct bt_conn *conn = connected_conns[i];

			FAIL("Failed to take sem_disconnected for %p: %d", (void *)conn, err);
		}
	}
}

static void discover_cas(size_t acceptor_cnt)
{
	k_sem_reset(&sem_cas_discovered);

	/* Do parallel discovery */
	for (size_t i = 0U; i < acceptor_cnt; i++) {
		struct bt_conn *conn = connected_conns[i];
		int err;

		printk("Discovering CAS on %p\n", (void *)conn);

		err = bt_cap_commander_discover(conn);
		if (err != 0) {
			FAIL("Failed to discover CAS on %p: %d\n", (void *)conn, err);
			return;
		}
	}

	for (size_t i = 0U; i < acceptor_cnt; i++) {
		const int err = k_sem_take(&sem_cas_discovered, SEM_TIMEOUT);

		if (err != 0) {
			const struct bt_conn *conn = connected_conns[i];

			FAIL("Failed to take sem_cas_discovered for %p: %d", (void *)conn, err);
		}
	}
}

static void discover_vcs(size_t acceptor_cnt)
{
	k_sem_reset(&sem_vcs_discovered);

	/* Do parallel discovery */
	for (size_t i = 0U; i < acceptor_cnt; i++) {
		struct bt_conn *conn = connected_conns[i];
		struct bt_vcp_vol_ctlr *vol_ctlr;
		int err;

		printk("Discovering VCS on %p\n", (void *)conn);

		err = bt_vcp_vol_ctlr_discover(conn, &vol_ctlr);
		if (err != 0) {
			FAIL("Failed to discover VCS on %p: %d\n", err);
			return;
		}
	}

	for (size_t i = 0U; i < acceptor_cnt; i++) {
		const int err = k_sem_take(&sem_vcs_discovered, SEM_TIMEOUT);

		if (err != 0) {
			const struct bt_conn *conn = connected_conns[i];

			FAIL("Failed to take sem_vcs_discovered for %p: %d", (void *)conn, err);
		}
	}
}

static void discover_mics(size_t acceptor_cnt)
{
	k_sem_reset(&sem_mics_discovered);

	for (size_t i = 0U; i < acceptor_cnt; i++) {
		struct bt_micp_mic_ctlr *mic_ctlr;
		int err;

		err = bt_micp_mic_ctlr_discover(connected_conns[i], &mic_ctlr);
		if (err != 0) {
			FAIL("Failed to discover MICS: %d\n", err);
			return;
		}
	}

	for (size_t i = 0U; i < acceptor_cnt; i++) {
		const int err = k_sem_take(&sem_mics_discovered, SEM_TIMEOUT);

		if (err != 0) {
			const struct bt_conn *conn = connected_conns[i];

			FAIL("Failed to take sem_mics_discovered for %p: %d", (void *)conn, err);
		}
	}
}

static void test_change_volume(void)
{
	union bt_cap_set_member members[CONFIG_BT_MAX_CONN];
	const struct bt_cap_commander_change_volume_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.members = members,
		.count = connected_conn_cnt,
		.volume = 177,
	};
	int err;

	printk("Changing volume to %u\n", param.volume);
	UNSET_FLAG(flag_volume_changed);

	for (size_t i = 0U; i < param.count; i++) {
		param.members[i].member = connected_conns[i];
	}

	err = bt_cap_commander_change_volume(&param);
	if (err != 0) {
		FAIL("Failed to change volume: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_volume_changed);
	printk("Volume changed to %u\n", param.volume);
}

static void test_change_volume_mute(bool mute)
{
	union bt_cap_set_member members[CONFIG_BT_MAX_CONN];
	const struct bt_cap_commander_change_volume_mute_state_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.members = members,
		.count = connected_conn_cnt,
		.mute = mute,
	};
	int err;

	printk("Changing volume mute state to %d\n", param.mute);
	UNSET_FLAG(flag_volume_mute_changed);

	for (size_t i = 0U; i < param.count; i++) {
		param.members[i].member = connected_conns[i];
	}

	err = bt_cap_commander_change_volume_mute_state(&param);
	if (err != 0) {
		FAIL("Failed to change volume mute: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_volume_mute_changed);
	printk("Volume mute state changed to %d\n", param.mute);
}

static void test_change_volume_offset(void)
{
	struct bt_cap_commander_change_volume_offset_member_param member_params[CONFIG_BT_MAX_CONN];
	const struct bt_cap_commander_change_volume_offset_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.param = member_params,
		.count = connected_conn_cnt,
	};
	int err;

	printk("Changing volume offset\n");
	UNSET_FLAG(flag_volume_offset_changed);

	for (size_t i = 0U; i < param.count; i++) {
		member_params[i].member.member = connected_conns[i];
		member_params[i].offset = 100 + i;
	}

	err = bt_cap_commander_change_volume_offset(&param);
	if (err != 0) {
		FAIL("Failed to change volume offset: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_volume_offset_changed);
	printk("Volume offset changed\n");
}

static void test_change_microphone_mute(bool mute)
{
	union bt_cap_set_member members[CONFIG_BT_MAX_CONN];
	const struct bt_cap_commander_change_microphone_mute_state_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.members = members,
		.count = connected_conn_cnt,
		.mute = mute,
	};
	int err;

	printk("Changing microphone mute state to %d\n", param.mute);
	UNSET_FLAG(flag_microphone_mute_changed);

	for (size_t i = 0U; i < param.count; i++) {
		param.members[i].member = connected_conns[i];
	}

	err = bt_cap_commander_change_microphone_mute_state(&param);
	if (err != 0) {
		FAIL("Failed to change microphone mute: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_microphone_mute_changed);
	printk("Microphone mute state changed to %d\n", param.mute);
}

static void test_change_microphone_gain(void)
{
	struct bt_cap_commander_change_microphone_gain_setting_member_param
		member_params[CONFIG_BT_MAX_CONN];
	const struct bt_cap_commander_change_microphone_gain_setting_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.param = member_params,
		.count = connected_conn_cnt,
	};
	int err;

	printk("Changing microphone gain\n");
	UNSET_FLAG(flag_microphone_gain_changed);

	for (size_t i = 0U; i < param.count; i++) {
		member_params[i].member.member = connected_conns[i];
		member_params[i].gain = 10 + i;
	}

	err = bt_cap_commander_change_microphone_gain_setting(&param);
	if (err != 0) {
		FAIL("Failed to change microphone gain: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_microphone_gain_changed);
	printk("Microphone gain changed\n");
}

static void test_main_cap_commander_capture_and_render(void)
{
	const size_t acceptor_cnt = get_dev_cnt() - 1; /* Assume all other devices are acceptors
							*/
	init(acceptor_cnt);

	/* Connect to and do discovery on all CAP acceptors */
	for (size_t i = 0U; i < acceptor_cnt; i++) {
		scan_and_connect();

		WAIT_FOR_FLAG(flag_mtu_exchanged);
	}

	/* TODO: We should use CSIP to find set members */
	discover_cas(acceptor_cnt);
	discover_cas(acceptor_cnt); /* verify that we can discover twice */

	if (IS_ENABLED(CONFIG_BT_CSIP_SET_COORDINATOR)) {
		if (IS_ENABLED(CONFIG_BT_VCP_VOL_CTLR)) {
			discover_vcs(acceptor_cnt);

			test_change_volume();

			test_change_volume_mute(true);
			test_change_volume_mute(false);

			if (IS_ENABLED(CONFIG_BT_VCP_VOL_CTLR_VOCS)) {
				test_change_volume_offset();
			}
		}

		if (IS_ENABLED(CONFIG_BT_MICP_MIC_CTLR)) {
			discover_mics(acceptor_cnt);

			test_change_microphone_mute(true);
			test_change_microphone_mute(false);

			if (IS_ENABLED(CONFIG_BT_MICP_MIC_CTLR_AICS)) {
				test_change_microphone_gain();
			}
		}
	}

	/* Disconnect all CAP acceptors */
	disconnect_acl(acceptor_cnt);

	PASS("CAP commander capture and rendering passed\n");
}

static const struct bst_test_instance test_cap_commander[] = {
	{
		.test_id = "cap_commander_capture_and_render",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_cap_commander_capture_and_render,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_cap_commander_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_cap_commander);
}

#else /* !CONFIG_BT_CAP_COMMANDER */

struct bst_test_list *test_cap_commander_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_CAP_COMMANDER */
