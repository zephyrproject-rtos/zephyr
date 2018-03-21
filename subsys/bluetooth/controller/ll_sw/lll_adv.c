#include <stdbool.h>
#include <stddef.h>

#include <zephyr/types.h>

#if defined(CONFIG_BT_CTLR_DEBUG_PINS)
#if defined(CONFIG_PRINTK)
#undef CONFIG_PRINTK
#endif
#endif

#include <toolchain.h>
#include <bluetooth/hci.h>

#if defined(CONFIG_SOC_FAMILY_NRF)
#include "hal/nrf5/ticker.h"
#endif /* CONFIG_SOC_FAMILY_NRF */

#include "hal/ccm.h"
#include "hal/radio.h"

#include "util/util.h"
#include "util/memq.h"

#include "ticker/ticker.h"

#include "pdu.h"

#include "ull_types.h"

#include "lll.h"
#include "lll_adv.h"

#include "lll_internal.h"
#include "lll_tim_internal.h"
#include "lll_chan_internal.h"
#include "lll_adv_internal.h"

#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

/*******************************************************************************
 * FIXME: Remove from here
 * NOTE: copied from ll_filter.c
 */
#define WL_SIZE 8
#define FILTER_IDX_NONE 0xFF
struct ll_filter {
	u8_t  enable_bitmask;
	u8_t  addr_type_bitmask;
	u8_t  bdaddr[WL_SIZE][BDADDR_SIZE];
};

struct ll_filter *ctrl_filter_get(bool whitelist);
/******************************************************************************/

static int init_reset(void);
static int prepare_cb(struct lll_prepare_param *prepare_param);
static int is_abort_cb(int prio, void *param);
static void abort_cb(struct lll_prepare_param *prepare_param, void *param);
static void isr_tx(void *param);
static void isr_rx(void *param);
static void isr_done(void *param);
static void isr_abort(void *param);
static void isr_cleanup(void *param);
static void isr_race(void *param);
static void chan_prepare(struct lll_adv *lll);
static inline int isr_rx_pdu(struct lll_adv *lll,
			     u8_t devmatch_ok, u8_t devmatch_id,
			     u8_t irkmatch_ok, u8_t irkmatch_id,
			     u8_t rssi_ready);
static inline bool isr_rx_sr_check(struct lll_adv *lll, struct pdu_adv *adv,
				   struct pdu_adv *sr, u8_t devmatch_ok,
				   u8_t *rl_idx);
static inline bool isr_rx_sr_adva_check(struct pdu_adv *adv,
					struct pdu_adv *sr);
#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
static inline u32_t isr_rx_sr_report(struct pdu_adv *pdu_adv_rx,
				     u8_t rssi_ready);
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

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

void lll_adv_prepare(void *param)
{
	struct lll_prepare_param *p = param;
	int err;

	printk("\t\tlll_adv_prepare (%p) enter.\n", p->param);

	err = lll_clk_on();
	printk("\t\tlll_clk_on: %d.\n", err);

	err = lll_prepare(is_abort_cb, abort_cb, prepare_cb, 0, p);

	printk("\t\tlll_adv_prepare (%p) exit (%d).\n", p->param, err);
}

static int init_reset(void)
{
	return 0;
}

static int prepare_cb(struct lll_prepare_param *prepare_param)
{
	struct lll_adv *lll = prepare_param->param;
	u32_t ticks_at_event;
	u32_t aa = 0x8e89bed6;
	u32_t remainder_us;
	u32_t remainder;
	int err = 0;

	printk("\t\t_prepare (%p) enter: expected %u, actual %u.\n",
	       prepare_param->param, prepare_param->ticks_at_expire,
	       ticker_ticks_now_get());
	DEBUG_RADIO_PREPARE_A(1);

	radio_reset();
	/* TODO: other Tx Power settings */
	radio_tx_power_set(0);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	/* TODO: if coded we use S8? */
	radio_phy_set(lll->phy_p, 1);
	radio_pkt_configure(8, PDU_AC_PAYLOAD_SIZE_MAX, (lll->phy_p << 1));
#else /* !CONFIG_BT_CTLR_ADV_EXT */
	radio_phy_set(0, 0);
	radio_pkt_configure(8, PDU_AC_PAYLOAD_SIZE_MAX, 0);
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

	radio_aa_set((u8_t *)&aa);
	radio_crc_configure(((0x5bUL) | ((0x06UL) << 8) | ((0x00UL) << 16)),
			    0x555555);

	lll->chan_map_curr = lll->chan_map;
	lll->chan_map_next = 0;

	chan_prepare(lll);

#if defined(CONFIG_BT_HCI_MESH_EXT)
	_radio.mesh_adv_end_us = 0;
#endif /* CONFIG_BT_HCI_MESH_EXT */


#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (ctrl_rl_enabled()) {
		struct ll_filter *filter =
			ctrl_filter_get(!!(_radio.advertiser.filter_policy));

		radio_filter_configure(filter->enable_bitmask,
				       filter->addr_type_bitmask,
				       (u8_t *)filter->bdaddr);
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
	remainder_us = radio_tmr_start(1, ticks_at_event, remainder);

	/* capture end of Tx-ed PDU, used to calculate HCTO. */
	radio_tmr_end_capture();

#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN)
	radio_gpio_pa_setup();
	radio_gpio_pa_lna_enable(remainder_us +
				 radio_tx_ready_delay_get(0, 0) -
				 CONFIG_BT_CTLR_GPIO_PA_OFFSET);
#else /* !CONFIG_BT_CTLR_GPIO_PA_PIN */
	ARG_UNUSED(remainder_us);
#endif /* !CONFIG_BT_CTLR_GPIO_PA_PIN */

#if (0 && defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
     (RADIO_TICKER_PREEMPT_PART_US <= RADIO_TICKER_PREEMPT_PART_MIN_US))
	/* check if preempt to start has changed */
	if (preempt_calc(&_radio.advertiser.hdr, RADIO_TICKER_ID_ADV,
			 ticks_at_expire) != 0) {
		_radio.state = STATE_STOP;
		radio_disable();
	} else
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */

	{
		/* Ticker Job Silence */
		/* FIXME:*/
#if 0 && (RADIO_TICKER_USER_ID_WORKER_PRIO == RADIO_TICKER_USER_ID_JOB_PRIO)
		u32_t ticker_status;

		ticker_status =
		    ticker_job_idle_get(RADIO_TICKER_INSTANCE_ID_RADIO,
					RADIO_TICKER_USER_ID_WORKER,
					ticker_job_disable, NULL);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY));
#endif
	}


	DEBUG_RADIO_PREPARE_A(1);
	printk("\t\t_prepare (%p) exit (%d).\n", prepare_param->param, err);

	return err;
}

static int is_abort_cb(int prio, void *param)
{
	struct lll_adv *lll = param;
	struct pdu_adv *pdu;

	pdu = lll_adv_data_curr_get(lll);
	if (pdu->type == PDU_ADV_TYPE_DIRECT_IND) {
		lll->chan_map_next = lll->chan_map;

		return 0;
	}

	return 1;
}

static void abort_cb(struct lll_prepare_param *prepare_param, void *param)
{
	int err;

	printk("\t\t_abort (%p) enter.\n", param);

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
	printk("\t\tlll_clk_off: %d.\n", err);

	lll_done(param);
	printk("\t\tlll_done (%p).\n", param);

	printk("\t\t_abort (%p) exit.\n", param);
}

static void isr_tx(void *param)
{
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

	radio_isr_set(isr_rx, param);
	radio_tmr_tifs_set(TIFS_US);
	radio_switch_complete_and_tx(0, 0, 0, 0);
	radio_pkt_rx_set(radio_pkt_scratch_get());

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

	/* capture end of CONNECT_IND PDU, used for calculating first
	 * slave event.
	 */
	radio_tmr_end_capture();

#if defined(CONFIG_BT_CTLR_SCAN_REQ_RSSI)
	radio_rssi_measure();
#endif /* CONFIG_BT_CTLR_SCAN_REQ_RSSI */

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	radio_gpio_lna_setup();
	radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() + TIFS_US - 4 -
				 radio_tx_chain_delay_get(0, 0) -
				 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#endif /* CONFIG_BT_CTLR_GPIO_LNA_PIN */
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

	if (crc_ok) {
		int err;

		err = isr_rx_pdu(param, devmatch_ok, devmatch_id, irkmatch_ok,
				 irkmatch_id, rssi_ready);
		if (!err) {
			return;
		}
	}

isr_rx_do_close:
	radio_isr_set(isr_done, param);
	radio_disable();
	return;
}

static void isr_done(void *param)
{
	struct lll_adv *lll = param;

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

#if defined(CONFIG_BT_HCI_MESH_EXT)
	if (_radio.advertiser.is_mesh &&
	    !_radio.mesh_adv_end_us) {
		_radio.mesh_adv_end_us = radio_tmr_end_get();
	}
#endif /* CONFIG_BT_HCI_MESH_EXT */

	if (!lll->chan_map_curr) {
		lll->chan_map_curr = lll->chan_map_next;
	}

	if (lll->chan_map_curr) {
		u32_t start_us;

		chan_prepare(lll);

#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN)
		start_us = radio_tmr_start_now(1);

		radio_gpio_pa_setup();
		radio_gpio_pa_lna_enable(start_us +
					 radio_tx_ready_delay_get(0, 0) -
					 CONFIG_BT_CTLR_GPIO_PA_OFFSET);
#else /* !CONFIG_BT_CTLR_GPIO_PA_PIN */
		ARG_UNUSED(start_us);

		radio_tx_enable();
#endif /* !CONFIG_BT_CTLR_GPIO_PA_PIN */

		/* capture end of Tx-ed PDU, used to calculate HCTO. */
		radio_tmr_end_capture();

		return;
	} else {
		struct ll_adv_set *ull = lll->hdr.parent;
		struct node_rx_hdr *node_rx;

		radio_filter_disable();

#if defined(CONFIG_BT_PERIPHERAL)
		if (!ull->is_hdcd)
#else /* !CONFIG_BT_PERIPHERAL */
		ARG_UNUSED(ull);
#endif /* !CONFIG_BT_PERIPHERAL */
		{
#if defined(CONFIG_BT_HCI_MESH_EXT)
			if (_radio.advertiser.is_mesh) {
				u32_t err;

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

			ull_rx_put(node_rx->link, node_rx);
			ull_rx_sched();
		}
#else /* !CONFIG_BT_CTLR_ADV_INDICATION */
		ARG_UNUSED(node_rx);
#endif /* !CONFIG_BT_CTLR_ADV_INDICATION */
	}

	isr_cleanup(param);
}

static void isr_abort(void *param)
{
	isr_cleanup(param);
}

static void isr_cleanup(void *param)
{
	int err;

	radio_isr_set(isr_race, param);
	radio_tmr_stop();

	err = lll_clk_off();
	printk("\t\tlll_clk_off: %d.\n", err);

	lll_done(NULL);
	printk("\t\tlll_done.\n");
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
	u8_t chan;
	u8_t upd = 0;

	pdu = lll_adv_data_latest_get(lll, &upd);
	scan_pdu = lll_adv_scan_rsp_latest_get(lll, &upd);
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
		radio_tmr_tifs_set(TIFS_US);
		radio_switch_complete_and_rx(0);
	} else {
		radio_isr_set(isr_done, lll);
		radio_switch_complete_and_disable();
	}

	chan = find_lsb_set(lll->chan_map_curr);
	LL_ASSERT(chan);

	printk("\t\tchan = %u.\n", chan);

	lll->chan_map_curr &= (lll->chan_map_curr - 1);

	chan_set(36 + chan);
}

static inline int isr_rx_pdu(struct lll_adv *lll,
			     u8_t devmatch_ok, u8_t devmatch_id,
			     u8_t irkmatch_ok, u8_t irkmatch_id,
			     u8_t rssi_ready)
{
	struct pdu_adv *pdu_rx, *pdu_adv;
#if defined(CONFIG_BT_CTLR_PRIVACY)
	/* An IRK match implies address resolution enabled */
	u8_t rl_idx = irkmatch_ok ? ctrl_rl_irk_idx(irkmatch_id) :
				    FILTER_IDX_NONE;
#else
	u8_t rl_idx = FILTER_IDX_NONE;
#endif /* CONFIG_BT_CTLR_PRIVACY */

	pdu_rx = (void *)radio_pkt_scratch_get();
	pdu_adv = lll_adv_data_curr_get(lll);
	if ((pdu_rx->type == PDU_ADV_TYPE_SCAN_REQ) &&
	    (pdu_rx->len == sizeof(struct pdu_adv_scan_req)) &&
	    isr_rx_sr_check(lll, pdu_adv, pdu_rx, devmatch_ok, &rl_idx)) {

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
		if (!IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT) ||
		    0 /* TODO: extended adv. scan req notification enabled */) {
			u32_t err;

			/* Generate the scan request event */
			err = isr_rx_sr_report(pdu_rx, rssi_ready);
			if (err) {
				/* Scan Response will not be transmitted */
				return err;
			}
		}
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

		radio_isr_set(isr_done, lll);
		radio_switch_complete_and_disable();
		radio_pkt_tx_set(lll_adv_scan_rsp_curr_get(lll));

#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN)
		radio_gpio_pa_setup();
		radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() + TIFS_US -
					 radio_rx_chain_delay_get(0, 0) -
					 CONFIG_BT_CTLR_GPIO_PA_OFFSET);
#endif /* CONFIG_BT_CTLR_GPIO_PA_PIN */

		/* assert if radio packet ptr is not set and radio started tx */
		LL_ASSERT(!radio_is_ready());

		return 0;

#if defined(CONFIG_BT_PERIPHERAL)
	} else if ((pdu_rx->type == PDU_ADV_TYPE_CONNECT_IND) &&
		   (pdu_rx->len == sizeof(struct pdu_adv_connect_ind)) &&
		   isr_adv_ci_check(pdu_adv, pdu_rx, devmatch_ok, &rl_idx) &&
		   ((_radio.fc_ena == 0) || (_radio.fc_req == _radio.fc_ack)) &&
		   (_radio.advertiser.conn)) {
		struct radio_le_conn_cmplt *radio_le_conn_cmplt;
		struct pdu_data *pdu_data;
		struct connection *conn;
		u32_t ticks_slot_offset;
		u32_t conn_interval_us;
		u32_t conn_offset_us;
		u32_t rx_ready_delay;
		u32_t ticker_status;

		if (IS_ENABLED(CONFIG_BT_CTLR_CHAN_SEL_2)) {
			node_rx = packet_rx_reserve_get(4);
		} else {
			node_rx = packet_rx_reserve_get(3);
		}

		if (!node_rx) {
			return 1;
		}

		_radio.state = STATE_STOP;
		radio_disable();

		/* acquire the slave context from advertiser */
		conn = _radio.advertiser.conn;
		_radio.advertiser.conn = NULL;

		/* Populate the slave context */
		conn->handle = mem_index_get(conn, _radio.conn_pool,
			CONNECTION_T_SIZE);
		memcpy(&conn->crc_init[0],
		       &pdu_rx->connect_ind.crc_init[0],
		       3);
		memcpy(&conn->access_addr[0],
		       &pdu_rx->connect_ind.access_addr[0],
		       4);
		memcpy(&conn->data_chan_map[0],
		       &pdu_rx->connect_ind.chan_map[0],
		       sizeof(conn->data_chan_map));
		conn->data_chan_count =
			util_ones_count_get(&conn->data_chan_map[0],
					    sizeof(conn->data_chan_map));
		conn->data_chan_hop = pdu_rx->connect_ind.hop;
		conn->conn_interval =
			pdu_rx->connect_ind.interval;
		conn_interval_us =
			pdu_rx->connect_ind.interval * 1250;
		conn->latency = pdu_rx->connect_ind.latency;
		memcpy((void *)&conn->slave.force, &conn->access_addr[0],
		       sizeof(conn->slave.force));
		conn->supervision_reload =
			RADIO_CONN_EVENTS((pdu_rx->connect_ind.timeout
					   * 10 * 1000), conn_interval_us);
		conn->procedure_reload = RADIO_CONN_EVENTS((40 * 1000 * 1000),
							   conn_interval_us);

#if defined(CONFIG_BT_CTLR_LE_PING)
		/* APTO in no. of connection events */
		conn->apto_reload = RADIO_CONN_EVENTS((30 * 1000 * 1000),
						      conn_interval_us);
		/* Dispatch LE Ping PDU 6 connection events (that peer would
		 * listen to) before 30s timeout
		 * TODO: "peer listens to" is greater than 30s due to latency
		 */
		conn->appto_reload = (conn->apto_reload > (conn->latency + 6)) ?
				     (conn->apto_reload - (conn->latency + 6)) :
				     conn->apto_reload;
#endif /* CONFIG_BT_CTLR_LE_PING */

		/* Prepare the rx packet structure */
		node_rx->hdr.handle = conn->handle;
		node_rx->hdr.type = NODE_RX_TYPE_CONNECTION;

		/* prepare connection complete structure */
		pdu_data = (void *)node_rx->pdu_data;
		radio_le_conn_cmplt = (void *)pdu_data->lldata;
		radio_le_conn_cmplt->status = 0x00;
		radio_le_conn_cmplt->role = 0x01;
#if defined(CONFIG_BT_CTLR_PRIVACY)
		radio_le_conn_cmplt->own_addr_type = pdu_rx->rx_addr;
		memcpy(&radio_le_conn_cmplt->own_addr[0],
		       &pdu_rx->connect_ind.adv_addr[0], BDADDR_SIZE);
		if (rl_idx != FILTER_IDX_NONE) {
			/* TODO: store rl_idx instead if safe */
			/* Store identity address */
			ll_rl_id_addr_get(rl_idx,
					  &radio_le_conn_cmplt->peer_addr_type,
					  &radio_le_conn_cmplt->peer_addr[0]);
			/* Mark it as identity address from RPA (0x02, 0x03) */
			radio_le_conn_cmplt->peer_addr_type += 2;

			/* Store peer RPA */
			memcpy(&radio_le_conn_cmplt->peer_rpa[0],
			       &pdu_rx->connect_ind.init_addr[0],
			       BDADDR_SIZE);
		} else {
			memset(&radio_le_conn_cmplt->peer_rpa[0], 0x0,
			       BDADDR_SIZE);
#else
		if (1) {
#endif /* CONFIG_BT_CTLR_PRIVACY */
			radio_le_conn_cmplt->peer_addr_type = pdu_rx->tx_addr;
			memcpy(&radio_le_conn_cmplt->peer_addr[0],
			       &pdu_rx->connect_ind.init_addr[0],
			       BDADDR_SIZE);
		}

		radio_le_conn_cmplt->interval =
			pdu_rx->connect_ind.interval;
		radio_le_conn_cmplt->latency =
			pdu_rx->connect_ind.latency;
		radio_le_conn_cmplt->timeout =
			pdu_rx->connect_ind.timeout;
		radio_le_conn_cmplt->mca =
			pdu_rx->connect_ind.sca;

		/* enqueue connection complete structure into queue */
		rx_fc_lock(conn->handle);
		packet_rx_enqueue();

		/* Use Channel Selection Algorithm #2 if peer too supports it */
		if (IS_ENABLED(CONFIG_BT_CTLR_CHAN_SEL_2)) {
			struct radio_le_chan_sel_algo *le_chan_sel_algo;

			/* Generate LE Channel Selection Algorithm event */
			node_rx = packet_rx_reserve_get(3);
			LL_ASSERT(node_rx);

			node_rx->hdr.handle = conn->handle;
			node_rx->hdr.type = NODE_RX_TYPE_CHAN_SEL_ALGO;

			pdu_data = (void *)node_rx->pdu_data;
			le_chan_sel_algo = (void *)pdu_data->lldata;

			if (pdu_rx->chan_sel) {
				u16_t aa_ls =
					((u16_t)conn->access_addr[1] << 8) |
					conn->access_addr[0];
				u16_t aa_ms =
					((u16_t)conn->access_addr[3] << 8) |
					 conn->access_addr[2];

				conn->data_chan_sel = 1;
				conn->data_chan_id = aa_ms ^ aa_ls;

				le_chan_sel_algo->chan_sel_algo = 0x01;
			} else {
				le_chan_sel_algo->chan_sel_algo = 0x00;
			}

			packet_rx_enqueue();
		}

		/* calculate the window widening */
		conn->slave.sca = pdu_rx->connect_ind.sca;
		conn->slave.window_widening_periodic_us =
			(((gc_lookup_ppm[_radio.sca] +
			   gc_lookup_ppm[conn->slave.sca]) *
			  conn_interval_us) + (1000000 - 1)) / 1000000;
		conn->slave.window_widening_max_us =
			(conn_interval_us >> 1) - RADIO_TIFS;
		conn->slave.window_size_event_us =
			pdu_rx->connect_ind.win_size * 1250;
		conn->slave.window_size_prepare_us = 0;

		rx_ready_delay = radio_rx_ready_delay_get(0, 0);

		/* calculate slave slot */
		conn->hdr.ticks_slot =
			HAL_TICKER_US_TO_TICKS(RADIO_TICKER_START_PART_US +
					       rx_ready_delay + 328 +
					       RADIO_TIFS + 328);
		conn->hdr.ticks_active_to_start = _radio.ticks_active_to_start;
		conn->hdr.ticks_xtal_to_start =
			HAL_TICKER_US_TO_TICKS(RADIO_TICKER_XTAL_OFFSET_US);
		conn->hdr.ticks_preempt_to_start =
			HAL_TICKER_US_TO_TICKS(RADIO_TICKER_PREEMPT_PART_MIN_US);
		ticks_slot_offset =
			(conn->hdr.ticks_active_to_start <
			 conn->hdr.ticks_xtal_to_start) ?
			conn->hdr.ticks_xtal_to_start :
			conn->hdr.ticks_active_to_start;
		conn_interval_us -=
			conn->slave.window_widening_periodic_us;

		conn_offset_us = radio_tmr_end_get();
		conn_offset_us +=
			((u64_t)pdu_rx->connect_ind.win_offset +
			 1) * 1250;
		conn_offset_us -= radio_tx_chain_delay_get(0, 0);
		conn_offset_us -= rx_ready_delay;
		conn_offset_us -= RADIO_TICKER_JITTER_US << 1;
		conn_offset_us -= RADIO_TICKER_JITTER_US;

		/* Stop Advertiser */
		ticker_status = ticker_stop(RADIO_TICKER_INSTANCE_ID_RADIO,
					    RADIO_TICKER_USER_ID_WORKER,
					    RADIO_TICKER_ID_ADV,
					    ticker_stop_adv_assert,
					    (void *)__LINE__);
		ticker_stop_adv_assert(ticker_status, (void *)__LINE__);

		/* Stop Direct Adv Stopper */
		if (pdu_adv->type == PDU_ADV_TYPE_DIRECT_IND) {
			/* Advertiser stop can expire while here in this ISR.
			 * Deferred attempt to stop can fail as it would have
			 * expired, hence ignore failure.
			 */
			ticker_stop(RADIO_TICKER_INSTANCE_ID_RADIO,
				    RADIO_TICKER_USER_ID_WORKER,
				    RADIO_TICKER_ID_ADV_STOP, NULL, NULL);
		}

		/* Start Slave */
		ticker_status = ticker_start(RADIO_TICKER_INSTANCE_ID_RADIO,
		     RADIO_TICKER_USER_ID_WORKER,
		     RADIO_TICKER_ID_FIRST_CONNECTION + conn->handle,
		     (_radio.ticks_anchor - ticks_slot_offset),
		     HAL_TICKER_US_TO_TICKS(conn_offset_us),
		     HAL_TICKER_US_TO_TICKS(conn_interval_us),
		     HAL_TICKER_REMAINDER(conn_interval_us), TICKER_NULL_LAZY,
		     (ticks_slot_offset + conn->hdr.ticks_slot),
		     event_slave_prepare, conn, ticker_success_assert,
		     (void *)__LINE__);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY));

		return 0;
#endif /* CONFIG_BT_PERIPHERAL */
	}

	return 1;
}

static inline bool isr_rx_sr_check(struct lll_adv *lll, struct pdu_adv *adv,
				   struct pdu_adv *sr, u8_t devmatch_ok,
				   u8_t *rl_idx)
{
#if defined(CONFIG_BT_CTLR_PRIVACY)
	return ((((_radio.advertiser.filter_policy & 0x01) == 0) &&
		 ctrl_rl_addr_allowed(sr->tx_addr, sr->scan_req.scan_addr,
				      rl_idx)) ||
		(((_radio.advertiser.filter_policy & 0x01) != 0) &&
		 (devmatch_ok || ctrl_irk_whitelisted(*rl_idx)))) &&
		isr_rx_sr_adva_check(adv, sr);
#else
	return (((lll->filter_policy & 0x01) == 0) || devmatch_ok) &&
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
static inline u32_t isr_rx_sr_report(struct pdu_adv *pdu_adv_rx,
				     u8_t rssi_ready)
{
	struct node_rx_pdu *node_rx;
	struct pdu_adv *pdu_adv;
	u8_t pdu_len;

	node_rx = ull_pdu_rx_alloc_peek(3);
	if (!node_rx) {
		return 1;
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
	((u8_t *)pdu_adv)[pdu_len] = (rssi_ready) ? (radio_rssi_get() & 0x7f) :
						    0x7f;

	ull_rx_put(node_rx->hdr.link, node_rx);
	ull_rx_sched();

	return 0;
}
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */
