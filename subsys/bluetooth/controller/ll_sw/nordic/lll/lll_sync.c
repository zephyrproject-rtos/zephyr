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
#include "lll_scan.h"
#include "lll_sync.h"

#include "lll_internal.h"
#include "lll_tim_internal.h"
#include "lll_prof_internal.h"
#include "lll_scan_internal.h"

#include "lll_df.h"
#include "lll_df_internal.h"

#include "ll_feat.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_lll_sync
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

static int init_reset(void);
static void prepare(void *param);
static int create_prepare_cb(struct lll_prepare_param *p);
static int prepare_cb(struct lll_prepare_param *p);
static int prepare_cb_common(struct lll_prepare_param *p, uint8_t chan_idx);
static void abort_cb(struct lll_prepare_param *prepare_param, void *param);
static int isr_rx(struct lll_sync *lll, uint8_t node_type, uint8_t crc_ok, uint8_t rssi_ready,
		  enum sync_status status);
static void isr_rx_adv_sync_estab(void *param);
static void isr_rx_adv_sync(void *param);
static void isr_rx_aux_chain(void *param);
static void isr_rx_done_cleanup(struct lll_sync *lll, uint8_t crc_ok, bool sync_term);
static void isr_done(void *param);
#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
static int create_iq_report(struct lll_sync *lll, uint8_t rssi_ready,
			    uint8_t packet_status);
static bool is_max_cte_reached(uint8_t max_cte_count, uint8_t cte_count);
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */
static uint8_t data_channel_calc(struct lll_sync *lll);
static enum sync_status sync_filtrate_by_cte_type(uint8_t cte_type_mask, uint8_t filter_policy);

static uint8_t trx_cnt;

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

void lll_sync_create_prepare(void *param)
{
	int err;

	prepare(param);

	/* Invoke common pipeline handling of prepare */
	err = lll_prepare(lll_is_abort_cb, abort_cb, create_prepare_cb, 0, param);
	LL_ASSERT(!err || err == -EINPROGRESS);
}

void lll_sync_prepare(void *param)
{
	int err;

	prepare(param);

	/* Invoke common pipeline handling of prepare */
	err = lll_prepare(lll_is_abort_cb, abort_cb, prepare_cb, 0, param);
	LL_ASSERT(!err || err == -EINPROGRESS);
}

static void prepare(void *param)
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
}

void lll_sync_aux_prepare_cb(struct lll_sync *lll,
			     struct lll_scan_aux *lll_aux)
{
	struct node_rx_pdu *node_rx;

	/* Initialize Trx count */
	trx_cnt = 0U;

	/* Start setting up Radio h/w */
	radio_reset();

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	radio_tx_power_set(lll_aux->tx_pwr_lvl);
#else
	radio_tx_power_set(RADIO_TXP_DEFAULT);
#endif

	radio_phy_set(lll_aux->phy, 1);
	radio_pkt_configure(8, LL_EXT_OCTETS_RX_MAX, (lll_aux->phy << 1));

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);

	radio_pkt_rx_set(node_rx->pdu);

	/* Set access address for sync */
	radio_aa_set(lll->access_addr);
	radio_crc_configure(((0x5bUL) | ((0x06UL) << 8) | ((0x00UL) << 16)),
			    (((uint32_t)lll->crc_init[2] << 16) |
			     ((uint32_t)lll->crc_init[1] << 8) |
			     ((uint32_t)lll->crc_init[0])));

	lll_chan_set(lll_aux->chan);

	radio_isr_set(isr_rx_aux_chain, lll);

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
	struct lll_df_sync_cfg *cfg;

	cfg = lll_df_sync_cfg_latest_get(&lll->df_cfg, NULL);

	if (cfg->is_enabled) {
		lll_df_conf_cte_rx_enable(cfg->slot_durations, cfg->ant_sw_len,
					  cfg->ant_ids, lll_aux->chan);
		cfg->cte_count = 0;

		radio_switch_complete_and_phy_end_disable();
	} else
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */
	{
		radio_switch_complete_and_disable();
	}
}

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING)
enum sync_status lll_sync_cte_is_allowed(uint8_t cte_type_mask, uint8_t filter_policy,
					 uint8_t rx_cte_time, uint8_t rx_cte_type)
{
	bool cte_ok;

	if (cte_type_mask == BT_HCI_LE_PER_ADV_CREATE_SYNC_CTE_TYPE_NO_FILTERING) {
		return SYNC_STAT_ALLOWED;
	}

	if (rx_cte_time > 0) {
		if ((cte_type_mask & BT_HCI_LE_PER_ADV_CREATE_SYNC_CTE_TYPE_NO_CTE) != 0) {
			cte_ok = false;
		} else {
			switch (rx_cte_type) {
			case BT_HCI_LE_AOA_CTE:
				cte_ok = !(cte_type_mask &
					   BT_HCI_LE_PER_ADV_CREATE_SYNC_CTE_TYPE_NO_AOA);
				break;
			case BT_HCI_LE_AOD_CTE_1US:
				cte_ok = !(cte_type_mask &
					   BT_HCI_LE_PER_ADV_CREATE_SYNC_CTE_TYPE_NO_AOD_1US);
				break;
			case BT_HCI_LE_AOD_CTE_2US:
				cte_ok = !(cte_type_mask &
					   BT_HCI_LE_PER_ADV_CREATE_SYNC_CTE_TYPE_NO_AOD_2US);
				break;
			default:
				/* Unknown or forbidden CTE type */
				cte_ok = false;
			}
		}
	} else {
		/* If there is no CTEInfo in advertising PDU, Radio will not parse the S0 byte and
		 * CTESTATUS register will hold zeros only.
		 * Zero value in CTETime field of CTESTATUS may be used to distinguish between PDU
		 * that includes CTEInfo or not. Allowed range for CTETime is 2-20.
		 */
		if ((cte_type_mask & BT_HCI_LE_PER_ADV_CREATE_SYNC_CTE_TYPE_ONLY_CTE) != 0) {
			cte_ok = false;
		} else {
			cte_ok = true;
		}
	}

	if (!cte_ok) {
		return filter_policy ? SYNC_STAT_READY_OR_CONT_SCAN : SYNC_STAT_TERM;
	}

	return SYNC_STAT_ALLOWED;
}
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING */

static int init_reset(void)
{
	return 0;
}

static int create_prepare_cb(struct lll_prepare_param *p)
{
	uint16_t event_counter;
	struct lll_sync *lll;
	uint8_t chan_idx;
	int err;

	DEBUG_RADIO_START_O(1);

	lll = p->param;

	/* Calculate the current event latency */
	lll->skip_event = lll->skip_prepare + p->lazy;

	/* Calculate the current event counter value */
	event_counter = lll->event_counter + lll->skip_event;

	/* Reset accumulated latencies */
	lll->skip_prepare = 0;

	chan_idx = data_channel_calc(lll);

	/* Update event counter to next value */
	lll->event_counter = (event_counter + 1);

	err = prepare_cb_common(p, chan_idx);
	if (err) {
		DEBUG_RADIO_START_O(1);

		return 0;
	}

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
	struct lll_df_sync_cfg *cfg;

	cfg = lll_df_sync_cfg_latest_get(&lll->df_cfg, NULL);

	if (cfg->is_enabled) {
		lll_df_conf_cte_rx_enable(cfg->slot_durations, cfg->ant_sw_len, cfg->ant_ids,
					  chan_idx);
		cfg->cte_count = 0;

		radio_switch_complete_and_phy_end_disable();
	} else
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */
	{
		radio_df_cte_inline_set_enabled(false);
		radio_switch_complete_and_disable();
	}

	/* RSSI enable must be called after radio_switch_XXX function because it clears
	 * RADIO->SHORTS register, thus disables all other shortcuts.
	 */
	radio_rssi_measure();

	radio_isr_set(isr_rx_adv_sync_estab, lll);

	DEBUG_RADIO_START_O(1);

	return 0;
}

static int prepare_cb(struct lll_prepare_param *p)
{
	uint16_t event_counter;
	struct lll_sync *lll;
	uint8_t chan_idx;
	int err;

	DEBUG_RADIO_START_O(1);

	lll = p->param;

	/* Calculate the current event latency */
	lll->skip_event = lll->skip_prepare + p->lazy;

	/* Calculate the current event counter value */
	event_counter = lll->event_counter + lll->skip_event;

	/* Reset accumulated latencies */
	lll->skip_prepare = 0;

	chan_idx = data_channel_calc(lll);

	/* Update event counter to next value */
	lll->event_counter = (event_counter + 1);

	err = prepare_cb_common(p, chan_idx);
	if (err) {
		DEBUG_RADIO_START_O(1);
		return 0;
	}

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
	struct lll_df_sync_cfg *cfg;

	cfg = lll_df_sync_cfg_latest_get(&lll->df_cfg, NULL);

	if (cfg->is_enabled) {
		lll_df_conf_cte_rx_enable(cfg->slot_durations, cfg->ant_sw_len, cfg->ant_ids,
					  chan_idx);
		cfg->cte_count = 0;

		radio_switch_complete_and_phy_end_disable();
	} else
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */
	{
		radio_switch_complete_and_disable();
	}

	/* RSSI enable must be called after radio_switch_XXX function because it clears
	 * RADIO->SHORTS register, thus disables all other shortcuts.
	 */
	radio_rssi_measure();

	radio_isr_set(isr_rx_adv_sync, lll);

	DEBUG_RADIO_START_O(1);

	return 0;
}

static int prepare_cb_common(struct lll_prepare_param *p, uint8_t chan_idx)
{
	struct node_rx_pdu *node_rx;
	uint32_t ticks_at_event;
	uint32_t ticks_at_start;
	uint32_t remainder_us;
	struct lll_sync *lll;
	struct ull_hdr *ull;
	uint32_t remainder;
	uint32_t hcto;

	lll = p->param;

	/* Current window widening */
	lll->window_widening_event_us += lll->window_widening_prepare_us;
	lll->window_widening_prepare_us = 0;
	if (lll->window_widening_event_us > lll->window_widening_max_us) {
		lll->window_widening_event_us =	lll->window_widening_max_us;
	}

	/* Reset chain PDU being scheduled by lll_sync context */
	lll->is_aux_sched = 0U;

	/* Initialize Trx count */
	trx_cnt = 0U;

	/* Start setting up Radio h/w */
	radio_reset();

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	radio_tx_power_set(lll->tx_pwr_lvl);
#else
	radio_tx_power_set(RADIO_TXP_DEFAULT);
#endif /* CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */

	radio_phy_set(lll->phy, 1);
	radio_pkt_configure(8, LL_EXT_OCTETS_RX_MAX, (lll->phy << 1));
	radio_aa_set(lll->access_addr);
	radio_crc_configure(((0x5bUL) | ((0x06UL) << 8) | ((0x00UL) << 16)),
			    (((uint32_t)lll->crc_init[2] << 16) |
			     ((uint32_t)lll->crc_init[1] << 8) |
			     ((uint32_t)lll->crc_init[0])));

	lll_chan_set(chan_idx);

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);

	radio_pkt_rx_set(node_rx->pdu);

	ticks_at_event = p->ticks_at_expire;
	ull = HDR_LLL2ULL(lll);
	ticks_at_event += lll_event_offset_get(ull);

	ticks_at_start = ticks_at_event;
	ticks_at_start += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

	remainder = p->remainder;
	remainder_us = radio_tmr_start(0, ticks_at_start, remainder);

	radio_tmr_aa_capture();

	hcto = remainder_us +
	       ((EVENT_JITTER_US + EVENT_TICKER_RES_MARGIN_US + lll->window_widening_event_us)
		<< 1) +
	       lll->window_size_event_us;
	hcto += radio_rx_ready_delay_get(lll->phy, 1);
	hcto += addr_us_get(lll->phy);
	hcto += radio_rx_chain_delay_get(lll->phy, 1);
	radio_tmr_hcto_configure(hcto);

	radio_tmr_end_capture();

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
		radio_isr_set(isr_done, lll);
		radio_disable();

		return -ECANCELED;
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
		radio_isr_set(isr_done, param);
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

static void isr_aux_setup(void *param)
{
	struct pdu_adv_aux_ptr *aux_ptr;
	struct node_rx_pdu *node_rx;
	uint32_t window_widening_us;
	uint32_t window_size_us;
	struct node_rx_ftr *ftr;
	uint32_t aux_offset_us;
	uint32_t aux_start_us;
	struct lll_sync *lll;
	uint8_t phy_aux;
	uint32_t hcto;

	lll_isr_status_reset();

	node_rx = param;
	ftr = &node_rx->hdr.rx_ftr;
	aux_ptr = ftr->aux_ptr;
	phy_aux = BIT(aux_ptr->phy);
	ftr->aux_phy = phy_aux;

	lll = ftr->param;

	/* Determine the window size */
	if (aux_ptr->offs_units) {
		window_size_us = OFFS_UNIT_300_US;
	} else {
		window_size_us = OFFS_UNIT_30_US;
	}

	/* Calculate the aux offset from start of the scan window */
	aux_offset_us = (uint32_t) aux_ptr->offs * window_size_us;

	/* Calculate the window widening that needs to be deducted */
	if (aux_ptr->ca) {
		window_widening_us = SCA_DRIFT_50_PPM_US(aux_offset_us);
	} else {
		window_widening_us = SCA_DRIFT_500_PPM_US(aux_offset_us);
	}

	/* Setup radio for auxiliary PDU scan */
	radio_phy_set(phy_aux, 1);
	radio_pkt_configure(8, LL_EXT_OCTETS_RX_MAX, (phy_aux << 1));

	lll_chan_set(aux_ptr->chan_idx);

	radio_pkt_rx_set(node_rx->pdu);

	radio_isr_set(isr_rx_aux_chain, lll);

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
	struct lll_df_sync_cfg *cfg;

	cfg = lll_df_sync_cfg_latest_get(&lll->df_cfg, NULL);

	if (cfg->is_enabled && is_max_cte_reached(cfg->max_cte_count, cfg->cte_count)) {
		lll_df_conf_cte_rx_enable(cfg->slot_durations, cfg->ant_sw_len, cfg->ant_ids,
					  aux_ptr->chan_idx);

		radio_switch_complete_and_phy_end_disable();
	} else
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */
	{
		radio_switch_complete_and_disable();
	}

	/* Setup radio rx on micro second offset. Note that radio_end_us stores
	 * PDU start time in this case.
	 */
	aux_start_us = ftr->radio_end_us + aux_offset_us;
	aux_start_us -= lll_radio_rx_ready_delay_get(phy_aux, 1);
	aux_start_us -= window_widening_us;
	aux_start_us -= EVENT_JITTER_US;
	radio_tmr_start_us(0, aux_start_us);

	/* Setup header complete timeout */
	hcto = ftr->radio_end_us + aux_offset_us;
	hcto += window_size_us;
	hcto += window_widening_us;
	hcto += EVENT_JITTER_US;
	hcto += radio_rx_chain_delay_get(phy_aux, 1);
	hcto += addr_us_get(phy_aux);
	radio_tmr_hcto_configure(hcto);

	/* capture end of Rx-ed PDU, extended scan to schedule auxiliary
	 * channel chaining, create connection or to create periodic sync.
	 */
	radio_tmr_end_capture();

	/* scanner always measures RSSI */
	radio_rssi_measure();

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	radio_gpio_lna_setup();

	radio_gpio_pa_lna_enable(aux_start_us +
				 radio_rx_ready_delay_get(phy_aux, 1) -
				 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#endif /* CONFIG_BT_CTLR_GPIO_LNA_PIN */
}

/**
 * @brief Common part of ISR responsbile for handling PDU receive.
 *
 * @param lll        Pointer to LLL sync object.
 * @param node_type  Type of a receive node to be set for handling by ULL.
 * @param crc_ok     Informs if received PDU has correct CRC.
 * @param rssi_ready Informs if RSSI for received PDU is ready.
 * @param status     Informs about periodic advertisement synchronization status.
 *
 * @return Zero in case of there is no chained PDU or there is a chained PDUs but spaced long enough
 *         to schedule its reception by ULL.
 * @return -EBUSY in case there is a chained PDU scheduled by LLL due to short spacing.
 */
static int isr_rx(struct lll_sync *lll, uint8_t node_type, uint8_t crc_ok, uint8_t rssi_ready,
		  enum sync_status status)
{
	int err;

	/* Check CRC and generate Periodic Advertising Report */
	if (crc_ok) {
		struct node_rx_pdu *node_rx;

		node_rx = ull_pdu_rx_alloc_peek(3);
		if (node_rx) {
			struct node_rx_ftr *ftr;
			struct pdu_adv *pdu;

			ull_pdu_rx_alloc();

			node_rx->hdr.type = node_type;

			ftr = &(node_rx->hdr.rx_ftr);
			ftr->param = lll;
			ftr->aux_failed = 0U;
			ftr->rssi = (rssi_ready) ? radio_rssi_get() :
						   BT_HCI_LE_RSSI_NOT_AVAILABLE;
			ftr->ticks_anchor = radio_tmr_start_get();
			ftr->radio_end_us = radio_tmr_end_get() -
					    radio_rx_chain_delay_get(lll->phy,
								     1);
			ftr->sync_status = status;

			pdu = (void *)node_rx->pdu;

			ftr->aux_lll_sched = lll_scan_aux_setup(pdu, lll->phy,
								0,
								isr_aux_setup,
								lll);
			if (ftr->aux_lll_sched) {
				lll->is_aux_sched = 1U;
				err = -EBUSY;
			} else {
				err = 0;
			}

			ull_rx_put(node_rx->hdr.link, node_rx);

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
			(void)create_iq_report(lll, rssi_ready,
					       BT_HCI_LE_CTE_CRC_OK);
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

			ull_rx_sched();
		} else {
			err = 0;
		}
	} else {
#if defined(CONFIG_BT_CTLR_DF_SAMPLE_CTE_FOR_PDU_WITH_BAD_CRC)
		err = create_iq_report(lll, rssi_ready, BT_HCI_LE_CTE_CRC_ERR_CTE_BASED_TIME);
		if (!err) {
			ull_rx_sched();
		}
#endif /* CONFIG_BT_CTLR_DF_SAMPLE_CTE_FOR_PDU_WITH_BAD_CRC */
		err = 0;
	}

	return err;
}

static void isr_rx_adv_sync_estab(void *param)
{
	enum sync_status sync_ok;
	struct lll_sync *lll;
	uint8_t rssi_ready;
	uint8_t trx_done;
	uint8_t crc_ok;
	int err;

	lll = param;

	/* Read radio status and events */
	trx_done = radio_is_done();
	if (trx_done) {
		crc_ok = radio_crc_is_valid();
		rssi_ready = radio_rssi_is_ready();
		sync_ok = sync_filtrate_by_cte_type(lll->cte_type, lll->filter_policy);
		trx_cnt = 1U;
	} else {
		crc_ok = rssi_ready = 0U;
		/* Initiated as allowed, crc_ok takes precended during handling of PDU
		 * reception in the situation.
		 */
		sync_ok = SYNC_STAT_ALLOWED;
	}

	/* Clear radio rx status and events */
	lll_isr_rx_status_reset();

	/* No Rx */
	if (!trx_done) {
		/* TODO: Combine the early exit with above if-then-else block
		 */
		goto isr_rx_done;
	}

	/* Save radio ready and address capture timestamp for later use for
	 * drift compensation.
	 */
	radio_tmr_aa_save(radio_tmr_aa_get());
	radio_tmr_ready_save(radio_tmr_ready_get());

	/* Handle regular PDU reception if CTE type is acceptable */
	if (sync_ok == SYNC_STAT_ALLOWED) {
		err = isr_rx(lll, NODE_RX_TYPE_SYNC, crc_ok, rssi_ready, SYNC_STAT_ALLOWED);
		if (err == -EBUSY) {
			return;
		}
	} else if (sync_ok == SYNC_STAT_TERM) {
		struct node_rx_pdu *node_rx;

		/* Verify if there are free RX buffers for:
		 * - reporting just received PDU
		 * - a buffer for receiving data in a connection
		 * - a buffer for receiving empty PDU
		 */
		node_rx = ull_pdu_rx_alloc_peek(3);
		if (node_rx) {
			struct node_rx_ftr *ftr;

			ull_pdu_rx_alloc();

			node_rx->hdr.type = NODE_RX_TYPE_SYNC;

			ftr = &node_rx->hdr.rx_ftr;
			ftr->param = lll;
			ftr->sync_status = SYNC_STAT_TERM;

			ull_rx_put(node_rx->hdr.link, node_rx);
			ull_rx_sched();
		}
	}

isr_rx_done:
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING) && \
	defined(CONFIG_BT_CTLR_CTEINLINE_SUPPORT)
	isr_rx_done_cleanup(lll, crc_ok, sync_ok == SYNC_STAT_TERM);
#else
	isr_rx_done_cleanup(lll, crc_ok, false);
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING && CONFIG_BT_CTLR_CTEINLINE_SUPPORT */
}

static void isr_rx_adv_sync(void *param)
{
	struct lll_sync *lll;
	uint8_t rssi_ready;
	uint8_t trx_done;
	uint8_t crc_ok;
	int err;

	lll = param;

	/* Read radio status and events */
	trx_done = radio_is_done();
	if (trx_done) {
		crc_ok = radio_crc_is_valid();
		rssi_ready = radio_rssi_is_ready();
		trx_cnt = 1U;
	} else {
		crc_ok = rssi_ready = 0U;
	}

	/* Clear radio rx status and events */
	lll_isr_rx_status_reset();

	/* No Rx */
	if (!trx_done) {
		/* TODO: Combine the early exit with above if-then-else block
		 */
		goto isr_rx_done;
	}

	/* Save radio ready and address capture timestamp for later use for
	 * drift compensation.
	 */
	radio_tmr_aa_save(radio_tmr_aa_get());
	radio_tmr_ready_save(radio_tmr_ready_get());

	/* When periodic advertisement is synchronized, the CTEType may change. It should not
	 * affect sychronization even when new CTE type is not allowed by sync parameters.
	 * Hence the SYNC_STAT_READY is set.
	 */
	err = isr_rx(lll, NODE_RX_TYPE_SYNC_REPORT, crc_ok, rssi_ready,
		     SYNC_STAT_READY_OR_CONT_SCAN);
	if (err == -EBUSY) {
		return;
	}

isr_rx_done:
	isr_rx_done_cleanup(lll, crc_ok, false);
}

static void isr_rx_aux_chain(void *param)
{
	struct lll_scan_aux *lll_aux;
	struct lll_sync *lll;
	uint8_t rssi_ready;
	uint8_t trx_done;
	uint8_t crc_ok;
	int err;

	lll = param;
	lll_aux = lll->lll_aux;
	if (!lll_aux) {
		/* auxiliary context not assigned (yet) in ULL execution
		 * context, drop current reception and abort further chain PDU
		 * receptions, if any.
		 */
		lll_isr_status_reset();

		goto isr_rx_aux_chain_done;
	}

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

	/* No Rx */
	if (!trx_done) {
		/* TODO: Combine the early exit with above if-then-else block
		 */
		goto isr_rx_aux_chain_done;
	}

	/* When periodic advertisement is synchronized, the CTEType may change. It should not
	 * affect sychronization even when new CTE type is not allowed by sync parameters.
	 * Hence the SYNC_STAT_READY is set.
	 */
	err = isr_rx(lll, NODE_RX_TYPE_EXT_AUX_REPORT, crc_ok, rssi_ready,
		     SYNC_STAT_READY_OR_CONT_SCAN);

	if (err == -EBUSY) {
		return;
	}

	if (!crc_ok) {
		struct node_rx_pdu *node_rx;

		node_rx = ull_pdu_rx_alloc();
		LL_ASSERT(node_rx);

		node_rx->hdr.type = NODE_RX_TYPE_EXT_AUX_RELEASE;

		node_rx->hdr.rx_ftr.param = lll;
		node_rx->hdr.rx_ftr.aux_failed = 1U;

		ull_rx_put(node_rx->hdr.link, node_rx);
		ull_rx_sched();
	}

isr_rx_aux_chain_done:
	if (lll->is_aux_sched) {
		lll->is_aux_sched = 0U;

		isr_rx_done_cleanup(lll, 1U, false);
	} else {
		lll_isr_cleanup(lll);
	}
}

static void isr_rx_done_cleanup(struct lll_sync *lll, uint8_t crc_ok, bool sync_term)
{
	struct event_done_extra *e;

	/* Calculate and place the drift information in done event */
	e = ull_event_done_extra_get();
	LL_ASSERT(e);

	e->type = EVENT_DONE_EXTRA_TYPE_SYNC;
	e->trx_cnt = trx_cnt;
	e->crc_valid = crc_ok;
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING) && \
	defined(CONFIG_BT_CTLR_CTEINLINE_SUPPORT)
	e->sync_term = sync_term;
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING && CONFIG_BT_CTLR_CTEINLINE_SUPPORT */
	if (trx_cnt) {
		e->drift.preamble_to_addr_us = addr_us_get(lll->phy);
		e->drift.start_to_address_actual_us =
			radio_tmr_aa_restore() - radio_tmr_ready_restore();
		e->drift.window_widening_event_us = lll->window_widening_event_us;

		/* Reset window widening, as anchor point sync-ed */
		lll->window_widening_event_us = 0U;
		lll->window_size_event_us = 0U;
	}

	lll_isr_cleanup(lll);
}

static void isr_done(void *param)
{
	lll_isr_status_reset();
	isr_rx_done_cleanup(param, ((trx_cnt > 1U) ? 1U : 0U), false);
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
		if (is_max_cte_reached(cfg->max_cte_count, cfg->cte_count)) {
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

				cfg->cte_count += 1U;

				ull_rx_put(iq_report->hdr.link, iq_report);
			} else {
				return -ENODATA;
			}
		}
	}

	return 0;
}

static bool is_max_cte_reached(uint8_t max_cte_count, uint8_t cte_count)
{
	return max_cte_count == BT_HCI_LE_SAMPLE_CTE_ALL || cte_count < max_cte_count;
}
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

static uint8_t data_channel_calc(struct lll_sync *lll)
{
	uint8_t data_chan_count;
	uint8_t *data_chan_map;

	/* Process channel map update, if any */
	if (lll->chm_first != lll->chm_last) {
		uint16_t instant_latency;

		instant_latency = (lll->event_counter + lll->skip_event - lll->chm_instant) &
				  EVENT_INSTANT_MAX;
		if (instant_latency <= EVENT_INSTANT_LATENCY_MAX) {
			/* At or past the instant, use channelMapNew */
			lll->chm_first = lll->chm_last;
		}
	}

	/* Calculate the radio channel to use */
	data_chan_map = lll->chm[lll->chm_first].data_chan_map;
	data_chan_count = lll->chm[lll->chm_first].data_chan_count;
	return lll_chan_sel_2(lll->event_counter + lll->skip_event, lll->data_chan_id,
			      data_chan_map, data_chan_count);
}

static enum sync_status sync_filtrate_by_cte_type(uint8_t cte_type_mask, uint8_t filter_policy)
{
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING) && \
	defined(CONFIG_BT_CTLR_CTEINLINE_SUPPORT)
	uint8_t rx_cte_time;
	uint8_t rx_cte_type;

	rx_cte_time = nrf_radio_cte_time_get(NRF_RADIO);
	rx_cte_type = nrf_radio_cte_type_get(NRF_RADIO);

	return lll_sync_cte_is_allowed(cte_type_mask, filter_policy, rx_cte_time, rx_cte_type);

#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING && CONFIG_BT_CTLR_CTEINLINE_SUPPORT */
	return SYNC_STAT_ALLOWED;
}
