/* btp_bap_unicast.h - Bluetooth BAP Tester */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/cap.h>

#define BTP_BAP_UNICAST_MAX_SNK_STREAMS_COUNT MIN(CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT, \
						  CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT)
#define BTP_BAP_UNICAST_MAX_SRC_STREAMS_COUNT MIN(CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT, \
						  CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT)
#define BTP_BAP_UNICAST_MAX_STREAMS_COUNT BTP_BAP_UNICAST_MAX_SNK_STREAMS_COUNT + \
					  BTP_BAP_UNICAST_MAX_SRC_STREAMS_COUNT
#define BTP_BAP_UNICAST_MAX_END_POINTS_COUNT CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT + \
					     CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT

struct btp_bap_unicast_group {
	struct bt_bap_qos_cfg qos[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT];
	struct bt_bap_unicast_group *cig;
	bool in_use;
};

struct btp_bap_unicast_stream {
	struct btp_bap_audio_stream audio_stream;
	uint8_t ase_id;
	uint8_t conn_id;
	uint8_t cig_id;
	uint8_t cis_id;
	struct bt_audio_codec_cfg codec_cfg;
	bool already_sent;
	bool in_use;
};

struct btp_bap_unicast_connection {
	bt_addr_le_t address;
	struct btp_bap_unicast_stream streams[BTP_BAP_UNICAST_MAX_STREAMS_COUNT];
	size_t configured_sink_stream_count;
	size_t configured_source_stream_count;
	struct bt_bap_ep *end_points[BTP_BAP_UNICAST_MAX_END_POINTS_COUNT];
	size_t end_points_count;
};

int btp_bap_unicast_init(void);
struct btp_bap_unicast_connection *btp_bap_unicast_conn_get(size_t conn_index);
int btp_bap_unicast_group_create(uint8_t cig_id, struct btp_bap_unicast_group **out_unicast_group);
struct btp_bap_unicast_group *btp_bap_unicast_group_find(uint8_t cig_id);
struct bt_bap_ep *btp_bap_unicast_end_point_find(struct btp_bap_unicast_connection *conn,
						 uint8_t ase_id);
struct btp_bap_unicast_stream *btp_bap_unicast_stream_find(struct btp_bap_unicast_connection *conn,
							   uint8_t ase_id);
struct btp_bap_unicast_stream *btp_bap_unicast_stream_alloc(
	struct btp_bap_unicast_connection *conn);
void btp_bap_unicast_stream_free(struct btp_bap_unicast_stream *stream);

uint8_t btp_bap_discover(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len);

uint8_t btp_ascs_configure_codec(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len);
uint8_t btp_ascs_configure_qos(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len);
uint8_t btp_ascs_enable(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len);
uint8_t btp_ascs_receiver_start_ready(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len);
uint8_t btp_ascs_receiver_stop_ready(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len);
uint8_t btp_ascs_disable(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len);
uint8_t btp_ascs_release(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len);
uint8_t btp_ascs_update_metadata(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len);
uint8_t btp_ascs_add_ase_to_cis(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len);
uint8_t btp_ascs_preconfigure_qos(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len);
