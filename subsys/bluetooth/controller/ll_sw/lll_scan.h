/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct lll_scan {
	struct lll_hdr hdr;

#if defined(CONFIG_BT_CENTRAL)
	/* NOTE: conn context SHALL be after lll_hdr,
	 *       check ull_conn_setup how it access the connection LLL
	 *       context.
	 */
	struct lll_conn *volatile conn;

	uint8_t  adv_addr[BDADDR_SIZE];
	uint32_t conn_win_offset_us;
	uint16_t conn_timeout;
#endif /* CONFIG_BT_CENTRAL */

	uint8_t  state:1;
	uint8_t  chan:2;
	uint8_t  filter_policy:2;
	uint8_t  type:1;
	uint8_t  init_addr_type:1;
	uint8_t  is_stop:1;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	/* Reference to aux context when scanning auxiliary PDU */
	struct lll_scan_aux *lll_aux;

	uint16_t duration_reload;
	uint16_t duration_expire;
	uint8_t  phy:3;
	uint8_t  is_adv_ind:1;
	uint8_t  is_aux_sched:1;
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	uint8_t  is_sync:1;
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CENTRAL)
	uint8_t  adv_addr_type:1;
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_CTLR_PRIVACY)
	uint8_t  rpa_gen:1;
	/* initiator only */
	uint8_t rl_idx;
#endif /* CONFIG_BT_CTLR_PRIVACY */

	uint8_t  init_addr[BDADDR_SIZE];

	uint16_t interval;
	uint32_t ticks_window;

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	int8_t tx_pwr_lvl;
#endif /* CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */
};

struct lll_scan_aux {
	struct lll_hdr hdr;

	uint8_t chan:6;
	uint8_t state:1;
	uint8_t is_chain_sched:1;

	uint8_t phy:3;

	uint32_t window_size_us;

#if defined(CONFIG_BT_CENTRAL)
	struct node_rx_pdu *node_conn_rx;
#endif /* CONFIG_BT_CENTRAL */
};


/* Define to check if filter is enabled and in addition if it is Extended Scan
 * Filtering.
 */
#define SCAN_FP_FILTER BIT(0)
#define SCAN_FP_EXT    BIT(1)

int lll_scan_init(void);
int lll_scan_reset(void);

void lll_scan_prepare(void *param);

extern uint8_t ull_scan_lll_handle_get(struct lll_scan *lll);
extern struct lll_scan *ull_scan_lll_is_valid_get(struct lll_scan *lll);
extern struct lll_scan_aux *ull_scan_aux_lll_is_valid_get(struct lll_scan_aux *lll);
