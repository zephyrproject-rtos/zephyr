/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/hfp_hf.h>
#include <zephyr/bluetooth/classic/hfp_ag.h>

#include <zephyr/shell/shell.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

#define HELP_NONE "[none]"

extern struct bt_conn *default_conn;

#if defined(CONFIG_BT_HFP_HF)

struct bt_conn *hf_conn;
struct bt_hfp_hf *hfp_hf;
struct bt_conn *hf_sco_conn;
static struct bt_hfp_hf_call *hfp_hf_call[CONFIG_BT_HFP_HF_MAX_CALLS];

static void hf_add_a_call(struct bt_hfp_hf_call *call)
{
	for (size_t index = 0; index < ARRAY_SIZE(hfp_hf_call); index++) {
		if (!hfp_hf_call[index]) {
			hfp_hf_call[index] = call;
			return;
		}
	}
}

static void hf_remove_a_call(struct bt_hfp_hf_call *call)
{
	for (size_t index = 0; index < ARRAY_SIZE(hfp_hf_call); index++) {
		if (call == hfp_hf_call[index]) {
			hfp_hf_call[index] = NULL;
			return;
		}
	}
}

static void hf_connected(struct bt_conn *conn, struct bt_hfp_hf *hf)
{
	hf_conn = conn;
	hfp_hf = hf;
	bt_shell_print("HF connected");
}

static void hf_disconnected(struct bt_hfp_hf *hf)
{
	hf_conn = NULL;
	hfp_hf = NULL;
	bt_shell_print("HF disconnected");
}

static void hf_sco_connected(struct bt_hfp_hf *hf, struct bt_conn *sco_conn)
{
	bt_shell_print("HF SCO connected %p", sco_conn);

	if (hf_sco_conn != NULL) {
		bt_shell_warn("HF SCO conn %p exists", hf_sco_conn);
		return;
	}

	hf_sco_conn = bt_conn_ref(sco_conn);
}

static void hf_sco_disconnected(struct bt_conn *sco_conn, uint8_t reason)
{
	bt_shell_print("HF SCO disconnected %p (reason %u)", sco_conn, reason);

	if (hf_sco_conn == sco_conn) {
		bt_conn_unref(hf_sco_conn);
		hf_sco_conn = NULL;
	} else {
		bt_shell_warn("Unknown SCO disconnected (%p != %p)", hf_sco_conn, sco_conn);
	}
}

void hf_service(struct bt_hfp_hf *hf, uint32_t value)
{
	bt_shell_print("HF service %d", value);
}

void hf_outgoing(struct bt_hfp_hf *hf, struct bt_hfp_hf_call *call)
{
	hf_add_a_call(call);
	bt_shell_print("HF call %p outgoing", call);
}

void hf_remote_ringing(struct bt_hfp_hf_call *call)
{
	bt_shell_print("HF remote call %p start ringing", call);
}

void hf_incoming(struct bt_hfp_hf *hf, struct bt_hfp_hf_call *call)
{
	hf_add_a_call(call);
	bt_shell_print("HF call %p incoming", call);
}

void hf_incoming_held(struct bt_hfp_hf_call *call)
{
	bt_shell_print("HF call %p is held", call);
}

void hf_accept(struct bt_hfp_hf_call *call)
{
	bt_shell_print("HF call %p accepted", call);
}

void hf_reject(struct bt_hfp_hf_call *call)
{
	hf_remove_a_call(call);
	bt_shell_print("HF call %p rejected", call);
}

void hf_terminate(struct bt_hfp_hf_call *call)
{
	hf_remove_a_call(call);
	bt_shell_print("HF call %p terminated", call);
}

void hf_held(struct bt_hfp_hf_call *call)
{
	bt_shell_print("hf call %p held", call);
}

void hf_retrieve(struct bt_hfp_hf_call *call)
{
	bt_shell_print("hf call %p retrieve", call);
}

void hf_signal(struct bt_hfp_hf *hf, uint32_t value)
{
	bt_shell_print("HF signal %d", value);
}

void hf_roam(struct bt_hfp_hf *hf, uint32_t value)
{
	bt_shell_print("HF roam %d", value);
}

void hf_battery(struct bt_hfp_hf *hf, uint32_t value)
{
	bt_shell_print("HF battery %d", value);
}

void hf_ring_indication(struct bt_hfp_hf_call *call)
{
	bt_shell_print("HF call %p ring", call);
}

void hf_dialing(struct bt_hfp_hf *hf, int err)
{
	bt_shell_print("HF start dialing call: err %d", err);
}

#if defined(CONFIG_BT_HFP_HF_CLI)
void hf_clip(struct bt_hfp_hf_call *call, char *number, uint8_t type)
{
	bt_shell_print("HF call %p CLIP %s %d", call, number, type);
}
#endif /* CONFIG_BT_HFP_HF_CLI */

#if defined(CONFIG_BT_HFP_HF_VOLUME)
static void hf_vgm(struct bt_hfp_hf *hf, uint8_t gain)
{
	bt_shell_print("HF VGM %d", gain);
}

static void hf_vgs(struct bt_hfp_hf *hf, uint8_t gain)
{
	bt_shell_print("HF VGS %d", gain);
}
#endif /* CONFIG_BT_HFP_HF_VOLUME */

static void hf_inband_ring(struct bt_hfp_hf *hf, bool inband)
{
	bt_shell_print("HF ring: %s", inband ? "in-band" : "no in-hand");
}

static void hf_operator(struct bt_hfp_hf *hf, uint8_t mode, uint8_t format, char *operator)
{
	bt_shell_print("HF mode %d, format %d, operator %s", mode, format, operator);
}

#if defined(CONFIG_BT_HFP_HF_CODEC_NEG)
static void hf_codec_negotiate(struct bt_hfp_hf *hf, uint8_t id)
{
	bt_shell_print("codec negotiation: %d", id);
}
#endif /* CONFIG_BT_HFP_HF_CODEC_NEG */

#if defined(CONFIG_BT_HFP_HF_ECNR)
static void hf_ecnr_turn_off(struct bt_hfp_hf *hf, int err)
{
	bt_shell_print("Turn off ECNR: %d", err);
}
#endif /* CONFIG_BT_HFP_HF_ECNR */

#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
static void hf_call_waiting(struct bt_hfp_hf_call *call, char *number, uint8_t type)
{
	bt_shell_print("3way call %p waiting. number %s type %d", call, number, type);
}
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */

#if defined(CONFIG_BT_HFP_HF_VOICE_RECG)
void hf_voice_recognition(struct bt_hfp_hf *hf, bool activate)
{
	bt_shell_print("Voice recognition %s", activate ? "activate" : "deactivate");
}

#if defined(CONFIG_BT_HFP_HF_ENH_VOICE_RECG)
void hf_vre_state(struct bt_hfp_hf *hf, uint8_t state)
{
	bt_shell_print("Voice recognition engine state %d", state);
}
#endif /* CONFIG_BT_HFP_HF_ENH_VOICE_RECG */

#if defined(CONFIG_BT_HFP_HF_VOICE_RECG_TEXT)
void hf_textual_representation(struct bt_hfp_hf *hf, char *id, uint8_t type, uint8_t operation,
			       char *text)
{
	bt_shell_print("Text id %s, type %d, operation %d, string %s", id, type, operation, text);
}
#endif /* CONFIG_BT_HFP_HF_VOICE_RECG_TEXT */
#endif /* CONFIG_BT_HFP_HF_VOICE_RECG */

void hf_request_phone_number(struct bt_hfp_hf *hf, const char *number)
{
	if (number) {
		bt_shell_print("Requested phone number %s", number);
	} else {
		bt_shell_print("Failed to request phone number");
	}
}

void hf_subscriber_number(struct bt_hfp_hf *hf, const char *number, uint8_t type, uint8_t service)
{
	bt_shell_print("Subscriber number %s, type %d, service %d", number, type, service);
}

static struct bt_hfp_hf_cb hf_cb = {
	.connected = hf_connected,
	.disconnected = hf_disconnected,
	.sco_connected = hf_sco_connected,
	.sco_disconnected = hf_sco_disconnected,
	.service = hf_service,
	.outgoing = hf_outgoing,
	.remote_ringing = hf_remote_ringing,
	.incoming = hf_incoming,
	.incoming_held = hf_incoming_held,
	.accept = hf_accept,
	.reject = hf_reject,
	.terminate = hf_terminate,
	.held = hf_held,
	.retrieve = hf_retrieve,
	.signal = hf_signal,
	.roam = hf_roam,
	.battery = hf_battery,
	.ring_indication = hf_ring_indication,
	.dialing = hf_dialing,
#if defined(CONFIG_BT_HFP_HF_CLI)
	.clip = hf_clip,
#endif /* CONFIG_BT_HFP_HF_CLI */
#if defined(CONFIG_BT_HFP_HF_VOLUME)
	.vgm = hf_vgm,
	.vgs = hf_vgs,
#endif /* CONFIG_BT_HFP_HF_VOLUME */
	.inband_ring = hf_inband_ring,
	.operator = hf_operator,
#if defined(CONFIG_BT_HFP_HF_CODEC_NEG)
	.codec_negotiate = hf_codec_negotiate,
#endif /* CONFIG_BT_HFP_HF_CODEC_NEG */
#if defined(CONFIG_BT_HFP_HF_ECNR)
	.ecnr_turn_off = hf_ecnr_turn_off,
#endif /* CONFIG_BT_HFP_HF_ECNR */
#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
	.call_waiting = hf_call_waiting,
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */
#if defined(CONFIG_BT_HFP_HF_VOICE_RECG)
	.voice_recognition = hf_voice_recognition,
#if defined(CONFIG_BT_HFP_HF_ENH_VOICE_RECG)
	.vre_state = hf_vre_state,
#endif /* CONFIG_BT_HFP_HF_ENH_VOICE_RECG */
#if defined(CONFIG_BT_HFP_HF_VOICE_RECG_TEXT)
	.textual_representation = hf_textual_representation,
#endif /* CONFIG_BT_HFP_HF_VOICE_RECG_TEXT */
#endif /* CONFIG_BT_HFP_HF_VOICE_RECG */
	.request_phone_number = hf_request_phone_number,
	.subscriber_number = hf_subscriber_number,
};

static int cmd_reg_enable(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_hfp_hf_register(&hf_cb);
	if (err) {
		shell_error(sh, "Callback register failed: %d", err);
	}

	return err;
}

static int cmd_connect(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	struct bt_hfp_hf *hf;
	uint8_t channel;

	channel = atoi(argv[1]);

	err = bt_hfp_hf_connect(default_conn, &hf, channel);
	if (err) {
		shell_error(sh, "Connect failed: %d", err);
	}

	return err;
}

static int cmd_disconnect(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_hfp_hf_disconnect(hfp_hf);
	if (err) {
		shell_error(sh, "Disconnect failed: %d", err);
	}

	return err;
}

static int cmd_sco_disconnect(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_conn_disconnect(hf_sco_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		shell_error(sh, "Disconnect failed: %d", err);
	}

	return err;
}

#if defined(CONFIG_BT_HFP_HF_CLI)
static int cmd_cli_enable(const struct shell *sh, size_t argc, char **argv)
{
	const char *action = argv[1];
	bool enable;
	int err;

	if (strcmp(action, "enable") == 0) {
		enable = true;
	} else if (strcmp(action, "disable") == 0) {
		enable = false;
	} else {
		shell_error(sh, "Invalid option.");
		return -ENOEXEC;
	}

	err = bt_hfp_hf_cli(hfp_hf, enable);
	if (err) {
		shell_error(sh, "Fail to send AT+CLIP=%d (err %d)", enable, err);
		return -ENOEXEC;
	}

	return 0;
}
#endif /* CONFIG_BT_HFP_HF_CLI */

#if defined(CONFIG_BT_HFP_HF_VOLUME)
static int cmd_vgm_enable(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t gain;
	int err;

	gain = atoi(argv[1]);

	err = bt_hfp_hf_vgm(hfp_hf, gain);
	if (err) {
		shell_error(sh, "Fail to send AT+VGM=%d (err %d)", gain, err);
	}

	return err;
}

static int cmd_vgs_enable(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t gain;
	int err;

	gain = atoi(argv[1]);

	err = bt_hfp_hf_vgs(hfp_hf, gain);
	if (err) {
		shell_error(sh, "Fail to send AT+VGS=%d (err %d)", gain, err);
	}

	return err;
}
#endif /* CONFIG_BT_HFP_HF_VOLUME */

static int cmd_operator(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_hfp_hf_get_operator(hfp_hf);
	if (err) {
		shell_error(sh, "Failed to read network operator: %d", err);
	}

	return err;
}

#if defined(CONFIG_BT_HFP_HF_CODEC_NEG)
static int cmd_audio_connect(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_hfp_hf_audio_connect(hfp_hf);
	if (err) {
		shell_error(sh, "Failed to start audio connection procedure: %d", err);
	}

	return err;
}

static int cmd_select_codec(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	uint8_t codec_id;

	codec_id = atoi(argv[1]);

	err = bt_hfp_hf_select_codec(hfp_hf, codec_id);
	if (err) {
		shell_error(sh, "Failed to select codec id: %d", err);
	}

	return err;
}

static int cmd_set_codecs(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	uint8_t codec_ids;

	codec_ids = atoi(argv[1]);

	err = bt_hfp_hf_set_codecs(hfp_hf, codec_ids);
	if (err) {
		shell_error(sh, "Failed to set codecs: %d", err);
	}

	return err;
}
#endif /* CONFIG_BT_HFP_HF_CODEC_NEG */

static int cmd_accept(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	int index;

	index = atoi(argv[1]);
	if ((index >= ((int)ARRAY_SIZE(hfp_hf_call))) || !hfp_hf_call[index]) {
		shell_error(sh, "Invalid call index: %d", index);
		return -EINVAL;
	}

	err = bt_hfp_hf_accept(hfp_hf_call[index]);
	if (err) {
		shell_error(sh, "Failed to accept call: %d", err);
	}

	return err;
}

static int cmd_reject(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	int index;

	index = atoi(argv[1]);
	if ((index >= ((int)ARRAY_SIZE(hfp_hf_call))) || !hfp_hf_call[index]) {
		shell_error(sh, "Invalid call index: %d", index);
		return -EINVAL;
	}

	err = bt_hfp_hf_reject(hfp_hf_call[index]);
	if (err) {
		shell_error(sh, "Failed to reject call: %d", err);
	}

	return err;
}

static int cmd_terminate(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	int index;

	index = atoi(argv[1]);
	if ((index >= ((int)ARRAY_SIZE(hfp_hf_call))) || !hfp_hf_call[index]) {
		shell_error(sh, "Invalid call index: %d", index);
		return -EINVAL;
	}

	err = bt_hfp_hf_terminate(hfp_hf_call[index]);
	if (err) {
		shell_error(sh, "Failed to terminate call: %d", err);
	}

	return err;
}

static int cmd_hold_incoming(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	int index;

	index = atoi(argv[1]);
	if ((index >= ((int)ARRAY_SIZE(hfp_hf_call))) || !hfp_hf_call[index]) {
		shell_error(sh, "Invalid call index: %d", index);
		return -EINVAL;
	}

	err = bt_hfp_hf_hold_incoming(hfp_hf_call[index]);
	if (err) {
		shell_error(sh, "Failed to put incoming call on hold: %d", err);
	}

	return err;
}

static int cmd_query_respond_hold_status(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_hfp_hf_query_respond_hold_status(hfp_hf);
	if (err) {
		shell_error(sh, "Failed to query respond and hold status: %d", err);
	}

	return err;
}

static int cmd_number_call(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_hfp_hf_number_call(hfp_hf, argv[1]);
	if (err) {
		shell_error(sh, "Failed to start phone number call: %d", err);
	}

	return err;
}

static int cmd_memory_dial(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_hfp_hf_memory_dial(hfp_hf, argv[1]);
	if (err) {
		shell_error(sh, "Failed to memory dial call: %d", err);
	}

	return err;
}

static int cmd_redial(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_hfp_hf_redial(hfp_hf);
	if (err) {
		shell_error(sh, "Failed to redial call: %d", err);
	}

	return err;
}

#if defined(CONFIG_BT_HFP_HF_ECNR)
static int cmd_turn_off_ecnr(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_hfp_hf_turn_off_ecnr(hfp_hf);
	if (err) {
		shell_error(sh, "Failed to turn off ecnr: %d", err);
	}

	return err;
}
#endif /* CONFIG_BT_HFP_HF_ECNR */

#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
static int cmd_call_waiting_notify(const struct shell *sh, size_t argc, char **argv)
{
	const char *action = argv[1];
	bool enable;
	int err;

	action = argv[1];

	if (strcmp(action, "enable") == 0) {
		enable = true;
	} else if (strcmp(action, "disable") == 0) {
		enable = false;
	} else {
		shell_error(sh, "Invalid option.");
		return -ENOEXEC;
	}

	err = bt_hfp_hf_call_waiting_notify(hfp_hf, enable);
	if (err) {
		shell_error(sh, "Failed to set call waiting notify: %d", err);
	}

	return err;
}

static int cmd_release_all_held(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_hfp_hf_release_all_held(hfp_hf);
	if (err) {
		shell_error(sh, "Failed to release all held: %d", err);
	}

	return err;
}

static int cmd_set_udub(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_hfp_hf_set_udub(hfp_hf);
	if (err) {
		shell_error(sh, "Failed to reject waiting call: %d", err);
	}

	return err;
}

static int cmd_release_active_accept_other(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_hfp_hf_release_active_accept_other(hfp_hf);
	if (err) {
		shell_error(sh, "Failed to release active calls and accept other call: %d", err);
	}

	return err;
}

static int cmd_hold_active_accept_other(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_hfp_hf_hold_active_accept_other(hfp_hf);
	if (err) {
		shell_error(sh, "Failed to hold all active calls and accept other call: %d", err);
	}

	return err;
}

static int cmd_join_conversation(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_hfp_hf_join_conversation(hfp_hf);
	if (err) {
		shell_error(sh, "Failed to join the conversation: %d", err);
	}

	return err;
}

static int cmd_explicit_call_transfer(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_hfp_hf_explicit_call_transfer(hfp_hf);
	if (err) {
		shell_error(sh, "Failed to explicit call transfer: %d", err);
	}

	return err;
}

static int cmd_release_specified_call(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	int index;

	index = atoi(argv[1]);
	if ((index >= ((int)ARRAY_SIZE(hfp_hf_call))) || !hfp_hf_call[index]) {
		shell_error(sh, "Invalid call index: %d", index);
		return -EINVAL;
	}

	err = bt_hfp_hf_release_specified_call(hfp_hf_call[index]);
	if (err) {
		shell_error(sh, "Failed to release specified call: %d", err);
	}

	return err;
}

static int cmd_private_consultation_mode(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	int index;

	index = atoi(argv[1]);
	if ((index >= ((int)ARRAY_SIZE(hfp_hf_call))) || !hfp_hf_call[index]) {
		shell_error(sh, "Invalid call index: %d", index);
		return -EINVAL;
	}

	err = bt_hfp_hf_private_consultation_mode(hfp_hf_call[index]);
	if (err) {
		shell_error(sh, "Failed to set private consultation mode: %d", err);
	}

	return err;
}
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */

#if defined(CONFIG_BT_HFP_HF_VOICE_RECG)
static int cmd_voice_recognition(const struct shell *sh, size_t argc, char **argv)
{
	const char *action;
	bool activate;
	int err;

	action = argv[1];

	if (strcmp(action, "activate") == 0) {
		activate = true;
	} else if (strcmp(action, "deactivate") == 0) {
		activate = false;
	} else {
		shell_error(sh, "Invalid option.");
		return -ENOEXEC;
	}

	err = bt_hfp_hf_voice_recognition(hfp_hf, activate);
	if (err) {
		shell_error(sh, "Failed to set voice recognition: %d", err);
	}

	return err;
}

#if defined(CONFIG_BT_HFP_HF_ENH_VOICE_RECG)
static int cmd_ready_to_accept_audio(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_hfp_hf_ready_to_accept_audio(hfp_hf);
	if (err) {
		shell_error(sh, "Failed to send ready to accept audio notify: %d", err);
	}

	return err;
}
#endif /* CONFIG_BT_HFP_HF_ENH_VOICE_RECG */
#endif /* CONFIG_BT_HFP_HF_VOICE_RECG */

static int cmd_request_phone_number(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_hfp_hf_request_phone_number(hfp_hf);
	if (err) {
		shell_error(sh, "Failed to request phone number: %d", err);
	}

	return err;
}

static int cmd_transmit_dtmf_code(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	int index;

	index = atoi(argv[1]);
	if ((index >= ((int)ARRAY_SIZE(hfp_hf_call))) || !hfp_hf_call[index]) {
		shell_error(sh, "Invalid call index: %d", index);
		return -EINVAL;
	}

	err = bt_hfp_hf_transmit_dtmf_code(hfp_hf_call[index], argv[2][0]);
	if (err) {
		shell_error(sh, "Failed to transmit DTMF Code: %d", err);
	}

	return err;
}

static int cmd_query_subscriber(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_hfp_hf_query_subscriber(hfp_hf);
	if (err) {
		shell_error(sh, "Failed to query subscriber: %d", err);
	}

	return err;
}

static int cmd_indicator_status(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	uint8_t status[4];
	size_t len;

	len = hex2bin(argv[1], strlen(argv[1]), status, sizeof(status));
	if (len == 0) {
		shell_error(sh, "Failed to parse status %s", argv[1]);
		return -EINVAL;
	}

	err = bt_hfp_hf_indicator_status(hfp_hf, (uint32_t)status[0]);
	if (err) {
		shell_error(sh, "Failed to set AG indicator activated/deactivated status: %d", err);
	}

	return err;
}

#if defined(CONFIG_BT_HFP_HF_HF_INDICATOR_ENH_SAFETY)
static int cmd_enhanced_safety(const struct shell *sh, size_t argc, char **argv)
{
	const char *action;
	bool enable;
	int err;

	action = argv[1];

	if (strcmp(action, "enable") == 0) {
		enable = true;
	} else if (strcmp(action, "disable") == 0) {
		enable = false;
	} else {
		shell_error(sh, "Invalid option.");
		return -ENOEXEC;
	}

	err = bt_hfp_hf_enhanced_safety(hfp_hf, enable);
	if (err) {
		shell_error(sh, "Failed to transfer enhanced safety status: %d", err);
	}

	return err;
}
#endif /* CONFIG_BT_HFP_HF_HF_INDICATOR_ENH_SAFETY */

#if defined(CONFIG_BT_HFP_HF_HF_INDICATOR_BATTERY)
static int cmd_battery(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	int level;

	level = atoi(argv[1]);

	err = bt_hfp_hf_battery(hfp_hf, level);
	if (err) {
		shell_error(sh, "Failed to transfer battery level: %d", err);
	}

	return err;
}
#endif /* CONFIG_BT_HFP_HF_HF_INDICATOR_BATTERY */

SHELL_STATIC_SUBCMD_SET_CREATE(hf_cmds,
	SHELL_CMD_ARG(reg, NULL, HELP_NONE, cmd_reg_enable, 1, 0),
	SHELL_CMD_ARG(connect, NULL, "<channel>", cmd_connect, 2, 0),
	SHELL_CMD_ARG(disconnect, NULL, HELP_NONE, cmd_disconnect, 1, 0),
	SHELL_CMD_ARG(sco_disconnect, NULL, HELP_NONE, cmd_sco_disconnect, 1, 0),
#if defined(CONFIG_BT_HFP_HF_CLI)
	SHELL_CMD_ARG(cli, NULL, "<enable/disable>", cmd_cli_enable, 2, 0),
#endif /* CONFIG_BT_HFP_HF_CLI */
#if defined(CONFIG_BT_HFP_HF_VOLUME)
	SHELL_CMD_ARG(vgm, NULL, "<gain>", cmd_vgm_enable, 2, 0),
	SHELL_CMD_ARG(vgs, NULL, "<gain>", cmd_vgs_enable, 2, 0),
#endif /* CONFIG_BT_HFP_HF_VOLUME */
	SHELL_CMD_ARG(operator, NULL, HELP_NONE, cmd_operator, 1, 0),
#if defined(CONFIG_BT_HFP_HF_CODEC_NEG)
	SHELL_CMD_ARG(audio_connect, NULL, HELP_NONE, cmd_audio_connect, 1, 0),
	SHELL_CMD_ARG(select_codec, NULL, "Codec ID", cmd_select_codec, 2, 0),
	SHELL_CMD_ARG(set_codecs, NULL, "Codec ID Map", cmd_set_codecs, 2, 0),
#endif /* CONFIG_BT_HFP_HF_CODEC_NEG */
	SHELL_CMD_ARG(accept, NULL, "<call index>", cmd_accept, 2, 0),
	SHELL_CMD_ARG(reject, NULL, "<call index>", cmd_reject, 2, 0),
	SHELL_CMD_ARG(terminate, NULL, "<call index>", cmd_terminate, 2, 0),
	SHELL_CMD_ARG(hold_incoming, NULL, "<call index>", cmd_hold_incoming, 2, 0),
	SHELL_CMD_ARG(query_respond_hold_status, NULL, HELP_NONE, cmd_query_respond_hold_status, 1,
		      0),
	SHELL_CMD_ARG(number_call, NULL, "<phone number>", cmd_number_call, 2, 0),
	SHELL_CMD_ARG(memory_dial, NULL, "<memory location>", cmd_memory_dial, 2, 0),
	SHELL_CMD_ARG(redial, NULL, HELP_NONE, cmd_redial, 1, 0),
#if defined(CONFIG_BT_HFP_HF_ECNR)
	SHELL_CMD_ARG(turn_off_ecnr, NULL, HELP_NONE, cmd_turn_off_ecnr, 1, 0),
#endif /* CONFIG_BT_HFP_HF_ECNR */
#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
	SHELL_CMD_ARG(call_waiting_notify, NULL, "<enable/disable>", cmd_call_waiting_notify, 2, 0),
	SHELL_CMD_ARG(release_all_held, NULL, HELP_NONE, cmd_release_all_held, 1, 0),
	SHELL_CMD_ARG(set_udub, NULL, HELP_NONE, cmd_set_udub, 1, 0),
	SHELL_CMD_ARG(release_active_accept_other, NULL, HELP_NONE, cmd_release_active_accept_other,
		      1, 0),
	SHELL_CMD_ARG(hold_active_accept_other, NULL, HELP_NONE, cmd_hold_active_accept_other, 1,
		      0),
	SHELL_CMD_ARG(join_conversation, NULL, HELP_NONE, cmd_join_conversation, 1, 0),
	SHELL_CMD_ARG(explicit_call_transfer, NULL, HELP_NONE, cmd_explicit_call_transfer, 1, 0),
	SHELL_CMD_ARG(release_specified_call, NULL, "<call index>", cmd_release_specified_call, 2,
		      0),
	SHELL_CMD_ARG(private_consultation_mode, NULL, "<call index>",
		      cmd_private_consultation_mode, 2, 0),
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */
#if defined(CONFIG_BT_HFP_HF_VOICE_RECG)
	SHELL_CMD_ARG(voice_recognition, NULL, "<activate/deactivate>", cmd_voice_recognition, 2,
		      0),
#if defined(CONFIG_BT_HFP_HF_ENH_VOICE_RECG)
	SHELL_CMD_ARG(ready_to_accept_audio, NULL, HELP_NONE, cmd_ready_to_accept_audio, 1, 0),
#endif /* CONFIG_BT_HFP_HF_ENH_VOICE_RECG */
#endif /* CONFIG_BT_HFP_HF_VOICE_RECG */
	SHELL_CMD_ARG(request_phone_number, NULL, HELP_NONE, cmd_request_phone_number, 1, 0),
	SHELL_CMD_ARG(transmit_dtmf_code, NULL, "<call index> <code(set 0-9, #,*,A-D)>",
		      cmd_transmit_dtmf_code, 3, 0),
	SHELL_CMD_ARG(query_subscriber, NULL, HELP_NONE, cmd_query_subscriber, 1, 0),
	SHELL_CMD_ARG(indicator_status, NULL, "<Activate/deactivate AG indicators bitmap>",
		      cmd_indicator_status, 2, 0),
#if defined(CONFIG_BT_HFP_HF_HF_INDICATOR_ENH_SAFETY)
	SHELL_CMD_ARG(enhanced_safety, NULL, "<enable/disable>", cmd_enhanced_safety, 2, 0),
#endif /* CONFIG_BT_HFP_HF_HF_INDICATOR_ENH_SAFETY */
#if defined(CONFIG_BT_HFP_HF_HF_INDICATOR_BATTERY)
	SHELL_CMD_ARG(battery, NULL, "<level>", cmd_battery, 2, 0),
#endif /* CONFIG_BT_HFP_HF_HF_INDICATOR_BATTERY */
	SHELL_SUBCMD_SET_END
);
#endif /* CONFIG_BT_HFP_HF */

#if defined(CONFIG_BT_HFP_AG)

struct bt_hfp_ag *hfp_ag;
struct bt_conn *hfp_ag_sco_conn;
static struct bt_hfp_ag_call *hfp_ag_call[CONFIG_BT_HFP_AG_MAX_CALLS];

static void ag_add_a_call(struct bt_hfp_ag_call *call)
{
	for (size_t index = 0; index < ARRAY_SIZE(hfp_ag_call); index++) {
		if (!hfp_ag_call[index]) {
			hfp_ag_call[index] = call;
			return;
		}
	}
}

static void ag_remove_a_call(struct bt_hfp_ag_call *call)
{
	for (size_t index = 0; index < ARRAY_SIZE(hfp_ag_call); index++) {
		if (call == hfp_ag_call[index]) {
			hfp_ag_call[index] = NULL;
			return;
		}
	}
}

static void ag_connected(struct bt_conn *conn, struct bt_hfp_ag *ag)
{
	if (conn != default_conn) {
		bt_shell_warn("The conn %p is not aligned with ACL conn %p", conn, default_conn);
	}
	hfp_ag = ag;
	bt_shell_print("AG connected");
}

static void ag_disconnected(struct bt_hfp_ag *ag)
{
	bt_shell_print("AG disconnected");
}

static void ag_sco_connected(struct bt_hfp_ag *ag, struct bt_conn *sco_conn)
{
	bt_shell_print("AG SCO connected %p", sco_conn);

	if (hfp_ag_sco_conn != NULL) {
		bt_shell_warn("AG SCO conn %p exists", hfp_ag_sco_conn);
		return;
	}

	hfp_ag_sco_conn = bt_conn_ref(sco_conn);
}

static void ag_sco_disconnected(struct bt_conn *sco_conn, uint8_t reason)
{
	bt_shell_print("AG SCO disconnected %p (reason %u)", sco_conn, reason);

	if (hfp_ag_sco_conn == sco_conn) {
		bt_conn_unref(hfp_ag_sco_conn);
		hfp_ag_sco_conn = NULL;
	} else {
		bt_shell_warn("Unknown SCO disconnected (%p != %p)", hfp_ag_sco_conn, sco_conn);
	}
}

static int ag_memory_dial(struct bt_hfp_ag *ag, const char *location, char **number)
{
	static char *phone = "123456789";

	if (strcmp(location, "0")) {
		return -ENOTSUP;
	}

	bt_shell_print("AG memory dial");

	*number = phone;

	return 0;
}

static int ag_number_call(struct bt_hfp_ag *ag, const char *number)
{
	static char *phone = "123456789";

	bt_shell_print("AG number call");

	if (strcmp(number, phone)) {
		return -ENOTSUP;
	}

	return 0;
}

static void ag_outgoing(struct bt_hfp_ag *ag, struct bt_hfp_ag_call *call, const char *number)
{
	bt_shell_print("AG outgoing call %p, number %s", call, number);
	ag_add_a_call(call);
}

static void ag_incoming(struct bt_hfp_ag *ag, struct bt_hfp_ag_call *call, const char *number)
{
	bt_shell_print("AG incoming call %p, number %s", call, number);
	ag_add_a_call(call);
}

static void ag_incoming_held(struct bt_hfp_ag_call *call)
{
	bt_shell_print("AG incoming call %p is held", call);
}

static void ag_ringing(struct bt_hfp_ag_call *call, bool in_band)
{
	bt_shell_print("AG call %p start ringing mode %d", call, in_band);
}

static void ag_accept(struct bt_hfp_ag_call *call)
{
	bt_shell_print("AG call %p accept", call);
}

static void ag_held(struct bt_hfp_ag_call *call)
{
	bt_shell_print("AG call %p held", call);
}

static void ag_retrieve(struct bt_hfp_ag_call *call)
{
	bt_shell_print("AG call %p retrieved", call);
}

static void ag_reject(struct bt_hfp_ag_call *call)
{
	bt_shell_print("AG call %p reject", call);
	ag_remove_a_call(call);
}

static void ag_terminate(struct bt_hfp_ag_call *call)
{
	bt_shell_print("AG call %p terminate", call);
	ag_remove_a_call(call);
}

static void ag_codec(struct bt_hfp_ag *ag, uint32_t ids)
{
	bt_shell_print("AG received codec id bit map %x", ids);
}

void ag_vgm(struct bt_hfp_ag *ag, uint8_t gain)
{
	bt_shell_print("AG received vgm %d", gain);
}

void ag_vgs(struct bt_hfp_ag *ag, uint8_t gain)
{
	bt_shell_print("AG received vgs %d", gain);
}

void ag_codec_negotiate(struct bt_hfp_ag *ag, int err)
{
	bt_shell_print("AG codec negotiation result %d", err);
}

void ag_audio_connect_req(struct bt_hfp_ag *ag)
{
	bt_shell_print("Receive audio connect request. "
		       "Input `hfp ag audio_connect` to start audio connect");
}

void ag_ecnr_turn_off(struct bt_hfp_ag *ag)
{
	bt_shell_print("encr is disabled");
}

#if defined(CONFIG_BT_HFP_AG_3WAY_CALL)
void ag_explicit_call_transfer(struct bt_hfp_ag *ag)
{
	bt_shell_print("explicit call transfer");
}
#endif /* CONFIG_BT_HFP_AG_3WAY_CALL */

#if defined(CONFIG_BT_HFP_AG_VOICE_RECG)
void ag_voice_recognition(struct bt_hfp_ag *ag, bool activate)
{
	bt_shell_print("AG Voice recognition %s", activate ? "activate" : "deactivate");
}

#if defined(CONFIG_BT_HFP_AG_ENH_VOICE_RECG)
void ag_ready_to_accept_audio(struct bt_hfp_ag *ag)
{
	bt_shell_print("hf is ready to accept audio");
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
	bt_shell_print("DTMF code is %c", code);
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
	bt_shell_print("indicator %d value %d", indicator, value);
}

static struct bt_hfp_ag_cb ag_cb = {
	.connected = ag_connected,
	.disconnected = ag_disconnected,
	.sco_connected = ag_sco_connected,
	.sco_disconnected = ag_sco_disconnected,
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
#if defined(CONFIG_BT_HFP_AG_ECNR)
	.ecnr_turn_off = ag_ecnr_turn_off,
#endif /* CONFIG_BT_HFP_AG_ECNR */
#if defined(CONFIG_BT_HFP_AG_3WAY_CALL)
	.explicit_call_transfer = ag_explicit_call_transfer,
#endif /* CONFIG_BT_HFP_AG_3WAY_CALL */
#if defined(CONFIG_BT_HFP_AG_VOICE_RECG)
	.voice_recognition = ag_voice_recognition,
#if defined(CONFIG_BT_HFP_AG_ENH_VOICE_RECG)
	.ready_to_accept_audio = ag_ready_to_accept_audio,
#endif /* CONFIG_BT_HFP_AG_ENH_VOICE_RECG */
#endif /* CONFIG_BT_HFP_AG_VOICE_RECG */
#if defined(CONFIG_BT_HFP_AG_VOICE_TAG)
	.request_phone_number = ag_request_phone_number,
#endif /* CONFIG_BT_HFP_AG_VOICE_TAG */
	.transmit_dtmf_code = ag_transmit_dtmf_code,
	.subscriber_number = ag_subscriber_number,
	.hf_indicator_value = ag_hf_indicator_value,
};

static int cmd_ag_reg_enable(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_hfp_ag_register(&ag_cb);
	if (err) {
		shell_error(sh, "Callback register failed: %d", err);
	}

	return err;
}

static int cmd_ag_connect(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	struct bt_hfp_ag *ag;
	uint8_t channel;

	channel = atoi(argv[1]);

	err = bt_hfp_ag_connect(default_conn, &ag, channel);
	if (err) {
		shell_error(sh, "Connect failed: %d", err);
	}

	return err;
}

static int cmd_ag_disconnect(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_hfp_ag_disconnect(hfp_ag);
	if (err) {
		shell_error(sh, "Disconnect failed: %d", err);
	}

	return err;
}

static int cmd_ag_sco_disconnect(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_conn_disconnect(hfp_ag_sco_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		shell_error(sh, "Disconnect failed: %d", err);
	}

	return err;
}

static int cmd_ag_remote_incoming(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_hfp_ag_remote_incoming(hfp_ag, argv[1]);
	if (err) {
		shell_error(sh, "Set remote incoming failed: %d", err);
	}

	return err;
}

static int cmd_ag_hold_incoming(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	int index;

	index = atoi(argv[1]);
	if ((index >= ((int)ARRAY_SIZE(hfp_ag_call))) || !hfp_ag_call[index]) {
		shell_error(sh, "Invalid call index: %d", index);
		return -EINVAL;
	}

	err = bt_hfp_ag_hold_incoming(hfp_ag_call[index]);
	if (err) {
		shell_error(sh, "Set remote incoming failed: %d", err);
	}

	return err;
}

static int cmd_ag_remote_reject(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	int index;

	index = atoi(argv[1]);
	if ((index >= ((int)ARRAY_SIZE(hfp_ag_call))) || !hfp_ag_call[index]) {
		shell_error(sh, "Invalid call index: %d", index);
		return -EINVAL;
	}

	err = bt_hfp_ag_remote_reject(hfp_ag_call[index]);
	if (err) {
		shell_error(sh, "Set remote reject failed: %d", err);
	}

	return err;
}

static int cmd_ag_remote_accept(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	int index;

	index = atoi(argv[1]);
	if ((index >= ((int)ARRAY_SIZE(hfp_ag_call))) || !hfp_ag_call[index]) {
		shell_error(sh, "Invalid call index: %d", index);
		return -EINVAL;
	}

	err = bt_hfp_ag_remote_accept(hfp_ag_call[index]);
	if (err) {
		shell_error(sh, "Set remote accept failed: %d", err);
	}

	return err;
}

static int cmd_ag_remote_terminate(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	int index;

	index = atoi(argv[1]);
	if ((index >= ((int)ARRAY_SIZE(hfp_ag_call))) || !hfp_ag_call[index]) {
		shell_error(sh, "Invalid call index: %d", index);
		return -EINVAL;
	}

	err = bt_hfp_ag_remote_terminate(hfp_ag_call[index]);
	if (err) {
		shell_error(sh, "Set remote terminate failed: %d", err);
	}

	return err;
}

static int cmd_ag_remote_ringing(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	int index;

	index = atoi(argv[1]);
	if ((index >= ((int)ARRAY_SIZE(hfp_ag_call))) || !hfp_ag_call[index]) {
		shell_error(sh, "Invalid call index: %d", index);
		return -EINVAL;
	}

	err = bt_hfp_ag_remote_ringing(hfp_ag_call[index]);
	if (err) {
		shell_error(sh, "Set remote ringing failed: %d", err);
	}

	return err;
}

static int cmd_ag_outgoing(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_hfp_ag_outgoing(hfp_ag, argv[1]);
	if (err) {
		shell_error(sh, "Set outgoing failed: %d", err);
	}

	return err;
}

static int cmd_ag_reject(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	int index;

	index = atoi(argv[1]);
	if ((index >= ((int)ARRAY_SIZE(hfp_ag_call))) || !hfp_ag_call[index]) {
		shell_error(sh, "Invalid call index: %d", index);
		return -EINVAL;
	}

	err = bt_hfp_ag_reject(hfp_ag_call[index]);
	if (err) {
		shell_error(sh, "Set reject failed: %d", err);
	}

	return err;
}

static int cmd_ag_accept(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	int index;

	index = atoi(argv[1]);
	if ((index >= ((int)ARRAY_SIZE(hfp_ag_call))) || !hfp_ag_call[index]) {
		shell_error(sh, "Invalid call index: %d", index);
		return -EINVAL;
	}

	err = bt_hfp_ag_accept(hfp_ag_call[index]);
	if (err) {
		shell_error(sh, "Set accept failed: %d", err);
	}

	return err;
}

static int cmd_ag_hold(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	int index;

	index = atoi(argv[1]);
	if ((index >= ((int)ARRAY_SIZE(hfp_ag_call))) || !hfp_ag_call[index]) {
		shell_error(sh, "Invalid call index: %d", index);
		return -EINVAL;
	}

	err = bt_hfp_ag_hold(hfp_ag_call[index]);
	if (err) {
		shell_error(sh, "Set hold failed: %d", err);
	}

	return err;
}

static int cmd_ag_retrieve(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	int index;

	index = atoi(argv[1]);
	if ((index >= ((int)ARRAY_SIZE(hfp_ag_call))) || !hfp_ag_call[index]) {
		shell_error(sh, "Invalid call index: %d", index);
		return -EINVAL;
	}

	err = bt_hfp_ag_retrieve(hfp_ag_call[index]);
	if (err) {
		shell_error(sh, "Set retrieve failed: %d", err);
	}

	return err;
}

static int cmd_ag_terminate(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	int index;

	index = atoi(argv[1]);
	if ((index >= ((int)ARRAY_SIZE(hfp_ag_call))) || !hfp_ag_call[index]) {
		shell_error(sh, "Invalid call index: %d", index);
		return -EINVAL;
	}

	err = bt_hfp_ag_terminate(hfp_ag_call[index]);
	if (err) {
		shell_error(sh, "Set terminate failed: %d", err);
	}

	return err;
}

static int cmd_ag_vgm(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	uint8_t vgm;

	vgm = atoi(argv[1]);

	err = bt_hfp_ag_vgm(hfp_ag, vgm);
	if (err) {
		shell_error(sh, "Set microphone gain failed: %d", err);
	}

	return err;
}

static int cmd_ag_vgs(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	uint8_t vgs;

	vgs = atoi(argv[1]);

	err = bt_hfp_ag_vgs(hfp_ag, vgs);
	if (err) {
		shell_error(sh, "Set speaker gain failed: %d", err);
	}

	return err;
}

static int cmd_ag_operator(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	uint8_t mode;

	mode = atoi(argv[1]);

	err = bt_hfp_ag_set_operator(hfp_ag, mode, argv[2]);
	if (err) {
		shell_error(sh, "Set network operator failed: %d", err);
	}

	return err;
}

#if defined(CONFIG_BT_HFP_AG_CODEC_NEG)
static int cmd_ag_audio_connect(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	uint8_t id;

	id = atoi(argv[1]);

	err = bt_hfp_ag_audio_connect(hfp_ag, id);
	if (err) {
		shell_error(sh, "Start audio connection procedure failed: %d", err);
	}

	return err;
}
#endif /* CONFIG_BT_HFP_AG_CODEC_NEG */

static int cmd_ag_inband_ringtone(const struct shell *sh, size_t argc, char **argv)
{
	const char *action;
	bool enable;
	int err;

	action = argv[1];

	if (strcmp(action, "enable") == 0) {
		enable = true;
	} else if (strcmp(action, "disable") == 0) {
		enable = false;
	} else {
		shell_error(sh, "Invalid option.");
		return -ENOEXEC;
	}

	err = bt_hfp_ag_inband_ringtone(hfp_ag, (bool)enable);
	if (err) {
		shell_error(sh, "Set inband ringtone failed: %d", err);
	}

	return err;
}

#if defined(CONFIG_BT_HFP_AG_3WAY_CALL)
static int cmd_ag_explicit_call_transfer(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_hfp_ag_explicit_call_transfer(hfp_ag);
	if (err) {
		shell_error(sh, "Explicit call transfer failed: %d", err);
	}

	return err;
}
#endif /* CONFIG_BT_HFP_AG_3WAY_CALL */

#if defined(CONFIG_BT_HFP_AG_VOICE_RECG)
static int cmd_ag_voice_recognition(const struct shell *sh, size_t argc, char **argv)
{
	const char *action;
	bool enable;
	int err;

	action = argv[1];

	if (strcmp(action, "activate") == 0) {
		enable = true;
	} else if (strcmp(action, "deactivate") == 0) {
		enable = false;
	} else {
		shell_error(sh, "Invalid option.");
		return -ENOEXEC;
	}

	err = bt_hfp_ag_voice_recognition(hfp_ag, enable);
	if (err) {
		shell_error(sh, "Set voice recognition failed: %d", err);
	}

	return err;
}

#if defined(CONFIG_BT_HFP_AG_ENH_VOICE_RECG)
static int cmd_ag_vre_state(const struct shell *sh, size_t argc, char **argv)
{
	const char *action;
	uint8_t state = 0;
	int err;

	action = argv[1];

	for (size_t index = 0; index < strlen(action); index++) {
		switch (action[index]) {
		case 'R':
			state |= BIT(0);
			break;
		case 'S':
			state |= BIT(1);
			break;
		case 'P':
			state |= BIT(2);
			break;
		}
	}

	err = bt_hfp_ag_vre_state(hfp_ag, state);
	if (err) {
		shell_error(sh, "Set voice recognition engine state failed: %d", err);
	}

	return err;
}
#endif /* CONFIG_BT_HFP_AG_ENH_VOICE_RECG */
#if defined(CONFIG_BT_HFP_AG_VOICE_RECG_TEXT)
static int cmd_ag_vre_text(const struct shell *sh, size_t argc, char **argv)
{
	const char *action;
	uint8_t state = 0;
	const char *id;
	uint8_t type;
	uint8_t operation;
	const char *text;
	int err;

	action = argv[1];
	id = argv[2];
	type = (uint8_t)atoi(argv[3]);
	operation = (uint8_t)atoi(argv[4]);
	text = argv[5];

	for (size_t index = 0; index < strlen(action); index++) {
		switch (action[index]) {
		case 'R':
			state |= BIT(0);
			break;
		case 'S':
			state |= BIT(1);
			break;
		case 'P':
			state |= BIT(2);
			break;
		}
	}

	err = bt_hfp_ag_vre_textual_representation(hfp_ag, state, id, type, operation, text);
	if (err) {
		shell_error(sh, "Set voice recognition engine textual representation failed: %d",
			    err);
	}

	return err;
}
#endif /* CONFIG_BT_HFP_AG_VOICE_RECG_TEXT */
#endif /* CONFIG_BT_HFP_AG_VOICE_RECG */

static int cmd_ag_subscriber(const struct shell *sh, size_t argc, char **argv)
{
	const char *action;

	action = argv[1];

	if (strcmp(action, "empty") == 0) {
		subscriber = false;
	} else if (strcmp(action, "notempty") == 0) {
		subscriber = true;
	} else {
		shell_error(sh, "Invalid option.");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_ag_signal_strength(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t strength;
	int err;

	strength = (uint8_t)atoi(argv[1]);

	err = bt_hfp_ag_signal_strength(hfp_ag, strength);
	if (err) {
		shell_error(sh, "Set signal strength failed: %d", err);
	}

	return err;
}

static int cmd_ag_roaming_status(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t status;
	int err;

	status = (uint8_t)atoi(argv[1]);

	err = bt_hfp_ag_roaming_status(hfp_ag, status);
	if (err) {
		shell_error(sh, "Set roaming status failed: %d", err);
	}

	return err;
}

static int cmd_ag_battery_level(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t level;
	int err;

	level = (uint8_t)atoi(argv[1]);

	err = bt_hfp_ag_battery_level(hfp_ag, level);
	if (err) {
		shell_error(sh, "Set battery level failed: %d", err);
	}

	return err;
}

static int cmd_ag_service_availability(const struct shell *sh, size_t argc, char **argv)
{
	bool available;
	const char *action;
	int err;

	action = argv[1];

	if (strcmp(action, "yes") == 0) {
		available = true;
	} else if (strcmp(action, "no") == 0) {
		available = false;
	} else {
		shell_error(sh, "Invalid option.");
		return -ENOEXEC;
	}

	err = bt_hfp_ag_service_availability(hfp_ag, available);
	if (err) {
		shell_error(sh, "Set service availability failed: %d", err);
	}

	return err;
}

#if defined(CONFIG_BT_HFP_AG_HF_INDICATORS)
static int cmd_ag_hf_indicator(const struct shell *sh, size_t argc, char **argv)
{
	bool enable;
	const char *action;
	int err;
	size_t indicator;

	indicator = atoi(argv[1]);
	action = argv[2];

	if (strcmp(action, "enable") == 0) {
		enable = true;
	} else if (strcmp(action, "disable") == 0) {
		enable = false;
	} else {
		shell_error(sh, "Invalid option.");
		return -ENOEXEC;
	}

	err = bt_hfp_ag_hf_indicator(hfp_ag, indicator, enable);
	if (err) {
		shell_error(sh, "Activate/deactivate HF indicator failed: %d", err);
	}

	return err;
}
#endif /* CONFIG_BT_HFP_HF_HF_INDICATORS */

#define HELP_AG_TEXTUAL_REPRESENTATION          \
	"<[R-ready][S-send][P-processing]> "        \
	"<id> <type> <operation> <text string>"

SHELL_STATIC_SUBCMD_SET_CREATE(ag_cmds,
	SHELL_CMD_ARG(reg, NULL, HELP_NONE, cmd_ag_reg_enable, 1, 0),
	SHELL_CMD_ARG(connect, NULL, "<channel>", cmd_ag_connect, 2, 0),
	SHELL_CMD_ARG(disconnect, NULL, HELP_NONE, cmd_ag_disconnect, 1, 0),
	SHELL_CMD_ARG(sco_disconnect, NULL, HELP_NONE, cmd_ag_sco_disconnect, 1, 0),
	SHELL_CMD_ARG(remote_incoming, NULL, "<number>", cmd_ag_remote_incoming, 2, 0),
	SHELL_CMD_ARG(hold_incoming, NULL, "<number>", cmd_ag_hold_incoming, 2, 0),
	SHELL_CMD_ARG(remote_reject, NULL, "<call index>", cmd_ag_remote_reject, 2, 0),
	SHELL_CMD_ARG(remote_accept, NULL, "<call index>", cmd_ag_remote_accept, 2, 0),
	SHELL_CMD_ARG(remote_terminate, NULL, "<call index>", cmd_ag_remote_terminate, 2, 0),
	SHELL_CMD_ARG(remote_ringing, NULL, "<call index>", cmd_ag_remote_ringing, 2, 0),
	SHELL_CMD_ARG(outgoing, NULL, "<number>", cmd_ag_outgoing, 2, 0),
	SHELL_CMD_ARG(reject, NULL, "<call index>", cmd_ag_reject, 2, 0),
	SHELL_CMD_ARG(accept, NULL, "<call index>", cmd_ag_accept, 2, 0),
	SHELL_CMD_ARG(hold, NULL, "<call index>", cmd_ag_hold, 2, 0),
	SHELL_CMD_ARG(retrieve, NULL, "<call index>", cmd_ag_retrieve, 2, 0),
	SHELL_CMD_ARG(terminate, NULL, "<call index>", cmd_ag_terminate, 2, 0),
	SHELL_CMD_ARG(vgm, NULL, "<gain>", cmd_ag_vgm, 2, 0),
	SHELL_CMD_ARG(vgs, NULL, "<gain>", cmd_ag_vgs, 2, 0),
	SHELL_CMD_ARG(operator, NULL, "<mode> <operator>", cmd_ag_operator, 3, 0),
#if defined(CONFIG_BT_HFP_AG_CODEC_NEG)
	SHELL_CMD_ARG(audio_connect, NULL, "<codec id>", cmd_ag_audio_connect, 2, 0),
#endif /* CONFIG_BT_HFP_AG_CODEC_NEG */
	SHELL_CMD_ARG(inband_ringtone, NULL, "<enable/disable>", cmd_ag_inband_ringtone, 2, 0),
#if defined(CONFIG_BT_HFP_AG_3WAY_CALL)
	SHELL_CMD_ARG(explicit_call_transfer, NULL, HELP_NONE, cmd_ag_explicit_call_transfer, 1, 0),
#endif /* CONFIG_BT_HFP_AG_3WAY_CALL */
#if defined(CONFIG_BT_HFP_AG_VOICE_RECG)
	SHELL_CMD_ARG(voice_recognition, NULL, "<activate/deactivate>", cmd_ag_voice_recognition, 2,
		      0),
#if defined(CONFIG_BT_HFP_AG_ENH_VOICE_RECG)
	SHELL_CMD_ARG(vre_state, NULL, "<[R-ready][S-send][P-processing]>", cmd_ag_vre_state, 2, 0),
#endif /* CONFIG_BT_HFP_AG_ENH_VOICE_RECG */
#if defined(CONFIG_BT_HFP_AG_VOICE_RECG_TEXT)
	SHELL_CMD_ARG(vre_text, NULL, HELP_AG_TEXTUAL_REPRESENTATION, cmd_ag_vre_text, 6, 0),
#endif /* CONFIG_BT_HFP_AG_VOICE_RECG_TEXT */
#endif /* CONFIG_BT_HFP_AG_VOICE_RECG */
	SHELL_CMD_ARG(subscriber, NULL, "<empty/notempty>", cmd_ag_subscriber, 2, 0),
	SHELL_CMD_ARG(signal_strength, NULL, "<signal strength>", cmd_ag_signal_strength, 2, 0),
	SHELL_CMD_ARG(roaming_status, NULL, "<roaming status>", cmd_ag_roaming_status, 2, 0),
	SHELL_CMD_ARG(battery_level, NULL, "<battery level>", cmd_ag_battery_level, 2, 0),
	SHELL_CMD_ARG(service_availability, NULL, "<yes/no>", cmd_ag_service_availability, 2, 0),
#if defined(CONFIG_BT_HFP_AG_HF_INDICATORS)
	SHELL_CMD_ARG(hf_indicator, NULL, "<indicator> <enable/disable>", cmd_ag_hf_indicator, 3,
		      0),
#endif /* CONFIG_BT_HFP_HF_HF_INDICATORS */
	SHELL_SUBCMD_SET_END
);
#endif /* CONFIG_BT_HFP_AG */

static int cmd_default(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		/* sh returns 1 when help is printed */
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(hfp_cmds,
#if defined(CONFIG_BT_HFP_HF)
	SHELL_CMD(hf, &hf_cmds, "HFP HF shell commands", cmd_default),
#endif /* CONFIG_BT_HFP_HF */
#if defined(CONFIG_BT_HFP_AG)
	SHELL_CMD(ag, &ag_cmds, "HFP AG shell commands", cmd_default),
#endif /* CONFIG_BT_HFP_AG */
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(hfp, &hfp_cmds, "Bluetooth HFP shell commands", cmd_default, 1, 1);
