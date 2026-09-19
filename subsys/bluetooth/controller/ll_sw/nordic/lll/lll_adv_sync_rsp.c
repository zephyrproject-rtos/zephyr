/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <soc.h>

#include <zephyr/sys/byteorder.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"
#include "hal/radio_df.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/dbuf.h"

#include "pdu_df.h"
#include "pdu_vendor.h"
#include "pdu.h"
#include "ull_adv_internal.h"
#include "lll.h"
#include "lll_vendor.h"
#include "lll_clock.h"
#include "lll_chan.h"
#include "lll_adv_types.h"
#include "lll_adv.h"
#include "lll_adv_pdu.h"
#include "lll_adv_sync.h"
#include "lll_adv_iso.h"
#include "lll_df_types.h"

#include "lll_internal.h"
#include "lll_adv_internal.h"
#include "lll_tim_internal.h"
#include "lll_prof_internal.h"
#include "lll_df_internal.h"
#include "ll_feat.h"

#include "ull_adv_types.h"

#include "hal/debug.h"



static int prepare_cb(struct lll_prepare_param *p);
static void abort_cb(struct lll_prepare_param *prepare_param, void *param);
static void isr_event_done(void *param);
static bool is_instant_or_past(uint16_t event_counter, uint16_t instant);
static void isr_subevent_tx_done(void *param);
static void isr_rx_response_slot(void *param);
static uint8_t pawr_data_chan_calc(struct lll_adv_sync *lll,
                                   uint16_t event_counter,
                                   uint8_t subevent);

uint32_t rsp_slot_anchor_us;
void lll_adv_sync_rsp_prepare(void *param)
{
	int err;

	err = lll_hfclock_on();
	LL_ASSERT_ERR(err >= 0);

	err = lll_prepare(lll_is_abort_cb, abort_cb, prepare_cb, 0, param);
	LL_ASSERT_ERR(!err || err == -EINPROGRESS);
}

static bool is_instant_or_past(uint16_t event_counter, uint16_t instant)
{
	uint16_t instant_latency;

	instant_latency = (event_counter - instant) & EVENT_INSTANT_MAX;

	return instant_latency <= EVENT_INSTANT_LATENCY_MAX;
}

static int prepare_cb(struct lll_prepare_param *p)
{
	struct ll_adv_sync_set *sync;
	struct lll_adv_sync *lll;
	struct pdu_adv *pdu;
	struct ull_hdr *ull;
	uint32_t ticks_at_event;
	uint32_t ticks_at_start;
	uint16_t event_counter;
	uint32_t remainder;
	uint32_t start_us;
	//uint32_t delay_us;
	uint8_t phy_s;
	uint8_t subevent;
	uint8_t is_modified;
	uint32_t ret;
	uint8_t data_chan_count;
	uint8_t *data_chan_map;
	uint8_t data_chan_use;
	struct subevent_data_meta *se;

	DEBUG_RADIO_START_A(1);

	lll = p->param;
	lll->latency_event = lll->latency_prepare + p->lazy;

	event_counter = lll->event_counter + lll->latency_event;
	lll->event_counter = (event_counter + 1);

	lll->latency_prepare = 0;
	lll->subevent_curr = 0U;
	lll->rsp_slot_curr = 0U;

	rsp_slot_anchor_us = 0U;

	if ((lll->chm_first != lll->chm_last) &&
	    is_instant_or_past(event_counter, lll->chm_instant)) {
		lll->chm_first = lll->chm_last;
	}
	// data_chan_map = lll->chm[lll->chm_first].data_chan_map;
	// data_chan_count = lll->chm[lll->chm_first].data_chan_count;
	data_chan_use = pawr_data_chan_calc(lll,event_counter,lll->subevent_curr);
	// data_chan_use = lll_chan_sel_2(event_counter, lll->data_chan_id,
	// 			       data_chan_map, data_chan_count);

	radio_reset();
#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	radio_tx_power_set(lll->adv->tx_pwr_lvl);
#else
	radio_tx_power_set(RADIO_TXP_DEFAULT);
#endif

	phy_s = lll->adv->phy_s;
	radio_phy_set(phy_s, lll->adv->phy_flags);
	radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT, PDU_AC_PAYLOAD_SIZE_MAX,
			    RADIO_PKT_CONF_PHY(phy_s));
	radio_aa_set(lll->access_addr);
	radio_crc_configure(PDU_CRC_POLYNOMIAL, sys_get_le24(lll->crc_init));
	lll_chan_set(data_chan_use);

	se = &lll->se_data[lll->subevent_curr];
	if (!se->is_data_set) {
		//lll_adv_sync_data_curr_get(lll);
		/** TODO: empty air packet */
		//pdu = empty_adv_sync_rsp_packet(lll_adv_sync_data_curr_get(lll));
		//pdu->

	}
	pdu = lll_adv_pdu_latest_get(&se->data, &is_modified);

	//delay_us = (uint32_t)lll->response_slot_delay * 1250U;
	radio_pkt_tx_set(pdu);
	radio_isr_set(isr_subevent_tx_done, lll);
	//radio_tmr_tifs_set(delay_us);
	// phy_s = lll->adv->phy_s;
	// radio_switch_complete_and_rx(phy_s);//switch to rx
	//radio_switch_complete_and_disable();
	if (IS_ENABLED(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER) &&
		IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT)) {
		/* Required under single time tIFS switching, to accumulate the packet
			* timer value at the time of clear on radio end.
			*/
		radio_switch_complete_end_capture_and_disable();
	} else {
		radio_switch_complete_and_disable();
	}

	ticks_at_event = p->ticks_at_expire;
	ull = HDR_LLL2ULL(lll);
	ticks_at_event += lll_event_offset_get(ull);

	ticks_at_start = ticks_at_event;
	ticks_at_start += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

	remainder = p->remainder;
	start_us = radio_tmr_start(1, ticks_at_start, remainder);

	/* capture end of Tx-ed PDU, used to calculate HCTO. */
	radio_tmr_end_capture();

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
	radio_gpio_pa_setup();

	radio_gpio_pa_lna_enable(start_us + radio_tx_ready_delay_get(lll->adv->phy_s, 1) -
				 HAL_RADIO_GPIO_PA_OFFSET);
#else /* !HAL_RADIO_GPIO_HAVE_PA_PIN */
	ARG_UNUSED(start_us);
#endif /* !HAL_RADIO_GPIO_HAVE_PA_PIN */

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
	(EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
	uint32_t overhead;

	overhead = lll_preempt_calc(ull, (TICKER_ID_ADV_SYNC_BASE +
					  ull_adv_sync_lll_handle_get(lll)), ticks_at_event);
	if (overhead) {
		LL_ASSERT_OVERHEAD(overhead);

		radio_isr_set(lll_isr_abort, lll);
		radio_disable();

		return -ECANCELED;
	}
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */


	ret = lll_prepare_done(lll);
	LL_ASSERT_ERR(!ret);

	DEBUG_RADIO_START_A(1);

	return 0;
}

static void abort_cb(struct lll_prepare_param *prepare_param, void *param)
{
	struct lll_adv_sync *lll;
	int err;

	if (!prepare_param) {
		radio_isr_set(isr_event_done, param);
		radio_disable();

		return;
	}

	err = lll_hfclock_off();
	LL_ASSERT_ERR(err >= 0);

	lll = prepare_param->param;
	lll->latency_prepare += (prepare_param->lazy + 1);

	lll_done(param);
}
/*@brief Setup radio to receive PAwR response slot
 *
 * @param[in] lll Pointer to the link layer LLL context.
*/

static void setup_response_slot_rx(struct lll_adv_sync *lll)
{
	struct ll_adv_sync_set *sync;
	struct node_rx_pdu *node_rx;
	uint8_t chan_idx;
	uint32_t slot_spacing_offset;
	uint32_t slot_spacing_start_us;
	uint32_t start_us;
	uint8_t phy_se;
	uint8_t is_modified;
	uint32_t overhead_us;
	struct pdu_adv *pdu;
	struct subevent_data_meta *se;
	uint32_t hcto;

	se = &lll->se_data[lll->subevent_curr];
	if (!se->is_data_set) {
		//lll_adv_sync_data_curr_get(lll);
		/** TODO: empty air packet */
		//pdu = empty_adv_sync_rsp_packet(lll_adv_sync_data_curr_get(lll));
		//pdu->

	}
	pdu = lll_adv_pdu_latest_get(&se->data, &is_modified);
	phy_se = lll->adv->phy_s;

	overhead_us = PDU_AC_US(pdu->len, lll->adv->phy_s, lll->adv->phy_flags);
	slot_spacing_offset = (uint32_t)lll->response_slot_delay * 1250U;
	/** overhead_us maybe use  ready time  replace*/
	rsp_slot_anchor_us = radio_tmr_end_get() - overhead_us + slot_spacing_offset;

	slot_spacing_offset -= overhead_us;
	slot_spacing_offset -= radio_tx_chain_delay_get(lll->adv->phy_s, lll->adv->phy_flags);

	slot_spacing_start_us = radio_tmr_end_get() + slot_spacing_offset;
	slot_spacing_start_us -= lll_radio_rx_ready_delay_get(lll->adv->phy_s, PHY_FLAGS_S8);
	slot_spacing_start_us -= EVENT_JITTER_US;
	//slot_spacing_start_us -= 1000U; /* 1ms margin for HCTO */

		/* Setup radio for auxiliary PDU scan */
	radio_phy_set(phy_se, PHY_FLAGS_S8);
	radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT, LL_EXT_OCTETS_RX_MAX,
			    RADIO_PKT_CONF_PHY(phy_se));
	radio_aa_set(lll->rsp_aa);//set receive AA
	//radio_crc_configure(PDU_CRC_POLYNOMIAL, sys_get_le24(lll->rsp_aa));
	radio_crc_configure(PDU_CRC_POLYNOMIAL, sys_get_le24(lll->crc_init));
#if defined(CONFIG_SOC_COMPATIBLE_NRF5340_CPUNET) || defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
	radio_tmr_tx_disable();
#endif
	radio_tmr_rx_enable();

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT_DBG(node_rx);
	radio_pkt_rx_set(node_rx->pdu);

	chan_idx = pawr_data_chan_calc(lll, lll->event_counter - 1, lll->subevent_curr);
	lll_chan_set(chan_idx);

	radio_isr_set(isr_rx_response_slot, lll);
	radio_switch_complete_and_disable();
	start_us = radio_tmr_start_us(0, slot_spacing_start_us);//RX
	LL_ASSERT_ERR(start_us == (slot_spacing_start_us + 1U));

	/* Setup header complete timeout */
	hcto = start_us;
	hcto += EVENT_JITTER_US;
	hcto += lll_radio_rx_ready_delay_get(phy_se, PHY_FLAGS_S8);
	hcto += radio_rx_chain_delay_get(phy_se, PHY_FLAGS_S8);
	hcto += addr_us_get(phy_se);
	radio_tmr_hcto_configure_abs(hcto);

}

static void isr_subevent_tx_done(void *param)
{
	struct subevent_data_meta *se;
	struct lll_adv_sync *lll;
	/* Call to ensure packet/event timer accumulates the elapsed time
	 * under single timer use.
	 */
	(void)radio_is_tx_done();
	/* Clear radio tx status and events */
	lll_isr_tx_status_reset();

	lll = param;
	se = &lll->se_data[lll->subevent_curr];

	if (se->response_slot_count > 0) {
		lll->rsp_slot_curr = 0U;
		setup_response_slot_rx(lll);
		return;
	}else {
		/** TODO: next subevent */
		// radio_isr_set(isr_event_done, lll);
		// radio_switch_complete_and_disable();
	}

}
static void isr_next_subevent_tx_done(void *param)
{
	struct subevent_data_meta *se;
	struct lll_adv_sync *lll;
	/* Call to ensure packet/event timer accumulates the elapsed time
	 * under single timer use.
	 */
	(void)radio_is_tx_done();
	/* Clear radio tx status and events */
	lll_isr_tx_status_reset();
	//__asm__ volatile("bkpt #0");
	lll = param;
	se = &lll->se_data[lll->subevent_curr];

	if (se->response_slot_count > 0) {
		lll->rsp_slot_curr = 0U;
		setup_response_slot_rx(lll);
		return;
	}else {
		/** TODO: next subevent */
		// radio_isr_set(isr_event_done, lll);
		// radio_switch_complete_and_disable();
	}

}
static void setup_next_subevent_tx(struct lll_adv_sync *lll)
{
	struct subevent_data_meta *se;
	struct pdu_adv *pdu;
	uint8_t is_modified;
	uint32_t start_se_us;
	uint32_t start_us;
	uint8_t phy_s;
	uint8_t chan_idx;

	#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	radio_tx_power_set(lll->adv->tx_pwr_lvl);
	#else
		radio_tx_power_set(RADIO_TXP_DEFAULT);
	#endif
	phy_s = lll->adv->phy_s;
	radio_phy_set(phy_s, lll->adv->phy_flags);
	radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT, PDU_AC_PAYLOAD_SIZE_MAX,
			    RADIO_PKT_CONF_PHY(phy_s));
	radio_crc_configure(PDU_CRC_POLYNOMIAL, sys_get_le24(lll->crc_init));
	radio_aa_set(lll->access_addr);
	chan_idx = pawr_data_chan_calc(lll, lll->event_counter - 1, lll->subevent_curr);
	lll_chan_set(chan_idx);
#if defined(CONFIG_SOC_COMPATIBLE_NRF5340_CPUNET) || defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
	radio_tmr_rx_disable();
#endif
	radio_tmr_tx_enable();
	se = &lll->se_data[lll->subevent_curr];
	if (!se->is_data_set) {
		//lll_adv_sync_data_curr_get(lll);
		/** TODO: empty air packet */
		//pdu = empty_adv_sync_rsp_packet(lll_adv_sync_data_curr_get(lll));
		//pdu->
	}
	pdu = lll_adv_pdu_latest_get(&se->data, &is_modified);
	radio_pkt_tx_set(pdu);

	//radio_isr_set(isr_subevent_tx_done, lll);
	radio_isr_set(isr_next_subevent_tx_done, lll);
	if (IS_ENABLED(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER) &&
		IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT)) {
		/* Required under single time tIFS switching, to accumulate the packet
			* timer value at the time of clear on radio end.
			*/
		radio_switch_complete_end_capture_and_disable();
	} else {
		radio_switch_complete_and_disable();
	}
	//cal next subevent anchor time
	start_se_us = rsp_slot_anchor_us +
				(uint32_t)lll->subevent_curr * lll->subevent_interval * 1250U -
				(uint32_t)lll->response_slot_delay * 1250U;
	if (start_se_us <= radio_tmr_end_get()) {
		LL_ASSERT_DBG(0);
	}
	start_us = radio_tmr_start_us(1U, start_se_us);//tx
	LL_ASSERT_ERR(start_us == (start_se_us + 1U));
	/* capture end of Tx-ed PDU, used to calculate HCTO. */
	radio_tmr_end_capture();

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
	radio_gpio_pa_setup();

	radio_gpio_pa_lna_enable(start_us + radio_tx_ready_delay_get(lll->adv->phy_s, 1) -
				 HAL_RADIO_GPIO_PA_OFFSET);
#else /* !HAL_RADIO_GPIO_HAVE_PA_PIN */
	ARG_UNUSED(start_us);
#endif /* !HAL_RADIO_GPIO_HAVE_PA_PIN */

}
static void isr_rx_response_slot(void *param)
{
	struct node_rx_pawr_response *pawr_rsp;
	struct subevent_data_meta *se;
	struct lll_adv_sync *lll;
	struct node_rx_pdu *node_rx;
	struct pdu_adv *pdu_rx;
	uint8_t *payload;
	uint8_t payload_len;
	uint8_t trx_done;
	uint8_t crc_ok;
	uint8_t phy_flags_rx;
	uint8_t rssi_ready;
	uint32_t start_us;
	uint32_t start_rx_us;
	uint32_t  hcto;
	uint8_t phy_se;

	lll = param;
	se = &lll->se_data[lll->subevent_curr];
	phy_se = lll->adv->phy_s;

	/* Read radio status and events */
	trx_done = radio_is_done();
	if (trx_done) {
		crc_ok = radio_crc_is_valid();
		rssi_ready = radio_rssi_is_ready();
		phy_flags_rx = radio_phy_flags_rx_get();
	} else {
		crc_ok = phy_flags_rx = rssi_ready = 0U;
	}

	/* Clear radio rx status and events */
	lll_isr_rx_status_reset();
	/* No Rx */
	if (!trx_done) {
		 /* 路径 A：RX 窗口/时序不对 */
	} else if (!crc_ok) {
    /* 路径 B：收到了但 CRC/AA/信道配置不对 */
	}
	if (crc_ok) {
		/** Received Response Slot Data*/
		node_rx = ull_pdu_rx_alloc_peek(4);
		LL_ASSERT_DBG(node_rx);
		if(node_rx) {

			ull_pdu_rx_alloc();//allocate
			pdu_rx = (void *)node_rx->pdu;

			pawr_rsp = (void *)node_rx;
			pawr_rsp->hdr.type = NODE_RX_TYPE_PAWR_RESPONSE;
			(void)memmove(pawr_rsp->data, pdu_rx->payload, pdu_rx->len);
			pawr_rsp->rx_ftr.param = lll;
			pawr_rsp->rssi = radio_rssi_get();
			pawr_rsp->crc_ok = 1U;
			pawr_rsp->subevent = lll->subevent_curr;
			pawr_rsp->response_slot = se->response_slot_start + lll->rsp_slot_curr;
			pawr_rsp->data_len =  pdu_rx->len;

			// if (pawr_rsp->data_len > 0U) {
			// 	memcpy(pawr_rsp->data, payload, pawr_rsp->data_len);
			// }

			ull_rx_put(node_rx->hdr.link, node_rx);
			ull_rx_sched();
		}
	}
	//cal next response slot anchor time
	start_rx_us = rsp_slot_anchor_us + (lll->rsp_slot_curr + 1) * lll->response_slot_spacing*125;
	lll->rsp_slot_curr++;
	if (lll->rsp_slot_curr < se->response_slot_count) {
		//start_rx_us = (uint32_t)lll->response_slot_spacing * 125U;
		/** Configure next response slot receive */
		node_rx = ull_pdu_rx_alloc_peek(1);
		LL_ASSERT_DBG(node_rx);
		radio_pkt_rx_set(node_rx->pdu);
		radio_isr_set(isr_rx_response_slot, lll);

		start_rx_us -= lll_radio_rx_ready_delay_get(phy_se, PHY_FLAGS_S8);
		start_rx_us -= EVENT_JITTER_US;

		radio_switch_complete_and_disable();
		start_us = radio_tmr_start_us(0, start_rx_us);//rx
		LL_ASSERT_ERR(start_us == (start_rx_us + 1U));

		/* Setup header complete timeout */
		hcto = start_us;
		hcto += EVENT_JITTER_US;
		hcto += lll_radio_rx_ready_delay_get(phy_se, PHY_FLAGS_S8);
		hcto += radio_rx_chain_delay_get(phy_se, PHY_FLAGS_S8);
		hcto += addr_us_get(phy_se);
		radio_tmr_hcto_configure_abs(hcto);

		return;
	}
	} else { /* TODO: Next Subevent */
		if ((lll->subevent_curr + 1) < (lll->num_subevents)) {
			lll->subevent_curr++;
			setup_next_subevent_tx(lll);
		} else {
			radio_isr_set(isr_event_done, lll);
			radio_switch_complete_and_disable();
		}
	}
}

static uint8_t pawr_data_chan_calc(struct lll_adv_sync *lll,
                                   uint16_t event_counter,
                                   uint8_t subevent)
{
    uint8_t *map = lll->chm[lll->chm_first].data_chan_map;
    uint8_t count = lll->chm[lll->chm_first].data_chan_count;
    uint16_t counter = event_counter ^ subevent;
    return lll_chan_sel_2(counter, lll->data_chan_id, map, count);
}
static void isr_event_done(void *param)
{
	struct lll_adv_sync *lll = param;

	if ((lll->chm_first != lll->chm_last) &&
	    is_instant_or_past(lll->event_counter, lll->chm_instant)) {
		struct node_rx_pdu *rx;

		rx = ull_pdu_rx_alloc();
		LL_ASSERT_ERR(rx);

		rx->hdr.type = NODE_RX_TYPE_SYNC_CHM_COMPLETE;
		rx->rx_ftr.param = lll;

		ull_rx_put_sched(rx->hdr.link, rx);
	}

	lll_isr_done(lll);
}
