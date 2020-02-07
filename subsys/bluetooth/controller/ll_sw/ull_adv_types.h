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
	u32_t interval;
	u8_t  sid:4;
	u8_t  is_created:1;
#else /* !CONFIG_BT_CTLR_ADV_EXT */
	u16_t interval;
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

	u8_t is_enabled:1;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	u8_t  own_addr_type:2;
	u8_t  id_addr_type:1;
	u8_t  id_addr[BDADDR_SIZE];
#endif /* CONFIG_BT_CTLR_PRIVACY */
};

struct ll_adv_sync_set {
	struct evt_hdr      evt;
	struct ull_hdr      ull;
	struct lll_adv_sync lll;

	u16_t interval;

	u8_t is_enabled:1;
	u8_t is_started:1;
};
