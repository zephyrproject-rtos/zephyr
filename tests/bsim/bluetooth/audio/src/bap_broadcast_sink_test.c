/*
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_BAP_BROADCAST_SINK)

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/sys/byteorder.h>
#include "common.h"

extern enum bst_result_t bst_result;

CREATE_FLAG(broadcaster_found);
CREATE_FLAG(flag_base_received);
CREATE_FLAG(flag_base_metadata_updated);
CREATE_FLAG(pa_synced);
CREATE_FLAG(flag_syncable);
CREATE_FLAG(pa_sync_lost);
CREATE_FLAG(flag_received);
CREATE_FLAG(flag_pa_request);
CREATE_FLAG(flag_bis_sync_requested);

static struct bt_bap_broadcast_sink *g_sink;
static struct bt_le_scan_recv_info broadcaster_info;
static bt_addr_le_t broadcaster_addr;
static struct bt_le_per_adv_sync *pa_sync;
static uint32_t broadcaster_broadcast_id;
static struct audio_test_stream broadcast_sink_streams[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];
static struct bt_bap_stream *streams[ARRAY_SIZE(broadcast_sink_streams)];
static uint32_t requested_bis_sync;
static struct bt_le_ext_adv *ext_adv;
static const struct bt_bap_scan_delegator_recv_state *req_recv_state;

#define SUPPORTED_CHAN_COUNTS          BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1, 2)
#define SUPPORTED_MIN_OCTETS_PER_FRAME 30
#define SUPPORTED_MAX_OCTETS_PER_FRAME 155
#define SUPPORTED_MAX_FRAMES_PER_SDU   1

/* We support 1 or 2 channels, so the maximum SDU size we support will be 2 times the maximum frame
 * size per frame we support
 */
#define SUPPORTED_MAX_SDU_SIZE (2 * SUPPORTED_MAX_FRAMES_PER_SDU * SUPPORTED_MAX_OCTETS_PER_FRAME)

BUILD_ASSERT(CONFIG_BT_ISO_RX_MTU >= SUPPORTED_MAX_SDU_SIZE);

#define SUPPORTED_CONTEXTS (BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA)

static const struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP_LC3(
	BT_AUDIO_CODEC_CAP_FREQ_ANY, BT_AUDIO_CODEC_CAP_DURATION_ANY, SUPPORTED_CHAN_COUNTS,
	SUPPORTED_MIN_OCTETS_PER_FRAME, SUPPORTED_MAX_OCTETS_PER_FRAME,
	SUPPORTED_MAX_FRAMES_PER_SDU, SUPPORTED_CONTEXTS);

static K_SEM_DEFINE(sem_started, 0U, ARRAY_SIZE(streams));
static K_SEM_DEFINE(sem_stopped, 0U, ARRAY_SIZE(streams));

/* Create a mask for the maximum BIS we can sync to using the number of streams
 * we have. We add an additional 1 since the bis indexes start from 1 and not
 * 0.
 */
static const uint32_t bis_index_mask = BIT_MASK(ARRAY_SIZE(streams) + 1U);
static uint32_t bis_index_bitfield;

static uint8_t count_bits(enum bt_audio_location chan_allocation)
{
	uint8_t cnt = 0U;

	while (chan_allocation != 0) {
		cnt += chan_allocation & 1U;
		chan_allocation >>= 1;
	}

	return cnt;
}

static bool valid_base_subgroup(const struct bt_bap_base_subgroup *subgroup)
{
	struct bt_audio_codec_cfg codec_cfg = {0};
	enum bt_audio_location chan_allocation;
	uint8_t frames_blocks_per_sdu;
	size_t min_sdu_size_required;
	uint16_t octets_per_frame;
	uint8_t chan_cnt;
	int ret;

	ret = bt_bap_base_subgroup_codec_to_codec_cfg(subgroup, &codec_cfg);
	if (ret < 0) {
		printk("Could not get subgroup codec_cfg: %d\n", ret);

		return false;
	}

	ret = bt_audio_codec_cfg_get_freq(&codec_cfg);
	if (ret >= 0) {
		const int freq = bt_audio_codec_cfg_freq_to_freq_hz(ret);

		if (freq < 0) {
			printk("Invalid subgroup frequency value: %d (%d)\n", ret, freq);

			return false;
		}
	} else {
		printk("Could not get subgroup frequency: %d\n", ret);

		return false;
	}

	ret = bt_audio_codec_cfg_get_frame_dur(&codec_cfg);
	if (ret >= 0) {
		const int frame_duration_us = bt_audio_codec_cfg_frame_dur_to_frame_dur_us(ret);

		if (frame_duration_us < 0) {
			printk("Invalid subgroup frame duration value: %d (%d)\n", ret,
			       frame_duration_us);

			return false;
		}
	} else {
		printk("Could not get subgroup frame duration: %d\n", ret);

		return false;
	}

	ret = bt_audio_codec_cfg_get_chan_allocation(&codec_cfg, &chan_allocation);
	if (ret == 0) {
		chan_cnt = count_bits(chan_allocation);
	} else {
		printk("Could not get subgroup channel allocation: %d\n", ret);
		/* Channel allocation is an optional field, and omitting it implicitly means mono */
		chan_cnt = 1U;
	}

	if (chan_cnt == 0 || (BIT(chan_cnt - 1) & SUPPORTED_CHAN_COUNTS) == 0) {
		printk("Unsupported channel count: %u\n", chan_cnt);

		return false;
	}

	ret = bt_audio_codec_cfg_get_octets_per_frame(&codec_cfg);
	if (ret > 0) {
		octets_per_frame = (uint16_t)ret;
	} else {
		printk("Could not get subgroup octets per frame: %d\n", ret);

		return false;
	}

	if (!IN_RANGE(octets_per_frame, SUPPORTED_MIN_OCTETS_PER_FRAME,
		      SUPPORTED_MAX_OCTETS_PER_FRAME)) {
		printk("Unsupported octets per frame: %u\n", octets_per_frame);

		return false;
	}

	ret = bt_audio_codec_cfg_get_frame_blocks_per_sdu(&codec_cfg, false);
	if (ret > 0) {
		frames_blocks_per_sdu = (uint8_t)ret;
	} else {
		printk("Could not get subgroup octets per frame: %d\n", ret);
		/* Frame blocks per SDU is optional and is implicitly 1 */
		frames_blocks_per_sdu = 1U;
	}

	/* An SDU can consist of X frame blocks, each with Y frames (one per channel) of size Z in
	 * them. The minimum SDU size required for this is X * Y * Z.
	 */
	min_sdu_size_required = chan_cnt * octets_per_frame * frames_blocks_per_sdu;
	if (min_sdu_size_required > SUPPORTED_MAX_SDU_SIZE) {
		printk("With %zu channels and %u octets per frame and %u frames per block, SDUs "
		       "shall be at minimum %zu, we only support %d\n",
		       chan_cnt, octets_per_frame, frames_blocks_per_sdu, min_sdu_size_required,
		       SUPPORTED_MAX_SDU_SIZE);

		return false;
	}

	return true;
}

static bool base_subgroup_cb(const struct bt_bap_base_subgroup *subgroup, void *user_data)
{
	static uint8_t metadata[CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE];
	static size_t metadata_size;
	uint8_t *meta;
	int ret;

	ret = bt_bap_base_get_subgroup_codec_meta(subgroup, &meta);
	if (ret < 0) {
		FAIL("Could not get subgroup meta: %d\n", ret);
		return false;
	}

	if (TEST_FLAG(flag_base_received) &&
	    ((size_t)ret != metadata_size || memcmp(meta, metadata, metadata_size) != 0)) {
		printk("Metadata updated\n");
		SET_FLAG(flag_base_metadata_updated);
	}

	metadata_size = (size_t)ret;
	(void)memcpy(metadata, meta, metadata_size);

	if (!valid_base_subgroup(subgroup)) {
		printk("Invalid or unsupported subgroup\n");
		return false;
	}

	return true;
}

static void base_recv_cb(struct bt_bap_broadcast_sink *sink, const struct bt_bap_base *base,
			 size_t base_size)
{
	uint32_t base_bis_index_bitfield = 0U;
	int ret;

	printk("Received BASE with %d subgroups from broadcast sink %p\n",
	       bt_bap_base_get_subgroup_count(base), sink);

	ret = bt_bap_base_foreach_subgroup(base, base_subgroup_cb, NULL);
	if (ret != 0) {
		FAIL("Failed to parse subgroups: %d\n", ret);
		return;
	}

	ret = bt_bap_base_get_bis_indexes(base, &base_bis_index_bitfield);
	if (ret != 0) {
		FAIL("Failed to BIS indexes: %d\n", ret);
		return;
	}

	bis_index_bitfield = base_bis_index_bitfield & bis_index_mask;

	SET_FLAG(flag_base_received);
}

static void syncable_cb(struct bt_bap_broadcast_sink *sink, const struct bt_iso_biginfo *biginfo)
{
	printk("Broadcast sink %p syncable with%s encryption\n",
	       sink, biginfo->encryption ? "" : "out");
	SET_FLAG(flag_syncable);
}

static struct bt_bap_broadcast_sink_cb broadcast_sink_cbs = {
	.base_recv = base_recv_cb,
	.syncable = syncable_cb,
};

static bool scan_check_and_sync_broadcast(struct bt_data *data, void *user_data)
{
	const struct bt_le_scan_recv_info *info = user_data;
	char le_addr[BT_ADDR_LE_STR_LEN];
	struct bt_uuid_16 adv_uuid;
	uint32_t broadcast_id;

	if (TEST_FLAG(broadcaster_found)) {
		/* no-op*/
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

	SET_FLAG(broadcaster_found);

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
	if (sync == pa_sync) {
		printk("PA sync %p synced for broadcast sink with broadcast ID 0x%06X\n", sync,
		       broadcaster_broadcast_id);

		SET_FLAG(pa_synced);
	}
}

static void bap_pa_sync_terminated_cb(struct bt_le_per_adv_sync *sync,
				      const struct bt_le_per_adv_sync_term_info *info)
{
	if (sync == pa_sync) {
		printk("PA sync %p lost with reason %u\n", sync, info->reason);
		pa_sync = NULL;

		SET_FLAG(pa_sync_lost);
	}
}

static struct bt_le_per_adv_sync_cb bap_pa_sync_cb = {
	.synced = bap_pa_sync_synced_cb,
	.term = bap_pa_sync_terminated_cb,
};

static struct bt_pacs_cap cap = {
	.codec_cap = &codec_cap,
};

static int pa_sync_req_cb(struct bt_conn *conn,
			  const struct bt_bap_scan_delegator_recv_state *recv_state,
			  bool past_avail, uint16_t pa_interval)
{
	if (recv_state->pa_sync_state == BT_BAP_PA_STATE_SYNCED ||
	    recv_state->pa_sync_state == BT_BAP_PA_STATE_INFO_REQ) {
		/* Already syncing */
		/* TODO: Terminate existing sync and then sync to new?*/
		return -EALREADY;
	}

	req_recv_state = recv_state;

	SET_FLAG(flag_pa_request);

	return 0;
}

static int pa_sync_term_req_cb(struct bt_conn *conn,
			       const struct bt_bap_scan_delegator_recv_state *recv_state)
{
	if (pa_sync == NULL || recv_state->pa_sync_state == BT_BAP_PA_STATE_NOT_SYNCED) {
		return -EALREADY;
	}

	req_recv_state = recv_state;

	UNSET_FLAG(flag_pa_request);

	return 0;
}

static int bis_sync_req_cb(struct bt_conn *conn,
			   const struct bt_bap_scan_delegator_recv_state *recv_state,
			   const uint32_t bis_sync_req[CONFIG_BT_BAP_BASS_MAX_SUBGROUPS])
{
	printk("BIS sync request received for %p: 0x%08x\n", recv_state, bis_sync_req[0]);
	/* We only care about a single subgroup in this test */
	requested_bis_sync = bis_sync_req[0];
	broadcaster_broadcast_id = recv_state->broadcast_id;
	if (bis_sync_req[0] != 0) {
		SET_FLAG(flag_bis_sync_requested);
	} else {
		UNSET_FLAG(flag_bis_sync_requested);
	}

	return 0;
}

static struct bt_bap_scan_delegator_cb scan_delegator_cbs = {
	.pa_sync_req = pa_sync_req_cb,
	.pa_sync_term_req = pa_sync_term_req_cb,
	.bis_sync_req = bis_sync_req_cb,
};

static void validate_stream_codec_cfg(const struct bt_bap_stream *stream)
{
	struct bt_audio_codec_cfg *codec_cfg = stream->codec_cfg;
	enum bt_audio_location chan_allocation;
	uint8_t frames_blocks_per_sdu;
	size_t min_sdu_size_required;
	uint16_t octets_per_frame;
	uint8_t chan_cnt;
	int ret;

	ret = bt_audio_codec_cfg_get_freq(codec_cfg);
	if (ret >= 0) {
		const int freq = bt_audio_codec_cfg_freq_to_freq_hz(ret);

		if (freq < 0) {
			FAIL("Invalid frequency value: %d (%d)\n", ret, freq);

			return;
		}
	} else {
		FAIL("Could not get frequency: %d\n", ret);

		return;
	}

	ret = bt_audio_codec_cfg_get_frame_dur(codec_cfg);
	if (ret >= 0) {
		const int frame_duration_us = bt_audio_codec_cfg_frame_dur_to_frame_dur_us(ret);

		if (frame_duration_us < 0) {
			FAIL("Invalid frame duration value: %d (%d)\n", ret, frame_duration_us);

			return;
		}
	} else {
		FAIL("Could not get frame duration: %d\n", ret);

		return;
	}

	/* The broadcast source sets the channel allocation in the BIS to
	 * BT_AUDIO_LOCATION_FRONT_LEFT
	 */
	ret = bt_audio_codec_cfg_get_chan_allocation(codec_cfg, &chan_allocation);
	if (ret == 0) {
		if (chan_allocation != BT_AUDIO_LOCATION_FRONT_LEFT) {
			FAIL("Unexpected channel allocation: 0x%08X", chan_allocation);

			return;
		}

		chan_cnt = count_bits(chan_allocation);
	} else {
		FAIL("Could not get subgroup channel allocation: %d\n", ret);

		return;
	}

	if (chan_cnt == 0 || (BIT(chan_cnt - 1) & SUPPORTED_CHAN_COUNTS) == 0) {
		FAIL("Unsupported channel count: %u\n", chan_cnt);

		return;
	}

	ret = bt_audio_codec_cfg_get_octets_per_frame(codec_cfg);
	if (ret > 0) {
		octets_per_frame = (uint16_t)ret;
	} else {
		FAIL("Could not get subgroup octets per frame: %d\n", ret);

		return;
	}

	if (!IN_RANGE(octets_per_frame, SUPPORTED_MIN_OCTETS_PER_FRAME,
		      SUPPORTED_MAX_OCTETS_PER_FRAME)) {
		FAIL("Unsupported octets per frame: %u\n", octets_per_frame);

		return;
	}

	ret = bt_audio_codec_cfg_get_frame_blocks_per_sdu(codec_cfg, false);
	if (ret > 0) {
		frames_blocks_per_sdu = (uint8_t)ret;
	} else {
		printk("Could not get octets per frame: %d\n", ret);
		/* Frame blocks per SDU is optional and is implicitly 1 */
		frames_blocks_per_sdu = 1U;
	}

	/* An SDU can consist of X frame blocks, each with Y frames (one per channel) of size Z in
	 * them. The minimum SDU size required for this is X * Y * Z.
	 */
	min_sdu_size_required = chan_cnt * octets_per_frame * frames_blocks_per_sdu;
	if (min_sdu_size_required > stream->qos->sdu) {
		FAIL("With %zu channels and %u octets per frame and %u frames per block, SDUs "
		     "shall be at minimum %zu, but the stream has been configured for %u\n",
		     chan_cnt, octets_per_frame, frames_blocks_per_sdu, min_sdu_size_required,
		     stream->qos->sdu);

		return;
	}
}

static void started_cb(struct bt_bap_stream *stream)
{
	struct bt_bap_ep_info info;
	int err;

	err = bt_bap_ep_get_info(stream->ep, &info);
	if (err != 0) {
		FAIL("Failed to get EP info: %d\n", err);
		return;
	}

	if (info.state != BT_BAP_EP_STATE_STREAMING) {
		FAIL("Unexpected EP state: %d\n", info.state);
		return;
	}

	if (info.dir != BT_AUDIO_DIR_SINK) {
		FAIL("Unexpected info.dir: %d\n", info.dir);
		return;
	}

	if (info.can_send) {
		FAIL("info.can_send is true\n");
		return;
	}

	if (!info.can_recv) {
		FAIL("info.can_recv is false\n");
		return;
	}

	if (info.paired_ep != NULL) {
		FAIL("Unexpected info.paired_ep: %p\n", info.paired_ep);
		return;
	}

	printk("Stream %p started\n", stream);
	k_sem_give(&sem_started);

	validate_stream_codec_cfg(stream);
}

static void stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	printk("Stream %p stopped with reason 0x%02X\n", stream, reason);
	k_sem_give(&sem_stopped);
}

static void recv_cb(struct bt_bap_stream *stream,
		    const struct bt_iso_recv_info *info,
		    struct net_buf *buf)
{
	struct audio_test_stream *test_stream = audio_test_stream_from_bap_stream(stream);

	if ((test_stream->rx_cnt % 100U) == 0U) {
		printk("[%zu]: Incoming audio on stream %p len %u and ts %u\n", test_stream->rx_cnt,
		       stream, buf->len, info->ts);
	}

	if (test_stream->rx_cnt > 0U && info->ts == test_stream->last_info.ts) {
		FAIL("Duplicated timestamp received: %u\n", test_stream->last_info.ts);
		return;
	}

	if (test_stream->rx_cnt > 0U && info->seq_num == test_stream->last_info.seq_num) {
		FAIL("Duplicated PSN received: %u\n", test_stream->last_info.seq_num);
		return;
	}

	if (info->flags & BT_ISO_FLAGS_ERROR) {
		/* Fail the test if we have not received what we expected */
		if (!TEST_FLAG(flag_received)) {
			FAIL("ISO receive error\n");
		}

		return;
	}

	if (info->flags & BT_ISO_FLAGS_LOST) {
		FAIL("ISO receive lost\n");
		return;
	}

	if (memcmp(buf->data, mock_iso_data, buf->len) == 0) {
		test_stream->rx_cnt++;

		if (test_stream->rx_cnt >= MIN_SEND_COUNT) {
			/* We set the flag is just one stream has received the expected */
			SET_FLAG(flag_received);
		}
	} else {
		FAIL("Unexpected data received\n");
	}
}

static struct bt_bap_stream_ops stream_ops = {
	.started = started_cb,
	.stopped = stopped_cb,
	.recv = recv_cb
};

static int init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return err;
	}

	printk("Bluetooth initialized\n");

	err = bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &cap);
	if (err) {
		FAIL("Capability register failed (err %d)\n", err);
		return err;
	}

	/* Test invalid input */
	err = bt_bap_broadcast_sink_register_cb(NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_sink_register_cb did not fail with NULL cb\n");
		return err;
	}

	err = bt_bap_broadcast_sink_register_cb(&broadcast_sink_cbs);
	if (err != 0) {
		FAIL("Sink callback register failed (err %d)\n", err);
		return err;
	}

	bt_bap_scan_delegator_register_cb(&scan_delegator_cbs);
	bt_le_per_adv_sync_cb_register(&bap_pa_sync_cb);
	bt_le_scan_cb_register(&bap_scan_cb);

	UNSET_FLAG(broadcaster_found);
	UNSET_FLAG(flag_base_received);
	UNSET_FLAG(pa_synced);

	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		streams[i] = bap_stream_from_audio_test_stream(&broadcast_sink_streams[i]);
		bt_bap_stream_cb_register(streams[i], &stream_ops);
	}

	return 0;
}

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

static int pa_sync_create(void)
{
	struct bt_le_per_adv_sync_param create_params = {0};

	bt_addr_le_copy(&create_params.addr, &broadcaster_addr);
	create_params.options = BT_LE_PER_ADV_SYNC_OPT_FILTER_DUPLICATE;
	create_params.sid = broadcaster_info.sid;
	create_params.skip = PA_SYNC_SKIP;
	create_params.timeout = interval_to_sync_timeout(broadcaster_info.interval);

	return bt_le_per_adv_sync_create(&create_params, &pa_sync);
}
static void test_pa_sync_delete(void)
{
	int err;

	err = bt_le_per_adv_sync_delete(pa_sync);
	if (err != 0) {
		FAIL("Unable to stop sink: %d", err);
		return;
	}

	pa_sync = NULL;
}

static void test_scan_and_pa_sync(void)
{
	int err;

	printk("Scanning for broadcast sources\n");
	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
	if (err != 0) {
		FAIL("Unable to start scan for broadcast sources: %d", err);
		return;
	}

	WAIT_FOR_FLAG(broadcaster_found);

	printk("Broadcast source found, stopping scan\n");
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

	printk("Waiting for PA sync\n");
	WAIT_FOR_FLAG(pa_synced);
}

static void test_broadcast_sink_create(void)
{
	int err;

	printk("Creating the broadcast sink\n");
	err = bt_bap_broadcast_sink_create(pa_sync, broadcaster_broadcast_id, &g_sink);
	if (err != 0) {
		FAIL("Unable to create the sink: %d\n", err);
		return;
	}
}

static void test_broadcast_sink_create_inval(void)
{
	int err;

	err = bt_bap_broadcast_sink_create(NULL, broadcaster_broadcast_id, &g_sink);
	if (err == 0) {
		FAIL("bt_bap_broadcast_sink_create did not fail with NULL sink\n");
		return;
	}

	err = bt_bap_broadcast_sink_create(pa_sync, INVALID_BROADCAST_ID, &g_sink);
	if (err == 0) {
		FAIL("bt_bap_broadcast_sink_create did not fail with invalid broadcast ID\n");
		return;
	}

	err = bt_bap_broadcast_sink_create(pa_sync, broadcaster_broadcast_id, NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_sink_create did not fail with NULL sink\n");
		return;
	}
}

static void test_broadcast_sync(void)
{
	int err;

	printk("Syncing the sink\n");
	err = bt_bap_broadcast_sink_sync(g_sink, bis_index_bitfield, streams, NULL);
	if (err != 0) {
		FAIL("Unable to sync the sink: %d\n", err);
		return;
	}

	/* Wait for all to be started */
	printk("Waiting for streams to be started\n");
	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		k_sem_take(&sem_started, K_FOREVER);
	}
}

static void test_broadcast_sync_inval(void)
{
	struct bt_bap_stream *tmp_streams[ARRAY_SIZE(streams) + 1] = {0};
	uint32_t bis_index;
	int err;

	err = bt_bap_broadcast_sink_sync(NULL, bis_index_bitfield, streams, NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_sink_sync did not fail with NULL sink\n");
		return;
	}

	bis_index = 0;
	err = bt_bap_broadcast_sink_sync(g_sink, bis_index, streams, NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_sink_sync did not fail with invalid BIS indexes: 0x%08X\n",
		     bis_index);
		return;
	}

	bis_index = BIT(0);
	err = bt_bap_broadcast_sink_sync(g_sink, bis_index, streams, NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_sink_sync did not fail with invalid BIS indexes: 0x%08X\n",
		     bis_index);
		return;
	}

	err = bt_bap_broadcast_sink_sync(g_sink, bis_index, NULL, NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_sink_sync did not fail with NULL streams\n");
		return;
	}

	memcpy(tmp_streams, streams, sizeof(streams));
	bis_index = 0U;
	for (size_t i = 0U; i < ARRAY_SIZE(tmp_streams); i++) {
		bis_index |= BIT(i + BT_ISO_BIS_INDEX_MIN);
	}

	err = bt_bap_broadcast_sink_sync(g_sink, bis_index, tmp_streams, NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_sink_sync did not fail with NULL streams[%zu]\n",
		     ARRAY_SIZE(tmp_streams) - 1);
		return;
	}

	bis_index = 0U;
	for (size_t i = 0U; i < CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT + 1; i++) {
		bis_index |= BIT(i + BT_ISO_BIS_INDEX_MIN);
	}

	err = bt_bap_broadcast_sink_sync(g_sink, bis_index, tmp_streams, NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_sink_sync did not fail with invalid BIS indexes: 0x%08X\n",
		     bis_index);
		return;
	}
}

static void test_broadcast_stop(void)
{
	int err;

	err = bt_bap_broadcast_sink_stop(g_sink);
	if (err != 0) {
		FAIL("Unable to stop sink: %d", err);
		return;
	}

	printk("Waiting for streams to be stopped\n");
	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		k_sem_take(&sem_stopped, K_FOREVER);
	}
}

static void test_broadcast_stop_inval(void)
{
	int err;

	err = bt_bap_broadcast_sink_stop(NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_sink_stop did not fail with NULL sink\n");
		return;
	}
}

static void test_broadcast_delete(void)
{
	int err;

	err = bt_bap_broadcast_sink_delete(g_sink);
	if (err != 0) {
		FAIL("Unable to stop sink: %d", err);
		return;
	}

	/* No "sync lost" event is generated when we initialized the disconnect */
	g_sink = NULL;
}

static void test_broadcast_delete_inval(void)
{
	int err;

	err = bt_bap_broadcast_sink_delete(NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_sink_delete did not fail with NULL sink\n");
		return;
	}
}

static void test_start_adv(void)
{
	const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
		BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_BASS_VAL),
			      BT_UUID_16_ENCODE(BT_UUID_PACS_VAL)),
		BT_DATA_BYTES(BT_DATA_SVC_DATA16, BT_UUID_16_ENCODE(BT_UUID_BASS_VAL)),
	};
	int err;

	/* Create a non-connectable non-scannable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CONN_NAME, NULL, &ext_adv);
	if (err != 0) {
		FAIL("Failed to create advertising set (err %d)\n", err);

		return;
	}

	err = bt_le_ext_adv_set_data(ext_adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err != 0) {
		FAIL("Failed to set advertising data (err %d)\n", err);

		return;
	}

	err = bt_le_ext_adv_start(ext_adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err != 0) {
		FAIL("Failed to start advertising set (err %d)\n", err);

		return;
	}
}

static void test_common(void)
{
	int err;

	err = init();
	if (err) {
		FAIL("Init failed (err %d)\n", err);
		return;
	}

	test_scan_and_pa_sync();

	test_broadcast_sink_create_inval();
	test_broadcast_sink_create();

	printk("Broadcast source PA synced, waiting for BASE\n");
	WAIT_FOR_FLAG(flag_base_received);
	printk("BASE received\n");

	printk("Waiting for BIG syncable\n");
	WAIT_FOR_FLAG(flag_syncable);

	test_broadcast_sync_inval();
	test_broadcast_sync();

	printk("Waiting for data\n");
	WAIT_FOR_FLAG(flag_received);
	backchannel_sync_send_all(); /* let other devices know we have received what we wanted */

	/* Ensure that we also see the metadata update */
	printk("Waiting for metadata update\n");
	WAIT_FOR_FLAG(flag_base_metadata_updated)

	backchannel_sync_send_all(); /* let other devices know we have received what we wanted */
}

static void test_main(void)
{
	test_common();

	backchannel_sync_send_all(); /* let the broadcast source know it can stop */

	/* The order of PA sync lost and BIG Sync lost is irrelevant
	 * and depend on timeout parameters. We just wait for PA first, but
	 * either way will work.
	 */
	printk("Waiting for PA disconnected\n");
	WAIT_FOR_FLAG(pa_sync_lost);

	printk("Waiting for streams to be stopped\n");
	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		k_sem_take(&sem_stopped, K_FOREVER);
	}

	PASS("Broadcast sink passed\n");
}

static void test_sink_disconnect(void)
{
	test_common();

	test_broadcast_stop_inval();
	test_broadcast_stop();

	/* Retry sync*/
	test_broadcast_sync();
	test_broadcast_stop();

	test_broadcast_delete_inval();
	test_broadcast_delete();

	backchannel_sync_send_all(); /* let the broadcast source know it can stop */

	PASS("Broadcast sink disconnect passed\n");
}

static void broadcast_sink_with_assistant(void)
{
	int err;

	err = init();
	if (err) {
		FAIL("Init failed (err %d)\n", err);
		return;
	}

	test_start_adv();
	WAIT_FOR_FLAG(flag_connected);

	printk("Waiting for PA sync request\n");
	WAIT_FOR_FLAG(flag_pa_request);

	test_scan_and_pa_sync();
	test_broadcast_sink_create();

	printk("Broadcast source PA synced, waiting for BASE\n");
	WAIT_FOR_FLAG(flag_base_received);
	printk("BASE received\n");

	printk("Waiting for BIG syncable\n");
	WAIT_FOR_FLAG(flag_syncable);

	printk("Waiting for BIG sync request\n");
	WAIT_FOR_FLAG(flag_bis_sync_requested);
	test_broadcast_sync();

	printk("Waiting for data\n");
	WAIT_FOR_FLAG(flag_received);
	backchannel_sync_send_all(); /* let other devices know we have received what we wanted */

	/* Ensure that we also see the metadata update */
	printk("Waiting for metadata update\n");
	WAIT_FOR_FLAG(flag_base_metadata_updated)
	backchannel_sync_send_all(); /* let other devices know we have received what we wanted */

	printk("Waiting for BIG sync terminate request\n");
	WAIT_FOR_UNSET_FLAG(flag_bis_sync_requested);
	test_broadcast_stop();

	printk("Waiting for PA sync terminate request\n");
	WAIT_FOR_UNSET_FLAG(flag_pa_request);
	test_pa_sync_delete();
	test_broadcast_delete();

	backchannel_sync_send_all(); /* let the broadcast source know it can stop */

	PASS("Broadcast sink with assistant passed\n");
}

static const struct bst_test_instance test_broadcast_sink[] = {
	{
		.test_id = "broadcast_sink",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	{
		.test_id = "broadcast_sink_disconnect",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_sink_disconnect,
	},
	{
		.test_id = "broadcast_sink_with_assistant",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = broadcast_sink_with_assistant,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_broadcast_sink_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_broadcast_sink);
}

#else /* !CONFIG_BT_BAP_BROADCAST_SINK */

struct bst_test_list *test_broadcast_sink_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_BAP_BROADCAST_SINK */
