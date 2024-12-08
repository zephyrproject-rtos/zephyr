/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stddef.h>

#include <zephyr/toolchain.h>

#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/radio_df.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/memq.h"
#include "util/dbuf.h"

#include "pdu_df.h"
#include "pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll_vendor.h"
#include "lll_clock.h"
#include "lll_df_types.h"
#include "lll_conn.h"
#include "lll_central.h"
#include "lll_chan.h"

#include "lll_internal.h"
#include "lll_df_internal.h"
#include "lll_tim_internal.h"

#include <soc.h>
#include "hal/debug.h"

static int init_reset(void);
static int prepare_cb(struct lll_prepare_param *p);

int lll_central_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int lll_central_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

void lll_central_prepare(void *param)
{
	int err;

	err = lll_hfclock_on();
	LL_ASSERT(err >= 0);

	/* Invoke common pipeline handling of prepare */
	err = lll_prepare(lll_conn_central_is_abort_cb, lll_conn_abort_cb,
			  prepare_cb, 0, param);
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
	struct pdu_data *pdu_data_tx;
	uint32_t ticks_at_event;
	uint32_t ticks_at_start;
	uint16_t event_counter;
	uint32_t remainder_us;
	uint8_t data_chan_use;
	struct lll_conn *lll;
	struct ull_hdr *ull;
	uint32_t remainder;
	uint8_t cte_len;
	uint32_t ret;

	DEBUG_RADIO_START_M(1);

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
	lll->lazy_prepare = p->lazy;
	lll->latency_event = lll->latency_prepare + lll->lazy_prepare;

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

	/* Prepare the Tx PDU */
	lll_conn_pdu_tx_prep(lll, &pdu_data_tx);
	pdu_data_tx->sn = lll->sn;
	pdu_data_tx->nesn = lll->nesn;

	/* Start setting up of Radio h/w */
	radio_reset();

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_TX)
	if (pdu_data_tx->cp) {
		cte_len = CTE_LEN_US(pdu_data_tx->octet3.cte_info.time);

		lll_df_cte_tx_configure(pdu_data_tx->octet3.cte_info.type,
					pdu_data_tx->octet3.cte_info.time,
					lll->df_tx_cfg.ant_sw_len,
					lll->df_tx_cfg.ant_ids);
	} else
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_TX */
	{
		cte_len = 0U;
	}

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	radio_tx_power_set(lll->tx_pwr_lvl);
#else
	radio_tx_power_set(RADIO_TXP_DEFAULT);
#endif

	radio_aa_set(lll->access_addr);
	radio_crc_configure(PDU_CRC_POLYNOMIAL,
					sys_get_le24(lll->crc_init));
	lll_chan_set(data_chan_use);

	lll_conn_tx_pkt_set(lll, pdu_data_tx);

	radio_isr_set(lll_conn_isr_tx, lll);

	radio_tmr_tifs_set(lll->tifs_rx_us);

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
	/* If CTE RX is enabled and the PHY is not CODED, store channel used for
	 * the connection event to report it with collected IQ samples.
	 * The configuration of the CTE receive may not change during the event,
	 * so config buffer is swapped in prepare and used in IRS handers.
	 */
	if (lll->phy_rx != PHY_CODED) {
		df_rx_cfg = &lll->df_rx_cfg;
		df_rx_params = dbuf_latest_get(&df_rx_cfg->hdr, NULL);

		if (df_rx_params->is_enabled == true) {
			lll->df_rx_cfg.chan = data_chan_use;
		}
	}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RX */

#if defined(CONFIG_BT_CTLR_PHY)
	radio_switch_complete_and_rx(lll->phy_rx);
#else /* !CONFIG_BT_CTLR_PHY */
	radio_switch_complete_and_rx(0);
#endif /* !CONFIG_BT_CTLR_PHY */

	ticks_at_event = p->ticks_at_expire;
	ull = HDR_LLL2ULL(lll);
	ticks_at_event += lll_event_offset_get(ull);

	ticks_at_start = ticks_at_event;
	ticks_at_start += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

	remainder = p->remainder;
	remainder_us = radio_tmr_start(1, ticks_at_start, remainder);

	/* capture end of Tx-ed PDU, used to calculate HCTO. */
	radio_tmr_end_capture();

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
	radio_gpio_pa_setup();

#if defined(CONFIG_BT_CTLR_PHY)
	radio_gpio_pa_lna_enable(remainder_us +
				 radio_tx_ready_delay_get(lll->phy_tx,
							  lll->phy_flags) -
				 HAL_RADIO_GPIO_PA_OFFSET);
#else /* !CONFIG_BT_CTLR_PHY */
	radio_gpio_pa_lna_enable(remainder_us +
				 radio_tx_ready_delay_get(0, 0) -
				 HAL_RADIO_GPIO_PA_OFFSET);
#endif /* !CONFIG_BT_CTLR_PHY */
#else /* !HAL_RADIO_GPIO_HAVE_PA_PIN */
	ARG_UNUSED(remainder_us);
#endif /* !HAL_RADIO_GPIO_HAVE_PA_PIN */

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
	(EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
	uint32_t overhead;

	overhead = lll_preempt_calc(ull, (TICKER_ID_CONN_BASE + lll->handle), ticks_at_event);
	/* check if preempt to start has changed */
	if (overhead) {
		LL_ASSERT_OVERHEAD(overhead);

		radio_isr_set(lll_isr_abort, lll);
		radio_disable();

		return -ECANCELED;
	}
#endif /* !CONFIG_BT_CTLR_XTAL_ADVANCED */

	ret = lll_prepare_done(lll);
	LL_ASSERT(!ret);

	DEBUG_RADIO_START_M(1);

	return 0;
}
