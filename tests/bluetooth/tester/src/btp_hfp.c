/* btp_hfp.c - Bluetooth HFP Tester */

/*
 * Copyright (c) 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/hfp_hf.h>
#include <zephyr/bluetooth/classic/hfp_ag.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "btp/btp.h"

#define LOG_MODULE_NAME bttester_hfp
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

static volatile uint8_t hf_check_signal_strength;
static uint8_t hfp_in_calling_status = 0xff;
uint8_t call_active = 0;
static bool audio_conn_created;
static volatile bool battery_charged_state;
#define MAX_COPS_NAME_SIZE (16)
static char cops_name[MAX_COPS_NAME_SIZE];
static char voice_tag[MAX_COPS_NAME_SIZE] = "\"1234567\",129"; // "+918067064000";
static uint8_t s_hfp_in_calling_status = 0xff;
static uint8_t wait_call = 0;
static uint8_t call_held = 0;
static bool mem_call_list = 0;
static bool ec_nr_disabled = 1;
static volatile uint8_t hf_check_mic_volume;
static volatile uint8_t hf_check_speaker_volume;
static volatile uint8_t hf_check_mic_volume;
static volatile uint8_t hf_check_speaker_volume;
static uint8_t codecs_negotiate_done = 0;
struct bt_conn *default_conn;
static volatile bool hf_accept_call;
static bool ring_alert;
static volatile bool roam_active_state;
static uint8_t signal_value;
static bool hf_auto_select_codec;
/* Store HFP connection for later use */
struct bt_hfp_hf *hfp_hf;
struct bt_conn *hf_sco_conn;
static struct bt_hfp_hf_call *hfp_hf_call[CONFIG_BT_HFP_HF_MAX_CALLS];

struct bt_hfp_ag *hfp_ag;
struct bt_hfp_ag *hfp_ag_ongoing;
struct bt_conn *hfp_ag_sco_conn;
static struct bt_hfp_ag_call *hfp_ag_call[CONFIG_BT_HFP_AG_MAX_CALLS];

static uint32_t conn_count = 0;
// static struct bt_hfp_ag_ongoing_call ag_ongoing_call_info[CONFIG_BT_HFP_AG_MAX_CALLS];

static size_t ag_ongoing_calls;

static bool has_ongoing_calls;


static void ag_add_a_call(struct bt_hfp_ag_call *call)
{
	ARRAY_FOR_EACH(hfp_ag_call, i) {
		if (!hfp_ag_call[i]) {
			hfp_ag_call[i] = call;
			return;
		}
	}
}

static void ag_remove_a_call(struct bt_hfp_ag_call *call)
{
	ARRAY_FOR_EACH(hfp_ag_call, i) {
		if (call == hfp_ag_call[i]) {
			hfp_ag_call[i] = NULL;
			return;
		}
	}
}

static void ag_remove_calls(void)
{
	ARRAY_FOR_EACH(hfp_ag_call, i) {
		if (hfp_ag_call[i] != NULL) {
			hfp_ag_call[i] = NULL;
		}
	}
}

static void ag_connected(struct bt_conn *conn, struct bt_hfp_ag *ag)
{
	hfp_ag = ag;
	LOG_DBG("AG connected");
}

static void ag_disconnected(struct bt_hfp_ag *ag)
{
	ag_remove_calls();
	LOG_DBG("AG disconnected");
}

static void ag_sco_connected(struct bt_hfp_ag *ag, struct bt_conn *sco_conn)
{
	struct btp_hfp_sco_connected_ev ev;

	if (hfp_ag_sco_conn != NULL) {
		return;
	}

	audio_conn_created = true;
	hfp_ag_sco_conn = bt_conn_ref(sco_conn);
	tester_event(BTP_SERVICE_ID_HFP, BTP_HFP_EV_SCO_CONNECTED, &ev, sizeof(ev));
}

static void ag_sco_disconnected(struct bt_conn *sco_conn, uint8_t reason)
{
	struct btp_hfp_sco_disconnected_ev ev;

	if (hfp_ag_sco_conn == sco_conn) {
		bt_conn_unref(hfp_ag_sco_conn);
		hfp_ag_sco_conn = NULL;
		audio_conn_created = false;
		tester_event(BTP_SERVICE_ID_HFP, BTP_HFP_EV_SCO_DISCONNECTED, &ev, sizeof(ev));
	}
}

static int ag_get_ongoing_call(struct bt_hfp_ag *ag)
{
	if (!has_ongoing_calls) {
		return -EINVAL;
	}

	hfp_ag_ongoing = ag;
	LOG_DBG("Please set ongoing calls");
	return 0;
}

static int ag_memory_dial(struct bt_hfp_ag *ag, const char *location, char **number)
{
	static char *phone = "1234567";

	*number = phone;

	return 0;
}

static int ag_number_call(struct bt_hfp_ag *ag, const char *number)
{
	static char *phone = "1234567";

	LOG_DBG("AG number call");

	if (strcmp(number, phone)) {
		return -ENOTSUP;
	}

	return 0;
}

static void ag_outgoing(struct bt_hfp_ag *ag, struct bt_hfp_ag_call *call, const char *number)
{
	LOG_DBG("AG outgoing call %p, number %s", call, number);
	ag_add_a_call(call);
}

static void ag_incoming(struct bt_hfp_ag *ag, struct bt_hfp_ag_call *call, const char *number)
{
	LOG_DBG("AG incoming call %p, number %s", call, number);
	ag_add_a_call(call);
}

static void ag_incoming_held(struct bt_hfp_ag_call *call)
{
	LOG_DBG("AG incoming call %p is held", call);
}

static void ag_ringing(struct bt_hfp_ag_call *call, bool in_band)
{
	LOG_DBG("AG call %p start ringing mode %d", call, in_band);
}

static void ag_accept(struct bt_hfp_ag_call *call)
{
	LOG_DBG("AG call %p accept", call);
}

static void ag_held(struct bt_hfp_ag_call *call)
{
	LOG_DBG("AG call %p held", call);
}

static void ag_retrieve(struct bt_hfp_ag_call *call)
{
	LOG_DBG("AG call %p retrieved", call);
}

static void ag_reject(struct bt_hfp_ag_call *call)
{
	LOG_DBG("AG call %p reject", call);
	ag_remove_a_call(call);
}

static void ag_terminate(struct bt_hfp_ag_call *call)
{
	LOG_DBG("AG call %p terminate", call);
	ag_remove_a_call(call);
}

static void ag_codec(struct bt_hfp_ag *ag, uint32_t ids)
{
	LOG_DBG("AG received codec id bit map %x", ids);
}

void ag_vgm(struct bt_hfp_ag *ag, uint8_t gain)
{
	LOG_DBG("AG received vgm %d", gain);
}

void ag_vgs(struct bt_hfp_ag *ag, uint8_t gain)
{
	LOG_DBG("AG received vgs %d", gain);
}

void ag_codec_negotiate(struct bt_hfp_ag *ag, int err)
{
	LOG_DBG("AG codec negotiation result %d", err);
}

void ag_audio_connect_req(struct bt_hfp_ag *ag)
{
	LOG_DBG("Receive audio connect request. "
		"Input `hfp ag audio_connect` to start audio connect");
}

void ag_ecnr_turn_off(struct bt_hfp_ag *ag)
{
	LOG_DBG("encr is disabled");
}

#if defined(CONFIG_BT_HFP_AG_3WAY_CALL)
void ag_explicit_call_transfer(struct bt_hfp_ag *ag)
{
	LOG_DBG("explicit call transfer");
}
#endif /* CONFIG_BT_HFP_AG_3WAY_CALL */

#if defined(CONFIG_BT_HFP_AG_VOICE_RECG)
void ag_voice_recognition(struct bt_hfp_ag *ag, bool activate)
{
	uint8_t state = 0;
	state |= BIT(0);
	bt_hfp_ag_vre_state(hfp_ag, state);
}

#if defined(CONFIG_BT_HFP_AG_ENH_VOICE_RECG)
void ag_ready_to_accept_audio(struct bt_hfp_ag *ag)
{
	LOG_DBG("hf is ready to accept audio");
}
#endif /* CONFIG_BT_HFP_AG_ENH_VOICE_RECG */
#endif /* CONFIG_BT_HFP_AG_VOICE_RECG */

#if defined(CONFIG_BT_HFP_AG_VOICE_TAG)
int ag_request_phone_number(struct bt_hfp_ag *ag, char **number)
{
	static bool valid_number;

	if (valid_number && number) {
		valid_number = false;
		*number = "123456789";
		return 0;
	}

	valid_number = true;
	return -EINVAL;
}
#endif /* CONFIG_BT_HFP_AG_VOICE_TAG */

void ag_transmit_dtmf_code(struct bt_hfp_ag *ag, char code)
{
	LOG_DBG("DTMF code is %c", code);
}

struct {
	char *number;
	uint8_t type;
	uint8_t service;
} ag_subscriber_number_info[] = {
	{
		.number = "12345678",
		.type = 128,
		.service = 4,
	},
	{
		.number = "87654321",
		.type = 128,
		.service = 4,
	},
};

static bool subscriber;

int ag_subscriber_number(struct bt_hfp_ag *ag, bt_hfp_ag_query_subscriber_func_t func)
{
	int err;

	if (subscriber && func) {
		for (size_t index = 0; index < ARRAY_SIZE(ag_subscriber_number_info); index++) {
			err = func(ag, ag_subscriber_number_info[index].number,
				   ag_subscriber_number_info[index].type,
				   ag_subscriber_number_info[index].service);
			if (err < 0) {
				break;
			}
		}
	}
	return 0;
}

void ag_hf_indicator_value(struct bt_hfp_ag *ag, enum hfp_ag_hf_indicators indicator,
			   uint32_t value)
{
	LOG_DBG("indicator %d value %d", indicator, value);
}

static struct bt_hfp_ag_cb ag_cb = {
	.connected = ag_connected,
	.disconnected = ag_disconnected,
	.sco_connected = ag_sco_connected,
	.sco_disconnected = ag_sco_disconnected,
	.get_ongoing_call = ag_get_ongoing_call,
	.memory_dial = ag_memory_dial,
	.number_call = ag_number_call,
	.outgoing = ag_outgoing,
	.incoming = ag_incoming,
	.incoming_held = ag_incoming_held,
	.ringing = ag_ringing,
	.accept = ag_accept,
	.held = ag_held,
	.retrieve = ag_retrieve,
	.reject = ag_reject,
	.terminate = ag_terminate,
	.codec = ag_codec,
	.codec_negotiate = ag_codec_negotiate,
	.audio_connect_req = ag_audio_connect_req,
	.vgm = ag_vgm,
	.vgs = ag_vgs,
// #if defined(CONFIG_BT_HFP_AG_ECNR)
// 	.ecnr_turn_off = ag_ecnr_turn_off,
// #endif /* CONFIG_BT_HFP_AG_ECNR */
#if defined(CONFIG_BT_HFP_AG_3WAY_CALL)
	.explicit_call_transfer = ag_explicit_call_transfer,
#endif /* CONFIG_BT_HFP_AG_3WAY_CALL */
#if defined(CONFIG_BT_HFP_AG_VOICE_RECG)
	.voice_recognition = ag_voice_recognition,
#if defined(CONFIG_BT_HFP_AG_ENH_VOICE_RECG)
	.ready_to_accept_audio = ag_ready_to_accept_audio,
#endif /* CONFIG_BT_HFP_AG_ENH_VOICE_RECG */
#endif /* CONFIG_BT_HFP_AG_VOICE_RECG */
// #if defined(CONFIG_BT_HFP_AG_VOICE_TAG)
// 	.request_phone_number = ag_request_phone_number,
// #endif /* CONFIG_BT_HFP_AG_VOICE_TAG */
// 	.transmit_dtmf_code = ag_transmit_dtmf_code,
// 	.subscriber_number = ag_subscriber_number,
// 	.hf_indicator_value = ag_hf_indicator_value,
};

/* HFP HF callbacks */

static void hf_add_a_call(struct bt_hfp_hf_call *call)
{
	ARRAY_FOR_EACH(hfp_hf_call, i) {
		if (!hfp_hf_call[i]) {
			hfp_hf_call[i] = call;
			return;
		}
	}
}

static void hf_remove_calls(void)
{
	ARRAY_FOR_EACH(hfp_hf_call, i) {
		if (hfp_hf_call[i] != NULL) {
			hfp_hf_call[i] = NULL;
		}
	}
}

static void hf_connected(struct bt_conn *conn, struct bt_hfp_hf *hf)
{
	default_conn = conn;
	hfp_hf = hf;
	conn_count++;
	LOG_DBG("HF connected");
}

static void hf_disconnected(struct bt_hfp_hf *hf)
{
	default_conn = NULL;
	hfp_hf = NULL;
	hf_remove_calls();
	LOG_DBG("HF disconnected");
}

static void hf_sco_connected(struct bt_hfp_hf *hf, struct bt_conn *sco_conn)
{
	LOG_DBG("HF SCO connected %p", sco_conn);

	if (hf_sco_conn != NULL) {
		LOG_ERR("HF SCO conn %p exists", hf_sco_conn);
		return;
	}

	hf_sco_conn = bt_conn_ref(sco_conn);
}

static void hf_sco_disconnected(struct bt_conn *sco_conn, uint8_t reason)
{
	LOG_DBG("HF SCO disconnected %p (reason %u)", sco_conn, reason);

	if (hf_sco_conn == sco_conn) {
		bt_conn_unref(hf_sco_conn);
		hf_sco_conn = NULL;
	} else {
		LOG_ERR("Unknown SCO disconnected (%p != %p)", hf_sco_conn, sco_conn);
	}
}

static void hf_signal(struct bt_hfp_hf *hf, uint32_t value)
{
	hf_check_signal_strength = value;
}

static void hf_retrieve(struct bt_hfp_hf_call *call)
{
	LOG_DBG("hf call %p retrieve", call);
}

static void hf_battery(struct bt_hfp_hf *hf, uint32_t value)
{
	if (value == 5) {
		battery_charged_state = true;
	} else {
		battery_charged_state = false;
	}
}

static void hf_ring_indication(struct bt_conn *conn)
{
	ring_alert = true;
}

void hf_remote_ringing(struct bt_hfp_hf_call *call)
{
	hf_add_a_call(call);
}

void hf_outgoing(struct bt_hfp_hf *hf, struct bt_hfp_hf_call *call)
{
	hf_add_a_call(call);
	bt_hfp_hf_accept(call);
}

static void hf_incoming(struct bt_hfp_hf *hf, struct bt_hfp_hf_call *call)
{
	hf_add_a_call(call);
	bt_hfp_hf_accept(call);
}

static void hf_roam(struct bt_conn *conn, uint32_t value)
{
	roam_active_state = value ? true : false;
}

void hf_subscriber_number(struct bt_hfp_hf *hf, const char *number, uint8_t type, uint8_t service)
{
}

#if defined(CONFIG_BT_HFP_HF_ECNR)
static void hf_ecnr_turn_off(struct bt_hfp_hf *hf, int err)
{
}
#endif /* CONFIG_BT_HFP_HF_ECNR */

#if defined(CONFIG_BT_HFP_HF_CODEC_NEG)
static void hf_codec_negotiate(struct bt_hfp_hf *hf, uint8_t id)
{
	bt_hfp_hf_select_codec(hfp_hf, id);
}
#endif /* CONFIG_BT_HFP_HF_CODEC_NEG */

/* Minimal set of callbacks needed for connection */
static struct bt_hfp_hf_cb hf_cb = {
	.connected = hf_connected,
	.disconnected = hf_disconnected,
	.sco_connected = hf_sco_connected,
	.sco_disconnected = hf_sco_disconnected,
	.signal = hf_signal,
	.retrieve = hf_retrieve,
	.battery = hf_battery,
	.ring_indication = hf_ring_indication,
	.remote_ringing = hf_remote_ringing,
	.incoming = hf_incoming,
	.outgoing = hf_outgoing,
	.roam = hf_roam,
	.subscriber_number = hf_subscriber_number,
#if defined(CONFIG_BT_HFP_HF_ECNR)
	.ecnr_turn_off = hf_ecnr_turn_off,
#endif /* CONFIG_BT_HFP_HF_ECNR */
#if defined(CONFIG_BT_HFP_HF_CODEC_NEG)
	.codec_negotiate = hf_codec_negotiate,
#endif /* CONFIG_BT_HFP_HF_CODEC_NEG */
};

static uint8_t read_supported_commands(const void *cmd, uint16_t cmd_len,
				void *rsp, uint16_t *rsp_len)
{
	struct btp_hfp_read_supported_commands_rp *rp = rsp;

	*rsp_len = tester_supported_commands(BTP_SERVICE_ID_HFP, rp->data);
	*rsp_len += sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t enable_slc(const void *cmd, uint16_t cmd_len,
			  void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_enable_slc_cmd *cp = cmd;
	struct btp_hfp_enable_slc_rp *rp = rsp;
	struct bt_conn *conn;
	struct bt_conn_info info;
	struct bt_hfp_ag *ag;
	struct bt_hfp_hf *hf;
	uint8_t channel = cp->channel;
	int err;

	/* Connect to HFP service */
	if (cp->is_ag == 1) {
		if (default_conn == NULL) {
			bt_hfp_ag_register(&ag_cb);
			conn = bt_conn_create_br(&cp->address.a, BT_BR_CONN_PARAM_DEFAULT);
			if (conn == NULL) {
				return BTP_STATUS_FAILED;
			}
			default_conn = conn;
		}
		if (default_conn) {
			bt_conn_get_info(default_conn, &info);
			if (info.state == BT_CONN_STATE_CONNECTED) {
				bt_hfp_ag_connect(default_conn, &ag, channel);
				return BTP_STATUS_SUCCESS;
			}
			else {
				default_conn = NULL;
			}
		}
	} else {
		if (default_conn == NULL) {
			conn = bt_conn_lookup_addr_br(&cp->address.a);
			if (conn != NULL)
			{
				bt_conn_unref(conn);
			}

			conn = bt_conn_create_br(&cp->address.a, BT_BR_CONN_PARAM_DEFAULT);
			if (conn == NULL)
			{
				return BTP_STATUS_FAILED;
			}
		}
		default_conn = conn;
		if (default_conn) {
			bt_conn_get_info(default_conn, &info);
			if (info.state == BT_CONN_STATE_CONNECTED) {
				err = bt_hfp_hf_connect(default_conn, &hf, channel);
				return BTP_STATUS_SUCCESS;
			} else {
				default_conn = NULL;
			}
		}
	}
	/* Set connection ID in response */
	rp->connection_id = 1;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}


static uint8_t disable_slc(const void *cmd, uint16_t cmd_len,
			   void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_disable_slc_cmd *cp = cmd;
	struct btp_hfp_disable_slc_rp *rp = rsp;
	uint8_t count = 0;

	if (hfp_ag) {
		bt_hfp_ag_disconnect(hfp_ag);
	} else {
		while (conn_count == 0) {
			count++;
			OSA_TimeDelay(500);
			if (count > 100)
			break;
		}
		bt_hfp_hf_disconnect(hfp_hf);
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t signal_strength_send(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_signal_strength_send_cmd *cp = cmd;
	struct btp_hfp_signal_strength_send_rp *rp = rsp;

	bt_hfp_ag_signal_strength(hfp_ag, cp->strength);

	return BTP_STATUS_SUCCESS;
}

static uint8_t signal_strength_verify(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_signal_strength_verify_cmd *cp = cmd;
	struct btp_hfp_signal_strength_verify_rp *rp = rsp;

	if (hf_check_signal_strength == cp->strength) {
		return BTP_STATUS_SUCCESS;
	} else {
		return BTP_STATUS_FAILED;
	}
}

static uint8_t control(const void *cmd, uint16_t cmd_len,
		       void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_control_cmd *cp = cmd;
	int err = 0;

	switch (cp->control_type) {
	case HFP_IMPAIR_SIGNAL:
		if (hf_check_signal_strength > 0) {
			hf_check_signal_strength--;
		}
		bt_hfp_ag_set_indicator(hfp_ag, BT_HFP_AG_SIGNAL_IND, hf_check_signal_strength);
		break;
	case HFP_AG_ANSWER_CALL:
		err = bt_hfp_ag_accept(hfp_ag_call[0]);
		s_hfp_in_calling_status = 3;
		break;
	case HFP_REJECT_CALL:
		if (hfp_ag) {
			err = bt_hfp_ag_reject(hfp_ag_call[0]);
		} else {
			// bt_hfp_hf_send_cmd(default_conn, BT_HFP_HF_AT_CHUP);
		}
		break;
	case HFP_DISABLE_IN_BAND:
		err = bt_hfp_ag_inband_ringtone(hfp_ag, false);
		break;
	case HFP_TWC_CALL:
		if (hfp_ag) {
			err = bt_hfp_ag_explicit_call_transfer(hfp_ag);
		}
		break;
	case HFP_ENABLE_VR:
		err = bt_hfp_ag_voice_recognition(hfp_ag, true);
		break;
	case HFP_SEND_BCC:
		if (hfp_ag) {
			bt_hfp_ag_audio_connect(hfp_ag, BT_HFP_AG_CODEC_CVSD);
			s_hfp_in_calling_status = 3;
		} else {
			bt_hfp_hf_audio_connect(hfp_hf);
		}
		break;
	case HFP_SEND_BCC_MSBC:
		if (hfp_ag) {
			bt_hfp_ag_audio_connect(hfp_ag, BT_HFP_AG_CODEC_MSBC);
		} else {
			bt_hfp_hf_audio_connect(hfp_hf);
		}
		break;
	case HFP_SEND_BCC_SWB:
		if (hfp_ag) {
			bt_hfp_ag_audio_connect(hfp_ag, BT_HFP_AG_CODEC_LC3_SWB);
		} else {
			bt_hfp_hf_audio_connect(hfp_hf);
		}
		break;
	case HFP_CLS_MEM_CALL_LIST:
		mem_call_list = 1;
		break;
	case HFP_ACCEPT_HELD_CALL:
		if (hfp_hf_call[0]) {
			err = bt_hfp_hf_hold_incoming(hfp_hf_call[0]);
		} else {
			err = -1;
		}
		break;
	case HFP_ACCEPT_INCOMING_HELD_CALL:
		if (hfp_hf_call[0]) {
			err = bt_hfp_hf_hold_incoming(hfp_hf_call[0]);
		} else {
			err = -1;
		}
		break;
	case HFP_REJECT_HELD_CALL:
		if (hfp_hf_call[0]) {
			err = bt_hfp_hf_reject(hfp_hf_call[0]);
		} else {
			err = -1;
		}
		break;
	case HFP_OUT_CALL:
		bt_hfp_ag_outgoing(hfp_ag, '7654321');
		break;
	case HFP_ENABLE_CLIP:
		err = bt_hfp_hf_cli(hfp_hf, true);
		break;
	case HFP_QUERY_LIST_CALL:
		// err = bt_hfp_hf_query_respond_hold_status(hfp_hf);
		break;
	case HFP_SEND_IIA:
		err = bt_hfp_hf_indicator_status(hfp_hf, 5);
		break;
	case HFP_ENABLE_SUB_NUMBER:
		err = bt_hfp_hf_query_subscriber(hfp_hf);
		break;
	case HFP_OUT_MEM_CALL:
		err = bt_hfp_hf_memory_dial(hfp_hf, "1");
		break;
	case HFP_EC_NR_DISABLE:
		err = bt_hfp_hf_turn_off_ecnr(hfp_hf);
		break;
	case HFP_DISABLE_VR:
		err = bt_hfp_ag_voice_recognition(hfp_ag, false);
		break;
	default:
		err = -1;
	}

	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ag_enable_call(const void *cmd, uint16_t cmd_len,
			      void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_enable_call_cmd *cp = cmd;
	struct btp_hfp_ag_enable_call_rp *rp = rsp;
	int err = 0;

	err = bt_hfp_ag_remote_incoming(hfp_ag, "1234567");
	if (err < 0) {
		return BTP_STATUS_FAILED;
	}
	return BTP_STATUS_SUCCESS;
}

static uint8_t ag_discoverable(const void *cmd, uint16_t cmd_len,
			       void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_discoverable_cmd *cp = cmd;
	struct btp_hfp_ag_discoverable_rp *rp = rsp;

	bt_hfp_ag_register(&ag_cb);

	return BTP_STATUS_SUCCESS;
}

static uint8_t hf_discoverable(const void *cmd, uint16_t cmd_len,
			       void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_discoverable_cmd *cp = cmd;
	struct btp_hfp_hf_discoverable_rp *rp = rsp;

	bt_hfp_hf_register(&hf_cb);

	return BTP_STATUS_SUCCESS;
}

static uint8_t verify_network_operator(const void *cmd, uint16_t cmd_len,
			   void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_verify_network_operator_cmd *cp = cmd;
	struct btp_hfp_verify_network_operator_rp *rp = rsp;

	if (strncmp(cp->op, cops_name, MAX_COPS_NAME_SIZE) == 0) {
		return BTP_STATUS_SUCCESS;
	}

	return BTP_STATUS_FAILED;
}

static uint8_t ag_disable_call_external(const void *cmd, uint16_t cmd_len,
					void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_disable_call_external_cmd *cp = cmd;
	struct btp_hfp_ag_disable_call_external_rp *rp = rsp;
	int err;

	err = bt_hfp_ag_remote_terminate(hfp_ag_call[0]);
	// if (err) {
	// 	return BTP_STATUS_FAILED;
	// }
	return BTP_STATUS_SUCCESS;
}

static uint8_t hf_answer_call(const void *cmd, uint16_t cmd_len,
			   void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_answer_call_cmd *cp = cmd;
	struct btp_hfp_hf_answer_call_rp *rp = rsp;

	hf_accept_call = true;
	bt_hfp_hf_accept(hfp_hf_call[0]);

	return BTP_STATUS_SUCCESS;
}

static uint8_t verify(const void *cmd, uint16_t cmd_len,
			   void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_verify_cmd *cp = cmd;
	struct btp_hfp_verify_rp *rp = rsp;

	switch (cp->verify_type) {
	case HFP_VERIFY_EC_NR_DISABLED:
		if (ec_nr_disabled) {
			return BTP_STATUS_SUCCESS;
		} else {
			return BTP_STATUS_FAILED;
		}
		break;
	case HFP_VERIFY_INBAND_RING:
		break;
	default:
		break;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t verify_voice_tag(const void *cmd, uint16_t cmd_len,
			        void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_verify_voice_tag_cmd *cp = cmd;
	struct btp_hfp_verify_voice_tag_rp *rp = rsp;

	if (strncmp(cp->voice_tag, voice_tag, MAX_COPS_NAME_SIZE) == 0) {
		return BTP_STATUS_SUCCESS;
	}

	return BTP_STATUS_FAILED;

}

static uint8_t speaker_mic_volume_send(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_speaker_mic_volume_send_cmd *cp = cmd;
	struct btp_hfp_speaker_mic_volume_send_rp *rp = rsp;
	int err;
	if (cp->speaker_mic == 0x0) {
		if (hfp_ag != NULL) {
			err = bt_hfp_ag_vgs(hfp_ag, cp->speaker_mic_volume);
			if (err) {
				return BTP_STATUS_FAILED;
			}
		}
		else if (default_conn != NULL) {
			err = bt_hfp_ag_vgs(hfp_ag, cp->speaker_mic_volume);
			if (err) {
				return BTP_STATUS_FAILED;
			}
		}
		hf_check_speaker_volume = cp->speaker_mic_volume;
	} else if (cp->speaker_mic == 0x1) {
		if (hfp_ag != NULL) {
			err = bt_hfp_ag_vgm(hfp_ag, cp->speaker_mic_volume);
			if (err) {
				return BTP_STATUS_FAILED;
			}
		}
		else if (default_conn != NULL) {
			err = bt_hfp_ag_vgm(hfp_ag, cp->speaker_mic_volume);
			if (err) {
				return BTP_STATUS_FAILED;
			}
		}

		hf_check_mic_volume = cp->speaker_mic_volume;
	} else {
		return BTP_STATUS_UNKNOWN_CMD;
	}
	return BTP_STATUS_SUCCESS;
}

static uint8_t enable_audio(const void *cmd, uint16_t cmd_len,
			    void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_enable_audio_cmd *cp = cmd;
	struct btp_hfp_enable_audio_rp *rp = rsp;
	// int err;

	if (hfp_ag){
		bt_hfp_ag_audio_connect(hfp_ag, BT_HFP_AG_CODEC_MSBC);
	} else {
		bt_hfp_hf_audio_connect(hfp_hf);
	}
	
	// if (err) {
	// 	return BTP_STATUS_FAILED;
	// }

	return BTP_STATUS_SUCCESS;
}

static uint8_t disable_audio(const void *cmd, uint16_t cmd_len,
			     void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_disable_audio_cmd *cp = cmd;
	struct btp_hfp_disable_audio_rp *rp = rsp;
	// int err;

	bt_conn_disconnect(hfp_ag_sco_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	// if (err) {
	// 	return BTP_STATUS_FAILED;
	// }

	return BTP_STATUS_SUCCESS;
}

static uint8_t enable_network(const void *cmd, uint16_t cmd_len,
			      void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_enable_network_cmd *cp = cmd;
	struct btp_hfp_enable_network_rp *rp = rsp;

	bt_hfp_ag_service_availability(hfp_ag, true);

	return BTP_STATUS_SUCCESS;
}

static uint8_t disable_network(const void *cmd, uint16_t cmd_len,
			       void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_disable_network_cmd *cp = cmd;
	struct btp_hfp_disable_network_rp *rp = rsp;

	bt_hfp_ag_service_availability(hfp_ag, false);

	return BTP_STATUS_SUCCESS;
}

static uint8_t make_roam_active(const void *cmd, uint16_t cmd_len,
				void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_make_roam_active_cmd *cp = cmd;
	struct btp_hfp_make_roam_active_rp *rp = rsp;

	bt_hfp_ag_roaming_status(hfp_ag, 1);
	return BTP_STATUS_SUCCESS;
}

static uint8_t make_roam_inactive(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_make_roam_inactive_cmd *cp = cmd;
	struct btp_hfp_make_roam_inactive_rp *rp = rsp;

	bt_hfp_ag_roaming_status(hfp_ag, 0);
	return BTP_STATUS_SUCCESS;
}

static uint8_t make_battery_not_full_charged(const void *cmd, uint16_t cmd_len,
					     void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_make_battery_not_full_charged_cmd *cp = cmd;
	struct btp_hfp_make_battery_not_full_charged_rp *rp = rsp;
	int err;

	if (hfp_ag != NULL){
		err = bt_hfp_ag_battery_level(hfp_ag, 3);
		if (err) {
			return BTP_STATUS_FAILED;
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t make_battery_full_charged(const void *cmd, uint16_t cmd_len,
					 void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_make_battery_full_charged_cmd *cp = cmd;
	struct btp_hfp_make_battery_full_charged_rp *rp = rsp;
	int err;

	if (hfp_ag != NULL){
		err = bt_hfp_ag_battery_level(hfp_ag, 5);
		if (err) {
			return BTP_STATUS_FAILED;
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t verify_battery_charged(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_verify_battery_charged_cmd *cp = cmd;
	struct btp_hfp_verify_battery_charged_rp *rp = rsp;

	if (battery_charged_state) {
		return BTP_STATUS_SUCCESS;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t verify_battery_discharged(const void *cmd, uint16_t cmd_len,
					 void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_verify_battery_discharged_cmd *cp = cmd;
	struct btp_hfp_verify_battery_discharged_rp *rp = rsp;

	if (!battery_charged_state) {
		return BTP_STATUS_SUCCESS;
	}

	return BTP_STATUS_FAILED;
}

static uint8_t speaker_mic_volume_verify(const void *cmd, uint16_t cmd_len,
			   void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_speaker_mic_volume_verify_cmd *cp = cmd;
	struct btp_hfp_speaker_mic_volume_verify_rp *rp = rsp;

	if (cp->speaker_mic == 0x1) {
		if (hf_check_mic_volume == cp->speaker_mic_volume) {
			return BTP_STATUS_SUCCESS;
		} else {
			return BTP_STATUS_FAILED;
		}
	} else if (cp->speaker_mic == 0x0) {
		if (hf_check_speaker_volume == cp->speaker_mic_volume) {
			return BTP_STATUS_SUCCESS;
		} else {
			return BTP_STATUS_FAILED;
		}
	} else {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ag_register(const void *cmd, uint16_t cmd_len,
			   void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_register_cmd *cp = cmd;
	struct btp_hfp_ag_register_rp *rp = rsp;
	int err;

	err = bt_hfp_ag_register(&ag_cb);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hf_register(const void *cmd, uint16_t cmd_len,
			   void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_register_cmd *cp = cmd;
	struct btp_hfp_hf_register_rp *rp = rsp;
	int err;

	err = bt_hfp_hf_register(&hf_cb);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t verify_roam_active(const void *cmd, uint16_t cmd_len,
			   void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_verify_roam_active_cmd *cp = cmd;
	struct btp_hfp_verify_roam_active_rp *rp = rsp;

	if (roam_active_state) {
		return BTP_STATUS_SUCCESS;
	}
	return BTP_STATUS_FAILED;
}

static uint8_t query_network_operator(const void *cmd, uint16_t cmd_len,
			   void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_query_network_operator_cmd *cp = cmd;
	struct btp_hfp_query_network_operator_rp *rp = rsp;
	int err;

	err = bt_hfp_hf_get_operator(hfp_hf);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ag_vre_text(const void *cmd, uint16_t cmd_len,
			   void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_vre_text_cmd *cp = cmd;
	struct btp_hfp_ag_vre_text_rp *rp = rsp;
	int err;

	err = bt_hfp_ag_vre_textual_representation(hfp_ag, 1, "2", cp->type, cp->operation, "1");
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

/* BTP API COMPLETION */

static const struct btp_handler hfp_handlers[] = {
	{
		.opcode = BTP_HFP_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = read_supported_commands
	},
	{
		.opcode = BTP_HFP_ENABLE_SLC,
		.expect_len = sizeof(struct btp_hfp_enable_slc_cmd),
		.func = enable_slc
	},
	{
		.opcode = BTP_HFP_DISABLE_SLC,
		.expect_len = sizeof(struct btp_hfp_disable_slc_cmd),
		.func = disable_slc
	},
	{
		.opcode = BTP_HFP_SIGNAL_STRENGTH_SEND,
		.expect_len = sizeof(struct btp_hfp_signal_strength_send_cmd),
		.func = signal_strength_send
	},
	{
		.opcode = BTP_HFP_CONTROL,
		.expect_len = sizeof(struct btp_hfp_control_cmd),
		.func = control
	},
	{
		.opcode = BTP_HFP_SIGNAL_STRENGTH_VERIFY,
		.expect_len = sizeof(struct btp_hfp_signal_strength_verify_cmd),
		.func = signal_strength_verify
	},
	{
		.opcode = BTP_HFP_AG_ENABLE_CALL,
		.expect_len = sizeof(struct btp_hfp_ag_enable_call_cmd),
		.func = ag_enable_call
	},
	{
		.opcode = BTP_HFP_AG_DISCOVERABLE,
		.expect_len = sizeof(struct btp_hfp_ag_discoverable_cmd),
		.func = ag_discoverable
	},
	{
		.opcode = BTP_HFP_HF_DISCOVERABLE,
		.expect_len = sizeof(struct btp_hfp_hf_discoverable_cmd),
		.func = hf_discoverable
	},

	{
		.opcode = BTP_HFP_VERIFY_NETWORK_OPERATOR,
		.expect_len = sizeof(struct btp_hfp_verify_network_operator_cmd),
		.func = verify_network_operator
	},

	{
		.opcode = BTP_HFP_AG_DISABLE_CALL_EXTERNAL,
		.expect_len = sizeof(struct btp_hfp_ag_disable_call_external_cmd),
		.func = ag_disable_call_external
	},

	{
		.opcode = BTP_HFP_HF_ANSWER_CALL,
		.expect_len = sizeof(struct btp_hfp_hf_answer_call_cmd),
		.func = hf_answer_call
	},

	{
		.opcode = BTP_HFP_VERIFY,
		.expect_len = sizeof(struct btp_hfp_verify_cmd),
		.func = verify
	},

	{
		.opcode = BTP_HFP_VERIFY_VOICE_TAG,
		.expect_len = sizeof(struct btp_hfp_verify_voice_tag_cmd),
		.func = verify_voice_tag
	},

	{
		.opcode = BTP_HFP_SPEAKER_MIC_VOLUME_SEND,
		.expect_len = sizeof(struct btp_hfp_speaker_mic_volume_send_cmd),
		.func = speaker_mic_volume_send
	},
	{
		.opcode = BTP_HFP_ENABLE_AUDIO,
		.expect_len = sizeof(struct btp_hfp_enable_audio_cmd),
		.func = enable_audio
	},
	{
		.opcode = BTP_HFP_DISABLE_AUDIO,
		.expect_len = sizeof(struct btp_hfp_disable_audio_cmd),
		.func = disable_audio
	},
	{
		.opcode = BTP_HFP_DISABLE_NETWORK,
		.expect_len = sizeof(struct btp_hfp_disable_network_cmd),
		.func = disable_network
	},
	{
		.opcode = BTP_HFP_ENABLE_NETWORK,
		.expect_len = sizeof(struct btp_hfp_enable_network_cmd),
		.func = enable_network
	},
	{
		.opcode = BTP_HFP_MAKE_ROAM_ACTIVE,
		.expect_len = sizeof(struct btp_hfp_make_roam_active_cmd),
		.func = make_roam_active
	},
	{
		.opcode = BTP_HFP_MAKE_ROAM_INACTIVE,
		.expect_len = sizeof(struct btp_hfp_make_roam_inactive_cmd),
		.func = make_roam_inactive
	},

	{
		.opcode = BTP_HFP_MAKE_BATTERY_NOT_FULL_CHARGED,
		.expect_len = sizeof(struct btp_hfp_make_battery_not_full_charged_cmd),
		.func = make_battery_not_full_charged
	},

	{
		.opcode = BTP_HFP_MAKE_BATTERY_FULL_CHARGED,
		.expect_len = sizeof(struct btp_hfp_make_battery_full_charged_cmd),
		.func = make_battery_full_charged
	},

	{
		.opcode = BTP_HFP_VERIFY_BATTERY_CHARGED,
		.expect_len = sizeof(struct btp_hfp_verify_battery_charged_cmd),
		.func = verify_battery_charged
	},

	{
		.opcode = BTP_HFP_VERIFY_BATTERY_DISCHARGED,
		.expect_len = sizeof(struct btp_hfp_verify_battery_discharged_cmd),
		.func = verify_battery_discharged
	},

	{
		.opcode = BTP_HFP_SPEAKER_MIC_VOLUME_VERIFY,
		.expect_len = sizeof(struct btp_hfp_speaker_mic_volume_verify_cmd),
		.func = speaker_mic_volume_verify
	},

	{
		.opcode = BTP_HFP_AG_REGISTER,
		.expect_len = sizeof(struct btp_hfp_ag_register_cmd),
		.func = ag_register
	},

	{
		.opcode = BTP_HFP_HF_REGISTER,
		.expect_len = sizeof(struct btp_hfp_hf_register_cmd),
		.func = hf_register
	},

	{
		.opcode = BTP_HFP_VERIFY_ROAM_ACTIVE,
		.expect_len = sizeof(struct btp_hfp_verify_roam_active_cmd),
		.func = verify_roam_active
	},

	{
		.opcode = BTP_HFP_QUERY_NETWORK_OPERATOR,
		.expect_len = sizeof(struct btp_hfp_query_network_operator_cmd),
		.func = query_network_operator
	},

	{
		.opcode = BTP_HFP_AG_VRE_TEXT,
		.expect_len = sizeof(struct btp_hfp_ag_vre_text_cmd),
		.func = ag_vre_text
	},
/* BTP BONDING COMPLETION */
};

uint8_t tester_init_hfp(void)
{
	tester_register_command_handlers(BTP_SERVICE_ID_HFP, hfp_handlers,
					 ARRAY_SIZE(hfp_handlers));

	hf_accept_call = false;
	hf_check_signal_strength = 5;

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_hfp(void)
{
	return BTP_STATUS_SUCCESS;
}
