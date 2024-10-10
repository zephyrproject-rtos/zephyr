/* btp_bap_broadcast.h - Bluetooth BAP Tester */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/cap.h>

struct btp_bap_broadcast_stream {
	struct btp_bap_audio_stream audio_stream;
	struct bt_audio_codec_cfg codec_cfg;
	uint8_t bis_id;
	uint8_t subgroup_id;
	bool bis_synced;
	uint8_t source_id;
	bool in_use;
	bool already_sent;
};

/* According to BT spec, a Broadcast Source can configure and establish one or more BIGs,
 * each containing one or more BISes that are used to transport broadcast Audio Streams.
 * For each BIG, the Broadcast Source shall generate a Broadcast_ID.
 * For the time being, let's treat broadcast source as one BIG.
 */
struct btp_bap_broadcast_remote_source {
	bt_addr_le_t address;
	uint32_t broadcast_id;
	struct btp_bap_broadcast_stream streams[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];
	struct bt_bap_stream *sink_streams[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];
	struct bt_bap_broadcast_sink *sink;
	struct bt_bap_qos_cfg qos;
	/* BIS Index bitfield read from Base */
	uint32_t bis_index_bitfield;
	/* BIS Index bitfield read from sync request */
	uint32_t requested_bis_sync;
	bool assistant_request;
	uint8_t sink_broadcast_code[BT_AUDIO_BROADCAST_CODE_SIZE];
	const struct bt_bap_scan_delegator_recv_state *sink_recv_state;
};

struct btp_bap_broadcast_local_source {
	uint32_t broadcast_id;
	struct bt_bap_qos_cfg qos;
	struct btp_bap_broadcast_stream streams[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];
	struct bt_audio_codec_cfg subgroup_codec_cfg[CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT];
	uint8_t stream_count;
	/* Only for BTP BAP commands */
	struct bt_bap_broadcast_source *bap_broadcast;
	/* Only for BTP CAP commands */
	struct bt_cap_broadcast_source *cap_broadcast;
};

int btp_bap_broadcast_init(void);
struct btp_bap_broadcast_local_source *btp_bap_broadcast_local_source_get(uint8_t source_id);
struct btp_bap_broadcast_stream *btp_bap_broadcast_stream_alloc(
	struct btp_bap_broadcast_local_source *source);

uint8_t btp_bap_broadcast_source_setup(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len);
uint8_t btp_bap_broadcast_source_release(const void *cmd, uint16_t cmd_len,
					 void *rsp, uint16_t *rsp_len);
uint8_t btp_bap_broadcast_adv_start(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len);
uint8_t btp_bap_broadcast_adv_stop(const void *cmd, uint16_t cmd_len,
				   void *rsp, uint16_t *rsp_len);
uint8_t btp_bap_broadcast_source_start(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len);
uint8_t btp_bap_broadcast_source_stop(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len);
uint8_t btp_bap_broadcast_sink_setup(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len);
uint8_t btp_bap_broadcast_sink_release(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len);
uint8_t btp_bap_broadcast_scan_start(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len);
uint8_t btp_bap_broadcast_scan_stop(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len);
uint8_t btp_bap_broadcast_sink_sync(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len);
uint8_t btp_bap_broadcast_sink_stop(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len);
uint8_t btp_bap_broadcast_sink_bis_sync(const void *cmd, uint16_t cmd_len,
					void *rsp, uint16_t *rsp_len);
uint8_t btp_bap_broadcast_discover_scan_delegators(const void *cmd, uint16_t cmd_len,
						   void *rsp, uint16_t *rsp_len);
uint8_t btp_bap_broadcast_assistant_scan_start(const void *cmd, uint16_t cmd_len,
					       void *rsp, uint16_t *rsp_len);
uint8_t btp_bap_broadcast_assistant_scan_stop(const void *cmd, uint16_t cmd_len,
					      void *rsp, uint16_t *rsp_len);
uint8_t btp_bap_broadcast_assistant_add_src(const void *cmd, uint16_t cmd_len,
					    void *rsp, uint16_t *rsp_len);
uint8_t btp_bap_broadcast_assistant_remove_src(const void *cmd, uint16_t cmd_len,
					       void *rsp, uint16_t *rsp_len);
uint8_t btp_bap_broadcast_assistant_modify_src(const void *cmd, uint16_t cmd_len,
					       void *rsp, uint16_t *rsp_len);
uint8_t btp_bap_broadcast_assistant_set_broadcast_code(const void *cmd, uint16_t cmd_len,
						       void *rsp, uint16_t *rsp_len);
uint8_t btp_bap_broadcast_assistant_send_past(const void *cmd, uint16_t cmd_len,
					      void *rsp, uint16_t *rsp_len);
