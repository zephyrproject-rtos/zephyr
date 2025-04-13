/** @file
 *  @brief Internal APIs for Bluetooth Handsfree profile handling.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hfp_internal.h"

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

#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
#define BT_HFP_HF_FEATURE_3WAY_CALL_ENABLE BT_HFP_HF_FEATURE_3WAY_CALL
#define BT_HFP_HF_SDP_FEATURE_3WAY_CALL_ENABLE BT_HFP_HF_SDP_FEATURE_3WAY_CALL
#else
#define BT_HFP_HF_FEATURE_3WAY_CALL_ENABLE 0
#define BT_HFP_HF_SDP_FEATURE_3WAY_CALL_ENABLE 0
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */

#if defined(CONFIG_BT_HFP_HF_ECS)
#define BT_HFP_HF_FEATURE_ECS_ENABLE BT_HFP_HF_FEATURE_ECS
#else
#define BT_HFP_HF_FEATURE_ECS_ENABLE 0
#endif /* CONFIG_BT_HFP_HF_ECS */

#if defined(CONFIG_BT_HFP_HF_ECC)
#define BT_HFP_HF_FEATURE_ECC_ENABLE BT_HFP_HF_FEATURE_ECC
#else
#define BT_HFP_HF_FEATURE_ECC_ENABLE 0
#endif /* CONFIG_BT_HFP_HF_ECC */

#if defined(CONFIG_BT_HFP_HF_VOICE_RECG)
#define BT_HFP_HF_FEATURE_VOICE_RECG_ENABLE BT_HFP_HF_FEATURE_VOICE_RECG
#define BT_HFP_HF_SDP_FEATURE_VOICE_RECG_ENABLE BT_HFP_HF_SDP_FEATURE_VOICE_RECG
#else
#define BT_HFP_HF_FEATURE_VOICE_RECG_ENABLE 0
#define BT_HFP_HF_SDP_FEATURE_VOICE_RECG_ENABLE 0
#endif /* CONFIG_BT_HFP_HF_VOICE_RECG */

#if defined(CONFIG_BT_HFP_HF_ENH_VOICE_RECG)
#define BT_HFP_HF_FEATURE_ENH_VOICE_RECG_ENABLE BT_HFP_HF_FEATURE_ENH_VOICE_RECG
#define BT_HFP_HF_SDP_FEATURE_ENH_VOICE_RECG_ENABLE BT_HFP_HF_SDP_FEATURE_ENH_VOICE_RECG
#else
#define BT_HFP_HF_FEATURE_ENH_VOICE_RECG_ENABLE 0
#define BT_HFP_HF_SDP_FEATURE_ENH_VOICE_RECG_ENABLE 0
#endif /* CONFIG_BT_HFP_HF_ENH_VOICE_RECG */

#if defined(CONFIG_BT_HFP_HF_VOICE_RECG_TEXT)
#define BT_HFP_HF_FEATURE_VOICE_RECG_TEXT_ENABLE BT_HFP_HF_FEATURE_VOICE_RECG_TEXT
#define BT_HFP_HF_SDP_FEATURE_VOICE_RECG_TEXT_ENABLE BT_HFP_HF_SDP_FEATURE_VOICE_RECG_TEXT
#else
#define BT_HFP_HF_FEATURE_VOICE_RECG_TEXT_ENABLE 0
#define BT_HFP_HF_SDP_FEATURE_VOICE_RECG_TEXT_ENABLE 0
#endif /* CONFIG_BT_HFP_HF_VOICE_RECG_TEXT */

#if defined(CONFIG_BT_HFP_HF_HF_INDICATORS)
#define BT_HFP_HF_FEATURE_HF_IND_ENABLE BT_HFP_HF_FEATURE_HF_IND
#else
#define BT_HFP_HF_FEATURE_HF_IND_ENABLE 0
#endif /* CONFIG_BT_HFP_HF_HF_INDICATORS */

/* HFP HF Supported features */
#define BT_HFP_HF_SUPPORTED_FEATURES (\
	BT_HFP_HF_FEATURE_CLI_ENABLE | \
	BT_HFP_HF_SDP_FEATURE_VOLUME_ENABLE |\
	BT_HFP_HF_CODEC_NEG_ENABLE | \
	BT_HFP_HF_FEATURE_ECNR_ENABLE | \
	BT_HFP_HF_FEATURE_3WAY_CALL_ENABLE | \
	BT_HFP_HF_FEATURE_ECS_ENABLE | \
	BT_HFP_HF_FEATURE_ECC_ENABLE | \
	BT_HFP_HF_FEATURE_VOICE_RECG_ENABLE | \
	BT_HFP_HF_FEATURE_ENH_VOICE_RECG_ENABLE | \
	BT_HFP_HF_FEATURE_VOICE_RECG_TEXT_ENABLE | \
	BT_HFP_HF_FEATURE_HF_IND_ENABLE)

/* HFP HF Supported features in SDP */
#define BT_HFP_HF_SDP_SUPPORTED_FEATURES (\
	BT_HFP_HF_SDP_FEATURE_CLI_ENABLE | \
	BT_HFP_HF_SDP_FEATURE_VOLUME_ENABLE | \
	BT_HFP_HF_SDP_FEATURE_ECNR_ENABLE | \
	BT_HFP_HF_SDP_FEATURE_3WAY_CALL_ENABLE | \
	BT_HFP_HF_SDP_FEATURE_VOICE_RECG_ENABLE | \
	BT_HFP_HF_SDP_FEATURE_ENH_VOICE_RECG_ENABLE | \
	BT_HFP_HF_SDP_FEATURE_VOICE_RECG_TEXT_ENABLE)

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

/* bt_hfp_hf flags: the flags defined here represent hfp hf parameters */
enum {
	BT_HFP_HF_FLAG_CONNECTED,     /* HFP HF SLC Established */
	BT_HFP_HF_FLAG_TX_ONGOING,    /* HFP HF TX is ongoing */
	BT_HFP_HF_FLAG_RX_ONGOING,    /* HFP HF RX is ongoing */
	BT_HFP_HF_FLAG_CODEC_CONN,    /* HFP HF codec connection setup */
	BT_HFP_HF_FLAG_CLCC_PENDING,  /* HFP HF CLCC is pending */
	BT_HFP_HF_FLAG_VRE_ACTIVATE,  /* VRE is activated */
	BT_HFP_HF_FLAG_BINP,          /* +BINP result code is received */
	/* Total number of flags - must be at the end of the enum */
	BT_HFP_HF_NUM_FLAGS,
};

/* bt_hfp_hf_call flags: the flags defined here represent hfp hf call parameters */
enum {
	BT_HFP_HF_CALL_IN_USING,      /* Object is in using */
	BT_HFP_HF_CALL_CLCC,          /* CLCC report received */
	BT_HFP_HF_CALL_INCOMING,      /* Incoming call */
	BT_HFP_HF_CALL_INCOMING_HELD, /* Incoming call held */
	BT_HFP_HF_CALL_OUTGOING_3WAY, /* Outgoing 3 way call */
	BT_HFP_HF_CALL_INCOMING_3WAY, /* Incoming 3 way call */

	/* Total number of flags - must be at the end of the enum */
	BT_HFP_HF_CALL_NUM_FLAGS,
};

/* bt_hfp_hf_call state: the flags defined here represent hfp hf call state parameters */
enum {
	/* Call state flags */
	BT_HFP_HF_CALL_STATE_TERMINATE, /* Call terminate */
	BT_HFP_HF_CALL_STATE_OUTGOING,  /* Call outgoing */
	BT_HFP_HF_CALL_STATE_INCOMING,  /* Call incoming */
	BT_HFP_HF_CALL_STATE_ALERTING,  /* Call alerting */
	BT_HFP_HF_CALL_STATE_WAITING,   /* Call waiting */
	BT_HFP_HF_CALL_STATE_ACTIVE,    /* Call active */
	BT_HFP_HF_CALL_STATE_HELD,      /* Call held */

	/* Total number of flags - must be at the end of the enum */
	BT_HFP_HF_CALL_STATE_NUM_FLAGS,
};

struct bt_hfp_hf_call {
	struct bt_hfp_hf *hf;
	uint8_t index;

	ATOMIC_DEFINE(flags, BT_HFP_HF_CALL_NUM_FLAGS);
	ATOMIC_DEFINE(state, BT_HFP_HF_CALL_STATE_NUM_FLAGS);
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

	uint32_t hf_ind;
	uint32_t ag_ind;
	uint32_t ind_enable;

	/* AT command initialization indicator */
	uint8_t cmd_init_seq;

	/* The features supported by AT+CHLD */
	uint32_t chld_features;

	/* Worker for pending TX */
	struct k_work work;

	struct k_work_delayable deferred_work;

	/* calls */
	struct bt_hfp_hf_call calls[CONFIG_BT_HFP_HF_MAX_CALLS];

	ATOMIC_DEFINE(flags, BT_HFP_HF_NUM_FLAGS);
};
