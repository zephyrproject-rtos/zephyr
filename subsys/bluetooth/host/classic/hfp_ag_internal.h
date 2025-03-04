/** @file
 *  @brief Internal APIs for Bluetooth Handsfree profile handling.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hfp_internal.h"

#if defined(CONFIG_BT_HFP_AG_EXT_ERR)
#define BT_HFP_AG_FEATURE_EXT_ERR_ENABLE BT_HFP_AG_FEATURE_EXT_ERR
#else
#define BT_HFP_AG_FEATURE_EXT_ERR_ENABLE 0
#endif /* CONFIG_BT_HFP_AG_EXT_ERR */

#if defined(CONFIG_BT_HFP_AG_CODEC_NEG)
#define BT_HFP_AG_FEATURE_CODEC_NEG_ENABLE BT_HFP_AG_FEATURE_CODEC_NEG
#else
#define BT_HFP_AG_FEATURE_CODEC_NEG_ENABLE 0
#endif /* CONFIG_BT_HFP_AG_CODEC_NEG */

#if defined(CONFIG_BT_HFP_AG_ECNR)
#define BT_HFP_AG_FEATURE_ECNR_ENABLE BT_HFP_AG_FEATURE_ECNR
#define BT_HFP_AG_SDP_FEATURE_ECNR_ENABLE BT_HFP_AG_SDP_FEATURE_ECNR
#else
#define BT_HFP_AG_FEATURE_ECNR_ENABLE 0
#define BT_HFP_AG_SDP_FEATURE_ECNR_ENABLE 0
#endif /* CONFIG_BT_HFP_AG_CODEC_NEG */

#if defined(CONFIG_BT_HFP_AG_ECS)
#define BT_HFP_AG_FEATURE_ECS_ENABLE BT_HFP_AG_FEATURE_ECS
#else
#define BT_HFP_AG_FEATURE_ECS_ENABLE 0
#endif /* CONFIG_BT_HFP_AG_ECS*/

#if defined(CONFIG_BT_HFP_AG_3WAY_CALL)
#define BT_HFP_AG_FEATURE_3WAY_CALL_ENABLE BT_HFP_AG_FEATURE_3WAY_CALL
#define BT_HFP_AG_SDP_FEATURE_3WAY_CALL_ENABLE BT_HFP_AG_SDP_FEATURE_3WAY_CALL
#else
#define BT_HFP_AG_FEATURE_3WAY_CALL_ENABLE 0
#define BT_HFP_AG_SDP_FEATURE_3WAY_CALL_ENABLE 0
#endif /* CONFIG_BT_HFP_AG_3WAY_CALL */

#if defined(CONFIG_BT_HFP_AG_ECC)
#define BT_HFP_AG_FEATURE_ECC_ENABLE BT_HFP_AG_FEATURE_ECC
#else
#define BT_HFP_AG_FEATURE_ECC_ENABLE 0
#endif /* CONFIG_BT_HFP_AG_ECC */

#if defined(CONFIG_BT_HFP_AG_VOICE_RECG)
#define BT_HFP_AG_FEATURE_VOICE_RECG_ENABLE BT_HFP_AG_FEATURE_VOICE_RECG
#define BT_HFP_AG_SDP_FEATURE_VOICE_RECG_ENABLE BT_HFP_AG_SDP_FEATURE_VOICE_RECG
#else
#define BT_HFP_AG_FEATURE_VOICE_RECG_ENABLE 0
#define BT_HFP_AG_SDP_FEATURE_VOICE_RECG_ENABLE 0
#endif /* CONFIG_BT_HFP_AG_VOICE_RECG */

#if defined(CONFIG_BT_HFP_AG_ENH_VOICE_RECG)
#define BT_HFP_AG_FEATURE_ENH_VOICE_RECG_ENABLE BT_HFP_AG_FEATURE_ENH_VOICE_RECG
#define BT_HFP_AG_SDP_FEATURE_ENH_VOICE_RECG_ENABLE BT_HFP_AG_SDP_FEATURE_ENH_VOICE_RECG
#else
#define BT_HFP_AG_FEATURE_ENH_VOICE_RECG_ENABLE 0
#define BT_HFP_AG_SDP_FEATURE_ENH_VOICE_RECG_ENABLE 0
#endif /* CONFIG_BT_HFP_AG_ENH_VOICE_RECG */

#if defined(CONFIG_BT_HFP_AG_VOICE_RECG_TEXT)
#define BT_HFP_AG_FEATURE_VOICE_RECG_TEXT_ENABLE BT_HFP_AG_FEATURE_VOICE_RECG_TEXT
#define BT_HFP_AG_SDP_FEATURE_VOICE_RECG_TEXT_ENABLE BT_HFP_AG_SDP_FEATURE_VOICE_RECG_TEXT
#else
#define BT_HFP_AG_FEATURE_VOICE_RECG_TEXT_ENABLE 0
#define BT_HFP_AG_SDP_FEATURE_VOICE_RECG_TEXT_ENABLE 0
#endif /* CONFIG_BT_HFP_AG_VOICE_RECG_TEXT */

#if defined(CONFIG_BT_HFP_AG_VOICE_TAG)
#define BT_HFP_AG_FEATURE_VOICE_TAG_ENABLE BT_HFP_AG_FEATURE_VOICE_TAG
#define BT_HFP_AG_SDP_FEATURE_VOICE_TAG_ENABLE BT_HFP_AG_SDP_FEATURE_VOICE_TAG
#else
#define BT_HFP_AG_FEATURE_VOICE_TAG_ENABLE 0
#define BT_HFP_AG_SDP_FEATURE_VOICE_TAG_ENABLE 0
#endif /* CONFIG_BT_HFP_AG_VOICE_TAG */

#if defined(CONFIG_BT_HFP_AG_HF_INDICATORS)
#define BT_HFP_AG_FEATURE_HF_IND_ENABLE BT_HFP_AG_FEATURE_HF_IND
#else
#define BT_HFP_AG_FEATURE_HF_IND_ENABLE 0
#endif /* CONFIG_BT_HFP_HF_HF_INDICATORS */

#if defined(CONFIG_BT_HFP_AG_REJECT_CALL)
#define BT_HFP_AG_FEATURE_REJECT_CALL_ENABLE BT_HFP_AG_FEATURE_REJECT_CALL
#else
#define BT_HFP_AG_FEATURE_REJECT_CALL_ENABLE 0
#endif /* CONFIG_BT_HFP_AG_REJECT_CALL */

/* HFP AG Supported features */
#define BT_HFP_AG_SUPPORTED_FEATURES (\
	BT_HFP_AG_FEATURE_3WAY_CALL_ENABLE | \
	BT_HFP_AG_FEATURE_INBAND_RINGTONE | \
	BT_HFP_AG_FEATURE_EXT_ERR_ENABLE | \
	BT_HFP_AG_FEATURE_CODEC_NEG_ENABLE | \
	BT_HFP_AG_FEATURE_ECNR_ENABLE | \
	BT_HFP_AG_FEATURE_ECS_ENABLE | \
	BT_HFP_AG_FEATURE_ECC_ENABLE | \
	BT_HFP_AG_FEATURE_VOICE_RECG_ENABLE | \
	BT_HFP_AG_FEATURE_ENH_VOICE_RECG_ENABLE | \
	BT_HFP_AG_FEATURE_VOICE_RECG_TEXT_ENABLE | \
	BT_HFP_AG_FEATURE_VOICE_TAG_ENABLE | \
	BT_HFP_AG_FEATURE_HF_IND_ENABLE | \
	BT_HFP_AG_FEATURE_REJECT_CALL_ENABLE)

/* HFP AG Supported features in SDP */
#define BT_HFP_AG_SDP_SUPPORTED_FEATURES (\
	BT_HFP_AG_SDP_FEATURE_3WAY_CALL_ENABLE | \
	BT_HFP_AG_SDP_FEATURE_INBAND_RINGTONE | \
	BT_HFP_AG_SDP_FEATURE_ECNR_ENABLE | \
	BT_HFP_AG_SDP_FEATURE_VOICE_RECG_ENABLE | \
	BT_HFP_AG_SDP_FEATURE_ENH_VOICE_RECG_ENABLE | \
	BT_HFP_AG_SDP_FEATURE_VOICE_RECG_TEXT_ENABLE | \
	BT_HFP_AG_SDP_FEATURE_VOICE_TAG_ENABLE)

/* bt_hfp_ag flags: the flags defined here represent HFP AG parameters */
enum {
	BT_HFP_AG_CMEE_ENABLE,   /* Extended Audio Gateway Error Result Code */
	BT_HFP_AG_CMER_ENABLE,   /* Indicator Events Reporting */
	BT_HFP_AG_CLIP_ENABLE,   /* Calling Line Identification notification */
	BT_HFP_AG_CCWA_ENABLE,   /* Call Waiting notification */
	BT_HFP_AG_INBAND_RING,   /* In-band ring */
	BT_HFP_AG_COPS_SET,      /* Query Operator selection */
	BT_HFP_AG_AUDIO_CONN,    /* Audio connection */
	BT_HFP_AG_CODEC_CONN,    /* Codec connection is ongoing */
	BT_HFP_AG_CODEC_CHANGED, /* Codec Id Changed */
	BT_HFP_AG_TX_ONGOING,    /* TX is ongoing */
	BT_HFP_AG_CREATING_SCO,  /* SCO is creating */
	BT_HFP_AG_VRE_ACTIVATE,  /* VRE is activated */
	BT_HFP_AG_VRE_R2A,       /* HF is ready to accept audio */

	/* Total number of flags - must be at the end of the enum */
	BT_HFP_AG_NUM_FLAGS,
};

/* bt_hfp_ag_call flags: the flags defined here represent HFP AG parameters */
enum {
	BT_HFP_AG_CALL_IN_USING,      /* Object is in using */
	BT_HFP_AG_CALL_INCOMING,      /* Incoming call */
	BT_HFP_AG_CALL_INCOMING_HELD, /* Incoming call held */
	BT_HFP_AG_CALL_OPEN_SCO,      /* Open SCO */
	BT_HFP_AG_CALL_OUTGOING_3WAY, /* Outgoing 3 way call */
	BT_HFP_AG_CALL_INCOMING_3WAY, /* Incoming 3 way call */

	/* Total number of flags - must be at the end of the enum */
	BT_HFP_AG_CALL_NUM_FLAGS,
};

typedef enum __packed {
	/** Session disconnected */
	BT_HFP_DISCONNECTED,
	/** Session in connecting state */
	BT_HFP_CONNECTING,
	/** Session in config state, HFP configuration */
	BT_HFP_CONFIG,
	/** Session ready for upper layer traffic on it */
	BT_HFP_CONNECTED,
	/** Session in disconnecting state */
	BT_HFP_DISCONNECTING,
} bt_hfp_state_t;

typedef enum __packed {
	/** Call terminate */
	BT_HFP_CALL_TERMINATE = 1,
	/** Call outgoing */
	BT_HFP_CALL_OUTGOING = 2,
	/** Call incoming */
	BT_HFP_CALL_INCOMING = 4,
	/** Call alerting */
	BT_HFP_CALL_ALERTING = 8,
	/** Call active */
	BT_HFP_CALL_ACTIVE = 16,
	/** Call hold */
	BT_HFP_CALL_HOLD = 32,
} bt_hfp_call_state_t;

#define AT_COPS_OPERATOR_MAX_LEN 16

struct bt_hfp_ag_call {
	struct bt_hfp_ag *ag;

	char number[CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN + 1];
	uint8_t type;

	ATOMIC_DEFINE(flags, BT_HFP_AG_CALL_NUM_FLAGS);

	/* HFP Call state */
	bt_hfp_call_state_t call_state;

	/* Calling Line Identification notification */
	struct k_work_delayable ringing_work;

	struct k_work_delayable deferred_work;
};

struct bt_hfp_ag {
	struct bt_rfcomm_dlc rfcomm_dlc;
	char buffer[HF_MAX_BUF_LEN];
	uint32_t hf_features;
	uint32_t ag_features;

	/* HF Supported Codec Ids*/
	uint32_t hf_codec_ids;
	uint8_t selected_codec_id;

	ATOMIC_DEFINE(flags, BT_HFP_AG_NUM_FLAGS);

	/* ACL Connection Object */
	struct bt_conn *acl_conn;

	/* HFP Connection state */
	bt_hfp_state_t state;

	/* AG Indicators */
	uint8_t indicator_value[BT_HFP_AG_IND_MAX];
	uint32_t indicator;

	/* HF Indicators */
	uint32_t hf_indicators_of_ag;
	uint32_t hf_indicators_of_hf;
	uint32_t hf_indicators;

	/* operator */
	uint8_t mode;
	char operator[AT_COPS_OPERATOR_MAX_LEN + 1];

	/* calls */
	struct bt_hfp_ag_call calls[CONFIG_BT_HFP_AG_MAX_CALLS];

	/* last dialing number and type */
	char last_number[CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN + 1];
	uint8_t type;

	/* SCO Connection Object */
	struct bt_sco_chan sco_chan;
	/* HFP TX pending */
	sys_slist_t tx_pending;

	/* Critical locker */
	struct k_sem lock;

	/* TX work */
	struct k_work_delayable tx_work;
};
