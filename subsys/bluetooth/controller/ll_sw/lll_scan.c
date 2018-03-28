#include <zephyr/types.h>

#include <toolchain.h>
#include <bluetooth/hci.h>

#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/nrf5/ticker.h"

#include "util/util.h"
#include "util/memq.h"
#include "util/mayfly.h"

#include "ticker/ticker.h"

#include "pdu.h"

#include "ull_types.h"

#include "lll.h"
#include "lll_scan.h"

#include "lll_internal.h"
#include "lll_tim_internal.h"
#include "lll_chan_internal.h"

#include "lll_filter.h"

#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

static int init_reset(void);
static int prepare_cb(struct lll_prepare_param *prepare_param);
static int is_abort_cb(void *next, int prio, void *curr,
		       lll_prepare_cb_t *resume_cb, int *resume_prio);
static void abort_cb(struct lll_prepare_param *prepare_param, void *param);
static void ticker_stop_cb(u32_t ticks_at_expire, u32_t remainder, u16_t lazy,
			   void *param);
static void ticker_op_start_cb(u32_t status, void *param);
static void isr_rx(void *param);
static void isr_tx(void *param);
static void isr_done(void *param);
static void isr_abort(void *param);
static void isr_cleanup(void *param);
static void isr_race(void *param);

static inline bool isr_rx_scan_check(struct lll_scan *lll, u8_t irkmatch_ok,
				     u8_t devmatch_ok, u8_t rl_idx);
static inline u32_t isr_rx_scan(struct lll_scan *lll, u8_t devmatch_ok,
				u8_t devmatch_id, u8_t irkmatch_ok,
				u8_t irkmatch_id, u8_t rl_idx, u8_t rssi_ready);
static inline bool isr_scan_init_check(struct lll_scan *lll,
				       struct pdu_adv *pdu, u8_t rl_idx);
static inline bool isr_scan_init_adva_check(struct lll_scan *lll,
					    struct pdu_adv *pdu, u8_t rl_idx);
static inline bool isr_scan_tgta_check(struct lll_scan *lll, bool init,
				       struct pdu_adv *pdu, u8_t rl_idx,
				       bool *dir_report);
static inline bool isr_scan_tgta_rpa_check(struct lll_scan *lll,
					   struct pdu_adv *pdu,
					   bool *dir_report);
static inline bool isr_scan_rsp_adva_matches(struct pdu_adv *srsp);
static u32_t isr_rx_scan_report(struct lll_scan *lll, u8_t rssi_ready,
				u8_t rl_idx, bool dir_report);


int lll_scan_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int lll_scan_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

void lll_scan_prepare(void *param)
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
	struct lll_scan *lll = prepare_param->param;
	struct node_rx_pdu *node_rx;
	u32_t aa = 0x8e89bed6;
	u32_t ticks_at_event;
	u32_t remainder_us;
	u32_t remainder;

	DEBUG_RADIO_START_O(1);

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);

	radio_reset();
	/* TODO: other Tx Power settings */
	radio_tx_power_set(0);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	/* TODO: if coded we use S8? */
	radio_phy_set(lll->phy, 1);
	radio_pkt_configure(8, PDU_AC_PAYLOAD_SIZE_MAX, (lll->phy << 1));
#else /* !CONFIG_BT_CTLR_ADV_EXT */
	radio_phy_set(0, 0);
	radio_pkt_configure(8, PDU_AC_PAYLOAD_SIZE_MAX, 0);
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

	radio_pkt_rx_set(node_rx->pdu);

	radio_aa_set((u8_t *)&aa);
	radio_crc_configure(((0x5bUL) | ((0x06UL) << 8) | ((0x00UL) << 16)),
			    0x555555);

	chan_set(37 + lll->chan);

	radio_isr_set(isr_rx, lll);

	radio_tmr_tifs_set(TIFS_US);
	radio_switch_complete_and_tx(0, 0, 0, 0);

#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (ctrl_rl_enabled()) {
		struct ll_filter *filter =
			ctrl_filter_get(!!(lll->filter_policy & 0x1));
		u8_t count, *irks = ctrl_irks_get(&count);

		radio_filter_configure(filter->enable_bitmask,
				       filter->addr_type_bitmask,
				       (u8_t *)filter->bdaddr);

		radio_ar_configure(count, irks);
	} else
#endif /* CONFIG_BT_CTLR_PRIVACY */

#if defined(CONFIG_BT_CTLR_FILTER)
	/* Setup Radio Filter */
	if (lll->filter_policy) {

		struct ll_filter *wl = ctrl_filter_get(true);

		radio_filter_configure(wl->enable_bitmask,
				       wl->addr_type_bitmask,
				       (u8_t *)wl->bdaddr);
	}
#endif /* CONFIG_BT_CTLR_FILTER */

	/* FIXME: use prepare to start ticks */
	/* TODO: kill the use of EVENT_OVERHEAD_START_US */
	ticks_at_event = prepare_param->ticks_at_expire +
			 HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US +
						EVENT_OVERHEAD_START_US);
	remainder = prepare_param->remainder;
	remainder_us = radio_tmr_start(0, ticks_at_event, remainder);

	/* capture end of Rx-ed PDU, for initiator to calculate first
	 * master event.
	 */
	radio_tmr_end_capture();

	/* scanner always measures RSSI */
	radio_rssi_measure();

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	radio_gpio_lna_setup();
	radio_gpio_pa_lna_enable(remainder_us +
				 radio_rx_ready_delay_get(0, 0) -
				 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#else /* !CONFIG_BT_CTLR_GPIO_LNA_PIN */
	ARG_UNUSED(remainder_us);
#endif /* !CONFIG_BT_CTLR_GPIO_LNA_PIN */

#if (0 && defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
     (EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US))
	/* check if preempt to start has changed */
	if (preempt_calc(&_radio.scanner.hdr, TICKER_ID_SCAN_BASE,
			 ticks_at_expire) != 0) {
		_radio.state = STATE_STOP;
		radio_disable();
	} else
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
	{
		u32_t ret;

		if (lll->ticks_window) {
			/* start window close timeout */
			ret = ticker_start(TICKER_INSTANCE_ID_CTLR,
					   TICKER_USER_ID_LLL,
					   TICKER_ID_SCAN_STOP,
					   ticks_at_event, lll->ticks_window,
					   TICKER_NULL_PERIOD,
					   TICKER_NULL_REMAINDER,
					   TICKER_NULL_LAZY, TICKER_NULL_SLOT,
					   ticker_stop_cb, lll,
					   ticker_op_start_cb,
					   (void *)__LINE__);
			LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
				  (ret == TICKER_STATUS_BUSY));
		}

		ret = lll_prepare_done(lll);
		LL_ASSERT(!ret);
	}

	DEBUG_RADIO_START_O(1);

	return 0;
}

static int resume_prepare_cb(struct lll_prepare_param *p)
{
	p->ticks_at_expire = ticker_ticks_now_get() -
			     HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	p->remainder = 0;
	p->lazy = 0;

	return prepare_cb(p);
}

static int is_abort_cb(void *next, int prio, void *curr,
		       lll_prepare_cb_t *resume_cb, int *resume_prio)
{
	struct lll_scan *lll = curr;

	/* TODO: check prio */
	if (next != curr) {
		int err;

		/* wrap back after the pre-empter */
		*resume_cb = resume_prepare_cb;
		*resume_prio = 0; /* TODO: */

		/* Retain HF clk */
		err = lll_clk_on();
		LL_ASSERT(!err || err == -EINPROGRESS);

		return -EAGAIN;
	}

	radio_isr_set(isr_done, lll);
	radio_disable();

	if (++lll->chan == 3) {
		lll->chan = 0;
	}

	chan_set(37 + lll->chan);

	return 0;
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

static void ticker_stop_cb(u32_t ticks_at_expire, u32_t remainder, u16_t lazy,
			   void *param)
{
	radio_isr_set(isr_cleanup, param);
	radio_disable();
}

static void ticker_op_start_cb(u32_t status, void *param)
{
	ARG_UNUSED(param);

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);
}

static void isr_rx(void *param)
{
	u8_t trx_done;
	u8_t crc_ok;
	u8_t devmatch_ok;
	u8_t devmatch_id;
	u8_t irkmatch_ok;
	u8_t irkmatch_id;
	u8_t rssi_ready;
	u8_t rl_idx;

	/* Read radio status and events */
	trx_done = radio_is_done();
	if (trx_done) {

#if defined(CONFIG_BT_CTLR_PROFILE_ISR)
		/* sample the packet timer here, use it to calculate ISR latency
		 * and generate the profiling event at the end of the ISR.
		 */
		radio_tmr_sample();
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

		crc_ok = radio_crc_is_valid();
		devmatch_ok = radio_filter_has_match();
		devmatch_id = radio_filter_match_get();
		irkmatch_ok = radio_ar_has_match();
		irkmatch_id = radio_ar_match_get();
		rssi_ready = radio_rssi_is_ready();
	} else {
		crc_ok = devmatch_ok = irkmatch_ok = rssi_ready = 0;
		devmatch_id = irkmatch_id = 0xFF;
	}

	/* Clear radio status and events */
	radio_status_reset();
	radio_tmr_status_reset();
	radio_filter_status_reset();
	radio_ar_status_reset();
	radio_rssi_status_reset();

#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN) || \
    defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	radio_gpio_pa_lna_disable();
#endif /* CONFIG_BT_CTLR_GPIO_PA_PIN || CONFIG_BT_CTLR_GPIO_LNA_PIN */

	if (!trx_done) {
		goto isr_rx_do_close;
	}

#if defined(CONFIG_BT_CTLR_PRIVACY)
	rl_idx = devmatch_ok ? ctrl_rl_idx(!!(_radio.scanner.filter_policy &
					      0x01),
					   devmatch_id) :
			       irkmatch_ok ? ctrl_rl_irk_idx(irkmatch_id) :
					     FILTER_IDX_NONE;
#else
	rl_idx = FILTER_IDX_NONE;
#endif
	if (crc_ok && isr_rx_scan_check(param, irkmatch_ok, devmatch_ok,
					rl_idx)) {
		u32_t err;

		err = isr_rx_scan(param, devmatch_ok, devmatch_id, irkmatch_ok,
				  irkmatch_id, rl_idx, rssi_ready);
		if (!err) {
			return;
		}
	}

isr_rx_do_close:
	radio_isr_set(isr_done, param);
	radio_disable();

	return;
}

static void isr_tx(void *param)
{
	struct node_rx_pdu *node_rx;
	u32_t hcto;

	/* TODO: MOVE to a common interface, isr_lll_radio_status? */
	/* Clear radio status and events */
	radio_status_reset();
	radio_tmr_status_reset();
	radio_filter_status_reset();
	radio_ar_status_reset();
	radio_rssi_status_reset();

#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN) || \
    defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	radio_gpio_pa_lna_disable();
#endif /* CONFIG_BT_CTLR_GPIO_PA_PIN || CONFIG_BT_CTLR_GPIO_LNA_PIN */
	/* TODO: MOVE ^^ */

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);

	radio_isr_set(isr_rx, param);
	radio_tmr_tifs_set(TIFS_US);
	radio_switch_complete_and_tx(0, 0, 0, 0);
	radio_pkt_rx_set(node_rx->pdu);

	/* assert if radio packet ptr is not set and radio started rx */
	LL_ASSERT(!radio_is_ready());

#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (ctrl_rl_enabled()) {
		u8_t count, *irks = ctrl_irks_get(&count);

		radio_ar_configure(count, irks);
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

	/* +/- 2us active clock jitter, +1 us hcto compensation */
	hcto = radio_tmr_tifs_base_get() + TIFS_US + 4 + 1;
	hcto += radio_rx_chain_delay_get(0, 0);
	hcto += addr_us_get(0);
	hcto -= radio_tx_chain_delay_get(0, 0);

	radio_tmr_hcto_configure(hcto);

	radio_rssi_measure();

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	radio_gpio_lna_setup();
	radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() + TIFS_US - 4 -
				 radio_tx_chain_delay_get(0, 0) -
				 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#endif /* CONFIG_BT_CTLR_GPIO_LNA_PIN */
}

static void isr_done(void *param)
{
	struct node_rx_pdu *node_rx;
	u32_t start_us;

	/* TODO: MOVE to a common interface, isr_lll_radio_status? */
	/* Clear radio status and events */
	radio_status_reset();
	radio_tmr_status_reset();
	radio_filter_status_reset();
	radio_ar_status_reset();
	radio_rssi_status_reset();

#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN) || \
    defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	radio_gpio_pa_lna_disable();
#endif /* CONFIG_BT_CTLR_GPIO_PA_PIN || CONFIG_BT_CTLR_GPIO_LNA_PIN */
	/* TODO: MOVE ^^ */

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);

	radio_tmr_tifs_set(TIFS_US);
	radio_switch_complete_and_tx(0, 0, 0, 0);
	radio_pkt_rx_set(node_rx->pdu);
	radio_rssi_measure();

#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (ctrl_rl_enabled()) {
		u8_t count, *irks = ctrl_irks_get(&count);

		radio_ar_configure(count, irks);
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

	radio_isr_set(isr_rx, param);

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	start_us = radio_tmr_start_now(0);

	radio_gpio_lna_setup();
	radio_gpio_pa_lna_enable(start_us +
				 radio_rx_ready_delay_get(0, 0) -
				 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#else /* !CONFIG_BT_CTLR_GPIO_LNA_PIN */
	ARG_UNUSED(start_us);

	radio_rx_enable();
#endif /* !CONFIG_BT_CTLR_GPIO_LNA_PIN */

	/* capture end of Rx-ed PDU, for initiator to calculate first
	 * master event.
	 */
	radio_tmr_end_capture();
}

static void isr_abort(void *param)
{
	/* Scanner stop can expire while here in this ISR.
	 * Deferred attempt to stop can fail as it would have
	 * expired, hence ignore failure.
	 */
	ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_LLL,
		    TICKER_ID_SCAN_STOP, NULL, NULL);

	isr_cleanup(param);
}

static void isr_cleanup(void *param)
{
	struct lll_scan *lll = param;
	int err;

	if (lll_is_done(param)) {
		return;
	}

	radio_filter_disable();

	if (++lll->chan == 3) {
		lll->chan = 0;
	}

#if defined(CONFIG_BT_HCI_MESH_EXT)
	if (_radio.advertiser.is_enabled && _radio.advertiser.is_mesh &&
	    !_radio.advertiser.retry) {
		mayfly_mesh_stop(NULL);
	}
#endif /* CONFIG_BT_HCI_MESH_EXT */

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

static inline bool isr_rx_scan_check(struct lll_scan *lll, u8_t irkmatch_ok,
				     u8_t devmatch_ok, u8_t rl_idx)
{
#if defined(CONFIG_BT_CTLR_PRIVACY)
	return (((_radio.scanner.filter_policy & 0x01) == 0) &&
		 (!devmatch_ok || ctrl_rl_idx_allowed(irkmatch_ok, rl_idx))) ||
		(((_radio.scanner.filter_policy & 0x01) != 0) &&
		 (devmatch_ok || ctrl_irk_whitelisted(rl_idx)));
#else
	return ((lll->filter_policy & 0x01) == 0) ||
		devmatch_ok;
#endif /* CONFIG_BT_CTLR_PRIVACY */
}

static inline u32_t isr_rx_scan(struct lll_scan *lll, u8_t devmatch_ok,
				u8_t devmatch_id, u8_t irkmatch_ok,
				u8_t irkmatch_id, u8_t rl_idx, u8_t rssi_ready)
{
	struct node_rx_pdu *node_rx;
	struct pdu_adv *pdu_adv_rx;
	bool dir_report = false;

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);

	pdu_adv_rx = (void *)node_rx->pdu;

	if (0) {
#if defined(CONFIG_BT_CONN)
	/* Initiator */
	} else if ((_radio.scanner.conn) && ((_radio.fc_ena == 0) ||
					   (_radio.fc_req == _radio.fc_ack)) &&
		   isr_scan_init_check(pdu_adv_rx, rl_idx) &&
		   ((radio_tmr_end_get() + 502 + (EVENT_JITTER_US << 1)) <
		    (HAL_TICKER_TICKS_TO_US(_radio.scanner.hdr.ticks_slot) -
		     EVENT_OVERHEAD_START_US))) {
		struct node_rx_pdu *node_rx;
		struct pdu_adv *pdu_adv_tx;
		struct pdu_data *pdu_data;
		struct connection *conn;
		u32_t ticks_slot_offset;
		u32_t conn_interval_us;
		struct node_rx_cc *cc;
		u32_t conn_offset_us;
		u32_t ticker_status;
		u32_t conn_space_us;
#if defined(CONFIG_BT_CTLR_PRIVACY)
		bt_addr_t *lrpa;
#endif /* CONFIG_BT_CTLR_PRIVACY */

		if (IS_ENABLED(CONFIG_BT_CTLR_CHAN_SEL_2)) {
			node_rx = packet_rx_reserve_get(4);
		} else {
			node_rx = packet_rx_reserve_get(3);
		}

		if (node_rx == 0) {
			return 1;
		}

		_radio.state = STATE_STOP;

		/* acquire the master context from scanner */
		conn = _radio.scanner.conn;
		_radio.scanner.conn = NULL;

		/* Tx the connect request packet */
		pdu_adv_tx = (void *)radio_pkt_scratch_get();
		pdu_adv_tx->type = PDU_ADV_TYPE_CONNECT_IND;

		if (IS_ENABLED(CONFIG_BT_CTLR_CHAN_SEL_2)) {
			pdu_adv_tx->chan_sel = 1;
		} else {
			pdu_adv_tx->chan_sel = 0;
		}

		pdu_adv_tx->rx_addr = pdu_adv_rx->tx_addr;
		pdu_adv_tx->len = sizeof(struct pdu_adv_connect_ind);
#if defined(CONFIG_BT_CTLR_PRIVACY)
		lrpa = ctrl_lrpa_get(rl_idx);
		if (_radio.scanner.rpa_gen && lrpa) {
			pdu_adv_tx->tx_addr = 1;
			memcpy(&pdu_adv_tx->connect_ind.init_addr[0],
			       lrpa->val, BDADDR_SIZE);
		} else {
#else
		if (1) {
#endif /* CONFIG_BT_CTLR_PRIVACY */
			pdu_adv_tx->tx_addr = _radio.scanner.init_addr_type;
			memcpy(&pdu_adv_tx->connect_ind.init_addr[0],
			       &_radio.scanner.init_addr[0], BDADDR_SIZE);
		}
		memcpy(&pdu_adv_tx->connect_ind.adv_addr[0],
			 &pdu_adv_rx->adv_ind.addr[0], BDADDR_SIZE);
		memcpy(&pdu_adv_tx->connect_ind.
		       access_addr[0], &conn->access_addr[0], 4);
		memcpy(&pdu_adv_tx->connect_ind.crc_init[0],
		       &conn->crc_init[0], 3);
		pdu_adv_tx->connect_ind.win_size = 1;

		conn_interval_us =
			(u32_t)_radio.scanner.conn_interval * 1250;

		conn_offset_us = radio_tmr_end_get() + 502 + 1250;

		/* The ticker module generates the timeout callbacks with a
		 * +/- half the 32KHz clock resolution. In order to achieve
		 * a microsecond resolution, in the case of negative remainder,
		 * the radio packet timer is started one 32KHz tick early,
		 * hence substract one tick unit from the measurement of the
		 * packet end.
		 */
		if (!_radio.remainder_anchor ||
		    (_radio.remainder_anchor & BIT(31))) {
			conn_offset_us -= HAL_TICKER_TICKS_TO_US(1);
		}

		if (_radio.scanner.win_offset_us == 0) {
			conn_space_us = conn_offset_us;
			pdu_adv_tx->connect_ind.win_offset = 0;
		} else {
			conn_space_us = _radio.scanner.win_offset_us;
			while ((conn_space_us & ((u32_t)1 << 31)) ||
			       (conn_space_us < conn_offset_us)) {
				conn_space_us += conn_interval_us;
			}
			pdu_adv_tx->connect_ind.win_offset =
				(conn_space_us - conn_offset_us) / 1250;
			pdu_adv_tx->connect_ind.win_size++;
		}

		conn_space_us -= radio_tx_ready_delay_get(0, 0);
		conn_space_us -= radio_tx_chain_delay_get(0, 0);

		/* Workaround: Due to the missing remainder param in
		 * ticker_start function for first interval; add a
		 * tick so as to use the ceiled value.
		 */
		conn_space_us += HAL_TICKER_TICKS_TO_US(1);

		pdu_adv_tx->connect_ind.interval =
			_radio.scanner.conn_interval;
		pdu_adv_tx->connect_ind.latency =
			_radio.scanner.conn_latency;
		pdu_adv_tx->connect_ind.timeout =
			_radio.scanner.conn_timeout;
		memcpy(&pdu_adv_tx->connect_ind.chan_map[0],
		       &conn->data_chan_map[0],
		       sizeof(pdu_adv_tx->connect_ind.chan_map));
		pdu_adv_tx->connect_ind.hop =
			conn->data_chan_hop;
		pdu_adv_tx->connect_ind.sca = _radio.sca;

		radio_switch_complete_and_disable();

		radio_pkt_tx_set(pdu_adv_tx);

#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN)
		radio_gpio_pa_setup();
		radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() +
					 RADIO_TIFS -
					 radio_rx_chain_delay_get(0, 0) -
					 CONFIG_BT_CTLR_GPIO_PA_OFFSET);
#endif /* CONFIG_BT_CTLR_GPIO_PA_PIN */

		/* assert if radio packet ptr is not set and radio started tx */
		LL_ASSERT(!radio_is_ready());

		/* block CPU so that there is no CRC error on pdu tx,
		 * this is only needed if we want the CPU to sleep.
		 * while(!radio_has_disabled())
		 * {cpu_sleep();}
		 * radio_status_reset();
		 */

		/* Populate the master context */
		conn->handle = mem_index_get(conn, _radio.conn_pool,
					     CONNECTION_T_SIZE);

		/* Prepare the rx packet structure */
		node_rx->hdr.handle = conn->handle;
		node_rx->hdr.type = NODE_RX_TYPE_CONNECTION;

		/* prepare connection complete structure */
		pdu_data = (void *)node_rx->pdu;
		cc = (void *)pdu_data->lldata;
		cc->status = 0x00;
		cc->role = 0x00;
#if defined(CONFIG_BT_CTLR_PRIVACY)
		cc->own_addr_type = pdu_adv_tx->tx_addr;
		memcpy(&cc->own_addr[0], &pdu_adv_tx->connect_ind.init_addr[0],
		       BDADDR_SIZE);

		if (irkmatch_ok && rl_idx != FILTER_IDX_NONE) {
			/* TODO: store rl_idx instead if safe */
			/* Store identity address */
			ll_rl_id_addr_get(rl_idx, &cc->peer_addr_type,
					  &cc->peer_addr[0]);
			/* Mark it as identity address from RPA (0x02, 0x03) */
			cc->peer_addr_type += 2;

			/* Store peer RPA */
			memcpy(&cc->peer_rpa[0],
			       &pdu_adv_tx->connect_ind.adv_addr[0],
			       BDADDR_SIZE);
		} else {
			memset(&cc->peer_rpa[0], 0x0, BDADDR_SIZE);
#else
		if (1) {
#endif /* CONFIG_BT_CTLR_PRIVACY */
			cc->peer_addr_type = pdu_adv_tx->rx_addr;
			memcpy(&cc->peer_addr[0],
			       &pdu_adv_tx->connect_ind.adv_addr[0],
			       BDADDR_SIZE);
		}

		cc->interval = _radio.scanner.conn_interval;
		cc->latency = _radio.scanner.conn_latency;
		cc->timeout = _radio.scanner.conn_timeout;
		cc->mca = pdu_adv_tx->connect_ind.sca;

		/* enqueue connection complete structure into queue */
		rx_fc_lock(conn->handle);
		packet_rx_enqueue();

		/* Use Channel Selection Algorithm #2 if peer too supports it */
		if (IS_ENABLED(CONFIG_BT_CTLR_CHAN_SEL_2)) {
			struct node_rx_cs *cs;

			/* Generate LE Channel Selection Algorithm event */
			node_rx = packet_rx_reserve_get(3);
			LL_ASSERT(node_rx);

			node_rx->hdr.handle = conn->handle;
			node_rx->hdr.type = NODE_RX_TYPE_CHAN_SEL_ALGO;

			pdu_data = (void *)node_rx->pdu;
			cs = (void *)pdu_data->lldata;

			if (pdu_adv_rx->chan_sel) {
				u16_t aa_ls =
					((u16_t)conn->access_addr[1] << 8) |
					conn->access_addr[0];
				u16_t aa_ms =
					((u16_t)conn->access_addr[3] << 8) |
					 conn->access_addr[2];

				conn->data_chan_sel = 1;
				conn->data_chan_id = aa_ms ^ aa_ls;

				cs->csa = 0x01;
			} else {
				cs->csa = 0x00;
			}

			packet_rx_enqueue();
		}

		/* Calculate master slot */
		conn->hdr.ticks_active_to_start = _radio.ticks_active_to_start;
		conn->hdr.ticks_xtal_to_start =	HAL_TICKER_US_TO_TICKS(
			EVENT_OVERHEAD_XTAL_US);
		conn->hdr.ticks_preempt_to_start = HAL_TICKER_US_TO_TICKS(
			EVENT_OVERHEAD_PREEMPT_MIN_US);
		conn->hdr.ticks_slot = _radio.scanner.ticks_conn_slot;
		ticks_slot_offset = max(conn->hdr.ticks_active_to_start,
					conn->hdr.ticks_xtal_to_start);

		/* Stop Scanner */
		ticker_status = ticker_stop(TICKER_INSTANCE_ID_CTLR,
					    TICKER_USER_ID_LLL,
					    TICKER_ID_SCAN_BASE,
					    ticker_stop_scan_assert,
					    (void *)__LINE__);
		ticker_stop_scan_assert(ticker_status, (void *)__LINE__);

		/* Scanner stop can expire while here in this ISR.
		 * Deferred attempt to stop can fail as it would have
		 * expired, hence ignore failure.
		 */
		ticker_stop(TICKER_INSTANCE_ID_CTLR,
			    TICKER_USER_ID_LLL,
			    TICKER_ID_SCAN_STOP, NULL, NULL);

		/* Start master */
		ticker_status =
			ticker_start(TICKER_INSTANCE_ID_CTLR,
				     TICKER_USER_ID_LLL,
				     TICKER_ID_CONN_BASE +
				     conn->handle,
				     (_radio.ticks_anchor - ticks_slot_offset),
				     HAL_TICKER_US_TO_TICKS(conn_space_us),
				     HAL_TICKER_US_TO_TICKS(conn_interval_us),
				     HAL_TICKER_REMAINDER(conn_interval_us),
				     TICKER_NULL_LAZY,
				     (ticks_slot_offset + conn->hdr.ticks_slot),
				     event_master_prepare, conn,
				     ticker_success_assert, (void *)__LINE__);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY));

		return 0;
#endif /* CONFIG_BT_CONN */

	/* Active scanner */
	} else if (((pdu_adv_rx->type == PDU_ADV_TYPE_ADV_IND) ||
		    (pdu_adv_rx->type == PDU_ADV_TYPE_SCAN_IND)) &&
		   lll->type &&
#if defined(CONFIG_BT_CONN)
		   !lll->conn) {
#else /* !CONFIG_BT_CONN */
		   1) {
#endif /* !CONFIG_BT_CONN */
		struct pdu_adv *pdu_adv_tx;
#if defined(CONFIG_BT_CTLR_PRIVACY)
		bt_addr_t *lrpa;
#endif /* CONFIG_BT_CTLR_PRIVACY */
		u32_t err;

		/* save the adv packet */
		err = isr_rx_scan_report(lll, rssi_ready,
					 irkmatch_ok ? rl_idx : FILTER_IDX_NONE,
					 false);
		if (err) {
			return err;
		}

		/* prepare the scan request packet */
		pdu_adv_tx = (void *)radio_pkt_scratch_get();
		pdu_adv_tx->type = PDU_ADV_TYPE_SCAN_REQ;
		pdu_adv_tx->rx_addr = pdu_adv_rx->tx_addr;
		pdu_adv_tx->len = sizeof(struct pdu_adv_scan_req);
#if defined(CONFIG_BT_CTLR_PRIVACY)
		lrpa = ctrl_lrpa_get(rl_idx);
		if (_radio.scanner.rpa_gen && lrpa) {
			pdu_adv_tx->tx_addr = 1;
			memcpy(&pdu_adv_tx->scan_req.scan_addr[0],
			       lrpa->val, BDADDR_SIZE);
		} else {
#else
		if (1) {
#endif /* CONFIG_BT_CTLR_PRIVACY */
			pdu_adv_tx->tx_addr = lll->init_addr_type;
			memcpy(&pdu_adv_tx->scan_req.scan_addr[0],
			       &lll->init_addr[0], BDADDR_SIZE);
		}
		memcpy(&pdu_adv_tx->scan_req.adv_addr[0],
		       &pdu_adv_rx->adv_ind.addr[0], BDADDR_SIZE);

		/* switch scanner state to active */
		lll->state = 1;
		radio_isr_set(isr_tx, lll);

		radio_tmr_tifs_set(TIFS_US);
		radio_switch_complete_and_rx(0);
		radio_pkt_tx_set(pdu_adv_tx);

		/* capture end of Tx-ed PDU, used to calculate HCTO. */
		radio_tmr_end_capture();

#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN)
		radio_gpio_pa_setup();
		radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() +
					 RADIO_TIFS -
					 radio_rx_chain_delay_get(0, 0) -
					 CONFIG_BT_CTLR_GPIO_PA_OFFSET);
#endif /* CONFIG_BT_CTLR_GPIO_PA_PIN */

		/* assert if radio packet ptr is not set and radio started tx */
		LL_ASSERT(!radio_is_ready());

		return 0;
	}
	/* Passive scanner or scan responses */
	else if (((pdu_adv_rx->type == PDU_ADV_TYPE_ADV_IND) ||
		  ((pdu_adv_rx->type == PDU_ADV_TYPE_DIRECT_IND) &&
		   (/* allow directed adv packets addressed to this device */
		    isr_scan_tgta_check(lll, false, pdu_adv_rx, rl_idx,
					&dir_report))) ||
		  (pdu_adv_rx->type == PDU_ADV_TYPE_NONCONN_IND) ||
		  (pdu_adv_rx->type == PDU_ADV_TYPE_SCAN_IND) ||
#if defined(CONFIG_BT_CTLR_ADV_EXT)
		  ((pdu_adv_rx->type == PDU_ADV_TYPE_EXT_IND) &&
		   (lll->phy)) ||
#endif /* CONFIG_BT_CTLR_ADV_EXT */
		  ((pdu_adv_rx->type == PDU_ADV_TYPE_SCAN_RSP) &&
		   (lll->state != 0) &&
		   isr_scan_rsp_adva_matches(pdu_adv_rx))) &&
		 (pdu_adv_rx->len != 0) &&
#if defined(CONFIG_BT_CONN)
		   !lll->conn) {
#else /* !CONFIG_BT_CONN */
		   1) {
#endif /* !CONFIG_BT_CONN */
		u32_t err;

		/* save the scan response packet */
		err = isr_rx_scan_report(lll, rssi_ready,
					 irkmatch_ok ? rl_idx :
						       FILTER_IDX_NONE,
					 dir_report);
		if (err) {
			return err;
		}
	}
	/* invalid PDU */
	else {
		/* ignore and close this rx/tx chain ( code below ) */
		return 1;
	}

	return 1;
}

static inline bool isr_scan_init_check(struct lll_scan *lll,
				       struct pdu_adv *pdu, u8_t rl_idx)
{
	return ((((lll->filter_policy & 0x01) != 0) ||
		isr_scan_init_adva_check(lll, pdu, rl_idx)) &&
		((pdu->type == PDU_ADV_TYPE_ADV_IND) ||
		((pdu->type == PDU_ADV_TYPE_DIRECT_IND) &&
		 (/* allow directed adv packets addressed to this device */
		  isr_scan_tgta_check(lll, true, pdu, rl_idx, NULL)))));
}

static inline bool isr_scan_init_adva_check(struct lll_scan *lll,
					    struct pdu_adv *pdu, u8_t rl_idx)
{
#if defined(CONFIG_BT_CTLR_PRIVACY)
	/* Only applies to initiator with no whitelist */
	if (rl_idx != FILTER_IDX_NONE) {
		return (rl_idx == lll->rl_idx);
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */
	return ((lll->adv_addr_type == pdu->tx_addr) &&
		!memcmp(lll->adv_addr, &pdu->adv_ind.addr[0], BDADDR_SIZE));
}

static inline bool isr_scan_tgta_check(struct lll_scan *lll, bool init,
				       struct pdu_adv *pdu, u8_t rl_idx,
				       bool *dir_report)
{
#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (ctrl_rl_addr_resolve(pdu->rx_addr,
				 pdu->direct_ind.tgt_addr, rl_idx)) {
		return true;
	} else if (init && _radio.scanner.rpa_gen && ctrl_lrpa_get(rl_idx)) {
		/* Initiator generating RPAs, and could not resolve TargetA:
		 * discard
		 */
		return false;
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

	return (((lll->init_addr_type == pdu->rx_addr) &&
		!memcmp(lll->init_addr, pdu->direct_ind.tgt_addr,
			BDADDR_SIZE))) ||
		  /* allow directed adv packets where TargetA address
		   * is resolvable private address (scanner only)
		   */
	       isr_scan_tgta_rpa_check(lll, pdu, dir_report);
}

static inline bool isr_scan_tgta_rpa_check(struct lll_scan *lll,
					   struct pdu_adv *pdu,
					   bool *dir_report)
{
	if (((lll->filter_policy & 0x02) != 0) &&
	    (pdu->rx_addr != 0) &&
	    ((pdu->direct_ind.tgt_addr[5] & 0xc0) == 0x40)) {

		if (dir_report) {
			*dir_report = true;
		}

		return true;
	}

	return false;
}

static inline bool isr_scan_rsp_adva_matches(struct pdu_adv *srsp)
{
	struct pdu_adv *sreq = (void *)radio_pkt_scratch_get();

	return ((sreq->rx_addr == srsp->tx_addr) &&
		(memcmp(&sreq->scan_req.adv_addr[0],
			&srsp->scan_rsp.addr[0], BDADDR_SIZE) == 0));
}

static u32_t isr_rx_scan_report(struct lll_scan *lll, u8_t rssi_ready,
				u8_t rl_idx, bool dir_report)
{
	struct node_rx_pdu *node_rx;
	struct pdu_adv *pdu_adv_rx;
	u8_t *extra;

	node_rx = ull_pdu_rx_alloc_peek(3);
	if (!node_rx) {
		return 1;
	}
	ull_pdu_rx_alloc();

	/* Prepare the report (adv or scan resp) */
	node_rx->hdr.handle = 0xffff;
	if (0) {

#if defined(CONFIG_BT_HCI_MESH_EXT)
	} else if (_radio.advertiser.is_enabled &&
		   _radio.advertiser.is_mesh) {
		node_rx->hdr.type = NODE_RX_TYPE_MESH_REPORT;
#endif /* CONFIG_BT_HCI_MESH_EXT */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	} else if (lll->phy) {
		switch (lll->phy) {
		case BIT(0):
			node_rx->hdr.type = NODE_RX_TYPE_EXT_1M_REPORT;
			break;

		case BIT(2):
			node_rx->hdr.type = NODE_RX_TYPE_EXT_CODED_REPORT;
			break;

		default:
			LL_ASSERT(0);
			break;
		}
#endif /* CONFIG_BT_CTLR_ADV_EXT */
	} else {
		node_rx->hdr.type = NODE_RX_TYPE_REPORT;
	}

	pdu_adv_rx = (void *)node_rx->pdu;
	extra = &((u8_t *)pdu_adv_rx)[offsetof(struct pdu_adv, payload) +
				      pdu_adv_rx->len];
	/* save the RSSI value */
	*extra = (rssi_ready) ? (radio_rssi_get() & 0x7f) : 0x7f;
	extra += PDU_AC_SZ_RSSI;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	/* save the resolving list index. */
	*extra = rl_idx;
	extra += PDU_AC_SZ_PRIV;
#endif /* CONFIG_BT_CTLR_PRIVACY */
#if defined(CONFIG_BT_CTLR_EXT_SCAN_FP)
	/* save the directed adv report flag */
	*extra = dir_report ? 1 : 0;
	extra += PDU_AC_SZ_SCFP;
#endif /* CONFIG_BT_CTLR_EXT_SCAN_FP */
#if defined(CONFIG_BT_HCI_MESH_EXT)
	if (node_rx->hdr.type == NODE_RX_TYPE_MESH_REPORT) {
		/* save the directed adv report flag */
		*extra = _radio.scanner.chan - 1;
		extra++;
		sys_put_le32(_radio.ticks_anchor, extra);
	}
#endif /* CONFIG_BT_CTLR_EXT_SCAN_FP */

	ull_rx_put(node_rx->hdr.link, node_rx);
	ull_rx_sched();

	return 0;
}
