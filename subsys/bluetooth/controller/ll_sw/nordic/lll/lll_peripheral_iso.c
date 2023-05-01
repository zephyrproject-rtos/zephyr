/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/sys/byteorder.h>

#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/dbuf.h"

#include "pdu_df.h"
#include "pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll_clock.h"
#include "lll/lll_df_types.h"
#include "lll_chan.h"
#include "lll_vendor.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"
#include "lll_peripheral_iso.h"

#include "lll_iso_tx.h"

#include "lll_internal.h"
#include "lll_tim_internal.h"

#include "ll_feat.h"

#include "hal/debug.h"

static int init_reset(void);
static int prepare_cb(struct lll_prepare_param *p);
static void abort_cb(struct lll_prepare_param *prepare_param, void *param);
static void isr_rx(void *param);
static void isr_tx(void *param);
static void next_cis_prepare(void *param);
static void isr_prepare_subevent(void *param);
static void isr_prepare_subevent_next_cis(void *param);
static void isr_prepare_subevent_common(void *param);
static void isr_done(void *param);

static uint8_t next_chan_use;
static uint16_t data_chan_id;
static uint16_t data_chan_prn_s;
static uint16_t data_chan_remap_idx;

static uint32_t trx_performed_bitmask;
static uint16_t cis_offset_first;
static uint16_t cis_handle_curr;
static uint8_t se_curr;
static uint8_t bn_tx;
static uint8_t bn_rx;
static uint8_t has_tx;

#if defined(CONFIG_BT_CTLR_LE_ENC)
static uint8_t mic_state;
#endif /* CONFIG_BT_CTLR_LE_ENC */

int lll_peripheral_iso_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int lll_peripheral_iso_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

void lll_peripheral_iso_prepare(void *param)
{
	struct lll_conn_iso_group *cig_lll;
	struct lll_prepare_param *p;
	uint16_t elapsed;
	int err;

	/* Initiate HF clock start up */
	err = lll_hfclock_on();
	LL_ASSERT(err >= 0);

	/* Instants elapsed */
	p = param;
	elapsed = p->lazy + 1U;

	/* Save the (latency + 1) for use in event and/or supervision timeout */
	cig_lll = p->param;
	cig_lll->latency_prepare += elapsed;

	/* Accumulate window widening */
	cig_lll->window_widening_prepare_us_frac +=
	    cig_lll->window_widening_periodic_us_frac * elapsed;
	if (cig_lll->window_widening_prepare_us_frac >
	    EVENT_US_TO_US_FRAC(cig_lll->window_widening_max_us)) {
		cig_lll->window_widening_prepare_us_frac =
			EVENT_US_TO_US_FRAC(cig_lll->window_widening_max_us);
	}

	/* Invoke common pipeline handling of prepare */
	err = lll_prepare(lll_is_abort_cb, abort_cb, prepare_cb, 0U, param);
	LL_ASSERT(!err || err == -EINPROGRESS);
}

void lll_peripheral_iso_flush(uint16_t handle, struct lll_conn_iso_stream *lll)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(lll);
}

static int init_reset(void)
{
	return 0;
}

static int prepare_cb(struct lll_prepare_param *p)
{
	struct lll_conn_iso_group *cig_lll = p->param;
	struct lll_conn_iso_stream *cis_lll;
	struct node_rx_pdu *node_rx;
	struct lll_conn *conn_lll;
	uint32_t ticks_at_event;
	uint32_t ticks_at_start;
	struct node_tx_iso *tx;
	uint64_t payload_count;
	uint16_t event_counter;
	uint8_t data_chan_use;
	struct ull_hdr *ull;
	uint32_t remainder;
	memq_link_t *link;
	uint32_t start_us;
	uint32_t hcto;
	uint16_t lazy;
	uint32_t ret;
	uint8_t phy;

	DEBUG_RADIO_START_S(1);

	/* Reset global static variables */
	trx_performed_bitmask = 0U;
#if defined(CONFIG_BT_CTLR_LE_ENC)
	mic_state = LLL_CONN_MIC_NONE;
#endif /* CONFIG_BT_CTLR_LE_ENC */

	/* Get the first CIS */
	cis_handle_curr = UINT16_MAX;
	do {
		cis_lll = ull_conn_iso_lll_stream_get_by_group(cig_lll, &cis_handle_curr);
	} while (cis_lll && !cis_lll->active);

	LL_ASSERT(cis_lll);

	/* Save first active CIS offset */
	cis_offset_first = cis_lll->offset;

	/* Get reference to ACL context */
	conn_lll = ull_conn_lll_get(cis_lll->acl_handle);

	/* Event counter value,  0-15 bit of cisEventCounter */
	event_counter = cis_lll->event_count;

	/* Calculate the radio channel to use for ISO event */
	data_chan_id = lll_chan_id(cis_lll->access_addr);
	data_chan_use = lll_chan_iso_event(event_counter, data_chan_id,
					   conn_lll->data_chan_map,
					   conn_lll->data_chan_count,
					   &data_chan_prn_s,
					   &data_chan_remap_idx);

	/* Store the current event latency */
	cig_lll->latency_event = cig_lll->latency_prepare;
	lazy = cig_lll->latency_prepare - 1U;

	/* Reset accumulated latencies */
	cig_lll->latency_prepare = 0U;

	/* current window widening */
	cig_lll->window_widening_event_us_frac +=
		cig_lll->window_widening_prepare_us_frac;
	cig_lll->window_widening_prepare_us_frac = 0;
	if (cig_lll->window_widening_event_us_frac >
	    EVENT_US_TO_US_FRAC(cig_lll->window_widening_max_us)) {
		cig_lll->window_widening_event_us_frac =
			EVENT_US_TO_US_FRAC(cig_lll->window_widening_max_us);
	}

	/* Adjust sn and nesn for skipped CIG events */
	cis_lll->sn += cis_lll->tx.bn * lazy;
	cis_lll->nesn += cis_lll->rx.bn * lazy;

	se_curr = 1U;
	bn_rx = 1U;
	has_tx = 0U;

	/* Start setting up of Radio h/w */
	radio_reset();

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	radio_tx_power_set(cis_lll->tx_pwr_lvl);
#else /* !CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */
	radio_tx_power_set(RADIO_TXP_DEFAULT);
#endif /* !CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */

	phy = cis_lll->rx.phy;
	radio_phy_set(phy, cis_lll->rx.phy_flags);
	radio_aa_set(cis_lll->access_addr);
	radio_crc_configure(PDU_CRC_POLYNOMIAL,
			    sys_get_le24(conn_lll->crc_init));
	lll_chan_set(data_chan_use);

	node_rx = ull_iso_pdu_rx_alloc_peek(1U);
	LL_ASSERT(node_rx);

	/* Encryption */
	if (false) {

#if defined(CONFIG_BT_CTLR_LE_ENC)
	} else if (conn_lll->enc_rx) {
		uint64_t payload_count;
		uint8_t pkt_flags;

		payload_count = (cis_lll->event_count * cis_lll->rx.bn) +
				(bn_rx - 1U);
		cis_lll->rx.ccm.counter = payload_count;

		pkt_flags = RADIO_PKT_CONF_FLAGS(RADIO_PKT_CONF_PDU_TYPE_DC,
						 phy,
						 RADIO_PKT_CONF_CTE_DISABLED);
		radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT,
				    (cis_lll->rx.max_pdu + PDU_MIC_SIZE),
				    pkt_flags);
		radio_pkt_rx_set(radio_ccm_rx_pkt_set(&cis_lll->rx.ccm, phy,
						      node_rx->pdu));
#endif /* CONFIG_BT_CTLR_LE_ENC */

	} else {
		uint8_t pkt_flags;

		pkt_flags = RADIO_PKT_CONF_FLAGS(RADIO_PKT_CONF_PDU_TYPE_DC,
						 phy,
						 RADIO_PKT_CONF_CTE_DISABLED);
		radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT,
				    cis_lll->rx.max_pdu, pkt_flags);
		radio_pkt_rx_set(node_rx->pdu);
	}

	radio_isr_set(isr_rx, cis_lll);

	radio_tmr_tifs_set(EVENT_IFS_US);

#if defined(CONFIG_BT_CTLR_PHY)
	radio_switch_complete_and_tx(cis_lll->rx.phy, 0U, cis_lll->tx.phy,
				     cis_lll->tx.phy_flags);
#else /* !CONFIG_BT_CTLR_PHY */
	radio_switch_complete_and_tx(0U, 0U, 0U, 0U);
#endif /* !CONFIG_BT_CTLR_PHY */

	ticks_at_event = p->ticks_at_expire;
	ull = HDR_LLL2ULL(cig_lll);
	ticks_at_event += lll_event_offset_get(ull);

	ticks_at_start = ticks_at_event;
	ticks_at_start += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US +
						 cis_offset_first);

	remainder = p->remainder;
	start_us = radio_tmr_start(0U, ticks_at_start, remainder);

	radio_tmr_ready_save(start_us);
	radio_tmr_aa_save(0U);
	radio_tmr_aa_capture();

	hcto = start_us +
	       ((EVENT_JITTER_US + EVENT_TICKER_RES_MARGIN_US +
		 EVENT_US_FRAC_TO_US(cig_lll->window_widening_event_us_frac)) <<
		1U);

#if defined(CONFIG_BT_CTLR_PHY)
	hcto += radio_rx_ready_delay_get(cis_lll->rx.phy, PHY_FLAGS_S8);
	hcto += addr_us_get(cis_lll->rx.phy);
	hcto += radio_rx_chain_delay_get(cis_lll->rx.phy, PHY_FLAGS_S8);
#else /* !CONFIG_BT_CTLR_PHY */
	hcto += radio_rx_ready_delay_get(0U, 0U);
	hcto += addr_us_get(0U);
	hcto += radio_rx_chain_delay_get(0U, 0U);
#endif /* !CONFIG_BT_CTLR_PHY */

	radio_tmr_hcto_configure(hcto);

#if defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
	radio_gpio_lna_setup();

#if defined(CONFIG_BT_CTLR_PHY)
	radio_gpio_pa_lna_enable(start_us +
				 radio_rx_ready_delay_get(cis_lll->rx.phy,
							  PHY_FLAGS_S8) -
				 HAL_RADIO_GPIO_LNA_OFFSET);
#else /* !CONFIG_BT_CTLR_PHY */
	radio_gpio_pa_lna_enable(start_us +
				 radio_rx_ready_delay_get(0U, 0U) -
				 HAL_RADIO_GPIO_LNA_OFFSET);
#endif /* !CONFIG_BT_CTLR_PHY */
#endif /* HAL_RADIO_GPIO_HAVE_LNA_PIN */

	if (false) {

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
	(EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
	/* check if preempt to start has changed */
	} else if (lll_preempt_calc(ull,
				    (TICKER_ID_CONN_ISO_BASE + cig_lll->handle),
				    ticks_at_event)) {
		radio_isr_set(lll_isr_abort, cig_lll);
		radio_disable();

		return -ECANCELED;
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */

	}

	/* Adjust the SN and NESN for skipped CIG events */
	uint16_t cis_handle = cis_handle_curr;

	while (true) {
		/* FIXME: Update below implementation when supporting  Flush Timeout */
		payload_count = cis_lll->event_count * cis_lll->tx.bn;
		do {
			link = memq_peek(cis_lll->memq_tx.head,
					 cis_lll->memq_tx.tail, (void **)&tx);
			if (link) {
				if (tx->payload_count < payload_count) {
					memq_dequeue(cis_lll->memq_tx.tail,
						     &cis_lll->memq_tx.head,
						     NULL);

					tx->next = link;
					ull_iso_lll_ack_enqueue(cis_lll->handle, tx);
				} else {
					break;
				}
			}
		} while (link);

		cis_lll = ull_conn_iso_lll_stream_get_by_group(cig_lll, &cis_handle);
		if (!cis_lll) {
			break;
		}

		if (cis_lll->active) {
			cis_lll->sn += cis_lll->tx.bn * lazy;
			cis_lll->nesn += cis_lll->rx.bn * lazy;
		}
	};

	/* Prepare is done */
	ret = lll_prepare_done(cig_lll);
	LL_ASSERT(!ret);

	DEBUG_RADIO_START_S(1);

	return 0;
}

static void abort_cb(struct lll_prepare_param *prepare_param, void *param)
{
	int err;

	/* NOTE: This is not a prepare being cancelled */
	if (!prepare_param) {
		struct lll_conn_iso_group *cig_lll = param;
		struct lll_conn_iso_stream *cis_lll;

		cis_lll = ull_conn_iso_lll_stream_get_by_group(cig_lll, NULL);

		/* Perform event abort here.
		 * After event has been cleanly aborted, clean up resources
		 * and dispatch event done.
		 */
		radio_isr_set(isr_done, cis_lll);
		radio_disable();

		return;
	}

	/* NOTE: Else clean the top half preparations of the aborted event
	 * currently in preparation pipeline.
	 */
	err = lll_hfclock_off();
	LL_ASSERT(err >= 0);

	lll_done(param);
}

static void isr_rx(void *param)
{
	struct lll_conn_iso_stream *cis_lll;
	struct lll_conn *conn_lll;
	struct pdu_cis *pdu_tx;
	uint64_t payload_count;
	uint8_t payload_index;
	uint32_t subevent_us;
	uint32_t start_us;
	uint8_t trx_done;
	uint8_t crc_ok;
	uint8_t cie;

	/* Read radio status and events */
	trx_done = radio_is_done();
	if (trx_done) {
		crc_ok = radio_crc_is_valid();
	} else {
		crc_ok = 0U;
	}

	/* Clear radio rx status and events */
	lll_isr_rx_status_reset();

	/* Get reference to CIS LLL context */
	cis_lll = param;

	/* No Rx */
	if (!trx_done) {
		if (se_curr < cis_lll->nse) {
			radio_isr_set(isr_prepare_subevent, param);
		} else {
			next_cis_prepare(param);
		}

		radio_disable();

		return;
	}

	/* Initialize Close Isochronous Event */
	cie = 0U;

	/* Save the AA captured for anchor point sync, this could be subsequent
	 * subevent if not synced to the first subevent.
	 */
	if (!radio_tmr_aa_restore()) {
		uint32_t se_offset_us;

		se_offset_us = cis_lll->sub_interval * (se_curr - 1U);
		radio_tmr_aa_save(radio_tmr_aa_get() - se_offset_us);
		radio_tmr_ready_save(radio_tmr_ready_get() - se_offset_us);
	}

	/* Close subevent, one tx-rx chain */
	radio_switch_complete_and_disable();

	/* FIXME: Do not call this for every event/subevent */
	ull_conn_iso_lll_cis_established(param);

	/* Set the bit corresponding to CIS index */
	trx_performed_bitmask |= (1U << LL_CIS_IDX_FROM_HANDLE(cis_lll->handle));

	/* Get reference to ACL context */
	conn_lll = ull_conn_lll_get(cis_lll->acl_handle);

	if (crc_ok) {
		struct node_rx_pdu *node_rx;
		struct pdu_cis *pdu_rx;

		/* Get reference to received PDU */
		node_rx = ull_iso_pdu_rx_alloc_peek(1U);
		LL_ASSERT(node_rx);

		pdu_rx = (void *)node_rx->pdu;

		/* Tx ACK */
		if (pdu_rx->nesn != cis_lll->sn) {
			/* Increment sequence number */
			cis_lll->sn++;

#if defined(CONFIG_BT_CTLR_LE_ENC)
			if (!cis_lll->npi) {
				/* Get reference to PDU Tx */
				struct node_tx_iso *node_tx;
				struct pdu_cis *pdu_tx;
				uint8_t payload_index;
				memq_link_t *link;

				payload_index = bn_tx - 1U;
				link = memq_peek_n(cis_lll->memq_tx.head,
						   cis_lll->memq_tx.tail,
						   payload_index,
						   (void **)&node_tx);
				pdu_tx = (void *)node_tx->pdu;
				if (pdu_tx->len) {
					/* if encrypted increment tx counter */
					if (conn_lll->enc_tx) {
						cis_lll->tx.ccm.counter++;
					}
				}
			}
#endif /* CONFIG_BT_CTLR_LE_ENC */

			/* Increment burst number */
			if (bn_tx <= cis_lll->tx.bn) {
				bn_tx++;
			}

			/* TODO: Tx Ack */

		}

		/* Rx receive */
		if (!pdu_rx->npi &&
		    (bn_rx <= cis_lll->rx.bn) &&
		    (pdu_rx->sn == cis_lll->nesn) &&
		    ull_iso_pdu_rx_alloc_peek(2U)) {
			struct node_rx_iso_meta *iso_meta;

			/* Increment next expected serial number */
			cis_lll->nesn++;

#if defined(CONFIG_BT_CTLR_LE_ENC)
			/* If required, wait for CCM to finish
			 */
			if (pdu_rx->len && conn_lll->enc_rx) {
				uint32_t done;

				done = radio_ccm_is_done();
				LL_ASSERT(done);

				if (!radio_ccm_mic_is_valid()) {
					/* Record MIC invalid */
					mic_state = LLL_CONN_MIC_FAIL;

					/* Close event */
					radio_isr_set(isr_done, param);
					radio_disable();

					return;
				}

				/* Increment counter */
				cis_lll->rx.ccm.counter++;

				/* Record MIC valid */
				mic_state = LLL_CONN_MIC_PASS;
			}
#endif /* CONFIG_BT_CTLR_LE_ENC */

			/* Enqueue Rx ISO PDU */
			node_rx->hdr.type = NODE_RX_TYPE_ISO_PDU;
			node_rx->hdr.handle = cis_lll->handle;
			iso_meta = &node_rx->hdr.rx_iso_meta;
			iso_meta->payload_number = (cis_lll->event_count *
						    cis_lll->rx.bn) +
						   (bn_rx - 1U);
			iso_meta->timestamp =
				HAL_TICKER_TICKS_TO_US(radio_tmr_start_get()) +
				radio_tmr_aa_restore() -
				addr_us_get(cis_lll->rx.phy);
			iso_meta->timestamp %=
				HAL_TICKER_TICKS_TO_US(BIT(HAL_TICKER_CNTR_MSBIT + 1U));
			iso_meta->status = 0U;

			ull_iso_pdu_rx_alloc();
			iso_rx_put(node_rx->hdr.link, node_rx);

#if !defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
			iso_rx_sched();
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */

			/* Increment burst number */
			bn_rx++;

		} else if (pdu_rx->npi) {
			if (bn_rx <= cis_lll->rx.bn) {
				/* Increment next expected serial number */
				cis_lll->nesn++;

				/* Increment burst number */
				bn_rx++;
			}
		}

		/* Close Isochronous Event */
		cie = cie || pdu_rx->cie;
	}

	/* FIXME: Consider Flush Timeout when resetting current burst number */
	if (!has_tx) {
		has_tx = 1U;

		/* Adjust sn when flushing Tx. Stop at sn != nesn, hence
		 * (bn < cis_lll->tx.bn).
		 */
		/* FIXME: When Flush Timeout is implemented */
		if (bn_tx < cis_lll->tx.bn) {
			cis_lll->sn += cis_lll->tx.bn - bn_tx;
		}

		/* Start transmitting new burst */
		bn_tx = 1U;
	}


	/* Close Isochronous Event */
	cie = cie || ((bn_rx > cis_lll->rx.bn) &&
		      (bn_tx > cis_lll->tx.bn) &&
		      (se_curr < cis_lll->nse));

	/* Get ISO data PDU */
	if (bn_tx > cis_lll->tx.bn) {
		payload_count = 0U;

		cis_lll->npi = 1U;

		pdu_tx = radio_pkt_empty_get();
		pdu_tx->ll_id = PDU_CIS_LLID_START_CONTINUE;
		pdu_tx->nesn = cis_lll->nesn;
		pdu_tx->sn = 0U; /* reserved RFU for NULL PDU */
		pdu_tx->cie = cie;
		pdu_tx->npi = 1U;
		pdu_tx->len = 0U;
	} else {
		struct node_tx_iso *tx;
		memq_link_t *link;

		payload_index = bn_tx - 1U;
		payload_count = cis_lll->event_count * cis_lll->tx.bn +
				payload_index;

		link = memq_peek_n(cis_lll->memq_tx.head, cis_lll->memq_tx.tail,
				   payload_index, (void **)&tx);
		if (!link || (tx->payload_count != payload_count)) {
			payload_index = 0U;
			do {
				link = memq_peek_n(cis_lll->memq_tx.head,
						   cis_lll->memq_tx.tail,
						   payload_index, (void **)&tx);
				payload_index++;
			} while (link &&
				 (tx->payload_count < payload_count));
		}

		if (!link || (tx->payload_count != payload_count)) {
			cis_lll->npi = 1U;

			pdu_tx = radio_pkt_empty_get();
			pdu_tx->ll_id = PDU_CIS_LLID_START_CONTINUE;
			pdu_tx->nesn = cis_lll->nesn;
			pdu_tx->cie = (bn_tx > cis_lll->tx.bn) &&
				      (bn_rx > cis_lll->rx.bn);
			pdu_tx->len = 0U;
			pdu_tx->sn = 0U; /* reserved RFU for NULL PDU */
			pdu_tx->npi = 1U;
		} else {
			cis_lll->npi = 0U;

			pdu_tx = (void *)tx->pdu;
			pdu_tx->nesn = cis_lll->nesn;
			pdu_tx->sn = cis_lll->sn;
			pdu_tx->cie = 0U;
			pdu_tx->npi = 0U;
		}
	}

	/* Initialize reserve bit */
	pdu_tx->rfu0 = 0U;
	pdu_tx->rfu1 = 0U;

	/* Encryption */
	if (false) {

#if defined(CONFIG_BT_CTLR_LE_ENC)
	} else if (pdu_tx->len && conn_lll->enc_tx) {
		uint8_t pkt_flags;

		cis_lll->tx.ccm.counter = payload_count;

		pkt_flags = RADIO_PKT_CONF_FLAGS(RADIO_PKT_CONF_PDU_TYPE_DC,
						 cis_lll->tx.phy,
						 RADIO_PKT_CONF_CTE_DISABLED);
		radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT,
				    (cis_lll->tx.max_pdu + PDU_MIC_SIZE),
				    pkt_flags);
		radio_pkt_tx_set(radio_ccm_tx_pkt_set(&cis_lll->tx.ccm,
						      pdu_tx));
#endif /* CONFIG_BT_CTLR_LE_ENC */

	} else {
		uint8_t pkt_flags;

		pkt_flags = RADIO_PKT_CONF_FLAGS(RADIO_PKT_CONF_PDU_TYPE_DC,
						 cis_lll->tx.phy,
						 RADIO_PKT_CONF_CTE_DISABLED);
		radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT,
				    cis_lll->tx.max_pdu, pkt_flags);
		radio_pkt_tx_set(pdu_tx);
	}

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
	uint32_t pa_lna_enable_us;

	radio_gpio_pa_setup();

	pa_lna_enable_us = radio_tmr_tifs_base_get() + EVENT_IFS_US -
			   HAL_RADIO_GPIO_PA_OFFSET;
#if defined(CONFIG_BT_CTLR_PHY)
	pa_lna_enable_us -= radio_rx_chain_delay_get(cis_lll->rx.phy,
						     PHY_FLAGS_S8);
#else /* !CONFIG_BT_CTLR_PHY */
	pa_lna_enable_us -= radio_rx_chain_delay_get(0U, 0U);
#endif /* !CONFIG_BT_CTLR_PHY */
	radio_gpio_pa_lna_enable(pa_lna_enable_us);
#endif /* HAL_RADIO_GPIO_HAVE_PA_PIN */

	/* assert if radio packet ptr is not set and radio started tx */
	LL_ASSERT(!radio_is_ready());

	/* Schedule next subevent */
	if (!cie && (se_curr < cis_lll->nse)) {
		/* Calculate the radio channel to use for next subevent
		 */
		next_chan_use =	lll_chan_iso_subevent(data_chan_id,
						      conn_lll->data_chan_map,
						      conn_lll->data_chan_count,
						      &data_chan_prn_s,
						      &data_chan_remap_idx);
	} else {
		struct lll_conn_iso_group *cig_lll;
		uint16_t event_counter;
		uint16_t cis_handle;

		/* Check for next active CIS */
		cig_lll = ull_conn_iso_lll_group_get_by_stream(cis_lll);
		cis_handle = cis_handle_curr;
		do {
			cis_lll = ull_conn_iso_lll_stream_get_by_group(cig_lll, &cis_handle);
		} while (cis_lll && !cis_lll->active);

		if (!cis_lll) {
			/* ISO Event Done */
			radio_isr_set(isr_done, param);

			return;
		}

		cis_handle_curr = cis_handle;

		/* Event counter value,  0-15 bit of cisEventCounter */
		event_counter = cis_lll->event_count;

		/* Calculate the radio channel to use for next CIS ISO event */
		data_chan_id = lll_chan_id(cis_lll->access_addr);
		next_chan_use = lll_chan_iso_event(event_counter, data_chan_id,
						   conn_lll->data_chan_map,
						   conn_lll->data_chan_count,
						   &data_chan_prn_s,
						   &data_chan_remap_idx);

		/* Reset indices for the next CIS */
		se_curr = 0U; /* isr_tx() will increase se_curr */
		bn_tx = 1U; /* FIXME: may be this should be previous event value? */
		bn_rx = 1U;
		has_tx = 0U;
	}

	/* Schedule next subevent reception */
	subevent_us = radio_tmr_aa_restore();
	subevent_us += cis_lll->offset - cis_offset_first +
		       (cis_lll->sub_interval * se_curr);
	subevent_us -= addr_us_get(cis_lll->rx.phy);

#if defined(CONFIG_BT_CTLR_PHY)
	subevent_us -= radio_rx_ready_delay_get(cis_lll->rx.phy,
						PHY_FLAGS_S8);
	subevent_us -= radio_rx_chain_delay_get(cis_lll->rx.phy,
						PHY_FLAGS_S8);
#else /* !CONFIG_BT_CTLR_PHY */
	subevent_us -= radio_rx_ready_delay_get(0U, 0U);
	subevent_us -= radio_rx_chain_delay_get(0U, 0U);
#endif /* !CONFIG_BT_CTLR_PHY */

	start_us = radio_tmr_start_us(0U, subevent_us);
	LL_ASSERT(start_us == (subevent_us + 1U));

	radio_isr_set(isr_tx, cis_lll);
}

static void isr_tx(void *param)
{
	struct lll_conn_iso_stream *cis_lll;
	struct lll_conn_iso_group *cig_lll;
	struct node_rx_pdu *node_rx;
	uint32_t subevent_us;
	uint32_t start_us;
	uint32_t hcto;

	lll_isr_tx_sub_status_reset();

	/* Get reference to CIS LLL context */
	cis_lll = param;

	node_rx = ull_iso_pdu_rx_alloc_peek(1U);
	LL_ASSERT(node_rx);

#if defined(CONFIG_BT_CTLR_LE_ENC)
	/* Get reference to ACL context */
	struct lll_conn *conn_lll = ull_conn_lll_get(cis_lll->acl_handle);
#endif /* CONFIG_BT_CTLR_LE_ENC */

	/* Encryption */
	if (false) {

#if defined(CONFIG_BT_CTLR_LE_ENC)
	} else if (conn_lll->enc_rx) {
		uint64_t payload_count;
		uint8_t pkt_flags;

		payload_count = (cis_lll->event_count * cis_lll->rx.bn) +
				(bn_rx - 1U);
		cis_lll->rx.ccm.counter = payload_count;

		pkt_flags = RADIO_PKT_CONF_FLAGS(RADIO_PKT_CONF_PDU_TYPE_DC,
						 cis_lll->rx.phy,
						 RADIO_PKT_CONF_CTE_DISABLED);
		radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT,
				    (cis_lll->rx.max_pdu + PDU_MIC_SIZE),
				    pkt_flags);
		radio_pkt_rx_set(radio_ccm_rx_pkt_set(&cis_lll->rx.ccm,
						      cis_lll->rx.phy,
						      node_rx->pdu));
#endif /* CONFIG_BT_CTLR_LE_ENC */

	} else {
		uint8_t pkt_flags;

		pkt_flags = RADIO_PKT_CONF_FLAGS(RADIO_PKT_CONF_PDU_TYPE_DC,
						 cis_lll->rx.phy,
						 RADIO_PKT_CONF_CTE_DISABLED);
		radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT,
				    cis_lll->rx.max_pdu, pkt_flags);
		radio_pkt_rx_set(node_rx->pdu);
	}

	radio_aa_set(cis_lll->access_addr);

	lll_chan_set(next_chan_use);

	radio_tmr_tx_disable();
	radio_tmr_rx_enable();

	radio_tmr_tifs_set(EVENT_IFS_US);

#if defined(CONFIG_BT_CTLR_PHY)
	radio_switch_complete_and_tx(cis_lll->rx.phy, 0U, cis_lll->tx.phy,
				     cis_lll->tx.phy_flags);
#else /* !CONFIG_BT_CTLR_PHY */
	radio_switch_complete_and_tx(0U, 0U, 0U, 0U);
#endif /* !CONFIG_BT_CTLR_PHY */

	cig_lll = ull_conn_iso_lll_group_get_by_stream(cis_lll);

	subevent_us = radio_tmr_aa_restore();
	subevent_us += cis_lll->offset - cis_offset_first +
		       (cis_lll->sub_interval * se_curr);
	subevent_us -= addr_us_get(cis_lll->rx.phy);

#if defined(CONFIG_BT_CTLR_PHY)
	subevent_us -= radio_rx_ready_delay_get(cis_lll->rx.phy,
						PHY_FLAGS_S8);
	subevent_us -= radio_rx_chain_delay_get(cis_lll->rx.phy,
						PHY_FLAGS_S8);
#else /* !CONFIG_BT_CTLR_PHY */
	subevent_us -= radio_rx_ready_delay_get(0U, 0U);
	subevent_us -= radio_rx_chain_delay_get(0U, 0U);
#endif /* !CONFIG_BT_CTLR_PHY */

	/* Compensate for the 1 us added by radio_tmr_start_us() */
	start_us = subevent_us + 1U;

	hcto = start_us +
	       ((EVENT_JITTER_US + EVENT_TICKER_RES_MARGIN_US +
		 EVENT_US_FRAC_TO_US(cig_lll->window_widening_event_us_frac)) <<
		1U);

#if defined(CONFIG_BT_CTLR_PHY)
	hcto += radio_rx_ready_delay_get(cis_lll->rx.phy, PHY_FLAGS_S8);
	hcto += addr_us_get(cis_lll->rx.phy);
	hcto += radio_rx_chain_delay_get(cis_lll->rx.phy, PHY_FLAGS_S8);
#else /* !CONFIG_BT_CTLR_PHY */
	hcto += radio_rx_ready_delay_get(0U, 0U);
	hcto += addr_us_get(0U);
	hcto += radio_rx_chain_delay_get(0U, 0U);
#endif /* !CONFIG_BT_CTLR_PHY */

	radio_tmr_hcto_configure(hcto);

#if defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
	radio_gpio_lna_setup();

#if defined(CONFIG_BT_CTLR_PHY)
	radio_gpio_pa_lna_enable(start_us +
				 radio_rx_ready_delay_get(cis_lll->rx.phy,
							  PHY_FLAGS_S8) -
				 HAL_RADIO_GPIO_LNA_OFFSET);
#else /* !CONFIG_BT_CTLR_PHY */
	radio_gpio_pa_lna_enable(start_us +
				 radio_rx_ready_delay_get(0U, 0U) -
				 HAL_RADIO_GPIO_LNA_OFFSET);
#endif /* !CONFIG_BT_CTLR_PHY */
#endif /* HAL_RADIO_GPIO_HAVE_LNA_PIN */

	radio_isr_set(isr_rx, cis_lll);

	se_curr++;
}

static void next_cis_prepare(void *param)
{
	struct lll_conn_iso_stream *cis_lll;
	struct lll_conn_iso_group *cig_lll;
	uint16_t cis_handle;

	/* Get reference to CIS LLL context */
	cis_lll = param;

	/* Check for next active CIS */
	cig_lll = ull_conn_iso_lll_group_get_by_stream(cis_lll);
	cis_handle = cis_handle_curr;
	do {
		cis_lll = ull_conn_iso_lll_stream_get_by_group(cig_lll, &cis_handle);
	} while (cis_lll && !cis_lll->active);

	if (!cis_lll) {
		/* ISO Event Done */
		radio_isr_set(isr_done, param);

		return;
	}

	cis_handle_curr = cis_handle;

	radio_isr_set(isr_prepare_subevent_next_cis, cis_lll);
}

static void isr_prepare_subevent(void *param)
{
	struct lll_conn_iso_stream *cis_lll;
	struct lll_conn *conn_lll;

	lll_isr_status_reset();

	/* Get reference to CIS LLL context */
	cis_lll = param;

	/* Get reference to ACL context */
	conn_lll = ull_conn_lll_get(cis_lll->acl_handle);

	/* Calculate the radio channel to use for next subevent
	 */
	next_chan_use = lll_chan_iso_subevent(data_chan_id,
					      conn_lll->data_chan_map,
					      conn_lll->data_chan_count,
					      &data_chan_prn_s,
					      &data_chan_remap_idx);

	isr_prepare_subevent_common(param);
}

static void isr_prepare_subevent_next_cis(void *param)
{
	struct lll_conn_iso_stream *cis_lll;
	struct lll_conn *conn_lll;
	uint16_t event_counter;

	lll_isr_status_reset();

	/* Get reference to CIS LLL context */
	cis_lll = param;

	/* Get reference to ACL context */
	conn_lll = ull_conn_lll_get(cis_lll->acl_handle);

	/* Event counter value,  0-15 bit of cisEventCounter */
	event_counter = cis_lll->event_count;

	/* Calculate the radio channel to use for next CIS ISO event */
	data_chan_id = lll_chan_id(cis_lll->access_addr);
	next_chan_use = lll_chan_iso_event(event_counter, data_chan_id,
					   conn_lll->data_chan_map,
					   conn_lll->data_chan_count,
					   &data_chan_prn_s,
					   &data_chan_remap_idx);

	/* Reset indices for the next CIS */
	se_curr = 0U; /* isr_prepare_subevent_common() will increase se_curr */
	bn_tx = 1U; /* FIXME: may be this should be previous event value? */
	bn_rx = 1U;
	has_tx = 0U;

	isr_prepare_subevent_common(param);
}

static void isr_prepare_subevent_common(void *param)
{
	struct lll_conn_iso_stream *cis_lll;
	struct lll_conn_iso_group *cig_lll;
	struct node_rx_pdu *node_rx;
	uint32_t subevent_us;
	uint32_t start_us;
	uint32_t hcto;

	/* Get reference to CIS LLL context */
	cis_lll = param;

	node_rx = ull_iso_pdu_rx_alloc_peek(1U);
	LL_ASSERT(node_rx);

#if defined(CONFIG_BT_CTLR_LE_ENC)
	/* Get reference to ACL context */
	struct lll_conn *conn_lll = ull_conn_lll_get(cis_lll->acl_handle);
#endif /* CONFIG_BT_CTLR_LE_ENC */

	/* Encryption */
	if (false) {

#if defined(CONFIG_BT_CTLR_LE_ENC)
	} else if (conn_lll->enc_rx) {
		uint64_t payload_count;
		uint8_t pkt_flags;

		payload_count = (cis_lll->event_count * cis_lll->rx.bn) +
				(bn_rx - 1U);
		cis_lll->rx.ccm.counter = payload_count;

		pkt_flags = RADIO_PKT_CONF_FLAGS(RADIO_PKT_CONF_PDU_TYPE_DC,
						 cis_lll->rx.phy,
						 RADIO_PKT_CONF_CTE_DISABLED);
		radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT,
				    (cis_lll->rx.max_pdu + PDU_MIC_SIZE),
				    pkt_flags);
		radio_pkt_rx_set(radio_ccm_rx_pkt_set(&cis_lll->rx.ccm,
						      cis_lll->rx.phy,
						      node_rx->pdu));
#endif /* CONFIG_BT_CTLR_LE_ENC */

	} else {
		uint8_t pkt_flags;

		pkt_flags = RADIO_PKT_CONF_FLAGS(RADIO_PKT_CONF_PDU_TYPE_DC,
						 cis_lll->rx.phy,
						 RADIO_PKT_CONF_CTE_DISABLED);
		radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT,
				    cis_lll->rx.max_pdu, pkt_flags);
		radio_pkt_rx_set(node_rx->pdu);
	}

	radio_aa_set(cis_lll->access_addr);

	lll_chan_set(next_chan_use);

	radio_tmr_tx_disable();
	radio_tmr_rx_enable();

	radio_tmr_tifs_set(EVENT_IFS_US);

#if defined(CONFIG_BT_CTLR_PHY)
	radio_switch_complete_and_tx(cis_lll->rx.phy, 0U, cis_lll->tx.phy,
				     cis_lll->tx.phy_flags);
#else /* !CONFIG_BT_CTLR_PHY */
	radio_switch_complete_and_tx(0U, 0U, 0U, 0U);
#endif /* !CONFIG_BT_CTLR_PHY */

	/* Anchor point sync-ed */
	if (trx_performed_bitmask) {
		subevent_us = radio_tmr_aa_restore();
		subevent_us += cis_lll->offset - cis_offset_first +
			       (cis_lll->sub_interval * se_curr);
		subevent_us -= addr_us_get(cis_lll->rx.phy);

#if defined(CONFIG_BT_CTLR_PHY)
		subevent_us -= radio_rx_ready_delay_get(cis_lll->rx.phy,
							PHY_FLAGS_S8);
		subevent_us -= radio_rx_chain_delay_get(cis_lll->rx.phy,
							PHY_FLAGS_S8);
#else /* !CONFIG_BT_CTLR_PHY */
		subevent_us -= radio_rx_ready_delay_get(0U, 0U);
		subevent_us -= radio_rx_chain_delay_get(0U, 0U);
#endif /* !CONFIG_BT_CTLR_PHY */
	} else {
		subevent_us = radio_tmr_ready_restore();
		subevent_us += cis_lll->offset - cis_offset_first +
			       (cis_lll->sub_interval * se_curr);
	}

	start_us = radio_tmr_start_us(0U, subevent_us);
	LL_ASSERT(!trx_performed_bitmask || (start_us == (subevent_us + 1U)));

	/* If no anchor point sync yet, continue to capture access address
	 * timestamp.
	 */
	if (!radio_tmr_aa_restore()) {
		radio_tmr_aa_capture();
	}

	cig_lll = ull_conn_iso_lll_group_get_by_stream(cis_lll);

	hcto = start_us +
	       ((EVENT_JITTER_US + EVENT_TICKER_RES_MARGIN_US +
		 EVENT_US_FRAC_TO_US(cig_lll->window_widening_event_us_frac)) <<
		1U);

#if defined(CONFIG_BT_CTLR_PHY)
	hcto += radio_rx_ready_delay_get(cis_lll->rx.phy, PHY_FLAGS_S8);
	hcto += addr_us_get(cis_lll->rx.phy);
	hcto += radio_rx_chain_delay_get(cis_lll->rx.phy, PHY_FLAGS_S8);
#else /* !CONFIG_BT_CTLR_PHY */
	hcto += radio_rx_ready_delay_get(0U, 0U);
	hcto += addr_us_get(0U);
	hcto += radio_rx_chain_delay_get(0U, 0U);
#endif /* !CONFIG_BT_CTLR_PHY */

	radio_tmr_hcto_configure(hcto);

#if defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
	radio_gpio_lna_setup();

#if defined(CONFIG_BT_CTLR_PHY)
	radio_gpio_pa_lna_enable(start_us +
				 radio_rx_ready_delay_get(cis_lll->rx.phy,
							  PHY_FLAGS_S8) -
				 HAL_RADIO_GPIO_LNA_OFFSET);
#else /* !CONFIG_BT_CTLR_PHY */
	radio_gpio_pa_lna_enable(start_us +
				 radio_rx_ready_delay_get(0U, 0U) -
				 HAL_RADIO_GPIO_LNA_OFFSET);
#endif /* !CONFIG_BT_CTLR_PHY */
#endif /* HAL_RADIO_GPIO_HAVE_LNA_PIN */

	radio_isr_set(isr_rx, cis_lll);

	se_curr++;
}

static void isr_done(void *param)
{
	struct lll_conn_iso_stream *cis_lll;
	struct event_done_extra *e;
	uint8_t bn;

	lll_isr_status_reset();

	/* Get reference to CIS LLL context */
	cis_lll = param;

	/* Adjust nesn when flushing Rx */
	/* FIXME: When Flush Timeout is implemented */
	if (bn_rx <= cis_lll->rx.bn) {
		cis_lll->nesn += cis_lll->rx.bn + 1U - bn_rx;
	}

	/* Generate ISO Data Invalid Status */
	bn = bn_rx;
	while (bn <= cis_lll->rx.bn) {
		struct node_rx_iso_meta *iso_meta;
		struct node_rx_pdu *node_rx;

		node_rx = ull_iso_pdu_rx_alloc_peek(2U);
		if (!node_rx) {
			break;
		}

		node_rx->hdr.type = NODE_RX_TYPE_ISO_PDU;
		node_rx->hdr.handle = cis_lll->handle;
		iso_meta = &node_rx->hdr.rx_iso_meta;
		iso_meta->payload_number = (cis_lll->event_count *
					    cis_lll->rx.bn) + (bn - 1U);
		if (trx_performed_bitmask) {
			iso_meta->timestamp =
				HAL_TICKER_TICKS_TO_US(radio_tmr_start_get()) +
				radio_tmr_aa_restore() -
				addr_us_get(cis_lll->rx.phy);
		} else {
			iso_meta->timestamp =
				HAL_TICKER_TICKS_TO_US(radio_tmr_start_get()) +
				radio_tmr_ready_restore();
		}
		iso_meta->timestamp %=
			HAL_TICKER_TICKS_TO_US(BIT(HAL_TICKER_CNTR_MSBIT + 1U));
		iso_meta->status = 1U;

		ull_iso_pdu_rx_alloc();
		iso_rx_put(node_rx->hdr.link, node_rx);

		bn++;
	}

#if !defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
	if (bn != bn_rx) {
		iso_rx_sched();
	}
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */

	e = ull_event_done_extra_get();
	LL_ASSERT(e);

	e->type = EVENT_DONE_EXTRA_TYPE_CIS;
	e->trx_performed_bitmask = trx_performed_bitmask;
	e->crc_valid = 1U;

#if defined(CONFIG_BT_CTLR_LE_ENC)
	e->mic_state = mic_state;
#endif /* CONFIG_BT_CTLR_LE_ENC */

	if (trx_performed_bitmask) {
		struct lll_conn_iso_group *cig_lll;
		uint32_t preamble_to_addr_us;

		cig_lll = ull_conn_iso_lll_group_get_by_stream(cis_lll);

#if defined(CONFIG_BT_CTLR_PHY)
		preamble_to_addr_us = addr_us_get(cis_lll->rx.phy);
#else /* !CONFIG_BT_CTLR_PHY */
		preamble_to_addr_us = addr_us_get(0U);
#endif /* !CONFIG_BT_CTLR_PHY */

		e->drift.start_to_address_actual_us =
			radio_tmr_aa_restore() - radio_tmr_ready_restore();
		e->drift.window_widening_event_us = EVENT_US_FRAC_TO_US(
				cig_lll->window_widening_event_us_frac);
		e->drift.preamble_to_addr_us = preamble_to_addr_us;

		/* Reset window widening, as anchor point sync-ed */
		cig_lll->window_widening_event_us_frac = 0U;
	}

	lll_isr_cleanup(param);
}
