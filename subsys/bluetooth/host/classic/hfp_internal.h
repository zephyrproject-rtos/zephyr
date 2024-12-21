/** @file
 *  @brief Internal APIs for Bluetooth Handsfree profile handling.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define BT_HFP_MAX_MTU       140
#define BT_HF_CLIENT_MAX_PDU BT_HFP_MAX_MTU

/* HFP AG Features */
#define BT_HFP_AG_FEATURE_3WAY_CALL       0x00000001 /* Three-way calling */
#define BT_HFP_AG_FEATURE_ECNR            0x00000002 /* EC and/or NR function */
#define BT_HFP_AG_FEATURE_VOICE_RECG      0x00000004 /* Voice recognition */
#define BT_HFP_AG_FEATURE_INBAND_RINGTONE 0x00000008 /* In-band ring capability */
#define BT_HFP_AG_FEATURE_VOICE_TAG       0x00000010 /* Attach no. to voice tag */
#define BT_HFP_AG_FEATURE_REJECT_CALL     0x00000020 /* Ability to reject call */
#define BT_HFP_AG_FEATURE_ECS             0x00000040 /* Enhanced call status */
#define BT_HFP_AG_FEATURE_ECC             0x00000080 /* Enhanced call control */
#define BT_HFP_AG_FEATURE_EXT_ERR         0x00000100 /* Extended error codes */
#define BT_HFP_AG_FEATURE_CODEC_NEG       0x00000200 /* Codec negotiation */
#define BT_HFP_AG_FEATURE_HF_IND          0x00000400 /* HF Indicators */
#define BT_HFP_AG_FEATURE_ESCO_S4         0x00000800 /* eSCO S4 Settings */
#define BT_HFP_AG_FEATURE_ENH_VOICE_RECG  0x00001000 /* Enhanced Voice Recognition Status */
#define BT_HFP_AG_FEATURE_VOICE_RECG_TEXT 0x00002000 /* Voice Recognition Text */

/* HFP HF Features */
#define BT_HFP_HF_FEATURE_ECNR            0x00000001 /* EC and/or NR function */
#define BT_HFP_HF_FEATURE_3WAY_CALL       0x00000002 /* Three-way calling */
#define BT_HFP_HF_FEATURE_CLI             0x00000004 /* CLI presentation */
#define BT_HFP_HF_FEATURE_VOICE_RECG      0x00000008 /* Voice recognition */
#define BT_HFP_HF_FEATURE_VOLUME          0x00000010 /* Remote volume control */
#define BT_HFP_HF_FEATURE_ECS             0x00000020 /* Enhanced call status */
#define BT_HFP_HF_FEATURE_ECC             0x00000040 /* Enhanced call control */
#define BT_HFP_HF_FEATURE_CODEC_NEG       0x00000080 /* CODEC Negotiation */
#define BT_HFP_HF_FEATURE_HF_IND          0x00000100 /* HF Indicators */
#define BT_HFP_HF_FEATURE_ESCO_S4         0x00000200 /* eSCO S4 Settings */
#define BT_HFP_HF_FEATURE_ENH_VOICE_RECG  0x00000400 /* Enhanced Voice Recognition Status */
#define BT_HFP_HF_FEATURE_VOICE_RECG_TEXT 0x00000800 /* Voice Recognition Text */

/* HFP HF Features in SDP */
#define BT_HFP_HF_SDP_FEATURE_ECNR            BIT(0) /* EC and/or NR function */
#define BT_HFP_HF_SDP_FEATURE_3WAY_CALL       BIT(1) /* Three-way calling */
#define BT_HFP_HF_SDP_FEATURE_CLI             BIT(2) /* CLI presentation */
#define BT_HFP_HF_SDP_FEATURE_VOICE_RECG      BIT(3) /* Voice recognition */
#define BT_HFP_HF_SDP_FEATURE_VOLUME          BIT(4) /* Remote volume control */
#define BT_HFP_HF_SDP_FEATURE_WBS             BIT(5) /* Wide Band Speech */
#define BT_HFP_HF_SDP_FEATURE_ENH_VOICE_RECG  BIT(6) /* Enhanced Voice Recognition Status */
#define BT_HFP_HF_SDP_FEATURE_VOICE_RECG_TEXT BIT(7) /* Voice Recognition Text */
#define BT_HFP_HF_SDP_FEATURE_SUPER_WBS       BIT(7) /* Super Wide Band Speech */

/* HFP AG Features in SDP */
#define BT_HFP_AG_SDP_FEATURE_3WAY_CALL       BIT(0) /* Three-way calling */
#define BT_HFP_AG_SDP_FEATURE_ECNR            BIT(1) /* EC and/or NR function */
#define BT_HFP_AG_SDP_FEATURE_VOICE_RECG      BIT(2) /* Voice recognition */
#define BT_HFP_AG_SDP_FEATURE_INBAND_RINGTONE BIT(3) /* In-band ring tone capability */
#define BT_HFP_AG_SDP_FEATURE_VOICE_TAG       BIT(4) /* Attach no. to voice tag */
#define BT_HFP_AG_SDP_FEATURE_WBS             BIT(5) /* Wide Band Speech */
#define BT_HFP_AG_SDP_FEATURE_ENH_VOICE_RECG  BIT(6) /* Enhanced Voice Recognition Status */
#define BT_HFP_AG_SDP_FEATURE_VOICE_RECG_TEXT BIT(7) /* Voice Recognition Text */
#define BT_HFP_AG_SDP_FEATURE_SUPER_WBS       BIT(8) /* Super Wide Band Speech */
#define HF_MAX_BUF_LEN       BT_HF_CLIENT_MAX_PDU
#define HF_MAX_AG_INDICATORS 20

#define BT_HFP_HF_VGM_GAIN_MAX 15
#define BT_HFP_HF_VGM_GAIN_MIN 0

#define BT_HFP_HF_VGS_GAIN_MAX 15
#define BT_HFP_HF_VGS_GAIN_MIN 0

/* HFP call setup status */
#define BT_HFP_CALL_SETUP_NONE            0
#define BT_HFP_CALL_SETUP_INCOMING        1
#define BT_HFP_CALL_SETUP_OUTGOING        2
#define BT_HFP_CALL_SETUP_REMOTE_ALERTING 3

/* HFP incoming call status */
#define BT_HFP_BTRH_ON_HOLD  0
#define BT_HFP_BTRH_ACCEPTED 1
#define BT_HFP_BTRH_REJECTED 2

/* HFP Call held status */
/* No calls held */
#define BT_HFP_CALL_HELD_NONE        0
/* Call is placed on hold or active/held calls swapped
 * The AG has both an active AND a held call
 */
#define BT_HFP_CALL_HELD_ACTIVE_HELD 1
/* Call on hold, no active call */
#define BT_HFP_CALL_HELD_HELD        2

/* HFP AT+CHLD command value */
/* Releases all held calls or sets User Determined User Busy
 * (UDUB) for a waiting call
 */
#define BT_HFP_CHLD_RELEASE_ALL                 0
/* Releases all active calls (if any exist) and accepts the
 * other (held or waiting) call.
 */
#define BT_HFP_CHLD_RELEASE_ACTIVE_ACCEPT_OTHER 1
/* Places all active calls (if any exist) on hold and accepts
 * the other (held or waiting) call
 */
#define BT_HFP_CALL_HOLD_ACTIVE_ACCEPT_OTHER    2
/* Adds a held call to the conversation */
#define BT_HFP_CALL_ACTIVE_HELD                 3
/* Connects the two calls and disconnects the subscriber from
 * both calls (Explicit Call Transfer).
 * Support for this value and its associated functionality is
 * optional for the HF.
 */
#define BT_HFP_CALL_QUITE	                    4
/* Release a specific active call */
#define BT_HFP_CALL_RELEASE_SPECIFIED_ACTIVE    10
/* Private Consultation Mode
 * place all parties of a multiparty call on hold with the
 * exception of the specified call.
 */
#define BT_HFP_CALL_PRIVATE_CNLTN_MODE          20

/* Active */
#define BT_HFP_CLCC_STATUS_ACTIVE         0
/* Held */
#define BT_HFP_CLCC_STATUS_HELD           1
/* Dialing (outgoing calls only) */
#define BT_HFP_CLCC_STATUS_DIALING        2
/* Alerting (outgoing calls only) */
#define BT_HFP_CLCC_STATUS_ALERTING       3
/* Incoming (incoming calls only) */
#define BT_HFP_CLCC_STATUS_INCOMING       4
/* Waiting (incoming calls only) */
#define BT_HFP_CLCC_STATUS_WAITING        5
/* Call held by Response and Hold */
#define BT_HFP_CLCC_STATUS_CALL_HELD_HOLD 6

/* BVRA Value */
/* BVRA Deactivation */
#define BT_HFP_BVRA_DEACTIVATION    0
/* BVRA Activation */
#define BT_HFP_BVRA_ACTIVATION      1
/* Ready to accept audio */
#define BT_HFP_BVRA_READY_TO_ACCEPT 2

/* a maximum of 4 characters in length, but
 * less than 4 characters in length is valid.
 */
#define BT_HFP_BVRA_TEXT_ID_MAX_LEN 4

/* BVRA VRE state */
/* BVRA VRE state: the AG is ready to accept audio input */
#define BT_HFP_BVRA_STATE_ACCEPT_INPUT BIT(0)
/* BVRA VRE state: the AG is sending audio to the HF */
#define BT_HFP_BVRA_STATE_SEND_AUDIO BIT(1)
/* BVRA VRE state: the AG is processing the audio input */
#define BT_HFP_BVRA_STATE_PROCESS_AUDIO BIT(2)

#define IS_VALID_DTMF(c) ((((c) >= '0') && ((c) <= '9')) || \
	(((c) >= 'A') && ((c) <= 'D')) || ((c) == '#') || ((c) == '*'))

/* HFP HF Indicators */
enum {
	HFP_HF_ENHANCED_SAFETY_IND = 1, /* Enhanced Safety */
	HFP_HF_BATTERY_LEVEL_IND = 2,   /* Remaining level of Battery */
	HFP_HF_IND_MAX
};

#define IS_VALID_BATTERY_LEVEL(level) (((level) >= 0) && ((level) <= 100))
