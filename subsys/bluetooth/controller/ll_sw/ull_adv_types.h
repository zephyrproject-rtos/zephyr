/*
 * Copyright (c) 2017-2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct ll_adv_set {
#if defined(CONFIG_BT_LL_SW_SPLIT)
	struct ull_hdr ull;
	struct lll_adv lll;
	u8_t  is_enabled:1;
#endif /* CONFIG_BT_LL_SW_SPLIT */

	struct evt_hdr evt;

#if defined(CONFIG_BT_PERIPHERAL)
	u8_t is_hdcd:1;

	memq_link_t       *link_cc_free;
	struct node_rx_cc *node_rx_cc_free;
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	u32_t interval;
#if defined(CONFIG_BT_LL_SW)
	u8_t  phy_p:3;
#endif /* CONFIG_BT_LL_SW */
#else /* !CONFIG_BT_CTLR_ADV_EXT */
	u16_t interval;
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_LL_SW)
	u8_t  chan_map:3;
	u8_t  filter_policy:2;
#endif /* CONFIG_BT_LL_SW */

#if defined(CONFIG_BT_CTLR_PRIVACY)
	u8_t  own_addr_type:2;
	u8_t  id_addr_type:1;
	u8_t  rl_idx;
	u8_t  id_addr[BDADDR_SIZE];
#endif /* CONFIG_BT_CTLR_PRIVACY */
};
