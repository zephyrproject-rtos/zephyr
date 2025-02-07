/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>

#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"
#include "hal/radio_df.h"

#include "util/util.h"
#include "util/memq.h"
#include "util/dbuf.h"

#include "pdu_df.h"
#include "pdu_vendor.h"
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

#include <zephyr/bluetooth/hci_types.h>

#include <soc.h>
#include "hal/debug.h"

static int init_reset(void);
static void prepare(void *param);
static int create_prepare_cb(struct lll_prepare_param *p);
static int prepare_cb(struct lll_prepare_param *p);
static int prepare_cb_common(struct lll_prepare_param *p, uint8_t chan_idx);
static int is_abort_cb(void *next, void *curr, lll_prepare_cb_t *resume_cb);
static void abort_cb(struct lll_prepare_param *prepare_param, void *param);
static int isr_rx(struct lll_sync *lll, uint8_t node_type, uint8_t crc_ok,
		  uint8_t phy_flags_rx, uint8_t cte_ready, uint8_t rssi_ready,
		  enum sync_status status);
static void isr_rx_adv_sync_estab(void *param);
static void isr_rx_adv_sync(void *param);
static void isr_rx_aux_chain(void *param);
static void isr_rx_done_cleanup(struct lll_sync *lll, uint8_t crc_ok, bool sync_term);
static void isr_done(void *param);
#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
static int iq_report_create_put(struct lll_sync *lll, uint8_t rssi_ready,
				uint8_t packet_status);
static int iq_report_incomplete_create_put(struct lll_sync *lll);
static void iq_report_incomplete_release_put(struct lll_sync *lll);
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
	err = lll_prepare(is_abort_cb, abort_cb, create_prepare_cb, 0, param);
	LL_ASSERT(!err || err == -EINPROGRESS);
}

void lll_sync_prepare(void *param)
{
	int err;

	prepare(param);

	/* Invoke common pipeline handling of prepare */
	err = lll_prepare(is_abort_cb, abort_cb, prepare_cb, 0, param);
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

	lll->lazy_prepare = p->lazy;

	/* Accumulate window widening */
	lll->window_widening_prepare_us += lll->window_widening_periodic_us *
					   (lll->lazy_prepare + 1U);
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

	radio_phy_set(lll_aux->phy, PHY_FLAGS_S8);
	radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT, LL_EXT_OCTETS_RX_MAX,
			    RADIO_PKT_CONF_PHY(lll_aux->phy));

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);

	radio_pkt_rx_set(node_rx->pdu);

	/* Set access address for sync */
	radio_aa_set(lll->access_addr);
	radio_crc_configure(PDU_CRC_POLYNOMIAL,
				sys_get_le24(lll->crc_init));

	lll_chan_set(lll_aux->chan);

	radio_isr_set(isr_rx_aux_chain, lll);

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
	struct lll_df_sync_cfg *cfg;

	cfg = lll_df_sync_cfg_latest_get(&lll->df_cfg, NULL);

	if (cfg->is_enabled) {
		int err;

		/* Prepare additional node for reporting insufficient memory for IQ samples
		 * reports.
		 */
		err = lll_df_iq_report_no_resources_prepare(lll);
		if (!err) {
			err = lll_df_conf_cte_rx_enable(cfg->slot_durations, cfg->ant_sw_len,
							cfg->ant_ids, lll_aux->chan,
							CTE_INFO_IN_PAYLOAD, lll_aux->phy);
			if (err) {
				lll->is_cte_incomplete = true;
			}
		} else {
			lll->is_cte_incomplete = true;
		}
		cfg->cte_count = 0;
	} else {
		/* If CTE reception is disabled, release additional node allocated to report
		 * insufficient memory for IQ samples.
		 */
		iq_report_incomplete_release_put(lll);
	}
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */
	radio_switch_complete_and_disable();
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
		return filter_policy ? SYNC_STAT_CONT_SCAN : SYNC_STAT_TERM;
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
	lll->skip_event = lll->skip_prepare + lll->lazy_prepare;

	/* Calculate the current event counter value */
	event_counter = lll->event_counter + lll->skip_event;

	/* Reset accumulated latencies */
	lll->skip_prepare = 0U;

	chan_idx = data_channel_calc(lll);

	/* Update event counter to next value */
	lll->event_counter = (event_counter + 1U);

	err = prepare_cb_common(p, chan_idx);
	if (err) {
		DEBUG_RADIO_START_O(1);

		return 0;
	}

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
	struct lll_df_sync_cfg *cfg;

	cfg = lll_df_sync_cfg_latest_get(&lll->df_cfg, NULL);
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

	if (false) {
#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
	} else if (cfg->is_enabled) {
		/* In case of call in create_prepare_cb, new sync event starts hence discard
		 * previous incomplete state.
		 */
		lll->is_cte_incomplete = false;

		/* Prepare additional node for reporting insufficient IQ report nodes issue */
		err = lll_df_iq_report_no_resources_prepare(lll);
		if (!err) {
			err = lll_df_conf_cte_rx_enable(cfg->slot_durations, cfg->ant_sw_len,
							cfg->ant_ids, chan_idx,
							CTE_INFO_IN_PAYLOAD, lll->phy);
			if (err) {
				lll->is_cte_incomplete = true;
			}
		} else {
			lll->is_cte_incomplete = true;
		}

		cfg->cte_count = 0;
	} else {
		/* If CTE reception is disabled, release additional node allocated to report
		 * insufficient memory for IQ samples.
		 */
		iq_report_incomplete_release_put(lll);
#else
	} else {
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */
		if (IS_ENABLED(CONFIG_BT_CTLR_DF)) {
			/* Disable CTE reception and sampling in Radio */
			radio_df_cte_inline_set_enabled(false);
		}
	}

	radio_switch_complete_and_disable();

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
	lll->skip_event = lll->skip_prepare + lll->lazy_prepare;

	/* Calculate the current event counter value */
	event_counter = lll->event_counter + lll->skip_event;

	/* Reset accumulated latencies */
	lll->skip_prepare = 0U;

	chan_idx = data_channel_calc(lll);

	/* Update event counter to next value */
	lll->event_counter = (event_counter + 1U);

	err = prepare_cb_common(p, chan_idx);
	if (err) {
		DEBUG_RADIO_START_O(1);

		return 0;
	}

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
	struct lll_df_sync_cfg *cfg;

	cfg = lll_df_sync_cfg_latest_get(&lll->df_cfg, NULL);

	if (cfg->is_enabled) {
		/* In case of call in prepare, new sync event starts hence discard previous
		 * incomplete state.
		 */
		lll->is_cte_incomplete = false;

		/* Prepare additional node for reporting insufficient IQ report nodes issue */
		err = lll_df_iq_report_no_resources_prepare(lll);
		if (!err) {
			err = lll_df_conf_cte_rx_enable(cfg->slot_durations, cfg->ant_sw_len,
							cfg->ant_ids, chan_idx,
							CTE_INFO_IN_PAYLOAD, lll->phy);
			if (err) {
				lll->is_cte_incomplete = true;
			}
		} else {
			lll->is_cte_incomplete = true;
		}
		cfg->cte_count = 0;
	} else {
		/* If CTE reception is disabled, release additional node allocated to report
		 * insufficient memory for IQ samples.
		 */
		iq_report_incomplete_release_put(lll);
	}
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

	radio_switch_complete_and_disable();

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
	uint32_t ret;

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

	radio_phy_set(lll->phy, PHY_FLAGS_S8);
	radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT, LL_EXT_OCTETS_RX_MAX,
			    RADIO_PKT_CONF_PHY(lll->phy));
	radio_aa_set(lll->access_addr);
	radio_crc_configure(PDU_CRC_POLYNOMIAL,
					sys_get_le24(lll->crc_init));

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
	hcto += radio_rx_ready_delay_get(lll->phy, PHY_FLAGS_S8);
	hcto += addr_us_get(lll->phy);
	hcto += radio_rx_chain_delay_get(lll->phy, PHY_FLAGS_S8);
	radio_tmr_hcto_configure(hcto);

	radio_tmr_end_capture();

#if defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
	radio_gpio_lna_setup();

	radio_gpio_pa_lna_enable(remainder_us +
				 radio_rx_ready_delay_get(lll->phy,
							  PHY_FLAGS_S8) -
				 HAL_RADIO_GPIO_LNA_OFFSET);
#endif /* HAL_RADIO_GPIO_HAVE_LNA_PIN */

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
	(EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
	uint32_t overhead;

	overhead = lll_preempt_calc(ull, (TICKER_ID_SCAN_SYNC_BASE + ull_sync_lll_handle_get(lll)),
				    ticks_at_event);
	/* check if preempt to start has changed */
	if (overhead) {
		LL_ASSERT_OVERHEAD(overhead);

		radio_isr_set(isr_done, lll);
		radio_disable();

		return -ECANCELED;
	}
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */

	ret = lll_prepare_done(lll);
	LL_ASSERT(!ret);

	DEBUG_RADIO_START_O(1);

	return 0;
}

static int is_abort_cb(void *next, void *curr, lll_prepare_cb_t *resume_cb)
{
	/* Sync context shall not resume when being preempted, i.e. they
	 * shall not use -EAGAIN as return value.
	 */
	ARG_UNUSED(resume_cb);

	/* Different radio event overlap */
	if (next != curr) {
		struct lll_scan_aux *lll_aux;
		struct lll_scan *lll;

		lll = ull_scan_lll_is_valid_get(next);
		if (lll) {
			/* Do not abort current periodic sync event as next
			 * event is a scan event.
			 */
			return 0;
		}

		lll_aux = ull_scan_aux_lll_is_valid_get(next);
		if (!IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC_SKIP_ON_SCAN_AUX) &&
		    lll_aux) {
			/* Do not abort current periodic sync event as next
			 * event is a scan aux event.
			 */
			return 0;
		}

#if defined(CONFIG_BT_CTLR_SCAN_AUX_SYNC_RESERVE_MIN)
		struct lll_sync *lll_sync_next;
		struct lll_sync *lll_sync_curr;

		lll_sync_next = ull_sync_lll_is_valid_get(next);
		if (!lll_sync_next) {
			lll_sync_curr = curr;

			/* Do not abort if near supervision timeout */
			if (lll_sync_curr->forced) {
				return 0;
			}

			/* Abort current event as next event is not a
			 * scan and not a scan aux event.
			 */
			return -ECANCELED;
		}

		lll_sync_curr = curr;
		if (lll_sync_curr->abort_count < lll_sync_next->abort_count) {
			if (lll_sync_curr->abort_count < UINT8_MAX) {
				lll_sync_curr->abort_count++;
			}

			/* Abort current event as next event has higher abort
			 * count.
			 */
			return -ECANCELED;
		}

		if (lll_sync_next->abort_count < UINT8_MAX) {
			lll_sync_next->abort_count++;
		}

#else /* !CONFIG_BT_CTLR_SCAN_AUX_SYNC_RESERVE_MIN */
		/* Abort current event as next event is not a
		 * scan and not a scan aux event.
		 */
		return -ECANCELED;
#endif /* !CONFIG_BT_CTLR_SCAN_AUX_SYNC_RESERVE_MIN */
	}

	/* Do not abort if current periodic sync event overlaps next interval
	 * or next event is a scan event.
	 */
	return 0;
}

static void abort_cb(struct lll_prepare_param *prepare_param, void *param)
{
	struct event_done_extra *e;
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
	lll->skip_prepare += (lll->lazy_prepare + 1U);

	/* Reset Sync context association with any Aux context as the chain reception is aborted. */
	lll->lll_aux = NULL;

	/* Extra done event, to check sync lost */
	e = ull_event_done_extra_get();
	LL_ASSERT(e);

	e->type = EVENT_DONE_EXTRA_TYPE_SYNC;
	e->trx_cnt = 0U;
	e->crc_valid = 0U;
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING) && \
	defined(CONFIG_BT_CTLR_CTEINLINE_SUPPORT)
	e->sync_term = 0U;
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING &&
	* CONFIG_BT_CTLR_CTEINLINE_SUPPORT
	*/

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
	uint32_t start_us;
	uint8_t phy_aux;
	uint32_t hcto;

	lll_isr_status_reset();

	node_rx = param;
	ftr = &node_rx->rx_ftr;
	aux_ptr = ftr->aux_ptr;
	phy_aux = BIT(PDU_ADV_AUX_PTR_PHY_GET(aux_ptr));
	ftr->aux_phy = phy_aux;

	lll = ftr->param;

	/* Determine the window size */
	if (aux_ptr->offs_units) {
		window_size_us = OFFS_UNIT_300_US;
	} else {
		window_size_us = OFFS_UNIT_30_US;
	}

	/* Calculate the aux offset from start of the scan window */
	aux_offset_us = (uint32_t) PDU_ADV_AUX_PTR_OFFSET_GET(aux_ptr) * window_size_us;

	/* Calculate the window widening that needs to be deducted */
	if (aux_ptr->ca) {
		window_widening_us = SCA_DRIFT_50_PPM_US(aux_offset_us);
	} else {
		window_widening_us = SCA_DRIFT_500_PPM_US(aux_offset_us);
	}

	/* Setup radio for auxiliary PDU scan */
	radio_phy_set(phy_aux, PHY_FLAGS_S8);
	radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT, LL_EXT_OCTETS_RX_MAX,
			    RADIO_PKT_CONF_PHY(phy_aux));

	lll_chan_set(aux_ptr->chan_idx);

	radio_pkt_rx_set(node_rx->pdu);

	radio_isr_set(isr_rx_aux_chain, lll);

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
	struct lll_df_sync_cfg *cfg;

	cfg = lll_df_sync_cfg_latest_get(&lll->df_cfg, NULL);

	if (cfg->is_enabled && is_max_cte_reached(cfg->max_cte_count, cfg->cte_count)) {
		int err;

		/* Prepare additional node for reporting insufficient memory for IQ samples
		 * reports.
		 */
		err = lll_df_iq_report_no_resources_prepare(lll);
		if (!err) {
			err = lll_df_conf_cte_rx_enable(cfg->slot_durations,
							cfg->ant_sw_len,
							cfg->ant_ids,
							aux_ptr->chan_idx,
							CTE_INFO_IN_PAYLOAD,
							PDU_ADV_AUX_PTR_PHY_GET(aux_ptr));
			if (err) {
				lll->is_cte_incomplete = true;
			}
		} else {
			lll->is_cte_incomplete = true;
		}
	} else if (!cfg->is_enabled) {
		/* If CTE reception is disabled, release additional node allocated to report
		 * insufficient memory for IQ samples.
		 */
		iq_report_incomplete_release_put(lll);
	}
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */
	radio_switch_complete_and_disable();

	/* Setup radio rx on micro second offset. Note that radio_end_us stores
	 * PDU start time in this case.
	 */
	aux_start_us = ftr->radio_end_us + aux_offset_us;
	aux_start_us -= lll_radio_rx_ready_delay_get(phy_aux, PHY_FLAGS_S8);
	aux_start_us -= window_widening_us;
	aux_start_us -= EVENT_JITTER_US;

	start_us = radio_tmr_start_us(0, aux_start_us);
	LL_ASSERT(start_us == (aux_start_us + 1U));

	/* Setup header complete timeout */
	hcto = start_us;
	hcto += EVENT_JITTER_US;
	hcto += window_widening_us;
	hcto += lll_radio_rx_ready_delay_get(phy_aux, PHY_FLAGS_S8);
	hcto += window_size_us;
	hcto += radio_rx_chain_delay_get(phy_aux, PHY_FLAGS_S8);
	hcto += addr_us_get(phy_aux);
	radio_tmr_hcto_configure_abs(hcto);

	/* capture end of Rx-ed PDU, extended scan to schedule auxiliary
	 * channel chaining, create connection or to create periodic sync.
	 */
	radio_tmr_end_capture();

	/* scanner always measures RSSI */
	radio_rssi_measure();

#if defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
	radio_gpio_lna_setup();

	radio_gpio_pa_lna_enable(start_us +
				 radio_rx_ready_delay_get(phy_aux,
							  PHY_FLAGS_S8) -
				 HAL_RADIO_GPIO_LNA_OFFSET);
#endif /* HAL_RADIO_GPIO_HAVE_LNA_PIN */
}

/**
 * @brief Common part of ISR responsible for handling PDU receive.
 *
 * @param lll          Pointer to LLL sync object.
 * @param node_type    Type of a receive node to be set for handling by ULL.
 * @param crc_ok       Informs if received PDU has correct CRC.
 * @param phy_flags_rx Received Coded PHY coding scheme (0 - S1, 1 - S8).
 * @param cte_ready    Informs if received PDU has CTEInfo present and IQ samples were collected.
 * @param rssi_ready   Informs if RSSI for received PDU is ready.
 * @param status       Informs about periodic advertisement synchronization status.
 *
 * @return Zero in case of there is no chained PDU or there is a chained PDUs but spaced long enough
 *         to schedule its reception by ULL.
 * @return -EBUSY in case there is a chained PDU scheduled by LLL due to short spacing.
 */
static int isr_rx(struct lll_sync *lll, uint8_t node_type, uint8_t crc_ok,
		  uint8_t phy_flags_rx, uint8_t cte_ready, uint8_t rssi_ready,
		  enum sync_status status)
{
	bool sched = false;
	int err;

	/* Check CRC and generate Periodic Advertising Report */
	if (crc_ok) {
		struct node_rx_pdu *node_rx;

		/* Verify if there are free RX buffers for:
		 * - reporting just received PDU
		 * - allocating an extra node_rx for periodic report incomplete
		 * - a buffer for receiving data in a connection
		 * - a buffer for receiving empty PDU
		 *
		 * If this is a reception of chained PDU, node_type is
		 * NODE_RX_TYPE_EXT_AUX_REPORT, then there is no need to reserve
		 * again a node_rx for periodic report incomplete.
		 */
		if (node_type != NODE_RX_TYPE_EXT_AUX_REPORT) {
			/* Reset Sync context association with any Aux context
			 * as a new chain is being setup for reception here.
			 */
			lll->lll_aux = NULL;

			node_rx = ull_pdu_rx_alloc_peek(4);
		} else {
			node_rx = ull_pdu_rx_alloc_peek(3);
		}

		if (node_rx) {
			struct node_rx_ftr *ftr;
			struct pdu_adv *pdu;

			ull_pdu_rx_alloc();

			node_rx->hdr.type = node_type;

			ftr = &(node_rx->rx_ftr);
			ftr->param = lll;
			ftr->lll_aux = lll->lll_aux;
			ftr->aux_failed = 0U;
			ftr->rssi = (rssi_ready) ? radio_rssi_get() :
						   BT_HCI_LE_RSSI_NOT_AVAILABLE;
			ftr->ticks_anchor = radio_tmr_start_get();
			ftr->radio_end_us = radio_tmr_end_get() -
					    radio_rx_chain_delay_get(lll->phy,
								     phy_flags_rx);
			ftr->phy_flags = phy_flags_rx;
			ftr->sync_status = status;
			ftr->sync_rx_enabled = lll->is_rx_enabled;

			if (node_type != NODE_RX_TYPE_EXT_AUX_REPORT) {
				ftr->extra = ull_pdu_rx_alloc();
			}

			pdu = (void *)node_rx->pdu;

			ftr->aux_lll_sched = lll_scan_aux_setup(pdu, lll->phy,
								phy_flags_rx,
								isr_aux_setup,
								lll);
			if (ftr->aux_lll_sched) {
				if (node_type != NODE_RX_TYPE_EXT_AUX_REPORT) {
					lll->is_aux_sched = 1U;
				}

				err = -EBUSY;
			} else {
				err = 0;
			}

			ull_rx_put(node_rx->hdr.link, node_rx);

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
			if (cte_ready) {
				/* If there is a periodic advertising report generate  IQ data
				 * report with valid packet_status if there were free nodes for
				 * that. Or report insufficient resources for IQ data report.
				 *
				 * Returned value is not checked because it does not matter if there
				 * is a IQ report to be send towards ULL. There is always periodic
				 * sync report to be send.
				 */
				(void)iq_report_create_put(lll, rssi_ready, BT_HCI_LE_CTE_CRC_OK);
			}
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

			sched = true;
		} else {
			if (node_type == NODE_RX_TYPE_EXT_AUX_REPORT) {
				err = -ENOMEM;
			} else {
				err = 0;
			}
		}
	} else {
#if defined(CONFIG_BT_CTLR_DF_SAMPLE_CTE_FOR_PDU_WITH_BAD_CRC)
		/* In case of reception of chained PDUs IQ samples report for a PDU with wrong
		 * CRC is handled by caller. It has to be that way to be sure the IQ report
		 * follows possible periodic advertising report.
		 */
		if (cte_ready && node_type != NODE_RX_TYPE_EXT_AUX_REPORT) {
			err = iq_report_create_put(lll, rssi_ready,
						   BT_HCI_LE_CTE_CRC_ERR_CTE_BASED_TIME);
			if (!err) {
				sched = true;
			}
		}
#endif /* CONFIG_BT_CTLR_DF_SAMPLE_CTE_FOR_PDU_WITH_BAD_CRC */

		err = 0;
	}

	if (sched) {
		ull_rx_sched();
	}

	return err;
}

static void isr_rx_adv_sync_estab(void *param)
{
	enum sync_status sync_ok;
	struct lll_sync *lll;
	uint8_t phy_flags_rx;
	uint8_t rssi_ready;
	uint8_t cte_ready;
	uint8_t trx_done;
	uint8_t crc_ok;
	int err;

	lll = param;

	/* Read radio status and events */
	trx_done = radio_is_done();
	if (trx_done) {
		crc_ok = radio_crc_is_valid();
		rssi_ready = radio_rssi_is_ready();
		phy_flags_rx = radio_phy_flags_rx_get();
		sync_ok = sync_filtrate_by_cte_type(lll->cte_type, lll->filter_policy);
		trx_cnt = 1U;

		if (IS_ENABLED(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)) {
			cte_ready = radio_df_cte_ready();
		} else {
			cte_ready = 0U;
		}
	} else {
		crc_ok = phy_flags_rx = rssi_ready = cte_ready = 0U;
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
#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
		LL_ASSERT(!lll->node_cte_incomplete);
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

		goto isr_rx_done;
	}

	/* Save radio ready and address capture timestamp for later use for
	 * drift compensation.
	 */
	radio_tmr_aa_save(radio_tmr_aa_get());
	radio_tmr_ready_save(radio_tmr_ready_get());

	/* Handle regular PDU reception if CTE type is acceptable */
	if (sync_ok == SYNC_STAT_ALLOWED) {
		err = isr_rx(lll, NODE_RX_TYPE_SYNC, crc_ok, phy_flags_rx,
			     cte_ready, rssi_ready, SYNC_STAT_ALLOWED);
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

			ftr = &node_rx->rx_ftr;
			ftr->param = lll;
			ftr->sync_status = SYNC_STAT_TERM;

			ull_rx_put_sched(node_rx->hdr.link, node_rx);
		}
	}

isr_rx_done:
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING) && \
	defined(CONFIG_BT_CTLR_CTEINLINE_SUPPORT)
	isr_rx_done_cleanup(lll, crc_ok, sync_ok != SYNC_STAT_ALLOWED);
#else
	isr_rx_done_cleanup(lll, crc_ok, false);
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING && CONFIG_BT_CTLR_CTEINLINE_SUPPORT */
}

static void isr_rx_adv_sync(void *param)
{
	struct lll_sync *lll;
	uint8_t phy_flags_rx;
	uint8_t rssi_ready;
	uint8_t cte_ready;
	uint8_t trx_done;
	uint8_t crc_ok;
	int err;

	lll = param;

	/* Read radio status and events */
	trx_done = radio_is_done();
	if (trx_done) {
		crc_ok = radio_crc_is_valid();
		rssi_ready = radio_rssi_is_ready();
		phy_flags_rx = radio_phy_flags_rx_get();
		trx_cnt = 1U;

		if (IS_ENABLED(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)) {
			cte_ready = radio_df_cte_ready();
		} else {
			cte_ready = 0U;
		}
	} else {
		crc_ok = phy_flags_rx = rssi_ready = cte_ready = 0U;
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
	 * affect synchronization even when new CTE type is not allowed by sync parameters.
	 * Hence the SYNC_STAT_READY is set.
	 */
	err = isr_rx(lll, NODE_RX_TYPE_SYNC_REPORT, crc_ok, phy_flags_rx, cte_ready, rssi_ready,
		     SYNC_STAT_READY);
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
	uint8_t phy_flags_rx;
	uint8_t rssi_ready;
	uint8_t cte_ready;
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

		rssi_ready = 0U;
		cte_ready = 0U;
		crc_ok =  0U;
		err = 0;

		goto isr_rx_aux_chain_done;
	}

	/* Read radio status and events */
	trx_done = radio_is_done();
	if (trx_done) {
		crc_ok = radio_crc_is_valid();
		phy_flags_rx = radio_phy_flags_rx_get();
		rssi_ready = radio_rssi_is_ready();

		if (IS_ENABLED(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)) {
			cte_ready = radio_df_cte_ready();
		} else {
			cte_ready = 0U;
		}
	} else {
		crc_ok = phy_flags_rx = rssi_ready = cte_ready = 0U;
	}

	/* Clear radio rx status and events */
	lll_isr_rx_status_reset();

	/* No Rx */
	if (!trx_done) {
		/* TODO: Combine the early exit with above if-then-else block
		 */

		err = 0;

		goto isr_rx_aux_chain_done;
	}

	/* When periodic advertisement is synchronized, the CTEType may change. It should not
	 * affect synchronization even when new CTE type is not allowed by sync parameters.
	 * Hence the SYNC_STAT_READY is set.
	 */
	err = isr_rx(lll, NODE_RX_TYPE_EXT_AUX_REPORT, crc_ok, phy_flags_rx, cte_ready, rssi_ready,
		     SYNC_STAT_READY);
	if (err == -EBUSY) {
		return;
	}

isr_rx_aux_chain_done:
	if (!crc_ok || err) {
		struct node_rx_pdu *node_rx;

		/* Generate message to release aux context and flag the report
		 * generated thereafter by HCI as incomplete.
		 */
		node_rx = ull_pdu_rx_alloc();
		LL_ASSERT(node_rx);

		node_rx->hdr.type = NODE_RX_TYPE_EXT_AUX_RELEASE;

		node_rx->rx_ftr.param = lll;
		node_rx->rx_ftr.aux_failed = 1U;

		ull_rx_put(node_rx->hdr.link, node_rx);

		if (!crc_ok) {
#if defined(CONFIG_BT_CTLR_DF_SAMPLE_CTE_FOR_PDU_WITH_BAD_CRC)
			if (cte_ready) {
				(void)iq_report_create_put(lll, rssi_ready,
							   BT_HCI_LE_CTE_CRC_ERR_CTE_BASED_TIME);
			}
#endif /* CONFIG_BT_CTLR_DF_SAMPLE_CTE_FOR_PDU_WITH_BAD_CRC */
		} else {
#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
			/* Report insufficient resources for IQ data report and release additional
			 * noder_rx_iq_data stored in lll_sync object, to avoid buffers leakage.
			 */
			iq_report_incomplete_create_put(lll);
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */
		}

		ull_rx_sched();
	}

	if (lll->is_aux_sched) {
		lll->is_aux_sched = 0U;

		isr_rx_done_cleanup(lll, 1U, false);
	} else {
		lll_isr_cleanup(lll_aux);
	}
}

static void isr_rx_done_cleanup(struct lll_sync *lll, uint8_t crc_ok, bool sync_term)
{
	struct event_done_extra *e;

	/* Reset Sync context association with any Aux context as the chain reception is done. */
	lll->lll_aux = NULL;

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

#if defined(CONFIG_BT_CTLR_SCAN_AUX_SYNC_RESERVE_MIN)
		/* Reset LLL abort count as LLL event is gracefully done and
		 * was not aborted by any other event when current event could
		 * have been using unreserved time space.
		 */
		lll->abort_count = 0U;
#endif /* CONFIG_BT_CTLR_SCAN_AUX_SYNC_RESERVE_MIN */
	}

	lll_isr_cleanup(lll);
}

static void isr_done(void *param)
{
	struct lll_sync *lll;

	lll_isr_status_reset();

	/* Generate incomplete data status and release aux context when
	 * sync event is using LLL scheduling.
	 */
	lll = param;

	/* LLL scheduling used for chain PDU reception is aborted/preempted */
	if (lll->is_aux_sched) {
		struct node_rx_pdu *node_rx;

		lll->is_aux_sched = 0U;

		/* Generate message to release aux context and flag the report
		 * generated thereafter by HCI as incomplete.
		 */
		node_rx = ull_pdu_rx_alloc();
		LL_ASSERT(node_rx);

		node_rx->hdr.type = NODE_RX_TYPE_EXT_AUX_RELEASE;

		node_rx->rx_ftr.param = lll;
		node_rx->rx_ftr.aux_failed = 1U;

		ull_rx_put_sched(node_rx->hdr.link, node_rx);
	}

	isr_rx_done_cleanup(param, ((trx_cnt) ? 1U : 0U), false);
}

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
static void iq_report_create(struct lll_sync *lll, uint8_t rssi_ready, uint8_t packet_status,
			     uint8_t slot_durations, struct node_rx_iq_report *iq_report)
{
	struct node_rx_ftr *ftr;
	uint8_t cte_info;
	uint8_t ant;

	cte_info = radio_df_cte_status_get();
	ant = radio_df_pdu_antenna_switch_pattern_get();

	iq_report->rx.hdr.type = NODE_RX_TYPE_SYNC_IQ_SAMPLE_REPORT;
	iq_report->sample_count = radio_df_iq_samples_amount_get();
	iq_report->packet_status = packet_status;
	iq_report->rssi_ant_id = ant;
	iq_report->cte_info = *(struct pdu_cte_info *)&cte_info;
	iq_report->local_slot_durations = slot_durations;
	/* Event counter is updated to next value during event preparation, hence
	 * it has to be subtracted to store actual event counter value.
	 */
	iq_report->event_counter = lll->event_counter - 1;

	ftr = &iq_report->rx.rx_ftr;
	ftr->param = lll;
	ftr->rssi =
		((rssi_ready) ? radio_rssi_get() : BT_HCI_LE_RSSI_NOT_AVAILABLE);
}

static void iq_report_incomplete_create(struct lll_sync *lll, struct node_rx_iq_report *iq_report)
{
	struct node_rx_ftr *ftr;

	iq_report->rx.hdr.type = NODE_RX_TYPE_SYNC_IQ_SAMPLE_REPORT;
	iq_report->sample_count = 0;
	iq_report->packet_status = BT_HCI_LE_CTE_INSUFFICIENT_RESOURCES;
	/* Event counter is updated to next value during event preparation,
	 * hence it has to be subtracted to store actual event counter
	 * value.
	 */
	iq_report->event_counter = lll->event_counter - 1;
	/* The PDU antenna is set in configuration, hence it is always
	 * available. BT 5.3 Core Spec. does not say if this field
	 * may be invalid in case of insufficient resources.
	 */
	iq_report->rssi_ant_id = radio_df_pdu_antenna_switch_pattern_get();
	/* According to BT 5.3, Vol 4, Part E, section 7.7.65.21 below
	 * fields have invalid values in case of insufficient resources.
	 */
	iq_report->cte_info =
		(struct pdu_cte_info){.time = 0, .rfu = 0, .type = 0};
	iq_report->local_slot_durations = 0;

	ftr = &iq_report->rx.rx_ftr;
	ftr->param = lll;

	ftr->rssi = BT_HCI_LE_RSSI_NOT_AVAILABLE;
	ftr->extra = NULL;
}

static int iq_report_create_put(struct lll_sync *lll, uint8_t rssi_ready, uint8_t packet_status)
{
	struct node_rx_iq_report *iq_report;
	struct lll_df_sync_cfg *cfg;
	int err;

	cfg = lll_df_sync_cfg_curr_get(&lll->df_cfg);

	if (cfg->is_enabled) {
		if (!lll->is_cte_incomplete &&
		    is_max_cte_reached(cfg->max_cte_count, cfg->cte_count)) {
			iq_report = ull_df_iq_report_alloc();
			LL_ASSERT(iq_report);

			iq_report_create(lll, rssi_ready, packet_status,
					 cfg->slot_durations, iq_report);
			err = 0;
		} else if (lll->is_cte_incomplete && is_max_cte_reached(cfg->max_cte_count,
									 cfg->cte_count)) {
			iq_report = lll->node_cte_incomplete;

			/* Reception of chained PDUs may be still in progress. Do not report
			 * insufficient resources multiple times.
			 */
			if (iq_report) {
				iq_report_incomplete_create(lll, iq_report);
				lll->node_cte_incomplete = NULL;

				/* Report ready to be send to ULL */
				err = 0;
			} else {
				/* Incomplete CTE was already reported */
				err = -ENODATA;
			}
		} else {
			err = -ENODATA;
		}
	} else {
		err = -ENODATA;
	}

	if (!err) {
		ull_rx_put(iq_report->rx.hdr.link, iq_report);

		cfg->cte_count += 1U;
	}

	return err;
}

static int iq_report_incomplete_create_put(struct lll_sync *lll)
{
	struct lll_df_sync_cfg *cfg;

	cfg = lll_df_sync_cfg_curr_get(&lll->df_cfg);

	if (cfg->is_enabled) {
		struct node_rx_iq_report *iq_report;

		iq_report = lll->node_cte_incomplete;

		/* Reception of chained PDUs may be still in progress. Do not report
		 * insufficient resources multiple times.
		 */
		if (iq_report) {
			iq_report_incomplete_create(lll, iq_report);

			lll->node_cte_incomplete = NULL;
			ull_rx_put(iq_report->rx.hdr.link, iq_report);

			return 0;
		} else {
			/* Incomplete CTE was already reported */
			return -ENODATA;
		}

	}

	return -ENODATA;
}

static void iq_report_incomplete_release_put(struct lll_sync *lll)
{
	if (lll->node_cte_incomplete) {
		struct node_rx_iq_report *iq_report = lll->node_cte_incomplete;

		iq_report->rx.hdr.type = NODE_RX_TYPE_IQ_SAMPLE_REPORT_LLL_RELEASE;

		ull_rx_put(iq_report->rx.hdr.link, iq_report);
		lll->node_cte_incomplete = NULL;
	}
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
