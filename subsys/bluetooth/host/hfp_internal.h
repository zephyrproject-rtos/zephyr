/** @file
 *  @brief Internal APIs for Bluetooth Handsfree profile handling.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define BT_HFP_MAX_MTU           140
#define BT_HF_CLIENT_MAX_PDU     BT_HFP_MAX_MTU

/* HFP AG Features */
#define BT_HFP_AG_FEATURE_3WAY_CALL     0x00000001 /* Three-way calling */
#define BT_HFP_AG_FEATURE_ECNR          0x00000002 /* EC and/or NR function */
#define BT_HFP_AG_FEATURE_VOICE_RECG    0x00000004 /* Voice recognition */
#define BT_HFP_AG_INBAND_RING_TONE      0x00000008 /* In-band ring capability */
#define BT_HFP_AG_VOICE_TAG             0x00000010 /* Attach no. to voice tag */
#define BT_HFP_AG_FEATURE_REJECT_CALL   0x00000020 /* Ability to reject call */
#define BT_HFP_AG_FEATURE_ECS           0x00000040 /* Enhanced call status */
#define BT_HFP_AG_FEATURE_ECC           0x00000080 /* Enhanced call control */
#define BT_HFP_AG_FEATURE_EXT_ERR       0x00000100 /* Extented error codes */
#define BT_HFP_AG_FEATURE_CODEC_NEG     0x00000200 /* Codec negotiation */
#define BT_HFP_AG_FEATURE_HF_IND        0x00000400 /* HF Indicators */
#define BT_HFP_AG_FEARTURE_ESCO_S4      0x00000800 /* eSCO S4 Settings */

/* HFP HF Features */
#define BT_HFP_HF_FEATURE_ECNR          0x00000001 /* EC and/or NR */
#define BT_HFP_HF_FEATURE_3WAY_CALL     0x00000002 /* Three-way calling */
#define BT_HFP_HF_FEATURE_CLI           0x00000004 /* CLI presentation */
#define BT_HFP_HF_FEATURE_VOICE_RECG    0x00000008 /* Voice recognition */
#define BT_HFP_HF_FEATURE_VOLUME        0x00000010 /* Remote volume control */
#define BT_HFP_HF_FEATURE_ECS           0x00000020 /* Enhanced call status */
#define BT_HFP_HF_FEATURE_ECC           0x00000040 /* Enhanced call control */
#define BT_HFP_HF_FEATURE_CODEC_NEG     0x00000080 /* CODEC Negotiation */
#define BT_HFP_HF_FEATURE_HF_IND        0x00000100 /* HF Indicators */
#define BT_HFP_HF_FEATURE_ESCO_S4       0x00000200 /* eSCO S4 Settings */

/* HFP HF Supported features */
#define BT_HFP_HF_SUPPORTED_FEATURES    (BT_HFP_HF_FEATURE_CLI | \
					 BT_HFP_HF_FEATURE_VOLUME)

#define HF_MAX_BUF_LEN                  BT_HF_CLIENT_MAX_PDU
#define HF_MAX_AG_INDICATORS            20

struct bt_hfp_hf {
	struct bt_rfcomm_dlc rfcomm_dlc;
	char hf_buffer[HF_MAX_BUF_LEN];
	struct at_client at;
	u32_t hf_features;
	u32_t ag_features;
	s8_t ind_table[HF_MAX_AG_INDICATORS];
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
