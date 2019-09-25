/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct lll_scan {
	struct lll_hdr hdr;

#if defined(CONFIG_BT_CENTRAL)
	/* NOTE: conn context has to be after lll_hdr */
	struct lll_conn *conn;
	u32_t conn_ticks_slot;
	u32_t conn_win_offset_us;
	u16_t conn_timeout;
#endif /* CONFIG_BT_CENTRAL */

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

	u16_t interval;
	u32_t ticks_window;
};

int lll_scan_init(void);
int lll_scan_reset(void);

void lll_scan_prepare(void *param);

extern u16_t ull_scan_lll_handle_get(struct lll_scan *lll);
