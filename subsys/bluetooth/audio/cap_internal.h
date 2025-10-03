/*  Bluetooth Audio Common Audio Profile internal header */

/*
 * Copyright (c) 2022-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/types.h>

bool bt_cap_acceptor_ccid_exist(const struct bt_conn *conn, uint8_t ccid);
bool bt_cap_acceptor_ccids_exist(const struct bt_conn *conn, const uint8_t ccids[],
				 uint8_t ccid_cnt);

void bt_cap_initiator_codec_configured(struct bt_cap_stream *cap_stream);
void bt_cap_initiator_qos_configured(struct bt_cap_stream *cap_stream);
void bt_cap_initiator_enabled(struct bt_cap_stream *cap_stream);
void bt_cap_initiator_started(struct bt_cap_stream *cap_stream);
void bt_cap_initiator_connected(struct bt_cap_stream *cap_stream);
void bt_cap_initiator_metadata_updated(struct bt_cap_stream *cap_stream);
void bt_cap_initiator_disabled(struct bt_cap_stream *cap_stream);
void bt_cap_initiator_stopped(struct bt_cap_stream *cap_stream);
void bt_cap_initiator_released(struct bt_cap_stream *cap_stream);
void bt_cap_stream_ops_register_bap(struct bt_cap_stream *cap_stream);
void bt_cap_initiator_cp_cb(struct bt_cap_stream *cap_stream, enum bt_bap_ascs_rsp_code rsp_code,
			    enum bt_bap_ascs_reason reason);

enum bt_cap_common_proc_state {
	BT_CAP_COMMON_PROC_STATE_ACTIVE,
	BT_CAP_COMMON_PROC_STATE_ABORTED,
	/* While the handover is technically a procedure, and arguably should use
	 * `bt_cap_common_set_proc`, we set the it as a flag instead so that we can rely on the API
	 * functions to perform their procedures
	 */
	BT_CAP_COMMON_PROC_STATE_HANDOVER,

	BT_CAP_COMMON_PROC_STATE_FLAG_NUM,
};

enum bt_cap_common_proc_type {
	BT_CAP_COMMON_PROC_TYPE_NONE,
	BT_CAP_COMMON_PROC_TYPE_START,
	BT_CAP_COMMON_PROC_TYPE_UPDATE,
	BT_CAP_COMMON_PROC_TYPE_STOP,
	BT_CAP_COMMON_PROC_TYPE_BROADCAST_RECEPTION_START,
	BT_CAP_COMMON_PROC_TYPE_BROADCAST_RECEPTION_STOP,
	BT_CAP_COMMON_PROC_TYPE_DISTRIBUTE_BROADCAST_CODE,
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
	BT_CAP_COMMON_SUBPROC_TYPE_CONNECT,
	BT_CAP_COMMON_SUBPROC_TYPE_START,
	BT_CAP_COMMON_SUBPROC_TYPE_META_UPDATE,
	BT_CAP_COMMON_SUBPROC_TYPE_DISABLE,
	BT_CAP_COMMON_SUBPROC_TYPE_STOP,
	BT_CAP_COMMON_SUBPROC_TYPE_RELEASE,
};

struct bt_cap_unicast_group {
	struct bt_bap_unicast_group *bap_unicast_group;
};

struct bt_cap_broadcast_source {
	struct bt_bap_broadcast_source *bap_broadcast;
};

struct bt_cap_initiator_proc_param {
	struct bt_cap_stream *stream;
	union {
		struct {
			struct bt_conn *conn;
			struct bt_bap_ep *ep;
			struct bt_audio_codec_cfg *codec_cfg;
			bool connected;
		} start;
		struct {
			/** Codec Specific Capabilities Metadata count */
			size_t meta_len;
			/** Codec Specific Capabilities Metadata */
			uint8_t meta[CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE];
		} meta_update;
		struct {
			bool release;
		} stop;
	};
	bool in_progress;
};

#if defined(CONFIG_BT_BAP_BROADCAST_ASSISTANT)
struct cap_broadcast_reception_start {

	bt_addr_le_t addr;
	uint8_t adv_sid;
	uint32_t broadcast_id;
	uint16_t pa_interval;
	uint8_t num_subgroups;
	struct bt_bap_bass_subgroup subgroups[CONFIG_BT_BAP_BASS_MAX_SUBGROUPS];
};

struct cap_broadcast_reception_stop {
	uint8_t src_id;
	uint8_t num_subgroups;
	struct bt_bap_bass_subgroup subgroups[CONFIG_BT_BAP_BASS_MAX_SUBGROUPS];
};

/* Note that although the broadcast_code will be the same for all
 * we nevertheless store a separate copy for each sink, for
 * consistency in the struct bt_cap_commander_proc_param
 * There is no memory savings by not having broadcast_code part of the
 * union: struct cap_broadcast_reception_start uses minimum 20 bytes
 * and struct cap_distribute_broadcast_code uses 17 bytes
 */
struct cap_distribute_broadcast_code {
	uint8_t src_id;
	uint8_t broadcast_code[BT_ISO_BROADCAST_CODE_SIZE];
};
#endif /* CONFIG_BT_BAP_BROADCAST_ASSISTANT */

struct bt_cap_commander_proc_param {
	struct bt_conn *conn;
	union {
#if defined(CONFIG_BT_VCP_VOL_CTLR)
		struct {
			uint8_t volume;
		} change_volume;
		struct {
			bool mute;
		} change_vol_mute;
#endif /* CONFIG_BT_VCP_VOL_CTLR */
#if defined(CONFIG_BT_VCP_VOL_CTLR_VOCS)
		struct {
			int16_t offset;
			struct bt_vocs *vocs;
		} change_offset;
#endif /* CONFIG_BT_VCP_VOL_CTLR_VOCS */
#if defined(CONFIG_BT_BAP_BROADCAST_ASSISTANT)
		struct cap_broadcast_reception_start broadcast_reception_start;
		struct cap_broadcast_reception_stop broadcast_reception_stop;
		struct cap_distribute_broadcast_code distribute_broadcast_code;
#endif /* CONFIG_BT_BAP_BROADCAST_ASSISTANT */
#if defined(CONFIG_BT_MICP_MIC_CTLR)
		struct {
			bool mute;
		} change_mic_mute;
#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
		struct {
			int8_t gain;
			struct bt_aics *aics;
		} change_gain;
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */
#endif /* CONFIG_BT_MICP_MIC_CTLR */
	};
};

#if defined(CONFIG_BT_CAP_HANDOVER)
struct bt_cap_handover_proc_param {
	union {
		struct {
			/* Struct containing the converted unicast group configuration */
			struct bt_cap_initiator_broadcast_create_param *broadcast_create_param;
			/* The resulting broadcast source */
			struct bt_cap_broadcast_source *broadcast_source;
			/* The advertising set to start the source on */
			struct bt_le_ext_adv *ext_adv;
			/* The source unicast group with the streams. */
			struct bt_cap_unicast_group *unicast_group;
			/* Set type */
			enum bt_cap_set_type type;

			/* The PA interval of the ext_adv */
			uint16_t pa_interval;
			/* The broadcast ID the broadcast source will use */
			uint32_t broadcast_id;

			struct bt_cap_commander_broadcast_reception_start_member_param
				reception_start_member_params[CONFIG_BT_MAX_CONN];
		} unicast_to_broadcast;
		struct {
			/* The existing broadcast source */
			struct bt_cap_broadcast_source *broadcast_source;

			/* The resulting unicast group */
			struct bt_cap_unicast_group *unicast_group;

			/* Broadcast ID of broadcast_source*/
			uint32_t broadcast_id;

			/* Advertising SID of broadcast_source*/
			uint8_t adv_sid;

			/* Advertising type of broadcast_source*/
			uint8_t adv_type;

			/* States used to determine when the broadcast source can be deleted */
			bool broadcast_stopped;
			bool reception_stopped;

			/* Array of connection objects that we are waiting for a receive state with
			 * a BIG sync lost event
			 */
			struct bt_conn *pending_recv_state_conns[MIN(
				CONFIG_BT_MAX_CONN,
				CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT)];

			/* Unicast group create param from caller */
			struct bt_cap_unicast_group_param *unicast_group_param;

			/* Unicast start param from caller */
			struct bt_cap_unicast_audio_start_param *unicast_start_param;
		} broadcast_to_unicast;
	};

	/* Flag to determine which of the two structs above to use */
	bool is_unicast_to_broadcast;
};
#endif /* CONFIG_BT_CAP_HANDOVER */

typedef void (*bt_cap_common_discover_func_t)(
	struct bt_conn *conn, int err, const struct bt_csip_set_coordinator_set_member *member,
	const struct bt_csip_set_coordinator_csis_inst *csis_inst);

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
#if defined(CONFIG_BT_CAP_HANDOVER)
	/* The handover parameters cannot be part of the union as they need to exist while we are
	 * performing CAP initiator and CAP Commander procedures
	 */
	struct bt_cap_handover_proc_param handover;
#endif /* CONFIG_BT_CAP_HANDOVER */
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
	bt_cap_common_discover_func_t discover_cb_func;
	uint16_t csis_start_handle;
	const struct bt_csip_set_coordinator_csis_inst *csis_inst;
};

struct bt_cap_common_proc *bt_cap_common_get_active_proc(void);
void bt_cap_common_clear_active_proc(void);
void bt_cap_common_set_proc(enum bt_cap_common_proc_type proc_type, size_t proc_cnt);
void bt_cap_common_set_subproc(enum bt_cap_common_subproc_type subproc_type);
void bt_cap_common_set_handover_active(void);
bool bt_cap_common_handover_is_active(void);
bool bt_cap_common_proc_is_type(enum bt_cap_common_proc_type proc_type);
bool bt_cap_common_subproc_is_type(enum bt_cap_common_subproc_type subproc_type);
struct bt_conn *bt_cap_common_get_member_conn(enum bt_cap_set_type type,
					      const union bt_cap_set_member *member);
bool bt_cap_common_test_and_set_proc_active(void);
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
int bt_cap_common_discover(struct bt_conn *conn, bt_cap_common_discover_func_t func);

bool bt_cap_initiator_broadcast_audio_start_valid_param(
	const struct bt_cap_initiator_broadcast_create_param *param);
bool bt_cap_initiator_valid_unicast_audio_start_param(
	const struct bt_cap_unicast_audio_start_param *param);
bool bt_cap_initiator_valid_unicast_group_param(const struct bt_cap_unicast_group_param *param);
bool bt_cap_initiator_stream_is_in_state(const struct bt_bap_stream *bap_stream,
					 enum bt_bap_ep_state state);
bool bt_cap_initiator_valid_unicast_audio_stop_param(
	const struct bt_cap_unicast_audio_stop_param *param);
int cap_initiator_unicast_audio_start(const struct bt_cap_unicast_audio_start_param *param);
int cap_initiator_unicast_audio_stop(const struct bt_cap_unicast_audio_stop_param *param);
enum bt_bap_ep_state bt_cap_initiator_stream_get_state(const struct bt_bap_stream *bap_stream);

int cap_commander_broadcast_reception_start(
	const struct bt_cap_commander_broadcast_reception_start_param *param);
int cap_commander_broadcast_reception_start(
	const struct bt_cap_commander_broadcast_reception_start_param *param);
int cap_commander_broadcast_reception_stop(
	const struct bt_cap_commander_broadcast_reception_stop_param *param);
bool bt_cap_commander_valid_broadcast_reception_stop_param(
	const struct bt_cap_commander_broadcast_reception_stop_param *param);
void cap_commander_register_broadcast_assistant_callbacks(void);

void bt_cap_handover_complete(void);
void bt_cap_handover_unicast_to_broadcast_setup_broadcast(void);
void bt_cap_handover_unicast_to_broadcast_reception_start(void);
void bt_cap_handover_unicast_proc_complete(void);
void bt_cap_handover_broadcast_source_stopped(uint8_t reason);
void bt_cap_handover_broadcast_audio_stopped(void);
void bt_cap_handover_receive_state_updated(const struct bt_conn *conn,
					   const struct bt_bap_scan_delegator_recv_state *state);
bool bt_cap_handover_is_handover_broadcast_source(
	const struct bt_cap_broadcast_source *cap_broadcast_source);
int bt_cap_handover_broadcast_reception_stopped(void);
