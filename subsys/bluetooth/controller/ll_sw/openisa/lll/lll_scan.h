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
	uint32_t conn_ticks_slot;
	uint32_t conn_win_offset_us;
	uint16_t conn_timeout;
#endif /* CONFIG_BT_CENTRAL */

	uint8_t  state:1;
	uint8_t  chan:2;
	uint8_t  filter_policy:2;
	uint8_t  adv_addr_type:1;
	uint8_t  init_addr_type:1;
	uint8_t  type:1;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	uint8_t  phy:3;
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_PRIVACY)
	uint8_t  rpa_gen:1;
	/* initiator only */
	uint8_t rl_idx;
#endif /* CONFIG_BT_CTLR_PRIVACY */

	uint8_t  init_addr[BDADDR_SIZE];
	uint8_t  adv_addr[BDADDR_SIZE];

	uint16_t interval;
	uint32_t ticks_window;
};

struct lll_scan_aux {
	struct lll_hdr hdr;
};

int lll_scan_init(void);
int lll_scan_reset(void);

void lll_scan_prepare(void *param);

extern uint8_t ull_scan_lll_handle_get(struct lll_scan *lll);
