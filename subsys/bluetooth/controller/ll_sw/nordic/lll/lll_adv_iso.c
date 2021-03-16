/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <soc.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/memq.h"

#include "pdu.h"

#include "lll.h"
#include "lll_clock.h"
#include "lll_chan.h"
#include "lll_vendor.h"
#include "lll_adv_types.h"
#include "lll_adv.h"
#include "lll_adv_pdu.h"
#include "lll_adv_iso.h"

#include "lll_internal.h"
#include "lll_adv_internal.h"
#include "lll_tim_internal.h"

#include "ll_feat.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_lll_adv_iso
#include "common/log.h"
#include "hal/debug.h"

static int init_reset(void);
static void prepare(void *param);
static void create_prepare_bh(void *param);
static void prepare_bh(void *param);
static int create_prepare_cb(struct lll_prepare_param *p);
static int prepare_cb(struct lll_prepare_param *p);
static int prepare_cb_common(struct lll_prepare_param *p);
static void isr_create_done(void *param);

int lll_adv_iso_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int lll_adv_iso_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

void lll_adv_iso_create_prepare(void *param)
{
	prepare(param);
	create_prepare_bh(param);
}

void lll_adv_iso_prepare(void *param)
{
	prepare(param);
	prepare_bh(param);
}

static int init_reset(void)
{
	return 0;
}

static void prepare(void *param)
{
	struct lll_prepare_param *p;
	struct lll_adv_iso *lll;
	uint16_t elapsed;
	int err;

	err = lll_hfclock_on();
	LL_ASSERT(err >= 0);

	p = param;

	/* Instants elapsed */
	elapsed = p->lazy + 1;

	lll = p->param;

	/* Save the (latency + 1) for use in event */
	lll->latency_prepare += elapsed;
}

static void create_prepare_bh(void *param)
{
	int err;

	/* Invoke common pipeline handling of prepare */
	err = lll_prepare(lll_is_abort_cb, lll_abort_cb, create_prepare_cb, 0,
			  param);
	LL_ASSERT(!err || err == -EINPROGRESS);
}

static void prepare_bh(void *param)
{
	int err;

	/* Invoke common pipeline handling of prepare */
	err = lll_prepare(lll_is_abort_cb, lll_abort_cb, prepare_cb, 0, param);
	LL_ASSERT(!err || err == -EINPROGRESS);
}

static int create_prepare_cb(struct lll_prepare_param *p)
{
	int err;

	err = prepare_cb_common(p);
	if (err) {
		DEBUG_RADIO_START_A(1);
		return 0;
	}

	radio_isr_set(isr_create_done, p->param);

	DEBUG_RADIO_START_A(1);
	return 0;
}

static int prepare_cb(struct lll_prepare_param *p)
{
	int err;

	err = prepare_cb_common(p);
	if (err) {
		DEBUG_RADIO_START_A(1);
		return 0;
	}

	radio_isr_set(lll_isr_done, p->param);

	DEBUG_RADIO_START_A(1);
	return 0;
}

static int prepare_cb_common(struct lll_prepare_param *p)
{
	struct lll_adv_iso *lll;
	uint32_t ticks_at_event;
	uint32_t ticks_at_start;
	uint16_t event_counter;
	uint8_t access_addr[4];
	uint16_t data_chan_id;
	uint8_t data_chan_use;
	uint8_t crc_init[3];
	struct pdu_bis *pdu;
	struct ull_hdr *ull;
	uint32_t remainder;
	uint32_t start_us;
	uint8_t phy;
	uint8_t bis;

	DEBUG_RADIO_START_A(1);

	lll = p->param;

	/* Deduce the latency */
	lll->latency_event = lll->latency_prepare - 1;

	/* Calculate the current event counter value */
	event_counter = ((lll->payload_count / lll->bn) & 0xFFFF) +
			(lll->latency_event * lll->bn);

	/* Update BIS packet counter to next value */
	lll->payload_count += (lll->latency_prepare * lll->bn);

	/* Reset accumulated latencies */
	lll->latency_prepare = 0;


	/* Calculate the Access Address for the BIS event */
	bis = 1U;
	util_bis_aa_le32(bis, lll->seed_access_addr, access_addr);
	data_chan_id = lll_chan_id(access_addr);

	/* Calculate the CRC init value for the BIS event,
	 * preset with the BaseCRCInit value from the BIGInfo data the most
	 * significant 2 octets and the BIS_Number for the specific BIS in the
	 * least significant octet.
	 */
	memcpy(&crc_init[1], lll->base_crc_init, sizeof(uint16_t));
	crc_init[0] = bis;

	/* Calculate the radio channel to use for ISO event */
	data_chan_use = lll_chan_iso_event(event_counter, data_chan_id,
					   &lll->data_chan_map[0],
					   lll->data_chan_count,
					   &lll->data_chan_prn_s,
					   &lll->data_chan_remap_idx);

	/* Start setting up of Radio h/w */
	radio_reset();

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	radio_tx_power_set(lll->tx_pwr_lvl);
#else
	radio_tx_power_set(RADIO_TXP_DEFAULT);
#endif

	phy = lll->phy;
	radio_phy_set(phy, 1);
	radio_pkt_configure(8, LL_BIS_OCTETS_TX_MAX, (phy << 1));
	radio_aa_set(access_addr);
	radio_crc_configure(((0x5bUL) | ((0x06UL) << 8) | ((0x00UL) << 16)),
			    (((uint32_t)crc_init[2] << 16) |
			     ((uint32_t)crc_init[1] << 8) |
			     ((uint32_t)crc_init[0])));
	lll_chan_set(data_chan_use);

	/* FIXME: get ISO data PDU */
	pdu = (void *)radio_pkt_empty_get();
	pdu->ll_id = PDU_BIS_LLID_START_CONTINUE;
	pdu->cssn = 0;
	pdu->cstf = 0;
	pdu->rfu = 0;
	pdu->len = 0;
	radio_pkt_tx_set(pdu);

	radio_switch_complete_and_disable();

	ticks_at_event = p->ticks_at_expire;
	ull = HDR_LLL2ULL(lll);
	ticks_at_event += lll_event_offset_get(ull);

	ticks_at_start = ticks_at_event;
	ticks_at_start += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

	remainder = p->remainder;
	start_us = radio_tmr_start(1, ticks_at_start, remainder);

#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN)
	radio_gpio_pa_setup();

	radio_gpio_pa_lna_enable(start_us + radio_tx_ready_delay_get(phy, 1) -
				 CONFIG_BT_CTLR_GPIO_PA_OFFSET);
#else /* !CONFIG_BT_CTLR_GPIO_PA_PIN */
	ARG_UNUSED(start_us);
#endif /* !CONFIG_BT_CTLR_GPIO_PA_PIN */

	if (0) {
#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
	(EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
	/* check if preempt to start has changed */
	} else if (lll_preempt_calc(ull, (TICKER_ID_ADV_ISO_BASE + lll->handle),
				    ticks_at_event)) {
		radio_isr_set(lll_isr_abort, lll);
		radio_disable();

		return -ECANCELED;
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
	} else {
		uint32_t ret;

		ret = lll_prepare_done(lll);
		LL_ASSERT(!ret);
	}

	return 0;
}

static void isr_create_done(void *param)
{
	struct event_done_extra *extra;

	lll_isr_status_reset();

	extra = ull_event_done_extra_get();
	LL_ASSERT(extra);

	extra->type = EVENT_DONE_EXTRA_TYPE_ADV_ISO_CREATED;

	lll_isr_cleanup(param);
}
