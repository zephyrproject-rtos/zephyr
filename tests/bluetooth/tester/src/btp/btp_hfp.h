/* btp_hfp.h - Bluetooth HFP tester headers */

/*
 * Copyright (c) 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 
/* HFP commands */
#define BTP_HFP_READ_SUPPORTED_COMMANDS  0x01

struct btp_hfp_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_HFP_ENABLE_SLC  0x02
struct btp_hfp_enable_slc_cmd {
	bt_addr_le_t address;
	uint8_t channel;
	uint8_t is_ag;
	uint8_t flags;
} __packed;

struct btp_hfp_enable_slc_rp {
	uint8_t connection_id;
} __packed;

#define BTP_HFP_DISABLE_SLC  0x03
struct btp_hfp_disable_slc_cmd {
	uint8_t flags;
} __packed;

struct btp_hfp_disable_slc_rp {
	
} __packed;

#define BTP_HFP_SIGNAL_STRENGTH_SEND  0x04
struct btp_hfp_signal_strength_send_cmd {
	uint8_t strength;
	uint8_t flags;
} __packed;

struct btp_hfp_signal_strength_send_rp {
	
} __packed;

#define BTP_HFP_CONTROL  0x05

enum btp_hfp_control_type
{
	HFP_ENABLE_INBAND_RING        = 0x01,
	HFP_DISABLE_CALL              = 0x02,
	HFP_MUTE_INBAND_RING          = 0x03,
	HFP_AG_ANSWER_CALL            = 0x04,
	HFP_REJECT_CALL               = 0x05,
	HFP_OUT_CALL                  = 0x06,
	HFP_CLS_MEM_CALL_LIST         = 0x07,
	HFP_OUT_MEM_CALL              = 0x08,
	HFP_OUT_MEM_OUTOFRANGE_CALL   = 0x09,
	HFP_OUT_LAST_CALL             = 0x0a,
	HFP_TWC_CALL                  = 0x0b,
	HFP_END_SECOND_CALL           = 0x0c,
	HFP_DISABLE_ACTIVE_CALL       = 0x0d,
	HFP_HELD_ACTIVE_CALL          = 0x0e,
	HFP_EC_NR_DISABLE             = 0x0f,
	HFP_ENABLE_BINP               = 0x10,
	HFP_ENABLE_SUB_NUMBER         = 0x11,
	HFP_ENABLE_VR                 = 0x12,
	HFP_DISABLE_VR                = 0x13,
	HFP_QUERY_LIST_CALL           = 0x14,
	HFP_REJECT_HELD_CALL          = 0x15,
	HFP_ACCEPT_HELD_CALL          = 0x16,
	HFP_ACCEPT_INCOMING_HELD_CALL = 0x17,
	HFP_SEND_IIA                  = 0x18,
	HFP_SEND_BCC                  = 0x19,
	HFP_DIS_CTR_CHN               = 0x1a,
	HFP_IMPAIR_SIGNAL             = 0x1b,
	HFP_JOIN_CONVERSATION_CALL    = 0x1c,
	HFP_EXPLICIT_TRANSFER_CALL    = 0x1d,
	HFP_ENABLE_CLIP               = 0x1e,
	HFP_DISABLE_IN_BAND           = 0x1f,
	HFP_DISCOVER_START            = 0x20,
	HFP_INTG_HELD_CALL            = 0x21,
	HFP_SEND_BCC_MSBC              = 0x22,
	HFP_SEND_BCC_SWB              = 0x23,
};
struct btp_hfp_control_cmd {
	enum btp_hfp_control_type control_type;
	uint8_t value;
	uint8_t flags;
} __packed;

struct btp_hfp_control_rp {

} __packed;

#define BTP_HFP_SIGNAL_STRENGTH_VERIFY  0x06
struct btp_hfp_signal_strength_verify_cmd {
	uint8_t strength;
	uint8_t flags;
} __packed;

struct btp_hfp_signal_strength_verify_rp {
	
} __packed;

#define BTP_HFP_AG_ENABLE_CALL  0x07
struct btp_hfp_ag_enable_call_cmd {
	uint8_t flags;
} __packed;

struct btp_hfp_ag_enable_call_rp {
	
} __packed;

#define BTP_HFP_AG_DISCOVERABLE  0x08
struct btp_hfp_ag_discoverable_cmd {
	uint8_t flags;
} __packed;

struct btp_hfp_ag_discoverable_rp {
	
} __packed;

#define BTP_HFP_HF_DISCOVERABLE  0x09
struct btp_hfp_hf_discoverable_cmd {
	uint8_t flags;
} __packed;

struct btp_hfp_hf_discoverable_rp {
	
} __packed;

#define BTP_HFP_ENABLE_AUDIO  0x10
struct btp_hfp_enable_audio_cmd {
	uint8_t flags;
} __packed;

struct btp_hfp_enable_audio_rp {
	
} __packed;

#define BTP_HFP_VERIFY_NETWORK_OPERATOR  0x0A
struct btp_hfp_verify_network_operator_cmd {
	char *op;
	uint8_t flags;
} __packed;

struct btp_hfp_verify_network_operator_rp {

} __packed;


#define BTP_HFP_AG_DISABLE_CALL_EXTERNAL  0x0B
struct btp_hfp_ag_disable_call_external_cmd {
	uint8_t flags;
} __packed;

struct btp_hfp_ag_disable_call_external_rp {

} __packed;


#define BTP_HFP_HF_ANSWER_CALL  0x0C
struct btp_hfp_hf_answer_call_cmd {
	uint8_t flags;
} __packed;

struct btp_hfp_hf_answer_call_rp {

} __packed;


#define BTP_HFP_VERIFY  0x0D
enum btp_hfp_verify_type
{
	HFP_VERIFY_INBAND_RING        = 0x01,
	HFP_VERIFY_INBAND_RING_MUTING = 0x02,
	HFP_VERIFY_IUT_ALERTING       = 0x03,
	HFP_VERIFY_IUT_NOT_ALERTING   = 0x04,
	HFP_VERIFY_EC_NR_DISABLED     = 0x05,
};
struct btp_hfp_verify_cmd {
	uint8_t verify_type;
	uint8_t flags;
} __packed;

struct btp_hfp_verify_rp {

} __packed;


#define BTP_HFP_VERIFY_VOICE_TAG  0x0E
struct btp_hfp_verify_voice_tag_cmd {
	uint8_t voice_tag;
	uint8_t flags;
} __packed;

struct btp_hfp_verify_voice_tag_rp {

} __packed;

#define BTP_HFP_SPEAKER_MIC_VOLUME_SEND  0x0F
struct btp_hfp_speaker_mic_volume_send_cmd {
	uint8_t speaker_mic;
	uint8_t speaker_mic_volume;
	uint8_t flags;
} __packed;

struct btp_hfp_speaker_mic_volume_send_rp {

} __packed;

#define BTP_HFP_DISABLE_AUDIO  0x11
struct btp_hfp_disable_audio_cmd {
	uint8_t flags;
} __packed;

struct btp_hfp_disable_audio_rp {

} __packed;

#define BTP_HFP_ENABLE_NETWORK  0x12
struct btp_hfp_enable_network_cmd {
	uint8_t flags;
} __packed;

struct btp_hfp_enable_network_rp {

} __packed;

#define BTP_HFP_DISABLE_NETWORK  0x13
struct btp_hfp_disable_network_cmd {
	uint8_t flags;
} __packed;

#define BTP_HFP_MAKE_ROAM_ACTIVE  0x14
struct btp_hfp_make_roam_active_cmd {
	uint8_t flags;
} __packed;

struct btp_hfp_make_roam_active_rp {

} __packed;

#define BTP_HFP_MAKE_ROAM_INACTIVE  0x15
struct btp_hfp_make_roam_inactive_cmd {
	uint8_t flags;
} __packed;

struct btp_hfp_make_roam_inactive_rp {

} __packed;

#define BTP_HFP_MAKE_BATTERY_NOT_FULL_CHARGED  0x16
struct btp_hfp_make_battery_not_full_charged_cmd {
	uint8_t flags;
} __packed;

struct btp_hfp_make_battery_not_full_charged_rp {

} __packed;


#define BTP_HFP_MAKE_BATTERY_FULL_CHARGED  0x17
struct btp_hfp_make_battery_full_charged_cmd {
	uint8_t flags;
} __packed;

struct btp_hfp_make_battery_full_charged_rp {

} __packed;


#define BTP_HFP_VERIFY_BATTERY_CHARGED  0x18
struct btp_hfp_verify_battery_charged_cmd {
	uint8_t flags;
} __packed;

struct btp_hfp_verify_battery_charged_rp {

} __packed;


#define BTP_HFP_VERIFY_BATTERY_DISCHARGED  0x19
struct btp_hfp_verify_battery_discharged_cmd {
	uint8_t flags;
} __packed;

#define BTP_HFP_SPEAKER_MIC_VOLUME_VERIFY  0x1A
struct btp_hfp_speaker_mic_volume_verify_cmd {
	uint8_t speaker_mic;
	uint8_t speaker_mic_volume;
	uint8_t flags;
} __packed;

struct btp_hfp_speaker_mic_volume_verify_rp {

} __packed;


#define BTP_HFP_AG_REGISTER  0x1B
struct btp_hfp_ag_register_cmd {
	uint8_t flags;
} __packed;

struct btp_hfp_ag_register_rp {

} __packed;


#define BTP_HFP_HF_REGISTER  0x1C
struct btp_hfp_hf_register_cmd {
	uint8_t flags;
} __packed;

struct btp_hfp_hf_register_rp {

} __packed;


#define BTP_HFP_VERIFY_ROAM_ACTIVE  0x1D
struct btp_hfp_verify_roam_active_cmd {
	uint8_t flags;
} __packed;

struct btp_hfp_verify_roam_active_rp {

} __packed;


#define BTP_HFP_QUERY_NETWORK_OPERATOR  0x1E
struct btp_hfp_query_network_operator_cmd {
	uint8_t flags;
} __packed;

struct btp_hfp_query_network_operator_rp {

} __packed;


#define BTP_HFP_AG_VRE_TEXT  0x1F
struct btp_hfp_ag_vre_text_cmd {
	uint8_t type;
	uint8_t operation;
	uint8_t flags;
} __packed;

struct btp_hfp_ag_vre_text_rp {
	
} __packed;


/* BTP HEAD FILE */

struct btp_hfp_verify_battery_discharged_rp {

} __packed;

struct btp_hfp_disable_network_rp {

} __packed;

#define BTP_HFP_EV_SCO_CONNECTED  0x81

struct btp_hfp_sco_connected_ev {

} __packed;

#define BTP_HFP_EV_SCO_DISCONNECTED  0x82

struct btp_hfp_sco_disconnected_ev {

} __packed;
