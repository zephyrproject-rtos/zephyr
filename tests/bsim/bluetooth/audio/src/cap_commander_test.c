/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/audio/micp.h>
#include <zephyr/bluetooth/audio/vcp.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include "bstests.h"
#include "common.h"
#include "bap_common.h"

#if defined(CONFIG_BT_CAP_COMMANDER)

#define SEM_TIMEOUT K_SECONDS(5)

extern enum bst_result_t bst_result;

static struct bt_conn *connected_conns[CONFIG_BT_MAX_CONN];
static volatile size_t connected_conn_cnt;

static struct bt_le_scan_recv_info broadcaster_info;
static bt_addr_le_t broadcaster_addr;
static struct bt_le_per_adv_sync *g_pa_sync;
static uint32_t broadcaster_broadcast_id;

static uint8_t received_base[UINT8_MAX];
static uint8_t received_base_size;
static uint8_t src_id[CONFIG_BT_MAX_CONN];

static struct k_sem sem_disconnected;
static struct k_sem sem_cas_discovered;
static struct k_sem sem_vcs_discovered;
static struct k_sem sem_mics_discovered;
static struct k_sem sem_bass_discovered;

CREATE_FLAG(flag_mtu_exchanged);
CREATE_FLAG(flag_cap_canceled);
CREATE_FLAG(flag_volume_changed);
CREATE_FLAG(flag_volume_mute_changed);
CREATE_FLAG(flag_volume_offset_changed);
CREATE_FLAG(flag_microphone_mute_changed);
CREATE_FLAG(flag_microphone_gain_changed);

CREATE_FLAG(flag_broadcast_reception_start);
CREATE_FLAG(flag_broadcast_reception_stop);
CREATE_FLAG(flag_broadcaster_found);
CREATE_FLAG(flag_base_received);
CREATE_FLAG(flag_recv_state_updated_with_bis_sync);
CREATE_FLAG(flag_pa_synced);
CREATE_FLAG(flag_pa_sync_lost);
CREATE_FLAG(flag_syncable);

static void cap_discovery_complete_cb(struct bt_conn *conn, int err,
				      const struct bt_csip_set_coordinator_set_member *member,
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
	if (err == -ECANCELED) {
		printk("CAP command cancelled for conn %p\n", conn);
		SET_FLAG(flag_cap_canceled);
		return;
	}

	if (err != 0) {
		FAIL("Failed to change volume for conn %p: %d\n", conn, err);
		return;
	}

	SET_FLAG(flag_volume_changed);
}

static void cap_volume_mute_changed_cb(struct bt_conn *conn, int err)
{
	if (err == -ECANCELED) {
		printk("CAP command cancelled for conn %p\n", conn);
		SET_FLAG(flag_cap_canceled);
		return;
	}

	if (err != 0) {
		FAIL("Failed to change volume mute for conn %p: %d\n", conn, err);
		return;
	}

	SET_FLAG(flag_volume_mute_changed);
}

#if defined(CONFIG_BT_VCP_VOL_CTLR_VOCS)
static void cap_volume_offset_changed_cb(struct bt_conn *conn, int err)
{
	if (err == -ECANCELED) {
		printk("CAP command cancelled for conn %p\n", conn);
		SET_FLAG(flag_cap_canceled);
		return;
	}

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
	if (err == -ECANCELED) {
		printk("CAP command cancelled for conn %p\n", conn);
		SET_FLAG(flag_cap_canceled);
		return;
	}

	if (err != 0) {
		FAIL("Failed to change microphone mute for conn %p: %d\n", conn, err);
		return;
	}

	SET_FLAG(flag_microphone_mute_changed);
}

#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
static void cap_microphone_gain_changed_cb(struct bt_conn *conn, int err)
{
	if (err == -ECANCELED) {
		printk("CAP command cancelled for conn %p\n", conn);
		SET_FLAG(flag_cap_canceled);
		return;
	}

	if (err != 0) {
		FAIL("Failed to change microphone gain for conn %p: %d\n", conn, err);
		return;
	}

	SET_FLAG(flag_microphone_gain_changed);
}
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */
#endif /* CONFIG_BT_MICP_MIC_CTLR */

#if defined(CONFIG_BT_BAP_BROADCAST_ASSISTANT)
static void cap_broadcast_reception_start_cb(struct bt_conn *conn, int err)
{
	if (err == -ECANCELED) {
		printk("CAP command cancelled for conn %p\n", conn);
		SET_FLAG(flag_cap_canceled);
		return;
	}

	if (err != 0) {
		FAIL("Failed to perform broadcast reception start for conn %p: %d\n", conn, err);
		return;
	}

	SET_FLAG(flag_broadcast_reception_start);
}

static void cap_broadcast_reception_stop_cb(struct bt_conn *conn, int err)
{
	if (err == -ECANCELED) {
		printk("CAP command cancelled for conn %p\n", conn);
		SET_FLAG(flag_cap_canceled);
		return;
	}

	if (err != 0) {
		FAIL("Failed to perform broadcast reception stop for conn %p: %d\n", conn, err);
		return;
	}

	SET_FLAG(flag_broadcast_reception_stop);
}
#endif /* CONFIG_BT_BAP_BROADCAST_ASSISTANT*/

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
#if defined(CONFIG_BT_BAP_BROADCAST_ASSISTANT)
	.broadcast_reception_start = cap_broadcast_reception_start_cb,
	.broadcast_reception_stop = cap_broadcast_reception_stop_cb,
#endif /* CONFIG_BT_BAP_BROADCAST_ASSISTANT*/
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

static int pa_sync_create(void)
{
	struct bt_le_per_adv_sync_param create_params = {0};
	int err;

	bt_addr_le_copy(&create_params.addr, &broadcaster_addr);
	create_params.options = 0;
	create_params.sid = broadcaster_info.sid;
	create_params.skip = PA_SYNC_SKIP;
	create_params.timeout = interval_to_sync_timeout(broadcaster_info.interval);

	err = bt_le_per_adv_sync_create(&create_params, &g_pa_sync);
	return err;
}

static bool scan_check_and_sync_broadcast(struct bt_data *data, void *user_data)
{
	const struct bt_le_scan_recv_info *info = user_data;
	char le_addr[BT_ADDR_LE_STR_LEN];
	struct bt_uuid_16 adv_uuid;
	uint32_t broadcast_id;

	if (TEST_FLAG(flag_broadcaster_found)) {
		/* no-op*/
		printk("NO OP\n");
		return false;
	}

	if (data->type != BT_DATA_SVC_DATA16) {
		return true;
	}

	if (data->data_len < BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE) {
		return true;
	}

	if (!bt_uuid_create(&adv_uuid.uuid, data->data, BT_UUID_SIZE_16)) {
		return true;
	}

	if (bt_uuid_cmp(&adv_uuid.uuid, BT_UUID_BROADCAST_AUDIO)) {
		return true;
	}

	broadcast_id = sys_get_le24(data->data + BT_UUID_SIZE_16);

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("Found broadcaster with ID 0x%06X and addr %s and sid 0x%02X\n", broadcast_id,
	       le_addr, info->sid);
	printk("Adv type %02X interval %u\n", info->adv_type, info->interval);

	SET_FLAG(flag_broadcaster_found);

	/* Store info for PA sync parameters */
	memcpy(&broadcaster_info, info, sizeof(broadcaster_info));
	bt_addr_le_copy(&broadcaster_addr, info->addr);
	broadcaster_broadcast_id = broadcast_id;

	/* Stop parsing */
	return false;
}

static void broadcast_scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *ad)
{
	if (info->interval != 0U) {
		bt_data_parse(ad, scan_check_and_sync_broadcast, (void *)info);
	}
}

static struct bt_le_scan_cb bap_scan_cb = {
	.recv = broadcast_scan_recv,
};

static void bap_pa_sync_synced_cb(struct bt_le_per_adv_sync *sync,
				  struct bt_le_per_adv_sync_synced_info *info)
{
	if (sync == g_pa_sync) {
		printk("PA sync %p synced for broadcast sink with broadcast ID 0x%06X\n", sync,
		       broadcaster_broadcast_id);
		SET_FLAG(flag_pa_synced);
	}
}

static void bap_pa_sync_terminated_cb(struct bt_le_per_adv_sync *sync,
				      const struct bt_le_per_adv_sync_term_info *info)
{
	if (sync == g_pa_sync) {
		printk("CAP commander test PA sync %p lost with reason %u\n", sync, info->reason);
		g_pa_sync = NULL;

		SET_FLAG(flag_pa_sync_lost);
	}
}

static bool base_store(struct bt_data *data, void *user_data)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(data);
	uint8_t base_size;
	int base_subgroup_count;

	/* Base is NULL if the data does not contain a valid BASE */
	if (base == NULL) {
		return true;
	}

	/* Can not fit all the received subgroups with the size CONFIG_BT_BAP_BASS_MAX_SUBGROUPS */
	base_subgroup_count = bt_bap_base_get_subgroup_count(base);
	if (base_subgroup_count < 0 || base_subgroup_count > CONFIG_BT_BAP_BASS_MAX_SUBGROUPS) {
		printk("Got invalid subgroup count: %d\n", base_subgroup_count);
		return true;
	}

	base_size = data->data_len - BT_UUID_SIZE_16; /* the BASE comes after the UUID */

	/* Compare BASE and copy if different */
	if (base_size != received_base_size || memcmp(base, received_base, base_size) != 0) {
		(void)memcpy(received_base, base, base_size);
		received_base_size = base_size;
	}

	/* Stop parsing */
	return false;
}

static void pa_recv(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_recv_info *info, struct net_buf_simple *buf)
{
	if (TEST_FLAG(flag_base_received)) {
		return;
	}

	bt_data_parse(buf, base_store, NULL);
	SET_FLAG(flag_base_received);
}

static struct bt_le_per_adv_sync_cb bap_pa_sync_cb = {
	.synced = bap_pa_sync_synced_cb,
	.term = bap_pa_sync_terminated_cb,
	.recv = pa_recv,
};

static void bap_broadcast_assistant_discover_cb(struct bt_conn *conn, int err,
						uint8_t recv_state_count)
{
	if (err == 0) {
		printk("BASS discover done with %u recv states\n", recv_state_count);
	} else {
		printk("BASS discover failed (%d)\n", err);
	}

	k_sem_give(&sem_bass_discovered);
}

static void bap_broadcast_assistant_add_src_cb(struct bt_conn *conn, int err)
{
	if (err == 0) {
		printk("BASS add source successful\n");
	} else {
		printk("BASS add source failed (%d)\n", err);
	}
}

static bool metadata_entry(struct bt_data *data, void *user_data)
{
	char metadata[CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE];

	(void)bin2hex(data->data, data->data_len, metadata, sizeof(metadata));

	printk("\t\tMetadata length %u, type %u, data: %s\n", data->data_len, data->type, metadata);

	return true;
}

static void
bap_broadcast_assistant_recv_state_cb(struct bt_conn *conn, int err,
				      const struct bt_bap_scan_delegator_recv_state *state)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char bad_code[BT_ISO_BROADCAST_CODE_SIZE * 2 + 1];
	size_t acceptor_count = get_dev_cnt() - 2;

	if (err != 0) {
		FAIL("BASS recv state read failed (%d)\n", err);
		return;
	}

	if (state == NULL) {
		/* Empty receive state */
		return;
	}

	bt_addr_le_to_str(&state->addr, le_addr, sizeof(le_addr));
	(void)bin2hex(state->bad_code, BT_ISO_BROADCAST_CODE_SIZE, bad_code, sizeof(bad_code));
	printk("BASS recv state: src_id %u, addr %s, sid %u, sync_state %u, "
	       "encrypt_state %u%s%s\n",
	       state->src_id, le_addr, state->adv_sid, state->pa_sync_state, state->encrypt_state,
	       state->encrypt_state == BT_BAP_BIG_ENC_STATE_BAD_CODE ? ", bad code" : "", bad_code);

	if (state->encrypt_state == BT_BAP_BIG_ENC_STATE_BAD_CODE) {
		FAIL("Encryption state is BT_BAP_BIG_ENC_STATE_BAD_CODE");
		return;
	}

	for (size_t index = 0; index < acceptor_count; index++) {
		if (conn == connected_conns[index]) {
			src_id[index] = state->src_id;
		}
	}

	for (uint8_t i = 0; i < state->num_subgroups; i++) {
		const struct bt_bap_bass_subgroup *subgroup = &state->subgroups[i];
		struct net_buf_simple buf;

		printk("\t[%d]: BIS sync %u, metadata_len %u\n", i, subgroup->bis_sync,
		       subgroup->metadata_len);

		net_buf_simple_init_with_data(&buf, (void *)subgroup->metadata,
					      subgroup->metadata_len);
		bt_data_parse(&buf, metadata_entry, NULL);

		if (subgroup->bis_sync != 0) {
			SET_FLAG(flag_recv_state_updated_with_bis_sync);
		}
	}

#if defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER)
	if (state->pa_sync_state == BT_BAP_PA_STATE_INFO_REQ) {
		err = bt_le_per_adv_sync_transfer(g_pa_sync, conn, BT_UUID_BASS_VAL);
		if (err != 0) {
			FAIL("Could not transfer periodic adv sync: %d\n", err);
			return;
		}
	}
#endif /* CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER */

	if (state->pa_sync_state == BT_BAP_PA_STATE_SYNCED) {
	}
}

static struct bt_bap_broadcast_assistant_cb ba_cbs = {
	.discover = bap_broadcast_assistant_discover_cb,
	.recv_state = bap_broadcast_assistant_recv_state_cb,
	.add_src = bap_broadcast_assistant_add_src_cb,
};

static bool check_audio_support_and_connect_cb(struct bt_data *data, void *user_data)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t *addr = user_data;
	const struct bt_uuid *uuid;
	uint16_t uuid_val;
	int err;

	printk("data->type %u\n", data->type);

	if (data->type != BT_DATA_SVC_DATA16) {
		return true; /* Continue parsing to next AD data type */
	}

	if (data->data_len < sizeof(uuid_val)) {
		return true; /* Continue parsing to next AD data type */
	}

	/* We are looking for the CAS service data */
	uuid_val = sys_get_le16(data->data);
	uuid = BT_UUID_DECLARE_16(uuid_val);
	if (bt_uuid_cmp(uuid, BT_UUID_CAS) != 0) {
		return true; /* Continue parsing to next AD data type */
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device found: %s\n", addr_str);

	printk("Stopping scan\n");
	if (bt_le_scan_stop()) {
		FAIL("Could not stop scan");
		return false;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM(BT_GAP_INIT_CONN_INT_MIN, BT_GAP_INIT_CONN_INT_MIN,
						 0, BT_GAP_MS_TO_CONN_TIMEOUT(4000)),
				&connected_conns[connected_conn_cnt]);
	if (err != 0) {
		FAIL("Could not connect to peer: %d", err);
	}

	return false; /* Stop parsing */
}

static void scan_recv_cb(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf)
{
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, info->addr);
	if (conn != NULL) {
		/* Already connected to this device */
		bt_conn_unref(conn);
		return;
	}

	/* Check for connectable, extended advertising */
	if (((info->adv_props & BT_GAP_ADV_PROP_EXT_ADV) != 0) &&
	    ((info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE)) != 0) {
		/* Check for TMAS support in advertising data */
		bt_data_parse(buf, check_audio_support_and_connect_cb, (void *)info->addr);
	}
}

static void init(size_t acceptor_cnt)
{
	static struct bt_le_scan_cb scan_callbacks = {
		.recv = scan_recv_cb,
	};
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
	err = bt_le_scan_cb_register(&scan_callbacks);
	if (err != 0) {
		FAIL("Failed to register scan callbacks (err %d)\n", err);
		return;
	}

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

	err = bt_bap_broadcast_assistant_register_cb(&ba_cbs);
	if (err != 0) {
		FAIL("Failed to register broadcast assistant callbacks (err %d)\n");
		return;
	}

	bt_le_per_adv_sync_cb_register(&bap_pa_sync_cb);
	bt_le_scan_cb_register(&bap_scan_cb);

	k_sem_init(&sem_disconnected, 0, acceptor_cnt);
	k_sem_init(&sem_cas_discovered, 0, acceptor_cnt);
	k_sem_init(&sem_bass_discovered, 0, acceptor_cnt);
	k_sem_init(&sem_vcs_discovered, 0, acceptor_cnt);
	k_sem_init(&sem_mics_discovered, 0, acceptor_cnt);

	UNSET_FLAG(flag_mtu_exchanged);
	UNSET_FLAG(flag_cap_canceled);
	UNSET_FLAG(flag_volume_changed);
	UNSET_FLAG(flag_volume_mute_changed);
	UNSET_FLAG(flag_volume_offset_changed);
	UNSET_FLAG(flag_microphone_mute_changed);
	UNSET_FLAG(flag_microphone_gain_changed);

	UNSET_FLAG(flag_broadcast_reception_start);
	UNSET_FLAG(flag_broadcast_reception_stop);
	UNSET_FLAG(flag_broadcaster_found);
	UNSET_FLAG(flag_base_received);
	UNSET_FLAG(flag_recv_state_updated_with_bis_sync);
	UNSET_FLAG(flag_pa_synced);
	UNSET_FLAG(flag_pa_sync_lost);
	UNSET_FLAG(flag_syncable);
}

static void scan_and_connect(void)
{
	int err;

	UNSET_FLAG(flag_connected);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
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

		if (err == 0) {
			connected_conn_cnt--;
		} else {
			const struct bt_conn *conn = connected_conns[i];

			FAIL("Failed to take sem_disconnected for %p: %d", (void *)conn, err);
			return;
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

static void discover_bass(size_t acceptor_cnt)
{
	k_sem_reset(&sem_bass_discovered);

	if (acceptor_cnt > 1) {
		FAIL("Current implementation does not support multiple connections for the "
		     "broadcast assistant");
		return;
	}

	for (size_t i = 0U; i < acceptor_cnt; i++) {
		int err;

		err = bt_bap_broadcast_assistant_discover(connected_conns[i]);
		if (err != 0) {
			FAIL("Failed to discover BASS on the sink (err %d)\n", err);
			return;
		}
	}

	for (size_t i = 0U; i < acceptor_cnt; i++) {
		const int err = k_sem_take(&sem_bass_discovered, SEM_TIMEOUT);

		if (err != 0) {
			FAIL("Failed to take sem_bass_discovered for %p: %d",
			     (void *)connected_conns[i], err);
		}
	}
}

static void pa_sync_to_broadcaster(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
	if (err != 0) {
		FAIL("Unable to start scan for broadcast sources: %d", err);
		return;
	}

	printk("Searching for a broadcaster\n");
	WAIT_FOR_FLAG(flag_broadcaster_found);

	err = bt_le_scan_stop();
	if (err != 0) {
		FAIL("bt_le_scan_stop failed with %d\n", err);
		return;
	}

	printk("Scan stopped, attempting to PA sync to the broadcaster with id 0x%06X\n",
	       broadcaster_broadcast_id);
	err = pa_sync_create();
	if (err != 0) {
		FAIL("Could not create Broadcast PA sync: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_pa_synced); /* todo from bap_pa_sync_synced_cb, bap_pa_sync_cb */

	printk("Broadcast source PA synced, waiting for BASE\n");
	WAIT_FOR_FLAG(flag_base_received);
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

static void init_change_volume(void)
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

	for (size_t i = 0U; i < param.count; i++) {
		param.members[i].member = connected_conns[i];
	}

	err = bt_cap_commander_change_volume(&param);
	if (err != 0) {
		FAIL("Failed to change volume: %d\n", err);
		return;
	}
}

static void test_change_volume(void)
{
	UNSET_FLAG(flag_volume_changed);
	init_change_volume();
	WAIT_FOR_FLAG(flag_volume_changed);
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

static void test_broadcast_reception_start(size_t acceptor_count)
{
	struct bt_cap_commander_broadcast_reception_start_param reception_start_param = {0};
	struct bt_cap_commander_broadcast_reception_start_member_param param[CONFIG_BT_MAX_CONN] = {
		0};
	int err;

	reception_start_param.type = BT_CAP_SET_TYPE_AD_HOC;
	reception_start_param.count = acceptor_count;
	reception_start_param.param = param;

	for (size_t i = 0; i < acceptor_count; i++) {
		uint32_t bis_sync[CONFIG_BT_BAP_BASS_MAX_SUBGROUPS];
		size_t num_subgroups;

		reception_start_param.param[i].member.member = connected_conns[i];
		bt_addr_le_copy(&reception_start_param.param[i].addr, &broadcaster_addr);
		reception_start_param.param[i].adv_sid = broadcaster_info.sid;
		reception_start_param.param[i].pa_interval = broadcaster_info.interval;
		reception_start_param.param[i].broadcast_id = broadcaster_broadcast_id;
		num_subgroups =
			bt_bap_base_get_subgroup_count((const struct bt_bap_base *)received_base);
		err = bt_bap_base_get_bis_indexes((const struct bt_bap_base *)received_base,
						  bis_sync);
		if (err != 0) {
			FAIL("Could not populate subgroup information: %d\n", err);
			return;
		}

		reception_start_param.param[i].num_subgroups = num_subgroups;
		for (size_t j = 0; j < num_subgroups; j++) {
			reception_start_param.param[i].subgroups[j].bis_sync = bis_sync[j];
		}
	}

	err = bt_cap_commander_broadcast_reception_start(&reception_start_param);
	if (err != 0) {
		FAIL("Could not initiate broadcast reception start: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_broadcast_reception_start);
}

static void test_broadcast_reception_stop(size_t acceptor_count)
{
	struct bt_cap_commander_broadcast_reception_stop_param reception_stop_param = {0};
	struct bt_cap_commander_broadcast_reception_stop_member_param param[CONFIG_BT_MAX_CONN] = {
		0};

	int err;

	reception_stop_param.type = BT_CAP_SET_TYPE_AD_HOC;
	reception_stop_param.param = param;
	reception_stop_param.count = acceptor_count;
	for (size_t i = 0; i < acceptor_count; i++) {
		uint8_t num_subgroups;

		reception_stop_param.param[i].member.member = connected_conns[i];

		reception_stop_param.param[i].src_id = src_id[i];
		num_subgroups =
			bt_bap_base_get_subgroup_count((const struct bt_bap_base *)received_base);
		reception_stop_param.param[i].num_subgroups = num_subgroups;
	}
	err = bt_cap_commander_broadcast_reception_stop(&reception_stop_param);
	if (err != 0) {
		FAIL("Could not initiate broadcast reception stop: %d\n", err);
		return;
	}
	WAIT_FOR_FLAG(flag_broadcast_reception_stop);
}

static void test_distribute_broadcast_code(size_t acceptor_count)
{
	struct bt_cap_commander_distribute_broadcast_code_param distribute_broadcast_code_param = {
		0};
	struct bt_cap_commander_distribute_broadcast_code_member_param param[CONFIG_BT_MAX_CONN] = {
		0};

	distribute_broadcast_code_param.type = BT_CAP_SET_TYPE_AD_HOC;
	distribute_broadcast_code_param.param = param;
	distribute_broadcast_code_param.count = acceptor_count;
	memcpy(distribute_broadcast_code_param.broadcast_code, BROADCAST_CODE,
	       sizeof(BROADCAST_CODE));
	for (size_t i = 0; i < acceptor_count; i++) {

		distribute_broadcast_code_param.param[i].member.member = connected_conns[i];
		distribute_broadcast_code_param.param[i].src_id = src_id[i];
	}

	bt_cap_commander_distribute_broadcast_code(&distribute_broadcast_code_param);
}

static void test_cancel(bool cap_in_progress)
{
	const int expected_err = cap_in_progress ? 0 : -EALREADY;
	int err;

	err = bt_cap_commander_cancel();
	if (err != expected_err) {
		FAIL("Could not cancel CAP command: %d\n", err);
		return;
	}
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

static void test_main_cap_commander_broadcast_reception(void)
{
	size_t acceptor_count;

	/* The test consists of N devices
	 * 1 device is the broadcast source
	 * 1 device is the CAP commander
	 * This leaves N - 2 devices for the acceptor
	 */
	acceptor_count = get_dev_cnt() - 2;
	printk("Acceptor count: %d\n", acceptor_count);

	init(acceptor_count);

	for (size_t i = 0U; i < acceptor_count; i++) {
		scan_and_connect();

		WAIT_FOR_FLAG(flag_mtu_exchanged);
	}

	/* TODO: We should use CSIP to find set members */
	discover_cas(acceptor_count);
	discover_bass(acceptor_count);

	pa_sync_to_broadcaster();

	test_broadcast_reception_start(acceptor_count);

	test_distribute_broadcast_code(acceptor_count);

	backchannel_sync_wait_any(); /* wait for the acceptor to receive data */

	test_broadcast_reception_stop(acceptor_count);

	backchannel_sync_wait_any(); /* wait for the acceptor to stop reception */

	/* Disconnect all CAP acceptors */
	disconnect_acl(acceptor_count);

	PASS("Broadcast reception passed\n");
}

static void test_main_cap_commander_cancel(void)
{
	size_t acceptor_count;

	/* The test consists of N devices
	 * 1 device is the broadcast source
	 * 1 device is the CAP commander
	 * This leaves N - 2 devices for the acceptor
	 */
	acceptor_count = get_dev_cnt() - 1;
	printk("Acceptor count: %d\n", acceptor_count);

	init(acceptor_count);

	for (size_t i = 0U; i < acceptor_count; i++) {
		scan_and_connect();

		WAIT_FOR_FLAG(flag_mtu_exchanged);
	}

	/* TODO: We should use CSIP to find set members */
	discover_cas(acceptor_count);

	if (IS_ENABLED(CONFIG_BT_CSIP_SET_COORDINATOR)) {
		if (IS_ENABLED(CONFIG_BT_VCP_VOL_CTLR)) {
			discover_vcs(acceptor_count);

			init_change_volume();

			test_cancel(true);
			WAIT_FOR_FLAG(flag_cap_canceled);
		}
	}

	test_cancel(false);

	/* Disconnect all CAP acceptors */
	disconnect_acl(acceptor_count);

	/* restore the default callback */
	cap_cb.volume_changed = cap_volume_changed_cb;

	PASS("Broadcast reception passed\n");
}

static const struct bst_test_instance test_cap_commander[] = {
	{
		.test_id = "cap_commander_capture_and_render",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_cap_commander_capture_and_render,
	},
	{
		.test_id = "cap_commander_broadcast_reception",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_cap_commander_broadcast_reception,
	},
	{
		.test_id = "cap_commander_cancel",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_cap_commander_cancel,
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
