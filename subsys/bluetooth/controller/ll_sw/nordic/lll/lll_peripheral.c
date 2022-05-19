/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
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

#include "util/util.h"
#include "util/memq.h"
#include "util/dbuf.h"
#include "util/util.h"

#include "pdu.h"

#include "lll.h"
#include "lll_vendor.h"
#include "lll_clock.h"
#include "lll_df_types.h"
#include "lll_conn.h"
#include "lll_peripheral.h"
#include "lll_chan.h"

#include "lll_internal.h"
#include "lll_df_internal.h"
#include "lll_tim_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_lll_periph
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

static int init_reset(void);
static int prepare_cb(struct lll_prepare_param *p);

int lll_periph_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int lll_periph_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

void lll_periph_prepare(void *param)
{
	struct lll_prepare_param *p;
	struct lll_conn *lll;
	int err;

	err = lll_hfclock_on();
	LL_ASSERT(err >= 0);

	p = param;

	lll = p->param;

	/* Accumulate window widening */
	lll->periph.window_widening_prepare_us +=
	    lll->periph.window_widening_periodic_us * (p->lazy + 1);
	if (lll->periph.window_widening_prepare_us >
	    lll->periph.window_widening_max_us) {
		lll->periph.window_widening_prepare_us =
			lll->periph.window_widening_max_us;
	}

	/* Invoke common pipeline handling of prepare */
	err = lll_prepare(lll_is_abort_cb, lll_conn_abort_cb, prepare_cb, 0, p);
	LL_ASSERT(!err || err == -EINPROGRESS);
}

static int init_reset(void)
{
	return 0;
}

static int prepare_cb(struct lll_prepare_param *p)
{
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
	struct lll_df_conn_rx_params *df_rx_params;
	struct lll_df_conn_rx_cfg *df_rx_cfg;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RX */
	uint32_t ticks_at_event;
	uint32_t ticks_at_start;
	uint16_t event_counter;
	uint32_t remainder_us;
	uint8_t data_chan_use;
	struct lll_conn *lll;
	struct ull_hdr *ull;
	uint32_t remainder;
	uint32_t hcto;

	DEBUG_RADIO_START_S(1);

	lll = p->param;

	/* Check if stopped (on disconnection between prepare and pre-empt)
	 */
	if (unlikely(lll->handle == 0xFFFF)) {
		radio_isr_set(lll_isr_early_abort, lll);
		radio_disable();

		return 0;
	}

	/* Reset connection event global variables */
	lll_conn_prepare_reset();

	/* Calculate the current event latency */
	lll->latency_event = lll->latency_prepare + p->lazy;

	/* Calculate the current event counter value */
	event_counter = lll->event_counter + lll->latency_event;

	/* Update event counter to next value */
	lll->event_counter = (event_counter + 1);

	/* Reset accumulated latencies */
	lll->latency_prepare = 0;

	if (lll->data_chan_sel) {
#if defined(CONFIG_BT_CTLR_CHAN_SEL_2)
		data_chan_use = lll_chan_sel_2(event_counter, lll->data_chan_id,
					       &lll->data_chan_map[0],
					       lll->data_chan_count);
#else /* !CONFIG_BT_CTLR_CHAN_SEL_2 */
		data_chan_use = 0;
		LL_ASSERT(0);
#endif /* !CONFIG_BT_CTLR_CHAN_SEL_2 */
	} else {
		data_chan_use = lll_chan_sel_1(&lll->data_chan_use,
					       lll->data_chan_hop,
					       lll->latency_event,
					       &lll->data_chan_map[0],
					       lll->data_chan_count);
	}

	/* current window widening */
	lll->periph.window_widening_event_us +=
		lll->periph.window_widening_prepare_us;
	lll->periph.window_widening_prepare_us = 0;
	if (lll->periph.window_widening_event_us >
	    lll->periph.window_widening_max_us) {
		lll->periph.window_widening_event_us =
			lll->periph.window_widening_max_us;
	}

	/* current window size */
	lll->periph.window_size_event_us +=
		lll->periph.window_size_prepare_us;
	lll->periph.window_size_prepare_us = 0;

	/* Ensure that empty flag reflects the state of the Tx queue, as a
	 * peripheral if this is the first connection event and as no prior PDU
	 * is transmitted, an incorrect acknowledgment by peer should not
	 * dequeue a PDU that has not been transmitted on air.
	 */
	if (!lll->empty) {
		memq_link_t *link;

		/* Check for any Tx PDU at the head of the queue */
		link = memq_peek(lll->memq_tx.head, lll->memq_tx.tail, NULL);
		if (!link) {
			/* Update empty flag to reflect that no valid non-empty
			 * PDU was transmitted prior to this connection event.
			 */
			lll->empty = 1U;
		}
	}

	/* Start setting up Radio h/w */
	radio_reset();
#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	radio_tx_power_set(lll->tx_pwr_lvl);
#else
	radio_tx_power_set(RADIO_TXP_DEFAULT);
#endif /* CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */

	radio_aa_set(lll->access_addr);
	radio_crc_configure(PDU_CRC_POLYNOMIAL,
				sys_get_le24(lll->crc_init));

	lll_chan_set(data_chan_use);

	radio_isr_set(lll_conn_isr_rx, lll);

	radio_tmr_tifs_set(EVENT_IFS_US);

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
#if defined(CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE)
	enum radio_end_evt_delay_state end_evt_delay;
#endif /* CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE */

#if defined(CONFIG_BT_CTLR_PHY)
	if (lll->phy_rx != PHY_CODED) {
#else
	if (1) {
#endif /* CONFIG_BT_CTLR_PHY */
		df_rx_cfg = &lll->df_rx_cfg;
		df_rx_params = dbuf_latest_get(&df_rx_cfg->hdr, NULL);

		if (df_rx_params->is_enabled == true) {
			lll_df_conf_cte_rx_enable(df_rx_params->slot_durations,
						  df_rx_params->ant_sw_len, df_rx_params->ant_ids,
						  data_chan_use, CTE_INFO_IN_S1_BYTE, lll->phy_rx);
			lll->df_rx_cfg.chan = data_chan_use;
		} else {
			lll_df_conf_cte_info_parsing_enable();
		}
#if defined(CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE)
		end_evt_delay = END_EVT_DELAY_ENABLED;
	} else {
		end_evt_delay = END_EVT_DELAY_DISABLED;
#endif /* CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE */
	}

#if defined(CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE)
/* Use special API for SOC that requires compensation for PHYEND event delay. */
#if defined(CONFIG_BT_CTLR_PHY)
	radio_switch_complete_with_delay_compensation_and_tx(lll->phy_rx, 0, lll->phy_tx,
							     lll->phy_flags, end_evt_delay);
#else /* !CONFIG_BT_CTLR_PHY */
	radio_switch_complete_with_delay_compensation_and_tx(0, 0, 0, 0, end_evt_delay);
#endif /* !CONFIG_BT_CTLR_PHY */

#endif /* CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE */
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RX */

	/* Use regular API for cases when:
	 * - CTE RX is not enabled,
	 * - SOC does not require compensation for PHYEND event delay.
	 */
	if (!IS_ENABLED(CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE)) {
#if defined(CONFIG_BT_CTLR_PHY)
		radio_switch_complete_and_tx(lll->phy_rx, 0, lll->phy_tx, lll->phy_flags);
#else /* !CONFIG_BT_CTLR_PHY && !CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE */
		radio_switch_complete_and_tx(0, 0, 0, 0);
#endif /* !CONFIG_BT_CTLR_PHY */
	}

	/* The call can use Radio interface that alternates NRF_RADIO->SHORTS. The register is
	 * set by radio_switch_complete_XXX functions, hence any changes done before are cleared.
	 */
	lll_conn_rx_pkt_set(lll);

	ticks_at_event = p->ticks_at_expire;
	ull = HDR_LLL2ULL(lll);
	ticks_at_event += lll_event_offset_get(ull);

	ticks_at_start = ticks_at_event;
	ticks_at_start += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

	remainder = p->remainder;
	remainder_us = radio_tmr_start(0, ticks_at_start, remainder);

	radio_tmr_aa_capture();
	radio_tmr_aa_save(0);

	hcto = remainder_us +
	       ((EVENT_JITTER_US + EVENT_TICKER_RES_MARGIN_US +
		 lll->periph.window_widening_event_us) << 1) +
	       lll->periph.window_size_event_us;

#if defined(CONFIG_BT_CTLR_PHY)
	hcto += radio_rx_ready_delay_get(lll->phy_rx, 1);
	hcto += addr_us_get(lll->phy_rx);
	hcto += radio_rx_chain_delay_get(lll->phy_rx, 1);
#else /* !CONFIG_BT_CTLR_PHY */
	hcto += radio_rx_ready_delay_get(0, 0);
	hcto += addr_us_get(0);
	hcto += radio_rx_chain_delay_get(0, 0);
#endif /* !CONFIG_BT_CTLR_PHY */

	radio_tmr_hcto_configure(hcto);

#if defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
	radio_gpio_lna_setup();

#if defined(CONFIG_BT_CTLR_PHY)
	radio_gpio_pa_lna_enable(remainder_us +
				 radio_rx_ready_delay_get(lll->phy_rx, 1) -
				 HAL_RADIO_GPIO_LNA_OFFSET);
#else /* !CONFIG_BT_CTLR_PHY */
	radio_gpio_pa_lna_enable(remainder_us +
				 radio_rx_ready_delay_get(0, 0) -
				 HAL_RADIO_GPIO_LNA_OFFSET);
#endif /* !CONFIG_BT_CTLR_PHY */
#endif /* HAL_RADIO_GPIO_HAVE_LNA_PIN */

#if defined(CONFIG_BT_CTLR_PROFILE_ISR) || \
	defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
	radio_tmr_end_capture();
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
	radio_rssi_measure();
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
	(EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
	/* check if preempt to start has changed */
	if (lll_preempt_calc(ull, (TICKER_ID_CONN_BASE + lll->handle),
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

	DEBUG_RADIO_START_S(1);

	return 0;
}
