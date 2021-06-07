/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>

#include <toolchain.h>
#include <sys/util.h>

#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"
#include "hal/radio_df.h"

#include "util/util.h"
#include "util/memq.h"

#include "pdu.h"

#include "lll.h"
#include "lll_vendor.h"
#include "lll_clock.h"
#include "lll_chan.h"
#include "lll_df_types.h"
#include "lll_sync.h"

#include "lll_internal.h"
#include "lll_tim_internal.h"
#include "lll_prof_internal.h"

#include "lll_df.h"
#include "lll_df_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_lll_sync
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

static int init_reset(void);
static int prepare_cb(struct lll_prepare_param *p);
static void abort_cb(struct lll_prepare_param *prepare_param, void *param);
static void isr_rx(void *param);

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
static inline int create_iq_report(struct lll_sync *lll, uint8_t rssi_ready,
				    uint8_t packet_status);
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

int lll_sync_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int lll_sync_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

void lll_sync_prepare(void *param)
{
	struct lll_prepare_param *p;
	struct lll_sync *lll;
	int err;

	/* Request to start HF Clock */
	err = lll_hfclock_on();
	LL_ASSERT(err >= 0);

	p = param;

	lll = p->param;

	/* Accumulate window widening */
	lll->window_widening_prepare_us += lll->window_widening_periodic_us *
					   (p->lazy + 1);
	if (lll->window_widening_prepare_us > lll->window_widening_max_us) {
		lll->window_widening_prepare_us = lll->window_widening_max_us;
	}

	/* Invoke common pipeline handling of prepare */
	err = lll_prepare(lll_is_abort_cb, abort_cb, prepare_cb, 0, p);
	LL_ASSERT(!err || err == -EINPROGRESS);
}

static int init_reset(void)
{
	return 0;
}

static int prepare_cb(struct lll_prepare_param *p)
{
	struct node_rx_pdu *node_rx;
	uint32_t ticks_at_event;
	uint32_t ticks_at_start;
	uint16_t event_counter;
	uint32_t remainder_us;
	uint8_t data_chan_use;
	struct lll_sync *lll;
	struct ull_hdr *ull;
	uint32_t remainder;
	uint32_t hcto;

	DEBUG_RADIO_START_O(1);

	lll = p->param;

	/* Calculate the current event latency */
	lll->skip_event = lll->skip_prepare + p->lazy;

	/* Calculate the current event counter value */
	event_counter = lll->event_counter + lll->skip_event;

	/* Update event counter to next value */
	lll->event_counter = (event_counter + 1);

	/* Reset accumulated latencies */
	lll->skip_prepare = 0;

	/* Current window widening */
	lll->window_widening_event_us += lll->window_widening_prepare_us;
	lll->window_widening_prepare_us = 0;
	if (lll->window_widening_event_us > lll->window_widening_max_us) {
		lll->window_widening_event_us =	lll->window_widening_max_us;
	}

	/* Calculate the radio channel to use */
	data_chan_use = lll_chan_sel_2(event_counter, lll->data_chan_id,
				       &lll->data_chan_map[0],
				       lll->data_chan_count);

	/* Start setting up Radio h/w */
	radio_reset();

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	radio_tx_power_set(lll->tx_pwr_lvl);
#else
	radio_tx_power_set(RADIO_TXP_DEFAULT);
#endif /* CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */

	radio_phy_set(lll->phy, 1);
	radio_pkt_configure(8, PDU_AC_PAYLOAD_SIZE_MAX, (lll->phy << 1));

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);

	radio_pkt_rx_set(node_rx->pdu);

	radio_aa_set(lll->access_addr);
	radio_crc_configure(((0x5bUL) | ((0x06UL) << 8) | ((0x00UL) << 16)),
			    (((uint32_t)lll->crc_init[2] << 16) |
			     ((uint32_t)lll->crc_init[1] << 8) |
			     ((uint32_t)lll->crc_init[0])));

	lll_chan_set(data_chan_use);

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
	struct lll_df_sync_cfg *cfg;

	cfg = lll_df_sync_cfg_latest_get(&lll->df_cfg, NULL);

	if (cfg->is_enabled) {
		lll_df_conf_cte_rx_enable(cfg->slot_durations, cfg->ant_sw_len, cfg->ant_ids,
					  data_chan_use);
		cfg->cte_count = 0;
	}
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

	radio_isr_set(isr_rx, lll);

	radio_tmr_tifs_set(EVENT_IFS_US);

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
	if (cfg->is_enabled) {
		radio_switch_complete_and_phy_end_disable();
	} else
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */
	{
		radio_switch_complete_and_disable();
	}

	ticks_at_event = p->ticks_at_expire;
	ull = HDR_LLL2ULL(lll);
	ticks_at_event += lll_event_offset_get(ull);

	ticks_at_start = ticks_at_event;
	ticks_at_start += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

	remainder = p->remainder;
	remainder_us = radio_tmr_start(0, ticks_at_start, remainder);

	radio_tmr_aa_capture();

	hcto = remainder_us +
	       ((EVENT_JITTER_US + EVENT_TICKER_RES_MARGIN_US +
		 lll->window_widening_event_us) << 1) +
	       lll->window_size_event_us;
	hcto += radio_rx_ready_delay_get(lll->phy, 1);
	hcto += addr_us_get(lll->phy);
	hcto += radio_rx_chain_delay_get(lll->phy, 1);
	radio_tmr_hcto_configure(hcto);

	radio_tmr_end_capture();
	radio_rssi_measure();

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	radio_gpio_lna_setup();

	radio_gpio_pa_lna_enable(remainder_us +
				 radio_rx_ready_delay_get(lll->phy, 1) -
				 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#endif /* CONFIG_BT_CTLR_GPIO_LNA_PIN */

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
	(EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
	/* check if preempt to start has changed */
	if (lll_preempt_calc(ull, (TICKER_ID_SCAN_SYNC_BASE +
				   ull_sync_lll_handle_get(lll)),
			     ticks_at_event)) {
		radio_isr_set(lll_isr_abort, lll);
		radio_disable();
	} else
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
	{
		uint32_t ret;

		ret = lll_prepare_done(lll);
		LL_ASSERT(!ret);
	}

	DEBUG_RADIO_START_O(1);
	return 0;
}

static void abort_cb(struct lll_prepare_param *prepare_param, void *param)
{
	struct lll_sync *lll;
	int err;

	/* NOTE: This is not a prepare being cancelled */
	if (!prepare_param) {
		/* Perform event abort here.
		 * After event has been cleanly aborted, clean up resources
		 * and dispatch event done.
		 */
		radio_isr_set(lll_isr_done, param);
		radio_disable();
		return;
	}

	/* NOTE: Else clean the top half preparations of the aborted event
	 * currently in preparation pipeline.
	 */
	err = lll_hfclock_off();
	LL_ASSERT(err >= 0);

	/* Accumulate the latency as event is aborted while being in pipeline */
	lll = prepare_param->param;
	lll->skip_prepare += (prepare_param->lazy + 1);

	lll_done(param);
}

static void isr_rx(void *param)
{
	struct event_done_extra *e;
	struct lll_sync *lll;
	uint8_t rssi_ready;
	uint8_t trx_done;
	uint8_t trx_cnt;
	uint8_t crc_ok;

	/* Read radio status and events */
	trx_done = radio_is_done();
	if (trx_done) {
		crc_ok = radio_crc_is_valid();
		rssi_ready = radio_rssi_is_ready();
	} else {
		crc_ok = rssi_ready = 0U;
	}

	/* Clear radio rx status and events */
	lll_isr_rx_status_reset();

	lll = param;

	/* No Rx */
	trx_cnt = 0U;
	if (!trx_done) {
		/* TODO: Combine the early exit with above if-then-else block
		 */
		goto isr_rx_done;
	}

	/* Rx-ed */
	trx_cnt = 1U;

	/* Check CRC and generate Periodic Advertising Report */
	if (crc_ok) {
		struct node_rx_pdu *node_rx;

		node_rx = ull_pdu_rx_alloc_peek(3);
		if (node_rx) {
			struct node_rx_ftr *ftr;

			ull_pdu_rx_alloc();

			node_rx->hdr.type = NODE_RX_TYPE_SYNC_REPORT;

			ftr = &(node_rx->hdr.rx_ftr);
			ftr->param = lll;
			ftr->rssi = (rssi_ready) ? radio_rssi_get() :
						   BT_HCI_LE_RSSI_NOT_AVAILABLE;
			ftr->ticks_anchor = radio_tmr_start_get();
			ftr->radio_end_us = radio_tmr_end_get() -
					    radio_rx_chain_delay_get(lll->phy,
								     1);

			ull_rx_put(node_rx->hdr.link, node_rx);

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
			create_iq_report(lll, rssi_ready, BT_HCI_LE_CTE_CRC_OK);

#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */
			ull_rx_sched();
		}
	}
#if defined(CONFIG_BT_CTLR_DF_SAMPLE_CTE_FOR_PDU_WITH_BAD_CRC)
	else {
		int err;

		err = create_iq_report(lll, rssi_ready,
				       BT_HCI_LE_CTE_CRC_ERR_CTE_BASED_TIME);
		if (!err) {
			ull_rx_sched();
		}
	}
#endif /* CONFIG_BT_CTLR_DF_SAMPLE_CTE_FOR_PDU_WITH_BAD_CRC */

isr_rx_done:
	/* Calculate and place the drift information in done event */
	e = ull_event_done_extra_get();
	LL_ASSERT(e);

	e->type = EVENT_DONE_EXTRA_TYPE_SYNC;
	e->trx_cnt = trx_cnt;
	e->crc_valid = crc_ok;

	e->drift.preamble_to_addr_us = addr_us_get(lll->phy);

	e->drift.start_to_address_actual_us = radio_tmr_aa_get() -
					      radio_tmr_ready_get();
	e->drift.window_widening_event_us = lll->window_widening_event_us;

	/* Reset window widening, as anchor point sync-ed */
	lll->window_widening_event_us = 0U;
	lll->window_size_event_us = 0U;

	lll_isr_cleanup(param);
}

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
static inline int create_iq_report(struct lll_sync *lll, uint8_t rssi_ready,
				   uint8_t packet_status)
{
	struct node_rx_iq_report *iq_report;
	struct lll_df_sync_cfg *cfg;
	struct node_rx_ftr *ftr;
	uint8_t sample_cnt;
	uint8_t cte_info;
	uint8_t ant;

	cfg = lll_df_sync_cfg_curr_get(&lll->df_cfg);

	if (cfg->is_enabled) {

		if (cfg->cte_count < cfg->max_cte_count) {
			sample_cnt = radio_df_iq_samples_amount_get();

			/* If there are no samples available, the CTEInfo was
			 * not detected and sampling was not started.
			 */
			if (sample_cnt > 0) {
				cte_info = radio_df_cte_status_get();
				ant = radio_df_pdu_antenna_switch_pattern_get();
				iq_report = ull_df_iq_report_alloc();

				iq_report->hdr.type = NODE_RX_TYPE_IQ_SAMPLE_REPORT;
				iq_report->sample_count = sample_cnt;
				iq_report->packet_status = packet_status;
				iq_report->rssi_ant_id = ant;
				iq_report->cte_info = *(struct pdu_cte_info *)&cte_info;
				iq_report->local_slot_durations = cfg->slot_durations;

				ftr = &iq_report->hdr.rx_ftr;
				ftr->param = lll;
				ftr->rssi = ((rssi_ready) ? radio_rssi_get() :
					     BT_HCI_LE_RSSI_NOT_AVAILABLE);

				ull_rx_put(iq_report->hdr.link, iq_report);
			} else {
				return -ENODATA;
			}
		}
	}

	return 0;
}
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */
