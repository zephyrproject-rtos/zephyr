/** @file
 *  @brief Internal APIs for Bluetooth Handsfree profile handling.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* HFP AG Supported features */
#define BT_HFP_AG_SUPPORTED_FEATURES (BT_HFP_AG_FEATURE_INBAND_RINGTONE | BT_HFP_AG_FEATURE_EXT_ERR)

/* bt_hfp_ag flags: the flags defined here represent HFP AG parameters */
enum {
	BT_HFP_AG_CMEE_ENABLE,   /* Extended Audio Gateway Error Result Code */
	BT_HFP_AG_CMER_ENABLE,   /* Indicator Events Reporting */
	BT_HFP_AG_CLIP_ENABLE,   /* Calling Line Identification notification */
	BT_HFP_AG_INBAND_RING,   /* In-band ring */
	BT_HFP_AG_COPS_SET,      /* Query Operator selection */
	BT_HFP_AG_INCOMING_CALL, /* Incoming call */
	BT_HFP_AG_CODEC_CONN,    /* Codec connection is ongoing */
	BT_HFP_AG_CODEC_CHANGED, /* Codec Id Changed */
	BT_HFP_AG_TX_ONGOING,    /* TX is ongoing */
	BT_HFP_AG_CREATING_SCO,  /* SCO is creating */

	/* Total number of flags - must be at the end of the enum */
	BT_HFP_AG_NUM_FLAGS,
};

/* HFP HF Indicators */
enum {
	HFP_HF_ENHANCED_SAFETY_IND = 1, /* Enhanced Safety */
	HFP_HF_BATTERY_LEVEL_IND = 2,   /* Remaining level of Battery */
	HFP_HF_IND_MAX
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
	BT_HFP_CALL_TERMINATE,
	/** Call outgoing */
	BT_HFP_CALL_OUTGOING,
	/** Call incoming */
	BT_HFP_CALL_INCOMING,
	/** Call alerting */
	BT_HFP_CALL_ALERTING,
	/** Call active */
	BT_HFP_CALL_ACTIVE,
	/** Call hold */
	BT_HFP_CALL_HOLD,
} bt_hfp_call_state_t;

#define AT_COPS_OPERATOR_MAX_LEN 16

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

	/* HFP Call state */
	bt_hfp_call_state_t call_state;

	/* Delayed work deferred tasks:
	 * - call status cleanup.
	 */
	struct k_work_delayable deferred_work;

	/* AG Indicators */
	uint8_t indicator_value[BT_HFP_AG_IND_MAX];
	uint32_t indicator;

	/* HF Indicators */
	uint32_t hf_indicators_of_ag;
	uint32_t hf_indicators_of_hf;
	uint8_t hf_indicator_value[HFP_HF_IND_MAX];

	/* operator */
	char operator[AT_COPS_OPERATOR_MAX_LEN + 1];

	/* SCO Connection Object */
	struct bt_sco_chan sco_chan;
	/* HFP TX pending */
	sys_slist_t tx_pending;

	/* Dial number or incoming number */
	char number[CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN + 1];

	/* Calling Line Identification notification */
	struct k_work_delayable ringing_work;

	/* Critical locker */
	struct k_sem lock;

	/* TX work */
	struct k_work_delayable tx_work;
};

/* Active */
#define HFP_AG_CLCC_STATUS_ACTIVE         0
/* Held */
#define HFP_AG_CLCC_STATUS_HELD           1
/* Dialing (outgoing calls only) */
#define HFP_AG_CLCC_STATUS_DIALING        2
/* Alerting (outgoing calls only) */
#define HFP_AG_CLCC_STATUS_ALERTING       3
/* Incoming (incoming calls only) */
#define HFP_AG_CLCC_STATUS_INCOMING       4
/* Waiting (incoming calls only) */
#define HFP_AG_CLCC_STATUS_WAITING        5
/* Call held by Response and Hold */
#define HFP_AG_CLCC_STATUS_CALL_HELD_HOLD 6
/* Invalid status */
#define HFP_AG_CLCC_STATUS_INVALID        0xFFU
