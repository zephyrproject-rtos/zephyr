/** @file
 *  @brief Bluetooth Common Audio Profile (CAP) Acceptor broadcast.
 *
 *  Copyright (c) 2024 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

#include "cap_acceptor.h"

LOG_MODULE_REGISTER(cap_acceptor_broadcast, LOG_LEVEL_INF);

#define NAME_LEN                          sizeof(CONFIG_SAMPLE_TARGET_BROADCAST_NAME) + 1
#define PA_SYNC_INTERVAL_TO_TIMEOUT_RATIO 20 /* Set the timeout relative to interval */
#define PA_SYNC_SKIP                      5

enum broadcast_flag {
	FLAG_BROADCAST_SYNC_REQUESTED,
	FLAG_BROADCAST_CODE_REQUIRED,
	FLAG_BROADCAST_CODE_RECEIVED,
	FLAG_BROADCAST_SYNCABLE,
	FLAG_BROADCAST_SYNCING,
	FLAG_BROADCAST_SYNCED,
	FLAG_BASE_RECEIVED,
	FLAG_PA_SYNCING,
	FLAG_PA_SYNCED,
	FLAG_SCANNING,
	FLAG_NUM,
};
ATOMIC_DEFINE(flags, FLAG_NUM);

static struct broadcast_sink {
	const struct bt_bap_scan_delegator_recv_state *req_recv_state;
	uint8_t sink_broadcast_code[BT_AUDIO_BROADCAST_CODE_SIZE];
	struct bt_bap_broadcast_sink *bap_broadcast_sink;
	struct bt_cap_stream broadcast_stream;
	struct bt_le_per_adv_sync *pa_sync;
	uint8_t received_base[UINT8_MAX];
	uint32_t requested_bis_sync;
	uint32_t broadcast_id;
} broadcast_sink;

uint64_t total_broadcast_rx_iso_packet_count; /* This value is exposed to test code */

/** Starts scanning if it passes a series of check to determine if we are in the right state */
static int check_start_scan(void)
{
	int err;

	if (atomic_test_bit(flags, FLAG_SCANNING)) {
		return -EALREADY;
	}

	if (atomic_test_bit(flags, FLAG_PA_SYNCED)) {
		return -EALREADY;
	}

	if (atomic_test_bit(flags, FLAG_BROADCAST_SYNCED)) {
		return -EALREADY;
	}

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
	if (err != 0) {
		LOG_ERR("Unable to start scan for CAP initiators: %d", err);

		return err;
	}

	atomic_set_bit(flags, FLAG_SCANNING);

	return 0;
}

static void broadcast_stream_started_cb(struct bt_bap_stream *bap_stream)
{
	LOG_INF("Started bap_stream %p", bap_stream);
	total_broadcast_rx_iso_packet_count = 0U;

	atomic_clear_bit(flags, FLAG_BROADCAST_SYNCING);
	atomic_set_bit(flags, FLAG_BROADCAST_SYNCED);
}

static void broadcast_stream_stopped_cb(struct bt_bap_stream *bap_stream, uint8_t reason)
{
	LOG_INF("Stopped bap_stream %p with reason 0x%02X", bap_stream, reason);

	atomic_clear_bit(flags, FLAG_BROADCAST_SYNCING);
	atomic_clear_bit(flags, FLAG_BROADCAST_SYNCED);

	if (IS_ENABLED(CONFIG_SAMPLE_SCAN_SELF)) {
		(void)check_start_scan();
	}
}

static void broadcast_stream_recv_cb(struct bt_bap_stream *bap_stream,
				     const struct bt_iso_recv_info *info, struct net_buf *buf)
{
	/* Triggered every time we receive an HCI data packet from the controller.
	 * A call to this does not indicate valid data
	 * (see the `info->flags` for which flags to check),
	 */

	if ((total_broadcast_rx_iso_packet_count % 100U) == 0U) {
		LOG_INF("Received %llu HCI ISO data packets", total_broadcast_rx_iso_packet_count);
	}

	total_broadcast_rx_iso_packet_count++;
}

static int create_broadcast_sink(void)
{
	int err;

	if (broadcast_sink.bap_broadcast_sink != NULL) {
		return -EALREADY;
	}

	LOG_INF("Creating broadcast sink for broadcast ID 0x%06X", broadcast_sink.broadcast_id);

	err = bt_bap_broadcast_sink_create(broadcast_sink.pa_sync, broadcast_sink.broadcast_id,
					   &broadcast_sink.bap_broadcast_sink);
	if (err != 0) {
		LOG_ERR("Failed to create broadcast sink: %d\n", err);

		return err;
	}

	return 0;
}

/** Performs a series of checks to see if we are ready to sync the broadcast sink */
static void check_sync_broadcast(void)
{
	struct bt_bap_stream *sync_stream = &broadcast_sink.broadcast_stream.bap_stream;
	uint32_t sync_bitfield;
	int err;

	if (!atomic_test_bit(flags, FLAG_BASE_RECEIVED)) {
		LOG_DBG("FLAG_BASE_RECEIVED");
		return;
	}

	if (!atomic_test_bit(flags, FLAG_BROADCAST_SYNCABLE)) {
		LOG_DBG("FLAG_BROADCAST_SYNCABLE");
		return;
	}

	if (atomic_test_bit(flags, FLAG_BROADCAST_CODE_REQUIRED) &&
	    !atomic_test_bit(flags, FLAG_BROADCAST_CODE_RECEIVED)) {
		LOG_DBG("FLAG_BROADCAST_CODE_REQUIRED");
		return;
	}

	if (!atomic_test_bit(flags, FLAG_BROADCAST_SYNC_REQUESTED)) {
		LOG_DBG("FLAG_BROADCAST_SYNC_REQUESTED");
		return;
	}

	if (!atomic_test_bit(flags, FLAG_PA_SYNCED)) {
		LOG_DBG("FLAG_PA_SYNCED");
		return;
	}

	if (atomic_test_bit(flags, FLAG_BROADCAST_SYNCED) ||
	    atomic_test_bit(flags, FLAG_BROADCAST_SYNCING)) {
		LOG_DBG("FLAG_BROADCAST_SYNCED");
		return;
	}

	if (broadcast_sink.requested_bis_sync == BT_BAP_BIS_SYNC_NO_PREF) {
		uint32_t base_bis;

		/* Get the first BIS index from the BASE */
		err = bt_bap_base_get_bis_indexes(
			(struct bt_bap_base *)broadcast_sink.received_base, &base_bis);
		if (err != 0) {
			LOG_ERR("Failed to get BIS indexes from BASE: %d", err);

			return;
		}

		sync_bitfield = 0U;
		for (uint8_t i = BT_ISO_BIS_INDEX_MIN; i < BT_ISO_BIS_INDEX_MAX; i++) {
			if (base_bis & BT_ISO_BIS_INDEX_BIT(i)) {
				sync_bitfield = BT_ISO_BIS_INDEX_BIT(i);

				break;
			}
		}
	} else {
		sync_bitfield = broadcast_sink.requested_bis_sync;
	}

	LOG_INF("Syncing to broadcast with bitfield 0x%08X", sync_bitfield);

	/* Sync the BIG */
	err = bt_bap_broadcast_sink_sync(broadcast_sink.bap_broadcast_sink, sync_bitfield,
					 &sync_stream, broadcast_sink.sink_broadcast_code);
	if (err != 0) {
		LOG_ERR("Failed to sync the broadcast sink: %d", err);
	} else {
		atomic_set_bit(flags, FLAG_BROADCAST_SYNCING);
	}
}

static void base_recv_cb(struct bt_bap_broadcast_sink *sink, const struct bt_bap_base *base,
			 size_t base_size)
{
	memcpy(broadcast_sink.received_base, base, base_size);

	if (!atomic_test_and_set_bit(flags, FLAG_BASE_RECEIVED)) {
		LOG_INF("BASE received");

		check_sync_broadcast();
	}
}

static void syncable_cb(struct bt_bap_broadcast_sink *sink, const struct bt_iso_biginfo *biginfo)
{
	if (!biginfo->encryption) {
		atomic_clear_bit(flags, FLAG_BROADCAST_CODE_REQUIRED);
	} else {
		atomic_set_bit(flags, FLAG_BROADCAST_CODE_REQUIRED);
	}

	if (!atomic_test_and_set_bit(flags, FLAG_BROADCAST_SYNCABLE)) {
		LOG_INF("BIGInfo received");

		check_sync_broadcast();
	}
}

static void pa_timer_handler(struct k_work *work)
{
	atomic_clear_bit(flags, FLAG_PA_SYNCING);

	if (broadcast_sink.pa_sync != NULL) {
		int err;

		err = bt_le_per_adv_sync_delete(broadcast_sink.pa_sync);
		if (err != 0) {
			LOG_ERR("Failed to delete PA sync: %d", err);
		}
	}

	if (broadcast_sink.req_recv_state != NULL) {
		enum bt_bap_pa_state pa_state;

		if (broadcast_sink.req_recv_state->pa_sync_state == BT_BAP_PA_STATE_INFO_REQ) {
			pa_state = BT_BAP_PA_STATE_NO_PAST;
		} else {
			pa_state = BT_BAP_PA_STATE_FAILED;
		}

		bt_bap_scan_delegator_set_pa_state(broadcast_sink.req_recv_state->src_id, pa_state);
	}

	LOG_INF("PA sync timeout");
}

static K_WORK_DELAYABLE_DEFINE(pa_timer, pa_timer_handler);

static uint16_t interval_to_sync_timeout(uint16_t pa_interval)
{
	uint16_t pa_timeout;

	if (pa_interval == BT_BAP_PA_INTERVAL_UNKNOWN) {
		/* Use maximum value to maximize chance of success */
		pa_timeout = BT_GAP_PER_ADV_MAX_TIMEOUT;
	} else {
		uint32_t interval_ms;
		uint32_t timeout;

		/* Add retries and convert to unit in 10's of ms */
		interval_ms = BT_GAP_PER_ADV_INTERVAL_TO_MS(pa_interval);
		timeout = (interval_ms * PA_SYNC_INTERVAL_TO_TIMEOUT_RATIO) / 10;

		/* Enforce restraints */
		pa_timeout = CLAMP(timeout, BT_GAP_PER_ADV_MIN_TIMEOUT, BT_GAP_PER_ADV_MAX_TIMEOUT);
	}

	return pa_timeout;
}

static int pa_sync_with_past(struct bt_conn *conn,
			     const struct bt_bap_scan_delegator_recv_state *recv_state,
			     uint16_t pa_interval)
{
	struct bt_le_per_adv_sync_transfer_param param = {0};
	int err;

	param.skip = PA_SYNC_SKIP;
	param.timeout = interval_to_sync_timeout(pa_interval);

	err = bt_le_per_adv_sync_transfer_subscribe(conn, &param);
	if (err != 0) {
		LOG_ERR("Could not do PAST subscribe: %d", err);

		return err;
	}

	err = bt_bap_scan_delegator_set_pa_state(recv_state->src_id, BT_BAP_PA_STATE_INFO_REQ);
	if (err != 0) {
		LOG_ERR("Failed to set PA state to BT_BAP_PA_STATE_INFO_REQ: %d", err);

		return err;
	}

	k_work_reschedule(&pa_timer, K_MSEC(param.timeout * 10));

	return 0;
}

static int pa_sync_without_past(const bt_addr_le_t *addr, uint8_t adv_sid, uint16_t pa_interval)
{
	struct bt_le_per_adv_sync_param param = {0};
	int err;

	bt_addr_le_copy(&param.addr, addr);
	param.options = BT_LE_PER_ADV_SYNC_OPT_FILTER_DUPLICATE;
	param.sid = adv_sid;
	param.skip = PA_SYNC_SKIP;
	param.timeout = interval_to_sync_timeout(pa_interval);

	err = bt_le_per_adv_sync_create(&param, &broadcast_sink.pa_sync);
	if (err != 0) {
		LOG_ERR("Failed to create PA sync: %d", err);

		return err;
	}

	k_work_reschedule(&pa_timer, K_MSEC(param.timeout * 10));

	return 0;
}

static int pa_sync_req_cb(struct bt_conn *conn,
			  const struct bt_bap_scan_delegator_recv_state *recv_state,
			  bool past_avail, uint16_t pa_interval)
{

	LOG_INF("Received request to sync to PA (PAST %savailble): %u", past_avail ? "" : "not ",
		recv_state->pa_sync_state);

	broadcast_sink.req_recv_state = recv_state;

	if (recv_state->pa_sync_state == BT_BAP_PA_STATE_SYNCED ||
	    recv_state->pa_sync_state == BT_BAP_PA_STATE_INFO_REQ ||
	    broadcast_sink.pa_sync != NULL) {
		/* Already syncing */
		LOG_WRN("Rejecting PA sync request because we are already syncing or synced");

		return -EALREADY;
	}

	if (IS_ENABLED(CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER) && past_avail) {
		int err;

		err = pa_sync_with_past(conn, recv_state, pa_interval);
		if (err != 0) {
			return err;
		}

		LOG_INF("Syncing with PAST");
	} else {
		int err;

		err = pa_sync_without_past(&recv_state->addr, recv_state->adv_sid, pa_interval);
		if (err != 0) {
			return err;
		}

		LOG_INF("Syncing without PAST");
	}

	broadcast_sink.broadcast_id = recv_state->broadcast_id;
	atomic_set_bit(flags, FLAG_PA_SYNCING);

	return 0;
}

static int pa_sync_term_req_cb(struct bt_conn *conn,
			       const struct bt_bap_scan_delegator_recv_state *recv_state)
{
	int err;

	broadcast_sink.req_recv_state = recv_state;

	err = bt_le_per_adv_sync_delete(broadcast_sink.pa_sync);
	if (err != 0) {
		LOG_ERR("Failed to delete PA sync: %d", err);

		return err;
	}

	k_work_cancel_delayable(&pa_timer);
	broadcast_sink.pa_sync = NULL;

	return 0;
}

static void broadcast_code_cb(struct bt_conn *conn,
			      const struct bt_bap_scan_delegator_recv_state *recv_state,
			      const uint8_t broadcast_code[BT_AUDIO_BROADCAST_CODE_SIZE])
{
	LOG_INF("Broadcast code received for %p", recv_state);

	broadcast_sink.req_recv_state = recv_state;

	(void)memcpy(broadcast_sink.sink_broadcast_code, broadcast_code,
		     BT_AUDIO_BROADCAST_CODE_SIZE);

	atomic_set_bit(flags, FLAG_BROADCAST_CODE_RECEIVED);
}

static uint32_t get_req_bis_sync(const uint32_t bis_sync_req[CONFIG_BT_BAP_BASS_MAX_SUBGROUPS])
{
	uint32_t bis_sync = 0U;

	for (int i = 0; i < CONFIG_BT_BAP_BASS_MAX_SUBGROUPS; i++) {
		bis_sync |= bis_sync_req[i];
	}

	return bis_sync;
}

static int bis_sync_req_cb(struct bt_conn *conn,
			   const struct bt_bap_scan_delegator_recv_state *recv_state,
			   const uint32_t bis_sync_req[CONFIG_BT_BAP_BASS_MAX_SUBGROUPS])
{
	const uint32_t new_bis_sync_req = get_req_bis_sync(bis_sync_req);

	LOG_INF("BIS sync request received for %p: 0x%08x", recv_state, bis_sync_req[0]);

	if (new_bis_sync_req != BT_BAP_BIS_SYNC_NO_PREF && POPCOUNT(new_bis_sync_req) > 1U) {
		LOG_WRN("Rejecting BIS sync request for 0x%08X as we do not support that",
			new_bis_sync_req);

		return -ENOMEM;
	}

	if (broadcast_sink.requested_bis_sync != new_bis_sync_req) {
		return 0; /* no op */
	}

	if (atomic_test_bit(flags, FLAG_BROADCAST_SYNCED)) {
		/* If the BIS sync request is received while we are already
		 * synced, it means that the requested BIS sync has changed.
		 */
		int err;

		/* The stream stopped callback will be called as part of this,
		 * and we do not need to wait for any events from the
		 * controller. Thus, when this returns, the broadcast sink is stopped
		 */
		err = bt_bap_broadcast_sink_stop(broadcast_sink.bap_broadcast_sink);
		if (err != 0) {
			LOG_ERR("Failed to stop Broadcast Sink: %d", err);

			return err;
		}

		err = bt_bap_broadcast_sink_delete(broadcast_sink.bap_broadcast_sink);
		if (err != 0) {
			LOG_ERR("Failed to delete Broadcast Sink: %d", err);

			return err;
		}
		broadcast_sink.bap_broadcast_sink = NULL;

		atomic_clear_bit(flags, FLAG_BROADCAST_SYNCED);
	}

	broadcast_sink.requested_bis_sync = new_bis_sync_req;
	if (broadcast_sink.requested_bis_sync != 0U) {
		atomic_set_bit(flags, FLAG_BROADCAST_SYNC_REQUESTED);
		check_sync_broadcast();
	} else {
		atomic_clear_bit(flags, FLAG_BROADCAST_SYNC_REQUESTED);
	}

	return 0;
}

static void bap_pa_sync_synced_cb(struct bt_le_per_adv_sync *sync,
				  struct bt_le_per_adv_sync_synced_info *info)
{
	if (sync == broadcast_sink.pa_sync ||
	    (broadcast_sink.req_recv_state != NULL &&
	     bt_addr_le_eq(info->addr, &broadcast_sink.req_recv_state->addr) &&
	     info->sid == broadcast_sink.req_recv_state->adv_sid)) {
		int err;

		LOG_INF("PA sync %p synced for broadcast sink", (void *)sync);

		if (broadcast_sink.pa_sync == NULL) {
			broadcast_sink.pa_sync = sync;
		}

		k_work_cancel_delayable(&pa_timer);
		atomic_set_bit(flags, FLAG_PA_SYNCED);
		atomic_clear_bit(flags, FLAG_PA_SYNCING);

		if (IS_ENABLED(CONFIG_SAMPLE_SCAN_SELF)) {
			int err;

			err = bt_le_scan_stop();
			if (err != 0) {
				LOG_ERR("Unable to stop scanning: %d", err);
			} else {
				atomic_clear_bit(flags, FLAG_SCANNING);
			}
		}

		err = create_broadcast_sink();
		if (err != 0) {
			LOG_ERR("Failed to create broadcast sink: %d", err);
		} else {
			check_sync_broadcast();
		}
	}
}

static void bap_pa_sync_terminated_cb(struct bt_le_per_adv_sync *sync,
				      const struct bt_le_per_adv_sync_term_info *info)
{
	if (sync == broadcast_sink.pa_sync) {
		int err;

		LOG_INF("PA sync %p lost with reason %u", (void *)sync, info->reason);

		/* Without PA we cannot sync to any new BIG - Clear data */
		broadcast_sink.requested_bis_sync = 0;
		broadcast_sink.pa_sync = NULL;
		k_work_cancel_delayable(&pa_timer);
		atomic_clear_bit(flags, FLAG_BROADCAST_SYNCABLE);
		atomic_clear_bit(flags, FLAG_PA_SYNCED);
		atomic_clear_bit(flags, FLAG_PA_SYNCING);
		atomic_clear_bit(flags, FLAG_BASE_RECEIVED);
		atomic_clear_bit(flags, FLAG_BROADCAST_CODE_REQUIRED);
		atomic_clear_bit(flags, FLAG_BROADCAST_CODE_RECEIVED);
		atomic_clear_bit(flags, FLAG_BROADCAST_SYNC_REQUESTED);

		err = bt_bap_scan_delegator_set_pa_state(broadcast_sink.req_recv_state->src_id,
							 BT_BAP_PA_STATE_NOT_SYNCED);
		if (err != 0) {
			LOG_ERR("Failed to set PA state to BT_BAP_PA_STATE_NOT_SYNCED: %d", err);
		}

		if (IS_ENABLED(CONFIG_SAMPLE_SCAN_SELF)) {
			(void)check_start_scan();
		}
	}
}
static bool scan_check_and_sync_broadcast(struct bt_data *data, void *user_data)
{
	const struct bt_le_scan_recv_info *info = user_data;
	struct bt_le_per_adv_sync_param param = {0};
	char le_addr[BT_ADDR_LE_STR_LEN];
	struct bt_uuid_16 adv_uuid;
	uint32_t broadcast_id;
	int err;

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

	bt_addr_le_copy(&param.addr, info->addr);
	param.options = BT_LE_PER_ADV_SYNC_OPT_FILTER_DUPLICATE;
	param.sid = info->sid;
	param.skip = PA_SYNC_SKIP;
	param.timeout = interval_to_sync_timeout(info->interval);

	err = bt_le_per_adv_sync_create(&param, &broadcast_sink.pa_sync);
	if (err != 0) {
		LOG_ERR("Failed to create PA sync: %d", err);
	} else {
		LOG_INF("Syncing without PAST from scan");

		broadcast_sink.broadcast_id = broadcast_id;
		atomic_set_bit(flags, FLAG_PA_SYNCING);
		k_work_reschedule(&pa_timer, K_MSEC(param.timeout * 10));

		/* Since we are scanning ourselves, we consider this as broadcast sync has been
		 * requested
		 */
		broadcast_sink.requested_bis_sync = BT_BAP_BIS_SYNC_NO_PREF;
		atomic_set_bit(flags, FLAG_BROADCAST_SYNC_REQUESTED);
	}

	/* Stop parsing */
	return false;
}

static bool is_substring(const char *substr, const char *str)
{
	const size_t str_len = strlen(str);
	const size_t sub_str_len = strlen(substr);

	if (sub_str_len > str_len) {
		return false;
	}

	for (size_t pos = 0; pos < str_len; pos++) {
		if (pos + sub_str_len > str_len) {
			return false;
		}

		if (strncasecmp(substr, &str[pos], sub_str_len) == 0) {
			return true;
		}
	}

	return false;
}

static bool data_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
	case BT_DATA_BROADCAST_NAME:
		memcpy(name, data->data, MIN(data->data_len, NAME_LEN - 1));
		return false;
	default:
		return true;
	}
}

static void broadcast_scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *ad)
{
	if (atomic_test_bit(flags, FLAG_PA_SYNCED) || atomic_test_bit(flags, FLAG_PA_SYNCING) ||
	    atomic_test_bit(flags, FLAG_BROADCAST_SYNCED) ||
	    atomic_test_bit(flags, FLAG_BROADCAST_SYNCING)) {
		/* If we are already synced or syncing, we do not care about scan reports */
		return;
	}

	/* Only consider advertisers with periodic advertising */
	if (info->interval != 0U) {
		/* call to bt_data_parse consumes netbufs so shallow clone for verbose output */

		/* If broadcast_sink.req_recv_state is NULL then we have been requested by a
		 * broadcast assistant to sync to a specific broadcast source. In that case we do
		 * not apply our own broadcast name filter.
		 */
		if (broadcast_sink.req_recv_state != NULL &&
		    strlen(CONFIG_SAMPLE_TARGET_BROADCAST_NAME) > 0U) {
			struct net_buf_simple buf_copy;
			char name[NAME_LEN] = {0};

			net_buf_simple_clone(ad, &buf_copy);
			bt_data_parse(&buf_copy, data_cb, name);
			if (!(is_substring(CONFIG_SAMPLE_TARGET_BROADCAST_NAME, name))) {
				return;
			}
		}
		bt_data_parse(ad, scan_check_and_sync_broadcast, (void *)info);
	}
}

int init_cap_acceptor_broadcast(void)
{
	static bool cbs_registered;
	int err;

	if (!cbs_registered) {
		static struct bt_bap_scan_delegator_cb scan_delegator_cbs = {
			.pa_sync_req = pa_sync_req_cb,
			.pa_sync_term_req = pa_sync_term_req_cb,
			.broadcast_code = broadcast_code_cb,
			.bis_sync_req = bis_sync_req_cb,
		};
		static struct bt_bap_broadcast_sink_cb broadcast_sink_cbs = {
			.base_recv = base_recv_cb,
			.syncable = syncable_cb,
		};
		static struct bt_bap_stream_ops broadcast_stream_ops = {
			.started = broadcast_stream_started_cb,
			.stopped = broadcast_stream_stopped_cb,
			.recv = broadcast_stream_recv_cb,
		};
		static struct bt_le_per_adv_sync_cb bap_pa_sync_cb = {
			.synced = bap_pa_sync_synced_cb,
			.term = bap_pa_sync_terminated_cb,
		};
		static struct bt_le_scan_cb bap_scan_cb = {
			.recv = broadcast_scan_recv,
		};

		err = bt_bap_scan_delegator_register(&scan_delegator_cbs);
		if (err != 0) {
			LOG_ERR("Scan delegator register failed (err %d)", err);

			return err;
		}

		err = bt_bap_broadcast_sink_register_cb(&broadcast_sink_cbs);
		if (err != 0) {
			LOG_ERR("Failed to register BAP broadcast sink callbacks: %d", err);

			return -ENOEXEC;
		}

		bt_cap_stream_ops_register(&broadcast_sink.broadcast_stream, &broadcast_stream_ops);
		bt_le_per_adv_sync_cb_register(&bap_pa_sync_cb);

		if (IS_ENABLED(CONFIG_SAMPLE_SCAN_SELF)) {
			bt_le_scan_cb_register(&bap_scan_cb);
		}

		cbs_registered = true;
	}

	if (IS_ENABLED(CONFIG_SAMPLE_SCAN_SELF)) {
		err = check_start_scan();
		if (err != 0) {
			LOG_ERR("Unable to start scan for CAP initiators: %d", err);

			return err;
		}
	}

	return 0;
}
