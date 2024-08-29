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

#if defined(CONFIG_BT_HFP_HF_CLI)
#define BT_HFP_HF_FEATURE_CLI_ENABLE BT_HFP_HF_FEATURE_CLI
#define BT_HFP_HF_SDP_FEATURE_CLI_ENABLE BT_HFP_HF_SDP_FEATURE_CLI
#else
#define BT_HFP_HF_FEATURE_CLI_ENABLE 0
#define BT_HFP_HF_SDP_FEATURE_CLI_ENABLE 0
#endif /* CONFIG_BT_HFP_HF_CLI */

#if defined(CONFIG_BT_HFP_HF_VOLUME)
#define BT_HFP_HF_FEATURE_VOLUME_ENABLE BT_HFP_HF_FEATURE_VOLUME
#define BT_HFP_HF_SDP_FEATURE_VOLUME_ENABLE BT_HFP_HF_SDP_FEATURE_VOLUME
#else
#define BT_HFP_HF_FEATURE_VOLUME_ENABLE 0
#define BT_HFP_HF_SDP_FEATURE_VOLUME_ENABLE 0
#endif /* CONFIG_BT_HFP_HF_VOLUME */

#if defined(CONFIG_BT_HFP_HF_CODEC_NEG)
#define BT_HFP_HF_CODEC_NEG_ENABLE BT_HFP_HF_FEATURE_CODEC_NEG
#else
#define BT_HFP_HF_CODEC_NEG_ENABLE 0
#endif /* CONFIG_BT_HFP_HF_CODEC_NEG */

#if defined(CONFIG_BT_HFP_HF_ECNR)
#define BT_HFP_HF_FEATURE_ECNR_ENABLE BT_HFP_HF_FEATURE_ECNR
#define BT_HFP_HF_SDP_FEATURE_ECNR_ENABLE BT_HFP_HF_SDP_FEATURE_ECNR
#else
#define BT_HFP_HF_FEATURE_ECNR_ENABLE 0
#define BT_HFP_HF_SDP_FEATURE_ECNR_ENABLE 0
#endif /* CONFIG_BT_HFP_HF_CODEC_NEG */

/* HFP HF Supported features */
#define BT_HFP_HF_SUPPORTED_FEATURES (\
	BT_HFP_HF_FEATURE_CLI_ENABLE | \
	BT_HFP_HF_SDP_FEATURE_VOLUME_ENABLE |\
	BT_HFP_HF_CODEC_NEG_ENABLE | \
	BT_HFP_HF_FEATURE_ECNR_ENABLE)

/* HFP HF Supported features in SDP */
#define BT_HFP_HF_SDP_SUPPORTED_FEATURES (\
	BT_HFP_HF_SDP_FEATURE_CLI_ENABLE | \
	BT_HFP_HF_SDP_FEATURE_VOLUME_ENABLE | \
	BT_HFP_HF_SDP_FEATURE_ECNR_ENABLE)

#define BT_HFP_HF_CODEC_CVSD_MASK BIT(BT_HFP_HF_CODEC_CVSD)

#if defined(CONFIG_BT_HFP_HF_CODEC_MSBC)
#define BT_HFP_HF_CODEC_MSBC_ENABLE BIT(BT_HFP_HF_CODEC_MSBC)
#else
#define BT_HFP_HF_CODEC_MSBC_ENABLE 0
#endif /* CONFIG_BT_HFP_HF_CODEC_MSBC */

#if defined(CONFIG_BT_HFP_HF_CODEC_LC3_SWB)
#define BT_HFP_HF_CODEC_LC3_SWB_ENABLE BIT(BT_HFP_HF_CODEC_LC3_SWB)
#else
#define BT_HFP_HF_CODEC_LC3_SWB_ENABLE 0
#endif /* CONFIG_BT_HFP_HF_CODEC_LC3_SWB */

/* HFP HF Supported Codec IDs*/
#define BT_HFP_HF_SUPPORTED_CODEC_IDS \
	BT_HFP_HF_CODEC_CVSD_MASK | \
	BT_HFP_HF_CODEC_MSBC_ENABLE | \
	BT_HFP_HF_CODEC_LC3_SWB_ENABLE

#define HF_MAX_BUF_LEN       BT_HF_CLIENT_MAX_PDU
#define HF_MAX_AG_INDICATORS 20

#define BT_HFP_HF_VGM_GAIN_MAX 15
#define BT_HFP_HF_VGM_GAIN_MIN 0

#define BT_HFP_HF_VGS_GAIN_MAX 15
#define BT_HFP_HF_VGS_GAIN_MIN 0

/* bt_hfp_hf flags: the flags defined here represent hfp hf parameters */
enum {
	BT_HFP_HF_FLAG_CONNECTED,     /* HFP HF SLC Established */
	BT_HFP_HF_FLAG_TX_ONGOING,    /* HFP HF TX is ongoing */
	BT_HFP_HF_FLAG_INCOMING,      /* HFP HF call incoming */
	BT_HFP_HF_FLAG_INCOMING_HELD, /* HFP HF call incoming is held */
	BT_HFP_HF_FLAG_QUERY_HOLD,    /* HFP HF query response and hold status */
	BT_HFP_HF_FLAG_ACTIVE,        /* HFP HF call active */
	BT_HFP_HF_FLAG_DIALING,       /* HFP HF call dialing */
	BT_HFP_HF_FLAG_CODEC_CONN,    /* HFP HF codec connection setup */
	/* Total number of flags - must be at the end of the enum */
	BT_HFP_HF_NUM_FLAGS,
};

struct bt_hfp_hf {
	struct bt_rfcomm_dlc rfcomm_dlc;
	/* ACL connection handle */
	struct bt_conn *acl;
	/* AT command sending queue */
	at_finish_cb_t backup_finish;
	struct k_fifo tx_pending;
	/* SCO Channel */
	struct bt_sco_chan chan;
	char hf_buffer[HF_MAX_BUF_LEN];
	struct at_client at;
	uint32_t hf_features;
	uint32_t ag_features;
	uint8_t hf_codec_ids;
	uint8_t vgm;
	uint8_t vgs;
	int8_t ind_table[HF_MAX_AG_INDICATORS];

	/* AT command initialization indicator */
	uint8_t cmd_init_seq;

	ATOMIC_DEFINE(flags, BT_HFP_HF_NUM_FLAGS);
};

enum hfp_hf_ag_indicators {
	HF_SERVICE_IND,
	HF_CALL_IND,
	HF_CALL_SETUP_IND,
	HF_CALL_HELD_IND,
	HF_SINGNAL_IND,
	HF_ROAM_IND,
	HF_BATTERY_IND
};

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
