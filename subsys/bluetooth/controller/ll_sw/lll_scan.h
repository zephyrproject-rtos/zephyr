/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct lll_scan {
	struct lll_hdr hdr;

	u8_t  state:1;
	u8_t  chan:2;
	u8_t  filter_policy:2;
	u8_t  adv_addr_type:1;
	u8_t  init_addr_type:1;
	u8_t  type:1;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	u8_t  phy:3;
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_PRIVACY)
	u8_t  rpa_gen:1;
	/* initiator only */
	u8_t rl_idx;
#endif /* CONFIG_BT_CTLR_PRIVACY */

	u8_t  init_addr[BDADDR_SIZE];
	u8_t  adv_addr[BDADDR_SIZE];

	u32_t ticks_window;

#if defined(CONFIG_BT_CENTRAL)
	struct connection *conn;
	u16_t conn_interval;
	u16_t conn_latency;
	u16_t conn_timeout;
	u32_t ticks_conn_slot;
	u32_t win_offset_us;
#endif /* CONFIG_BT_CENTRAL */
};

void lll_scan_prepare(void *param);
