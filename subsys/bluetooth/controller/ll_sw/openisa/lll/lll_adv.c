/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>

#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/sys/byteorder.h>
#include <soc.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mfifo.h"

#include "ticker/ticker.h"

#include "pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll_vendor.h"
#include "lll_adv_types.h"
#include "lll_adv.h"
#include "lll_adv_pdu.h"
#include "lll_df_types.h"
#include "lll_conn.h"
#include "lll_chan.h"
#include "lll_filter.h"

#include "lll_internal.h"
#include "lll_tim_internal.h"
#include "lll_adv_internal.h"
#include "lll_prof_internal.h"

#include "hal/debug.h"

static int init_reset(void);
static int prepare_cb(struct lll_prepare_param *prepare_param);
static int is_abort_cb(void *next, void *curr, lll_prepare_cb_t *resume_cb);
static void abort_cb(struct lll_prepare_param *prepare_param, void *param);
static void isr_tx(void *param);
static void isr_rx(void *param);
static void isr_done(void *param);
static void isr_abort(void *param);
static void isr_cleanup(void *param);
static void isr_race(void *param);
static void chan_prepare(struct lll_adv *lll);
static inline int isr_rx_pdu(struct lll_adv *lll,
			     uint8_t devmatch_ok, uint8_t devmatch_id,
			     uint8_t irkmatch_ok, uint8_t irkmatch_id,
			     uint8_t rssi_ready);
static inline bool isr_rx_sr_check(struct lll_adv *lll, struct pdu_adv *adv,
				   struct pdu_adv *sr, uint8_t devmatch_ok,
				   uint8_t *rl_idx);
static inline bool isr_rx_sr_adva_check(struct pdu_adv *adv,
					struct pdu_adv *sr);
#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
static inline int isr_rx_sr_report(struct pdu_adv *pdu_adv_rx,
				   uint8_t rssi_ready);
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */
static inline bool isr_rx_ci_check(struct lll_adv *lll, struct pdu_adv *adv,
				   struct pdu_adv *ci, uint8_t devmatch_ok,
				   uint8_t *rl_idx);
static inline bool isr_rx_ci_tgta_check(struct lll_adv *lll,
					struct pdu_adv *adv, struct pdu_adv *ci,
					uint8_t rl_idx);
static inline bool isr_rx_ci_adva_check(struct pdu_adv *adv,
					struct pdu_adv *ci);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
#define PAYLOAD_BASED_FRAG_COUNT \
	DIV_ROUND_UP(CONFIG_BT_CTLR_ADV_DATA_LEN_MAX, \
			 PDU_AC_PAYLOAD_SIZE_MAX)
#define BT_CTLR_ADV_AUX_SET  CONFIG_BT_CTLR_ADV_AUX_SET
#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
#define BT_CTLR_ADV_SYNC_SET CONFIG_BT_CTLR_ADV_SYNC_SET
#else /* !CONFIG_BT_CTLR_ADV_PERIODIC */
#define BT_CTLR_ADV_SYNC_SET 0
#endif /* !CONFIG_BT_CTLR_ADV_PERIODIC */
#else
#define PAYLOAD_FRAG_COUNT   1
#define BT_CTLR_ADV_AUX_SET  0
#define BT_CTLR_ADV_SYNC_SET 0
#endif

#define PDU_MEM_SIZE       MROUND(PDU_AC_LL_HEADER_SIZE + \
				  PDU_AC_PAYLOAD_SIZE_MAX)
#define PDU_MEM_COUNT_MIN  (BT_CTLR_ADV_SET + \
			    (BT_CTLR_ADV_SET * PAYLOAD_FRAG_COUNT) + \
			    (BT_CTLR_ADV_AUX_SET * PAYLOAD_FRAG_COUNT) + \
			    (BT_CTLR_ADV_SYNC_SET * PAYLOAD_FRAG_COUNT))
#define PDU_MEM_FIFO_COUNT ((BT_CTLR_ADV_SET * PAYLOAD_FRAG_COUNT * 2) + \
			    (CONFIG_BT_CTLR_ADV_DATA_BUF_MAX * \
			     PAYLOAD_FRAG_COUNT))
#define PDU_MEM_COUNT      (PDU_MEM_COUNT_MIN + PDU_MEM_FIFO_COUNT)
#define PDU_POOL_SIZE      (PDU_MEM_SIZE * PDU_MEM_COUNT)

/* Free AD data PDU buffer pool */
static struct {
	void *free;
	uint8_t pool[PDU_POOL_SIZE];
} mem_pdu;

/* FIFO to return stale AD data PDU buffers from LLL to thread context */
static MFIFO_DEFINE(pdu_free, sizeof(void *), PDU_MEM_FIFO_COUNT);

/* Semaphore to wakeup thread waiting for free AD data PDU buffers */
static struct k_sem sem_pdu_free;

int lll_adv_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int lll_adv_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int lll_adv_data_init(struct lll_adv_pdu *pdu)
{
	struct pdu_adv *p;

	p = mem_acquire(&mem_pdu.free);
	if (!p) {
		return -ENOMEM;
	}

	p->len = 0U;
	pdu->pdu[0] = (void *)p;

	return 0;
}

int lll_adv_data_reset(struct lll_adv_pdu *pdu)
{
	/* NOTE: this function is used on HCI reset to mem-zero the structure
	 *       members that otherwise was zero-ed by the architecture
	 *       startup code that zero-ed the .bss section.
	 *       pdu[0] element in the array is not initialized as subsequent
	 *       call to lll_adv_data_init will allocate a PDU buffer and
	 *       assign that.
	 */

	pdu->first = 0U;
	pdu->last = 0U;
	pdu->pdu[1] = NULL;

	return 0;
}

int lll_adv_data_release(struct lll_adv_pdu *pdu)
{
	uint8_t last;
	void *p;

	last = pdu->last;
	p = pdu->pdu[last];
	pdu->pdu[last] = NULL;
	mem_release(p, &mem_pdu.free);

	last++;
	if (last == DOUBLE_BUFFER_SIZE) {
		last = 0U;
	}
	p = pdu->pdu[last];
	if (p) {
		pdu->pdu[last] = NULL;
		mem_release(p, &mem_pdu.free);
	}

	return 0;
}

struct pdu_adv *lll_adv_pdu_alloc(struct lll_adv_pdu *pdu, uint8_t *idx)
{
	uint8_t first, last;
	struct pdu_adv *p;
	int err;

	first = pdu->first;
	last = pdu->last;
	if (first == last) {
		last++;
		if (last == DOUBLE_BUFFER_SIZE) {
			last = 0U;
		}
	} else {
		uint8_t first_latest;

		pdu->last = first;
		/* NOTE: Ensure that data is synchronized so that an ISR
		 *        vectored, after pdu->last has been updated, does
		 *        access the latest value.
		 */
		cpu_dmb();
		first_latest = pdu->first;
		if (first_latest != first) {
			last++;
			if (last == DOUBLE_BUFFER_SIZE) {
				last = 0U;
			}
		}
	}

	*idx = last;

	p = (void *)pdu->pdu[last];
	if (p) {
		return p;
	}

	p = MFIFO_DEQUEUE_PEEK(pdu_free);
	if (p) {
		err = k_sem_take(&sem_pdu_free, K_NO_WAIT);
		LL_ASSERT(!err);

		MFIFO_DEQUEUE(pdu_free);
		pdu->pdu[last] = (void *)p;

		return p;
	}

	p = mem_acquire(&mem_pdu.free);
	if (p) {
		pdu->pdu[last] = (void *)p;

		return p;
	}

	err = k_sem_take(&sem_pdu_free, K_FOREVER);
	LL_ASSERT(!err);

	p = MFIFO_DEQUEUE(pdu_free);
	LL_ASSERT(p);

	pdu->pdu[last] = (void *)p;

	return p;
}

struct pdu_adv *lll_adv_pdu_latest_get(struct lll_adv_pdu *pdu,
				       uint8_t *is_modified)
{
	uint8_t first;

	first = pdu->first;
	if (first != pdu->last) {
		uint8_t free_idx;
		uint8_t pdu_idx;
		void *p;

		if (!MFIFO_ENQUEUE_IDX_GET(pdu_free, &free_idx)) {
			return NULL;
		}

		pdu_idx = first;

		first += 1U;
		if (first == DOUBLE_BUFFER_SIZE) {
			first = 0U;
		}
		pdu->first = first;
		*is_modified = 1U;

		p = pdu->pdu[pdu_idx];
		pdu->pdu[pdu_idx] = NULL;

		MFIFO_BY_IDX_ENQUEUE(pdu_free, free_idx, p);
		k_sem_give(&sem_pdu_free);
	}

	return (void *)pdu->pdu[first];
}

void lll_adv_prepare(void *param)
{
	struct lll_prepare_param *p = param;
	int err;

	err = lll_clk_on();
	LL_ASSERT(!err || err == -EINPROGRESS);

	err = lll_prepare(is_abort_cb, abort_cb, prepare_cb, 0, p);
	LL_ASSERT(!err || err == -EINPROGRESS);
}

static int init_reset(void)
{
	return 0;
}

static int prepare_cb(struct lll_prepare_param *prepare_param)
{
	struct lll_adv *lll = prepare_param->param;
	uint32_t aa = sys_cpu_to_le32(PDU_AC_ACCESS_ADDR);
	uint32_t ticks_at_event, ticks_at_start;
	uint32_t remainder_us;
	struct ull_hdr *ull;
	uint32_t remainder;

	DEBUG_RADIO_START_A(1);

#if defined(CONFIG_BT_PERIPHERAL)
	/* Check if stopped (on connection establishment race between LLL and
	 * ULL.
	 */
	if (unlikely(lll->conn && lll->conn->central.initiated)) {
		int err;

		err = lll_clk_off();
		LL_ASSERT(!err || err == -EBUSY);

		lll_done(NULL);

		DEBUG_RADIO_START_A(0);
		return 0;
	}
#endif /* CONFIG_BT_PERIPHERAL */

	radio_reset();
	/* TODO: other Tx Power settings */
	radio_tx_power_set(RADIO_TXP_DEFAULT);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	/* TODO: if coded we use S8? */
	radio_phy_set(lll->phy_p, 1);
	radio_pkt_configure(8, PDU_AC_LEG_PAYLOAD_SIZE_MAX, (lll->phy_p << 1));
#else /* !CONFIG_BT_CTLR_ADV_EXT */
	radio_phy_set(0, 0);
	radio_pkt_configure(8, PDU_AC_LEG_PAYLOAD_SIZE_MAX, 0);
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

	radio_aa_set((uint8_t *)&aa);
	radio_crc_configure(((0x5bUL) | ((0x06UL) << 8) | ((0x00UL) << 16)),
			    0x555555);

	lll->chan_map_curr = lll->chan_map;

	chan_prepare(lll);

#if defined(CONFIG_BT_HCI_MESH_EXT)
	_radio.mesh_adv_end_us = 0;
#endif /* CONFIG_BT_HCI_MESH_EXT */


#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (ull_filter_lll_rl_enabled()) {
		struct lll_filter *filter =
			ull_filter_lll_get(!!(lll->filter_policy));

		radio_filter_configure(filter->enable_bitmask,
				       filter->addr_type_bitmask,
				       (uint8_t *)filter->bdaddr);
	} else
#endif /* CONFIG_BT_CTLR_PRIVACY */

		if (IS_ENABLED(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST) && lll->filter_policy) {
			/* Setup Radio Filter */
			struct lll_filter *fal = ull_filter_lll_get(true);

			radio_filter_configure(fal->enable_bitmask,
					       fal->addr_type_bitmask,
					       (uint8_t *)fal->bdaddr);
		}

	ticks_at_event = prepare_param->ticks_at_expire;
	ull = HDR_LLL2ULL(lll);
	ticks_at_event += lll_event_offset_get(ull);

	ticks_at_start = ticks_at_event;
	ticks_at_start += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

	remainder = prepare_param->remainder;
	remainder_us = radio_tmr_start(1, ticks_at_start, remainder);

	/* capture end of Tx-ed PDU, used to calculate HCTO. */
	radio_tmr_end_capture();

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
	radio_gpio_pa_setup();
	radio_gpio_pa_lna_enable(remainder_us +
				 radio_tx_ready_delay_get(0, 0) -
				 HAL_RADIO_GPIO_PA_OFFSET);
#else /* !HAL_RADIO_GPIO_HAVE_PA_PIN */
	ARG_UNUSED(remainder_us);
#endif /* !HAL_RADIO_GPIO_HAVE_PA_PIN */

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
	(EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
	/* check if preempt to start has changed */
	if (lll_preempt_calc(ull, (TICKER_ID_ADV_BASE +
				   ull_adv_lll_handle_get(lll)),
			     ticks_at_event)) {
		radio_isr_set(isr_abort, lll);
		radio_disable();
	} else
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
	{
		uint32_t ret;

		ret = lll_prepare_done(lll);
		LL_ASSERT(!ret);
	}

	DEBUG_RADIO_START_A(1);

	return 0;
}

#if defined(CONFIG_BT_PERIPHERAL)
static int resume_prepare_cb(struct lll_prepare_param *p)
{
	struct ull_hdr *ull = HDR_LLL2ULL(p->param);

	p->ticks_at_expire = ticker_ticks_now_get() - lll_event_offset_get(ull);
	p->remainder = 0;
	p->lazy = 0;

	return prepare_cb(p);
}
#endif /* CONFIG_BT_PERIPHERAL */

static int is_abort_cb(void *next, void *curr, lll_prepare_cb_t *resume_cb)
{
#if defined(CONFIG_BT_PERIPHERAL)
	struct lll_adv *lll = curr;
	struct pdu_adv *pdu;
#endif /* CONFIG_BT_PERIPHERAL */

	/* TODO: prio check */
	if (next != curr) {
		if (0) {
#if defined(CONFIG_BT_PERIPHERAL)
		} else if (lll->is_hdcd) {
			int err;

			/* wrap back after the pre-empter */
			*resume_cb = resume_prepare_cb;

			/* Retain HF clk */
			err = lll_clk_on();
			LL_ASSERT(!err || err == -EINPROGRESS);

			return -EAGAIN;
#endif /* CONFIG_BT_PERIPHERAL */
		} else {
			return -ECANCELED;
		}
	}

#if defined(CONFIG_BT_PERIPHERAL)
	pdu = lll_adv_data_curr_get(lll);
	if (pdu->type == PDU_ADV_TYPE_DIRECT_IND) {
		return 0;
	}
#endif /* CONFIG_BT_PERIPHERAL */

	return -ECANCELED;
}

static void abort_cb(struct lll_prepare_param *prepare_param, void *param)
{
	int err;

	/* NOTE: This is not a prepare being cancelled */
	if (!prepare_param) {
		/* Perform event abort here.
		 * After event has been cleanly aborted, clean up resources
		 * and dispatch event done.
		 */
		radio_isr_set(isr_abort, param);
		radio_disable();
		return;
	}

	/* NOTE: Else clean the top half preparations of the aborted event
	 * currently in preparation pipeline.
	 */
	err = lll_clk_off();
	LL_ASSERT(!err || err == -EBUSY);

	lll_done(param);
}

static void isr_tx(void *param)
{
	uint32_t hcto;

	/* TODO: MOVE to a common interface, isr_lll_radio_status? */
	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_latency_capture();
	}

	/* Clear radio status and events */
	radio_status_reset();
	radio_tmr_status_reset();

	if (IS_ENABLED(HAL_RADIO_GPIO_HAVE_PA_PIN) ||
	    IS_ENABLED(HAL_RADIO_GPIO_HAVE_LNA_PIN)) {
		radio_gpio_pa_lna_disable();
	}
	/* TODO: MOVE ^^ */

	/* setup tIFS switching */
	radio_tmr_tifs_set(EVENT_IFS_US);
	radio_switch_complete_and_tx(0, 0, 0, 0);

	radio_pkt_rx_set(radio_pkt_scratch_get());
	/* assert if radio packet ptr is not set and radio started rx */
	LL_ASSERT(!radio_is_ready());

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_cputime_capture();
	}

	radio_isr_set(isr_rx, param);

#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (ull_filter_lll_rl_enabled()) {
		uint8_t count, *irks = ull_filter_lll_irks_get(&count);

		radio_ar_configure(count, irks);
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

	/* +/- 2us active clock jitter, +1 us hcto compensation */
	hcto = radio_tmr_tifs_base_get() + EVENT_IFS_US + 4 + 1;
	hcto += radio_rx_chain_delay_get(0, 0);
	hcto += addr_us_get(0);
	hcto -= radio_tx_chain_delay_get(0, 0);
	radio_tmr_hcto_configure(hcto);

	/* capture end of CONNECT_IND PDU, used for calculating first
	 * peripheral event.
	 */
	radio_tmr_end_capture();

	if (IS_ENABLED(CONFIG_BT_CTLR_SCAN_REQ_RSSI) ||
	    IS_ENABLED(CONFIG_BT_CTLR_CONN_RSSI)) {
		radio_rssi_measure();
	}

#if defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		/* PA/LNA enable is overwriting packet end used in ISR
		 * profiling, hence back it up for later use.
		 */
		lll_prof_radio_end_backup();
	}

	radio_gpio_lna_setup();
	radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() + EVENT_IFS_US - 4 -
				 radio_tx_chain_delay_get(0, 0) -
				 HAL_RADIO_GPIO_LNA_OFFSET);
#endif /* HAL_RADIO_GPIO_HAVE_LNA_PIN */

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		/* NOTE: as scratch packet is used to receive, it is safe to
		 * generate profile event using rx nodes.
		 */
		lll_prof_send();
	}
}

static void isr_rx(void *param)
{
	uint8_t trx_done;
	uint8_t crc_ok;
	uint8_t devmatch_ok;
	uint8_t devmatch_id;
	uint8_t irkmatch_ok;
	uint8_t irkmatch_id;
	uint8_t rssi_ready;

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_latency_capture();
	}

	/* Read radio status and events */
	trx_done = radio_is_done();
	if (trx_done) {
		crc_ok = radio_crc_is_valid();
		devmatch_ok = radio_filter_has_match();
		devmatch_id = radio_filter_match_get();
		if (IS_ENABLED(CONFIG_BT_CTLR_PRIVACY)) {
			irkmatch_ok = radio_ar_has_match();
			irkmatch_id = radio_ar_match_get();
		} else {
			irkmatch_ok = 0U;
			irkmatch_id = FILTER_IDX_NONE;
		}
		rssi_ready = radio_rssi_is_ready();
	} else {
		crc_ok = devmatch_ok = irkmatch_ok = rssi_ready = 0U;
		devmatch_id = irkmatch_id = FILTER_IDX_NONE;
	}

	/* Clear radio status and events */
	lll_isr_status_reset();

	if (IS_ENABLED(HAL_RADIO_GPIO_HAVE_PA_PIN) ||
	    IS_ENABLED(HAL_RADIO_GPIO_HAVE_LNA_PIN)) {
		radio_gpio_pa_lna_disable();
	}

	if (!trx_done) {
		goto isr_rx_do_close;
	}

	if (crc_ok) {
		int err;

		err = isr_rx_pdu(param, devmatch_ok, devmatch_id, irkmatch_ok,
				 irkmatch_id, rssi_ready);
		if (!err) {
			if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
				lll_prof_send();
			}

			return;
		}
	}

isr_rx_do_close:
	radio_isr_set(isr_done, param);
	radio_disable();
}

static void isr_done(void *param)
{
	struct node_rx_hdr *node_rx;
	struct lll_adv *lll = param;

	/* TODO: MOVE to a common interface, isr_lll_radio_status? */
	/* Clear radio status and events */
	lll_isr_status_reset();

	if (IS_ENABLED(HAL_RADIO_GPIO_HAVE_PA_PIN) ||
	    IS_ENABLED(HAL_RADIO_GPIO_HAVE_LNA_PIN)) {
		radio_gpio_pa_lna_disable();
	}
	/* TODO: MOVE ^^ */

#if defined(CONFIG_BT_HCI_MESH_EXT)
	if (_radio.advertiser.is_mesh &&
	    !_radio.mesh_adv_end_us) {
		_radio.mesh_adv_end_us = radio_tmr_end_get();
	}
#endif /* CONFIG_BT_HCI_MESH_EXT */

#if defined(CONFIG_BT_PERIPHERAL)
	if (!lll->chan_map_curr && lll->is_hdcd) {
		lll->chan_map_curr = lll->chan_map;
	}
#endif /* CONFIG_BT_PERIPHERAL */

	if (lll->chan_map_curr) {
		uint32_t start_us;

		chan_prepare(lll);

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
		start_us = radio_tmr_start_now(1);

		radio_gpio_pa_setup();
		radio_gpio_pa_lna_enable(start_us +
					 radio_tx_ready_delay_get(0, 0) -
					 HAL_RADIO_GPIO_PA_OFFSET);
#else /* !HAL_RADIO_GPIO_HAVE_PA_PIN */
		ARG_UNUSED(start_us);

		radio_tx_enable();
#endif /* !HAL_RADIO_GPIO_HAVE_PA_PIN */

		/* capture end of Tx-ed PDU, used to calculate HCTO. */
		radio_tmr_end_capture();

		return;
	}

	radio_filter_disable();

#if defined(CONFIG_BT_PERIPHERAL)
	if (!lll->is_hdcd)
#endif /* CONFIG_BT_PERIPHERAL */
	{
#if defined(CONFIG_BT_HCI_MESH_EXT)
		if (_radio.advertiser.is_mesh) {
			uint32_t err;

			err = isr_close_adv_mesh();
			if (err) {
				return 0;
			}
		}
#endif /* CONFIG_BT_HCI_MESH_EXT */
	}

#if defined(CONFIG_BT_CTLR_ADV_INDICATION)
	node_rx = ull_pdu_rx_alloc_peek(3);
	if (node_rx) {
		ull_pdu_rx_alloc();

		/* TODO: add other info by defining a payload struct */
		node_rx->type = NODE_RX_TYPE_ADV_INDICATION;

		ull_rx_put_sched(node_rx->link, node_rx);
	}
#else /* !CONFIG_BT_CTLR_ADV_INDICATION */
	ARG_UNUSED(node_rx);
#endif /* !CONFIG_BT_CTLR_ADV_INDICATION */

	isr_cleanup(param);
}

static void isr_abort(void *param)
{
	/* Clear radio status and events */
	lll_isr_status_reset();

	if (IS_ENABLED(HAL_RADIO_GPIO_HAVE_PA_PIN) ||
	    IS_ENABLED(HAL_RADIO_GPIO_HAVE_LNA_PIN)) {
		radio_gpio_pa_lna_disable();
	}

	radio_filter_disable();

	isr_cleanup(param);
}

static void isr_cleanup(void *param)
{
	int err;

	radio_isr_set(isr_race, param);
	radio_tmr_stop();

	err = lll_clk_off();
	LL_ASSERT(!err || err == -EBUSY);

	lll_done(NULL);
}

static void isr_race(void *param)
{
	/* NOTE: lll_disable could have a race with ... */
	radio_status_reset();
}

static void chan_prepare(struct lll_adv *lll)
{
	struct pdu_adv *pdu;
	struct pdu_adv *scan_pdu;
	uint8_t chan;
	uint8_t upd = 0U;

	pdu = lll_adv_data_latest_get(lll, &upd);
	LL_ASSERT(pdu);
	scan_pdu = lll_adv_scan_rsp_latest_get(lll, &upd);
	LL_ASSERT(scan_pdu);

#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (upd) {
		/* Copy the address from the adv packet we will send into the
		 * scan response.
		 */
		memcpy(&scan_pdu->scan_rsp.addr[0],
		       &pdu->adv_ind.addr[0], BDADDR_SIZE);
	}
#else
	ARG_UNUSED(scan_pdu);
	ARG_UNUSED(upd);
#endif /* !CONFIG_BT_CTLR_PRIVACY */

	radio_pkt_tx_set(pdu);

	if ((pdu->type != PDU_ADV_TYPE_NONCONN_IND) &&
	    (!IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT) ||
	     (pdu->type != PDU_ADV_TYPE_EXT_IND))) {
		radio_isr_set(isr_tx, lll);
		radio_tmr_tifs_set(EVENT_IFS_US);
		radio_switch_complete_and_rx(0);
	} else {
		radio_isr_set(isr_done, lll);
		radio_switch_complete_and_disable();
	}

	chan = find_lsb_set(lll->chan_map_curr);
	LL_ASSERT(chan);

	lll->chan_map_curr &= (lll->chan_map_curr - 1);

	lll_chan_set(36 + chan);
}

static inline int isr_rx_pdu(struct lll_adv *lll,
			     uint8_t devmatch_ok, uint8_t devmatch_id,
			     uint8_t irkmatch_ok, uint8_t irkmatch_id,
			     uint8_t rssi_ready)
{
	struct pdu_adv *pdu_rx, *pdu_adv;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	/* An IRK match implies address resolution enabled */
	uint8_t rl_idx = irkmatch_ok ? ull_filter_lll_rl_irk_idx(irkmatch_id) :
				    FILTER_IDX_NONE;
#else
	uint8_t rl_idx = FILTER_IDX_NONE;
#endif /* CONFIG_BT_CTLR_PRIVACY */

	pdu_rx = (void *)radio_pkt_scratch_get();
	pdu_adv = lll_adv_data_curr_get(lll);

	if ((pdu_rx->type == PDU_ADV_TYPE_SCAN_REQ) &&
	    (pdu_rx->len == sizeof(struct pdu_adv_scan_req)) &&
	    (pdu_adv->type != PDU_ADV_TYPE_DIRECT_IND) &&
	    isr_rx_sr_check(lll, pdu_adv, pdu_rx, devmatch_ok, &rl_idx)) {
		radio_isr_set(isr_done, lll);
		radio_switch_complete_and_disable();
		radio_pkt_tx_set(lll_adv_scan_rsp_curr_get(lll));

		/* assert if radio packet ptr is not set and radio started tx */
		LL_ASSERT(!radio_is_ready());

		if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
			lll_prof_cputime_capture();
		}

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
		if (!IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT) ||
		    0 /* TODO: extended adv. scan req notification enabled */) {
			uint32_t err;

			/* Generate the scan request event */
			err = isr_rx_sr_report(pdu_rx, rssi_ready);
			if (err) {
				/* Scan Response will not be transmitted */
				return err;
			}
		}
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
		if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
			/* PA/LNA enable is overwriting packet end used in ISR
			 * profiling, hence back it up for later use.
			 */
			lll_prof_radio_end_backup();
		}

		radio_gpio_pa_setup();
		radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() +
					 EVENT_IFS_US -
					 radio_rx_chain_delay_get(0, 0) -
					 HAL_RADIO_GPIO_PA_OFFSET);
#endif /* HAL_RADIO_GPIO_HAVE_PA_PIN */
		return 0;

#if defined(CONFIG_BT_PERIPHERAL)
	} else if ((pdu_rx->type == PDU_ADV_TYPE_CONNECT_IND) &&
		   (pdu_rx->len == sizeof(struct pdu_adv_connect_ind)) &&
		   isr_rx_ci_check(lll, pdu_adv, pdu_rx, devmatch_ok,
				   &rl_idx) &&
		   lll->conn) {
		struct node_rx_ftr *ftr;
		struct node_rx_pdu *rx;

		if (IS_ENABLED(CONFIG_BT_CTLR_CHAN_SEL_2)) {
			rx = ull_pdu_rx_alloc_peek(4);
		} else {
			rx = ull_pdu_rx_alloc_peek(3);
		}

		if (!rx) {
			return -ENOBUFS;
		}

		radio_isr_set(isr_abort, lll);
		radio_disable();

		/* assert if radio started tx */
		LL_ASSERT(!radio_is_ready());

		if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
			lll_prof_cputime_capture();
		}

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
		if (rssi_ready) {
			lll->conn->rssi_latest =  radio_rssi_get();
		}
#endif /* CONFIG_BT_CTLR_CONN_RSSI */
		/* Stop further LLL radio events */
		lll->conn->central.initiated = 1;

		rx = ull_pdu_rx_alloc();

		rx->hdr.type = NODE_RX_TYPE_CONNECTION;
		rx->hdr.handle = 0xffff;

		memcpy(rx->pdu, pdu_rx, (offsetof(struct pdu_adv, connect_ind) +
					 sizeof(struct pdu_adv_connect_ind)));

		ftr = &(rx->hdr.rx_ftr);
		ftr->param = lll;
		ftr->ticks_anchor = radio_tmr_start_get();
		ftr->radio_end_us = radio_tmr_end_get() -
				    radio_tx_chain_delay_get(0, 0);

#if defined(CONFIG_BT_CTLR_PRIVACY)
		ftr->rl_idx = irkmatch_ok ? rl_idx : FILTER_IDX_NONE;
#endif /* CONFIG_BT_CTLR_PRIVACY */

		if (IS_ENABLED(CONFIG_BT_CTLR_CHAN_SEL_2)) {
			ftr->extra = ull_pdu_rx_alloc();
		}

		ull_rx_put_sched(rx->hdr.link, rx);

		return 0;
#endif /* CONFIG_BT_PERIPHERAL */
	}

	return -EINVAL;
}

static inline bool isr_rx_sr_check(struct lll_adv *lll, struct pdu_adv *adv,
				   struct pdu_adv *sr, uint8_t devmatch_ok,
				   uint8_t *rl_idx)
{
#if defined(CONFIG_BT_CTLR_PRIVACY)
	return ((((lll->filter_policy & BT_LE_ADV_FP_FILTER_SCAN_REQ) == 0) &&
		 ull_filter_lll_rl_addr_allowed(sr->tx_addr,
						sr->scan_req.scan_addr,
						rl_idx)) ||
		(((lll->filter_policy & BT_LE_ADV_FP_FILTER_SCAN_REQ) != 0) &&
		 (devmatch_ok || ull_filter_lll_irk_in_fal(*rl_idx)))) &&
		isr_rx_sr_adva_check(adv, sr);
#else
	return (((lll->filter_policy & BT_LE_ADV_FP_FILTER_SCAN_REQ) == 0U) ||
		 devmatch_ok) &&
		isr_rx_sr_adva_check(adv, sr);
#endif /* CONFIG_BT_CTLR_PRIVACY */
}

static inline bool isr_rx_sr_adva_check(struct pdu_adv *adv,
					struct pdu_adv *sr)
{
	return (adv->tx_addr == sr->rx_addr) &&
		!memcmp(adv->adv_ind.addr, sr->scan_req.adv_addr, BDADDR_SIZE);
}

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
static inline int isr_rx_sr_report(struct pdu_adv *pdu_adv_rx,
				   uint8_t rssi_ready)
{
	struct node_rx_pdu *node_rx;
	struct pdu_adv *pdu_adv;
	uint8_t pdu_len;

	node_rx = ull_pdu_rx_alloc_peek(3);
	if (!node_rx) {
		return -ENOBUFS;
	}
	ull_pdu_rx_alloc();

	/* Prepare the report (scan req) */
	node_rx->hdr.type = NODE_RX_TYPE_SCAN_REQ;
	node_rx->hdr.handle = 0xffff;

	/* Make a copy of PDU into Rx node (as the received PDU is in the
	 * scratch buffer), and save the RSSI value.
	 */
	pdu_adv = (void *)node_rx->pdu;
	pdu_len = offsetof(struct pdu_adv, payload) + pdu_adv_rx->len;
	memcpy(pdu_adv, pdu_adv_rx, pdu_len);

	node_rx->hdr.rx_ftr.rssi = (rssi_ready) ? (radio_rssi_get() & 0x7f) :
						  0x7f;

	ull_rx_put_sched(node_rx->hdr.link, node_rx);

	return 0;
}
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

static inline bool isr_rx_ci_check(struct lll_adv *lll, struct pdu_adv *adv,
				   struct pdu_adv *ci, uint8_t devmatch_ok,
				   uint8_t *rl_idx)
{
	/* LL 4.3.2: filter policy shall be ignored for directed adv */
	if (adv->type == PDU_ADV_TYPE_DIRECT_IND) {
#if defined(CONFIG_BT_CTLR_PRIVACY)
		return ull_filter_lll_rl_addr_allowed(ci->tx_addr,
						      ci->connect_ind.init_addr,
						      rl_idx) &&
#else
		return (1) &&
#endif
		       isr_rx_ci_adva_check(adv, ci) &&
		       isr_rx_ci_tgta_check(lll, adv, ci, *rl_idx);
	}

#if defined(CONFIG_BT_CTLR_PRIVACY)
	return ((((lll->filter_policy & BT_LE_ADV_FP_FILTER_CONN_IND) == 0) &&
		 ull_filter_lll_rl_addr_allowed(ci->tx_addr,
						ci->connect_ind.init_addr,
						rl_idx)) ||
		(((lll->filter_policy & BT_LE_ADV_FP_FILTER_CONN_IND) != 0) &&
		 (devmatch_ok || ull_filter_lll_irk_in_fal(*rl_idx)))) &&
	       isr_rx_ci_adva_check(adv, ci);
#else
	return (((lll->filter_policy & BT_LE_ADV_FP_FILTER_CONN_IND) == 0) ||
		(devmatch_ok)) &&
	       isr_rx_ci_adva_check(adv, ci);
#endif /* CONFIG_BT_CTLR_PRIVACY */
}

static inline bool isr_rx_ci_tgta_check(struct lll_adv *lll,
					struct pdu_adv *adv, struct pdu_adv *ci,
					uint8_t rl_idx)
{
#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (rl_idx != FILTER_IDX_NONE && lll->rl_idx != FILTER_IDX_NONE) {
		return rl_idx == lll->rl_idx;
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */
	return (adv->rx_addr == ci->tx_addr) &&
	       !memcmp(adv->direct_ind.tgt_addr, ci->connect_ind.init_addr,
		       BDADDR_SIZE);
}

static inline bool isr_rx_ci_adva_check(struct pdu_adv *adv,
					struct pdu_adv *ci)
{
	return (adv->tx_addr == ci->rx_addr) &&
		(((adv->type == PDU_ADV_TYPE_DIRECT_IND) &&
		 !memcmp(adv->direct_ind.adv_addr, ci->connect_ind.adv_addr,
			 BDADDR_SIZE)) ||
		 (!memcmp(adv->adv_ind.addr, ci->connect_ind.adv_addr,
			  BDADDR_SIZE)));
}
