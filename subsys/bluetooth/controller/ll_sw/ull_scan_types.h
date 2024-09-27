/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct ll_scan_set {
	struct ull_hdr  ull;
	struct lll_scan lll;

	uint32_t ticks_window;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	struct node_rx_pdu *node_rx_scan_term;
	uint16_t duration_lazy;

	uint8_t is_stop:1;
#endif /* CONFIG_BT_CTLR_ADV_EXT */

	uint8_t is_enabled:1;
	uint8_t own_addr_type:2;

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	struct {
		uint8_t filter_policy:1;
		uint8_t cancelled:1;
		uint8_t state:2;

		/* Non-Null when creating sync, reset in ISR context on
		 * synchronisation state and checked in Thread context when
		 * cancelling sync create, hence the volatile keyword.
		 */
		struct ll_sync_set *volatile sync;
	} periodic;
#endif
};

struct ll_scan_aux_chain {
	struct lll_scan_aux lll;

	/* lll_scan or lll_sync */
	void *volatile parent;

	/* Current nodes in this chain */
	/* TODO - do we need both head and tail? */
	struct node_rx_pdu *rx_head;
	struct node_rx_pdu *rx_last;

	/* current ticker timeout for this chain */
	uint32_t ticker_ticks;

	/* Next chain in list (if any) */
	struct ll_scan_aux_chain *next;

	/* Current total advertising data */
	uint16_t data_len;

	/* This chain is LLL scheduled */
	uint8_t is_lll_sched:1;
	/* Last emitted node_rx's aux_sched (only used in sync contexts) */
	uint8_t aux_sched:1;
};

struct ll_scan_aux_set {
	struct ull_hdr      ull;

#if defined(CONFIG_BT_CTLR_SCAN_AUX_USE_CHAINS)
	struct ll_scan_aux_chain *sched_chains;
	struct ll_scan_aux_chain *active_chains;
	struct ll_scan_aux_chain *flushing_chains;
#else /* !CONFIG_BT_CTLR_SCAN_AUX_USE_CHAINS */
	struct lll_scan_aux lll;

	/* lll_scan or lll_sync */
	void *volatile parent;

	struct node_rx_pdu *rx_head;
	struct node_rx_pdu *rx_last;

	uint16_t data_len;

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	struct node_rx_pdu *rx_incomplete;
#endif
#endif /* !CONFIG_BT_CTLR_SCAN_AUX_USE_CHAINS */
};
