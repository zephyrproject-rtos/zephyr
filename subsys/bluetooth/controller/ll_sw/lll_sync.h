/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct lll_sync {
	struct lll_hdr hdr;

	uint8_t access_addr[4];
	uint8_t crc_init[3];

	uint16_t skip_prepare;
	uint16_t skip_event;
	uint16_t event_counter;

	uint8_t data_chan_map[5];
	uint8_t data_chan_count:6;
	uint16_t data_chan_id;

	uint32_t window_widening_periodic_us;
	uint32_t window_widening_max_us;
	uint32_t window_widening_prepare_us;
	uint32_t window_widening_event_us;
	uint32_t window_size_event_us;

	uint8_t phy:3;
	uint8_t is_rx_enabled:1;

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
	struct lll_df_sync df_cfg;
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */
};

int lll_sync_init(void);
int lll_sync_reset(void);
void lll_sync_prepare(void *param);

extern uint16_t ull_sync_lll_handle_get(struct lll_sync *lll);
