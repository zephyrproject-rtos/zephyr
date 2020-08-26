/*
 * Copyright (c) 2017-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct ll_adv_set {
	struct evt_hdr evt;
	struct ull_hdr ull;
	struct lll_adv lll;

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
	uint8_t  id_addr_type:1;
	uint8_t  id_addr[BDADDR_SIZE];
#endif /* CONFIG_BT_CTLR_PRIVACY */
};

#if defined(CONFIG_BT_CTLR_ADV_EXT)
struct ll_adv_aux_set {
	struct evt_hdr     evt;
	struct ull_hdr     ull;
	struct lll_adv_aux lll;

	uint16_t interval;

	uint8_t is_started:1;
};

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
struct ll_adv_sync_set {
	struct evt_hdr      evt;
	struct ull_hdr      ull;
	struct lll_adv_sync lll;

	uint16_t interval;

	uint8_t is_enabled:1;
	uint8_t is_started:1;
};
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_EXT */
