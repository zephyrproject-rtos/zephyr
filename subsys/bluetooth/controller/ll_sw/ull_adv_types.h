/*
 * Copyright (c) 2017-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
struct lll_df_adv_cfg;
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

struct ll_adv_set {
	struct ull_hdr ull;
	struct lll_adv lll;

#if defined(CONFIG_BT_CTLR_AD_DATA_BACKUP)
	/* Legacy AD Data backup when switching to legacy directed advertising
	 * or to Extended Advertising.
	 */
	struct {
		uint8_t len;
		uint8_t data[PDU_AC_DATA_SIZE_MAX];
	} ad_data_backup;
#endif /* CONFIG_BT_CTLR_AD_DATA_BACKUP */

#if defined(CONFIG_BT_PERIPHERAL)
	memq_link_t        *link_cc_free;
	struct node_rx_pdu *node_rx_cc_free;
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	uint32_t interval;
	uint8_t  rnd_addr[BDADDR_SIZE];
	uint8_t  sid:4;
	uint8_t  is_created:1;
#if defined(CONFIG_BT_CTLR_HCI_ADV_HANDLE_MAPPING)
	uint8_t  hci_handle;
#endif
	uint16_t event_counter;
	uint16_t max_events;
	uint32_t ticks_remain_duration;
#else /* !CONFIG_BT_CTLR_ADV_EXT */
	uint16_t interval;
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

	uint8_t is_enabled:1;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	uint8_t  own_addr_type:2;
	uint8_t  peer_addr_type:1;
	uint8_t  peer_addr[BDADDR_SIZE];
#endif /* CONFIG_BT_CTLR_PRIVACY */

#if defined(CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN)
	uint8_t  own_id_addr[BDADDR_SIZE];
#endif /* CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN */

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	struct lll_df_adv_cfg *df_cfg;
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */
#if defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
	uint32_t delay;
	uint32_t delay_remain;
	uint32_t ticks_at_expire;
#endif /* CONFIG_BT_CTLR_JIT_SCHEDULING */
};

struct ll_adv_aux_set {
	struct ull_hdr     ull;
	struct lll_adv_aux lll;

	uint16_t interval;

	uint16_t data_chan_id;
	struct {
		uint8_t data_chan_map[PDU_CHANNEL_MAP_SIZE];
		uint8_t data_chan_count:6;
	} chm[DOUBLE_BUFFER_SIZE];
	uint8_t  chm_first;
	uint8_t  chm_last;

	uint8_t is_started:1;
};

struct ll_adv_sync_set {
	struct ull_hdr      ull;
	struct lll_adv_sync lll;

	uint16_t interval;

	uint8_t is_enabled:1;
	uint8_t is_started:1;
	uint8_t is_data_cmplt:1;

	uint32_t aux_remainder;
};

struct ll_adv_iso_set {
	struct ull_hdr        ull;
	struct lll_adv_iso    lll;

	struct {
		struct node_rx_hdr hdr;
	} node_rx_complete;
	struct {
		struct node_rx_hdr hdr;
		union {
			uint8_t    pdu[0] __aligned(4);
			uint8_t    reason;
		};
	} node_rx_terminate;

#if defined(CONFIG_BT_CTLR_HCI_ADV_HANDLE_MAPPING)
	uint8_t  hci_handle;
#endif /* CONFIG_BT_CTLR_HCI_ADV_HANDLE_MAPPING */
};
