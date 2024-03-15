/*  Bluetooth Audio Common Audio Profile internal header */

/*
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/sys/atomic.h>

bool bt_cap_acceptor_ccid_exist(const struct bt_conn *conn, uint8_t ccid);

void bt_cap_initiator_codec_configured(struct bt_cap_stream *cap_stream);
void bt_cap_initiator_qos_configured(struct bt_cap_stream *cap_stream);
void bt_cap_initiator_enabled(struct bt_cap_stream *cap_stream);
void bt_cap_initiator_started(struct bt_cap_stream *cap_stream);
void bt_cap_initiator_metadata_updated(struct bt_cap_stream *cap_stream);
void bt_cap_initiator_released(struct bt_cap_stream *cap_stream);
void bt_cap_stream_ops_register_bap(struct bt_cap_stream *cap_stream);

enum bt_cap_common_proc_state {
	BT_CAP_COMMON_PROC_STATE_ACTIVE,
	BT_CAP_COMMON_PROC_STATE_ABORTED,

	BT_CAP_COMMON_PROC_STATE_FLAG_NUM,
};

enum bt_cap_common_proc_type {
	BT_CAP_COMMON_PROC_TYPE_NONE,
	BT_CAP_COMMON_PROC_TYPE_START,
	BT_CAP_COMMON_PROC_TYPE_UPDATE,
	BT_CAP_COMMON_PROC_TYPE_STOP,
	BT_CAP_COMMON_PROC_TYPE_VOLUME_CHANGE,
	BT_CAP_COMMON_PROC_TYPE_VOLUME_OFFSET_CHANGE,
	BT_CAP_COMMON_PROC_TYPE_VOLUME_MUTE_CHANGE,
	BT_CAP_COMMON_PROC_TYPE_MICROPHONE_GAIN_CHANGE,
	BT_CAP_COMMON_PROC_TYPE_MICROPHONE_MUTE_CHANGE,
};

enum bt_cap_common_subproc_type {
	BT_CAP_COMMON_SUBPROC_TYPE_NONE,
	BT_CAP_COMMON_SUBPROC_TYPE_CODEC_CONFIG,
	BT_CAP_COMMON_SUBPROC_TYPE_QOS_CONFIG,
	BT_CAP_COMMON_SUBPROC_TYPE_ENABLE,
	BT_CAP_COMMON_SUBPROC_TYPE_START,
	BT_CAP_COMMON_SUBPROC_TYPE_META_UPDATE,
	BT_CAP_COMMON_SUBPROC_TYPE_RELEASE,
};

struct bt_cap_initiator_proc_param {
	struct bt_cap_stream *stream;
	union {
		struct {
			struct bt_conn *conn;
			struct bt_bap_ep *ep;
			struct bt_audio_codec_cfg codec_cfg;
		} start;
		struct {
			/** Codec Specific Capabilities Metadata count */
			size_t meta_len;
			/** Codec Specific Capabilities Metadata */
			uint8_t meta[CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE];
		} meta_update;
	};
};

struct bt_cap_commander_proc_param {
	struct bt_conn *conn;
	union {
#if defined(CONFIG_BT_VCP_VOL_CTLR)
		struct {
			uint8_t volume;
		} change_volume;
		struct {
			bool mute;
		} change_mute;
#endif /* CONFIG_BT_VCP_VOL_CTLR */
#if defined(CONFIG_BT_VCP_VOL_CTLR_VOCS)
		struct {
			int16_t offset;
			struct bt_vocs *vocs;
		} change_offset;
#endif /* CONFIG_BT_VCP_VOL_CTLR_VOCS */
#if defined(CONFIG_BT_MICP_MIC_CTLR)
#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
		struct {
			int8_t gain;
			struct bt_aics *aics;
		} change_gain;
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */
#endif /* CONFIG_BT_MICP_MIC_CTLR */
	};
};

struct bt_cap_common_proc_param {
	union {
#if defined(CONFIG_BT_CAP_INITIATOR_UNICAST)
		struct bt_cap_initiator_proc_param
			initiator[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT];
#endif /* CONFIG_BT_CAP_INITIATOR_UNICAST */
#if defined(CONFIG_BT_CAP_COMMANDER)
		struct bt_cap_commander_proc_param commander[CONFIG_BT_MAX_CONN];
#endif /* CONFIG_BT_CAP_COMMANDER */
	};
};

struct bt_cap_common_proc {
	ATOMIC_DEFINE(proc_state_flags, BT_CAP_COMMON_PROC_STATE_FLAG_NUM);
	/* Total number of items (streams or connections) in the procedure*/
	size_t proc_cnt;
	/* Number of items where a subprocedure have been started */
	size_t proc_initiated_cnt;
	/* Number of items done with the procedure */
	size_t proc_done_cnt;
	enum bt_cap_common_proc_type proc_type;
	int err;
	struct bt_conn *failed_conn;
	struct bt_cap_common_proc_param proc_param;
#if defined(CONFIG_BT_CAP_INITIATOR_UNICAST)
	enum bt_cap_common_subproc_type subproc_type;
#endif /* CONFIG_BT_CAP_INITIATOR_UNICAST */
};

struct bt_cap_common_client {
	struct bt_conn *conn;
	struct bt_gatt_discover_params param;
	uint16_t csis_start_handle;
	const struct bt_csip_set_coordinator_csis_inst *csis_inst;
	bool cas_found;
};

struct bt_cap_common_proc *bt_cap_common_get_active_proc(void);
void bt_cap_common_clear_active_proc(void);
void bt_cap_common_start_proc(enum bt_cap_common_proc_type proc_type, size_t proc_cnt);
void bt_cap_common_set_subproc(enum bt_cap_common_subproc_type subproc_type);
bool bt_cap_common_subproc_is_type(enum bt_cap_common_subproc_type subproc_type);
struct bt_conn *bt_cap_common_get_member_conn(enum bt_cap_set_type type,
					      const union bt_cap_set_member *member);
bool bt_cap_common_proc_is_active(void);
bool bt_cap_common_proc_is_aborted(void);
bool bt_cap_common_proc_all_handled(void);
bool bt_cap_common_proc_is_done(void);
void bt_cap_common_abort_proc(struct bt_conn *conn, int err);
bool bt_cap_common_conn_in_active_proc(const struct bt_conn *conn);
bool bt_cap_common_stream_in_active_proc(const struct bt_cap_stream *cap_stream);
void bt_cap_common_disconnected(struct bt_conn *conn, uint8_t reason);
struct bt_cap_common_client *bt_cap_common_get_client_by_acl(const struct bt_conn *acl);
struct bt_cap_common_client *
bt_cap_common_get_client_by_csis(const struct bt_csip_set_coordinator_csis_inst *csis_inst);
struct bt_cap_common_client *bt_cap_common_get_client(enum bt_cap_set_type type,
						      const union bt_cap_set_member *member);

typedef void (*bt_cap_common_discover_func_t)(
	struct bt_conn *conn, int err, const struct bt_csip_set_coordinator_csis_inst *csis_inst);
int bt_cap_common_discover(struct bt_conn *conn, bt_cap_common_discover_func_t func);
