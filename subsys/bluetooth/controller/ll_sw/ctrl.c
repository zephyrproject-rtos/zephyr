/*
 * Copyright (c) 2016-2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>

#include <soc.h>
#include <device.h>
#include <clock_control.h>
#include <bluetooth/hci.h>
#include <misc/util.h>

#include "ll.h"

#if defined(CONFIG_SOC_FAMILY_NRF)
#include <drivers/clock_control/nrf5_clock_control.h>
#include <drivers/entropy/nrf5_entropy.h>
#endif /* CONFIG_SOC_FAMILY_NRF */

#include "hal/cpu.h"
#include "hal/ecb.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/radio_txp.h"
#include "hal/ticker.h"
#include "hal/debug.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"

#include "ticker/ticker.h"

#include "pdu.h"
#include "ctrl.h"
#include "ctrl_internal.h"

#include "ll_filter.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#include "common/log.h"

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
#define RADIO_RSSI_SAMPLE_COUNT	10
#define RADIO_RSSI_THRESHOLD	4
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

#define SILENT_CONNECTION	0

/* Macro to convert time in us to connection interval units */
#define RADIO_CONN_EVENTS(x, y) ((u16_t)(((x) + (y) - 1) / (y)))

/* Macro to return packet time */
#if defined(CONFIG_BT_CTLR_PHY_CODED)
#define RADIO_PKT_TIME(octets, phy) \
	(((phy) & BIT(2)) ? \
	 (80 + 256 + 16 + 24 + ((((2 + (octets) + 4) * 8) + 24 + 3) * 8)) : \
	 (((octets) + 14) * 8 / BIT(((phy) & 0x03) >> 1)))
#else /* !CONFIG_BT_CTLR_PHY_CODED */
#define RADIO_PKT_TIME(octets, phy) \
	(((octets) + 14) * 8 / BIT(((phy) & 0x03) >> 1))
#endif /* !CONFIG_BT_CTLR_PHY_CODED */

/* Inter Frame Space */
#define RADIO_TIFS    150 /* BT Spec. defined */
/* Inter Event Space */
#define RADIO_TIES_US 625 /* Implementation defined */

/* Implementation defines */
#define RADIO_TICKER_JITTER_US           16
#define RADIO_TICKER_START_PART_US       300
#define RADIO_TICKER_XTAL_OFFSET_US      1200
#define RADIO_TICKER_PREEMPT_PART_US     0
#define RADIO_TICKER_PREEMPT_PART_MIN_US 0
#define RADIO_TICKER_PREEMPT_PART_MAX_US RADIO_TICKER_XTAL_OFFSET_US

enum role {
	ROLE_NONE,
	ROLE_ADV,
	ROLE_SCAN,
	ROLE_SLAVE,
	ROLE_MASTER,
};

enum state {
	STATE_NONE,
	STATE_RX,
	STATE_TX,
	STATE_CLOSE,
	STATE_STOP,
	STATE_ABORT,
};

struct advertiser {
	struct shdr hdr;

	u8_t chan_map_current:3;
	u8_t rfu:3;
	u8_t is_hdcd:1;
	u8_t is_enabled:1;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	u8_t phy_p:3;
#endif /* CONFIG_BT_CTLR_ADV_EXT */
	u8_t chan_map:3;
	u8_t filter_policy:2;
#if defined(CONFIG_BT_CTLR_PRIVACY)
	u8_t rl_idx;
#endif /* CONFIG_BT_CTLR_PRIVACY */

	struct radio_adv_data adv_data;
	struct radio_adv_data scan_data;

	struct connection *conn;
};

struct scanner {
	struct shdr hdr;

	u8_t  is_enabled:1;
	u8_t  state:1;
	u8_t  chan:2;
	u8_t  rfu:4;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	u8_t  phy:3;
#endif /* CONFIG_BT_CTLR_ADV_EXT */
	u8_t  type:1;
	u8_t  filter_policy:2;
	u8_t  adv_addr_type:1;
	u8_t  init_addr_type:1;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	u8_t  rpa_gen:1;
	/* initiator only */
	u8_t rl_idx;
#endif /* CONFIG_BT_CTLR_PRIVACY */
	u8_t  init_addr[BDADDR_SIZE];
	u8_t  adv_addr[BDADDR_SIZE];

	u32_t ticks_window;

	u16_t conn_interval;
	u16_t conn_latency;
	u16_t conn_timeout;
	u32_t ticks_conn_slot;
	struct connection *conn;

	u32_t win_offset_us;
};

static struct {
	struct device *hf_clock;
	struct device *entropy;

	u32_t ticks_anchor;
	u32_t remainder_anchor;

	u8_t  is_k32src_stable;

	u8_t  volatile ticker_id_prepare;
	u8_t  volatile ticker_id_event;
	u8_t  volatile ticker_id_stop;

	enum  role volatile role;
	enum  state state;

	struct advertiser advertiser;
	struct scanner scanner;

	void  *conn_pool;
	void  *conn_free;
	u8_t  connection_count;
	struct connection *conn_curr;

	u8_t  packet_counter;
	u8_t  crc_expire;

	u8_t  data_chan_map[5];
	u8_t  data_chan_count;
	u8_t  sca;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	/* DLE global settings */
	u16_t default_tx_octets;
	u16_t default_tx_time;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
	u16_t default_phy_tx;
	u16_t default_phy_rx;
#endif /* CONFIG_BT_CTLR_PHY */

	/** @todo below members to be made role specific and quota managed for
	 * Rx-es.
	 */
	/* Advertiser, Scanner, and Connections Rx data pool */
	void  *pkt_rx_data_pool;
	void  *pkt_rx_data_free;
	u16_t packet_data_octets_max;
	u16_t packet_rx_data_pool_size;
	u16_t packet_rx_data_size;
	u8_t  packet_rx_data_count;
	/* Free queue Rx data buffers */
	struct radio_pdu_node_rx **packet_rx;
	u8_t  packet_rx_count;
	u8_t  volatile packet_rx_last;
	u8_t  packet_rx_acquire;

	/* Controller to Host event-cum-data queue */
	void        *link_rx_pool;
	void        *link_rx_free;
	memq_link_t *link_rx_head;
	memq_link_t *volatile link_rx_tail;
	u8_t        link_rx_data_quota;

	/* Connections common Tx ctrl and data pool */
	void  *pkt_tx_ctrl_pool;
	void  *pkt_tx_ctrl_free;
	void  *pkt_tx_data_pool;
	void  *pkt_tx_data_free;
	u16_t packet_tx_data_size;

	/* Host to Controller Tx, and Controller to Host Num complete queue */
	struct pdu_data_q_tx *pkt_tx;
	struct pdu_data_q_tx *pkt_release;
	u8_t  packet_tx_count;
	u8_t  volatile packet_tx_first;
	u8_t  packet_tx_last;
	u8_t  packet_release_first;
	u8_t  volatile packet_release_last;

	u16_t fc_handle[TRIPLE_BUFFER_SIZE];
	u8_t  volatile fc_req;
	u8_t  fc_ack;
	u8_t  fc_ena;

	u32_t ticks_active_to_start;

	struct connection *conn_upd;
} _radio;

static u16_t const gc_lookup_ppm[] = { 500, 250, 150, 100, 75, 50, 30, 20 };

static void common_init(void);
static void ticker_success_assert(u32_t status, void *params);
static void ticker_stop_adv_assert(u32_t status, void *params);
static void ticker_stop_scan_assert(u32_t status, void *params);
static void ticker_update_adv_assert(u32_t status, void *params);
static void ticker_update_slave_assert(u32_t status, void *params);
static void event_inactive(u32_t ticks_at_expire, u32_t remainder,
			   u16_t lazy, void *context);

#if defined(RADIO_UNIT_TEST) && \
	defined(CONFIG_BT_CTLR_CHAN_SEL_2)
static void chan_sel_2_ut(void);
#endif /* CONFIG_BT_CTLR_CHAN_SEL_2 */

static void adv_setup(void);
static void event_adv(u32_t ticks_at_expire, u32_t remainder, u16_t lazy,
		      void *context);
static void event_scan(u32_t ticks_at_expire, u32_t remainder, u16_t lazy,
		       void *context);
static void event_slave_prepare(u32_t ticks_at_expire, u32_t remainder,
				u16_t lazy, void *context);
static void event_slave(u32_t ticks_at_expire, u32_t remainder, u16_t lazy,
			void *context);
static void event_master_prepare(u32_t ticks_at_expire, u32_t remainder,
				 u16_t lazy, void *context);
static void event_master(u32_t ticks_at_expire, u32_t remainder, u16_t lazy,
			 void *context);
static void rx_packet_set(struct connection *conn,
			  struct pdu_data *pdu_data_rx);
static void tx_packet_set(struct connection *conn,
			  struct pdu_data *pdu_data_tx);
static void prepare_pdu_data_tx(struct connection *conn,
				struct pdu_data **pdu_data_tx);
static void packet_rx_allocate(u8_t max);
static inline u8_t packet_rx_acquired_count_get(void);
static inline struct radio_pdu_node_rx *packet_rx_reserve_get(u8_t count);
static void packet_rx_enqueue(void);
static void packet_tx_enqueue(u8_t max);
static struct pdu_data *empty_tx_enqueue(struct connection *conn);
static void ctrl_tx_enqueue(struct connection *conn,
			    struct radio_pdu_node_tx *node_tx);
static void pdu_node_tx_release(u16_t handle,
				struct radio_pdu_node_tx *node_tx);
static void connection_release(struct connection *conn);
static void terminate_ind_rx_enqueue(struct connection *conn, u8_t reason);
static u8_t conn_update(struct connection *conn, struct pdu_data *pdu_data_rx);

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
    defined(CONFIG_BT_CTLR_SCHED_ADVANCED)
static u32_t conn_update_req(struct connection *conn);
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED && CONFIG_BT_CTLR_SCHED_ADVANCED */

static u32_t chan_map_update(struct connection *conn,
			     struct pdu_data *pdu_data_rx);

#if defined(CONFIG_BT_CTLR_PHY)
static inline u8_t phy_upd_ind_recv(struct radio_pdu_node_rx *node_rx,
				    u8_t *rx_enqueue);
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_LE_ENC)
static void enc_req_reused_send(struct connection *conn,
				struct radio_pdu_node_tx *node_tx);
static u8_t enc_rsp_send(struct connection *conn);
static u8_t start_enc_rsp_send(struct connection *conn,
			       struct pdu_data *pdu_ctrl_tx);
static u8_t pause_enc_rsp_send(struct connection *conn, u8_t req);
#endif /* CONFIG_BT_CTLR_LE_ENC */

static u8_t unknown_rsp_send(struct connection *conn, u8_t type);
static u8_t feature_rsp_send(struct connection *conn,
			     struct pdu_data *pdu_data_rx);
static u8_t version_ind_send(struct connection *conn,
			     struct pdu_data *pdu_data_rx, u8_t *rx_enqueue);

#if defined(CONFIG_BT_CTLR_LE_PING)
static u8_t ping_resp_send(struct connection *conn);
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ) || \
    defined(CONFIG_BT_CTLR_PHY)
static u8_t reject_ext_ind_send(struct connection *conn, u8_t reject_opcode,
				u8_t error_code);
#endif

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
#if !defined(CONFIG_BT_CTLR_PHY)
static void length_resp_send(struct connection *conn,
			     struct radio_pdu_node_tx *node_tx,
			     u16_t eff_rx_octets, u16_t eff_tx_octets);
#else /* CONFIG_BT_CTLR_PHY */
static void length_resp_send(struct connection *conn,
			     struct radio_pdu_node_tx *node_tx,
			     u16_t eff_rx_octets, u16_t eff_rx_time,
			     u16_t eff_tx_octets, u16_t eff_tx_time);
#endif /* CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
static u8_t phy_rsp_send(struct connection *conn, struct pdu_data *pdu_data_rx);
#endif /* CONFIG_BT_CTLR_PHY */

static u32_t role_disable(u8_t ticker_id_primary, u8_t ticker_id_stop);
static void rx_fc_lock(u16_t handle);

/*****************************************************************************
 *RADIO
 ****************************************************************************/
u32_t radio_init(void *hf_clock, u8_t sca, void *entropy,
		 u8_t connection_count_max,
		 u8_t rx_count_max, u8_t tx_count_max,
		 u16_t packet_data_octets_max,
		 u16_t packet_tx_data_size, u8_t *mem_radio,
		 u16_t mem_size)
{
	u32_t retcode;
	u8_t *mem_radio_end;

	/* intialise hf_clock device to use in prepare */
	_radio.hf_clock = hf_clock;

	/* initialise SCA */
	_radio.sca = sca;

	/* intialise entropy device to use in ISRs */
	_radio.entropy = entropy;

	/* initialised radio mem end variable */
	mem_radio_end = mem_radio + mem_size;

	/* initialise connection context memory */
	_radio.connection_count = connection_count_max;
	_radio.conn_pool = mem_radio;
	mem_radio += (sizeof(struct connection) * _radio.connection_count);

	/* initialise rx and tx queue counts */

	/* additional for pdu to NACK or receive empty PDU,
	 * 1 scan resp and 1* ctrl event.
	 */
	rx_count_max += 3;
	 /* additional pdu to send enc_req ctrl pdu */
	tx_count_max += 1;
	_radio.packet_rx_count = (rx_count_max + 1);
	_radio.packet_tx_count = (tx_count_max + 1);
	_radio.link_rx_data_quota = rx_count_max;

	/* initialise rx queue memory */
	_radio.packet_rx = (void *)mem_radio;
	mem_radio +=
		(sizeof(struct radio_pdu_node_rx *)*_radio.packet_rx_count);

	/* initialise tx queue memory */
	_radio.pkt_tx = (void *)mem_radio;
	mem_radio += (sizeof(struct pdu_data_q_tx) * _radio.packet_tx_count);

	/* initialise tx release queue memory */
	_radio.pkt_release = (void *)mem_radio;
	mem_radio += (sizeof(struct pdu_data_q_tx) * _radio.packet_tx_count);

	/* initialise rx memory size and count */
	_radio.packet_data_octets_max = packet_data_octets_max;
	if ((PDU_AC_SIZE_MAX + PDU_AC_SIZE_EXTRA) <
	    (offsetof(struct pdu_data, lldata) +
			_radio.packet_data_octets_max)) {
		_radio.packet_rx_data_pool_size =
		    (MROUND(offsetof(struct radio_pdu_node_rx, pdu_data) +
			    offsetof(struct pdu_data, lldata) +
			    _radio.packet_data_octets_max) * rx_count_max);
	} else {
		_radio.packet_rx_data_pool_size =
			(MROUND(offsetof(struct radio_pdu_node_rx, pdu_data) +
			  (PDU_AC_SIZE_MAX + PDU_AC_SIZE_EXTRA)) * rx_count_max);
	}
	_radio.packet_rx_data_size = PACKET_RX_DATA_SIZE_MIN;
	_radio.packet_rx_data_count = (_radio.packet_rx_data_pool_size /
				       _radio.packet_rx_data_size);

	/* initialise rx data pool memory */
	_radio.pkt_rx_data_pool = mem_radio;
	mem_radio += _radio.packet_rx_data_pool_size;

	/* initialise rx link pool memory */
	_radio.link_rx_pool = mem_radio;
	mem_radio += (sizeof(memq_link_t) * (_radio.packet_rx_count +
					     _radio.connection_count));

	/* initialise tx ctrl pool memory */
	_radio.pkt_tx_ctrl_pool = mem_radio;
	mem_radio += PACKET_TX_CTRL_SIZE_MIN * PACKET_MEM_COUNT_TX_CTRL;

	/* initialise tx data memory size and count */
	_radio.packet_tx_data_size =
		MROUND(offsetof(struct radio_pdu_node_tx, pdu_data) +
		       offsetof(struct pdu_data, lldata) +
		       packet_tx_data_size);

	/* initialise tx data pool memory */
	_radio.pkt_tx_data_pool = mem_radio;
	mem_radio += (_radio.packet_tx_data_size * tx_count_max);

	/* check for sufficient memory allocation for stack
	 * configuration.
	 */
	retcode = (mem_radio - mem_radio_end);
	if (retcode) {
		return (retcode + mem_size);
	}

	/* enable connection handle based on-off flow control feature.
	 * This is a simple flow control to rx data only on one selected
	 * connection handle.
	 * TODO: replace this feature with host-to-controller flowcontrol
	 * implementation/design.
	 */
	_radio.fc_ena = 1;

	/* memory allocations */
	common_init();

#if defined(RADIO_UNIT_TEST) && defined(CONFIG_BT_CTLR_CHAN_SEL_2)
	chan_sel_2_ut();
#endif /* RADIO_UNIT_TEST && CONFIG_BT_CTLR_CHAN_SEL_2 */

	radio_setup();

	return retcode;
}

struct device *radio_hf_clock_get(void)
{
	return _radio.hf_clock;
}

void ll_reset(void)
{
	u16_t conn_handle;

	/* disable advertiser events */
	role_disable(RADIO_TICKER_ID_ADV, RADIO_TICKER_ID_ADV_STOP);

	/* disable oberver events */
	role_disable(RADIO_TICKER_ID_SCAN, RADIO_TICKER_ID_SCAN_STOP);

	/* disable connection events */
	for (conn_handle = 0; conn_handle < _radio.connection_count;
	     conn_handle++) {
		role_disable(RADIO_TICKER_ID_FIRST_CONNECTION + conn_handle,
			     TICKER_NULL);
	}

	/* reset controller context members */
	_radio.advertiser.is_enabled = 0;
	_radio.advertiser.conn = NULL;
	_radio.scanner.is_enabled = 0;
	_radio.scanner.conn = NULL;
	_radio.packet_rx_data_size = PACKET_RX_DATA_SIZE_MIN;
	_radio.packet_rx_data_count = (_radio.packet_rx_data_pool_size /
				       _radio.packet_rx_data_size);
	_radio.packet_rx_last = 0;
	_radio.packet_rx_acquire = 0;
	_radio.link_rx_data_quota = _radio.packet_rx_count - 1;
	_radio.packet_tx_first = 0;
	_radio.packet_tx_last = 0;
	_radio.packet_release_first = 0;
	_radio.packet_release_last = 0;

	/* reset FC feature */
	/* TODO: remove this feature related all code in the future */
	_radio.fc_ack = _radio.fc_req;

	/* reset whitelist and resolving list */
	ll_filter_reset(false);

	/* memory allocations */
	common_init();
}

static void common_init(void)
{
	memq_link_t *link;

	/* initialise connection pool. */
	if (_radio.connection_count) {
		mem_init(_radio.conn_pool, CONNECTION_T_SIZE,
			 _radio.connection_count,
			 &_radio.conn_free);
	} else {
		_radio.conn_free = NULL;
	}

	/* initialise rx pool. */
	mem_init(_radio.pkt_rx_data_pool,
		 _radio.packet_rx_data_size,
		 _radio.packet_rx_data_count,
		 &_radio.pkt_rx_data_free);

	/* initialise rx link pool. */
	mem_init(_radio.link_rx_pool, (sizeof(memq_link_t)),
		 (_radio.packet_rx_count + _radio.connection_count),
		 &_radio.link_rx_free);

	/* initialise ctrl tx pool. */
	mem_init(_radio.pkt_tx_ctrl_pool, PACKET_TX_CTRL_SIZE_MIN,
		 PACKET_MEM_COUNT_TX_CTRL, &_radio.pkt_tx_ctrl_free);

	/* initialise data tx pool. */
	mem_init(_radio.pkt_tx_data_pool, _radio.packet_tx_data_size,
		 (_radio.packet_tx_count - 1), &_radio.pkt_tx_data_free);

	/* initialise the event-cum-data memq */
	link = mem_acquire(&_radio.link_rx_free);
	LL_ASSERT(link);
	memq_init(link, &_radio.link_rx_head, (void *)&_radio.link_rx_tail);

	/* initialise advertiser channel map */
	_radio.advertiser.chan_map = 0x07;

	/* initialise connection channel map */
	_radio.data_chan_map[0] = 0xFF;
	_radio.data_chan_map[1] = 0xFF;
	_radio.data_chan_map[2] = 0xFF;
	_radio.data_chan_map[3] = 0xFF;
	_radio.data_chan_map[4] = 0x1F;
	_radio.data_chan_count = 37;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	/* Initialize the DLE defaults */
	_radio.default_tx_octets = RADIO_LL_LENGTH_OCTETS_RX_MIN;
	_radio.default_tx_time = RADIO_PKT_TIME(RADIO_LL_LENGTH_OCTETS_RX_MIN,
						0);
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
	/* Initialize the PHY defaults */
	_radio.default_phy_tx = BIT(0);
	_radio.default_phy_rx = BIT(0);

#if defined(CONFIG_BT_CTLR_PHY_2M)
	_radio.default_phy_tx |= BIT(1);
	_radio.default_phy_rx |= BIT(1);
#endif /* CONFIG_BT_CTLR_PHY_2M */

#if defined(CONFIG_BT_CTLR_PHY_CODED)
	_radio.default_phy_tx |= BIT(2);
	_radio.default_phy_rx |= BIT(2);
#endif /* CONFIG_BT_CTLR_PHY_CODED */
#endif /* CONFIG_BT_CTLR_PHY */

	/* allocate the rx queue */
	packet_rx_allocate(0xFF);
}

static inline u32_t addr_us_get(u8_t phy)
{
	switch (phy) {
	default:
	case BIT(0):
		return 40;
	case BIT(1):
		return 24;
	case BIT(2):
		return 376;
	}
}

#if defined(SILENT_CONNECTION)
static inline u32_t empty_pkt_us_get(u8_t phy)
{
	switch (phy) {
	default:
	case BIT(0):
		return 80;
	case BIT(1):
		return 44;
	case BIT(2):
		return 720;
	}
}
#endif

static inline void isr_radio_state_tx(void)
{
	u32_t hcto;

	_radio.state = STATE_RX;

	hcto = radio_tmr_tifs_base_get()
		+ RADIO_TIFS + 4 + 1; /* 1us, end jitter */

	radio_tmr_tifs_set(RADIO_TIFS);

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	radio_gpio_lna_setup();
#endif /* CONFIG_BT_CTLR_GPIO_LNA_PIN */

	switch (_radio.role) {
	case ROLE_ADV:
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
		radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() +
					 RADIO_TIFS - 4 -
					 radio_tx_chain_delay_get(0, 0) -
					 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#endif /* CONFIG_BT_CTLR_GPIO_LNA_PIN */
		break;

	case ROLE_SCAN:
		radio_switch_complete_and_tx(0, 0, 0, 0);
		radio_pkt_rx_set(_radio.packet_rx
					[_radio.packet_rx_last]->pdu_data);

		/* assert if radio packet ptr is not set and radio started rx */
		LL_ASSERT(!radio_is_ready());

#if defined(CONFIG_BT_CTLR_PRIVACY)
		if (ctrl_rl_enabled()) {
			u8_t count, *irks = ctrl_irks_get(&count);

			radio_ar_configure(count, irks);
		}
#endif /* CONFIG_BT_CTLR_PRIVACY */

		hcto += radio_rx_chain_delay_get(0, 0);
		hcto += addr_us_get(0);
		hcto -= radio_tx_chain_delay_get(0, 0);

		radio_tmr_hcto_configure(hcto);
		radio_rssi_measure();

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
		radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() +
					 RADIO_TIFS - 4 -
					 radio_tx_chain_delay_get(0, 0) -
					 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#endif /* CONFIG_BT_CTLR_GPIO_LNA_PIN */
		break;

	case ROLE_MASTER:

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
		if (_radio.packet_counter == 0) {
			radio_rssi_measure();
		}
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

		/* fall thru */

	case ROLE_SLAVE:

#if defined(CONFIG_BT_CTLR_PHY)
		radio_switch_complete_and_tx(_radio.conn_curr->phy_rx, 0,
					     _radio.conn_curr->phy_tx,
					     _radio.conn_curr->phy_flags);
#else /* !CONFIG_BT_CTLR_PHY */
		radio_switch_complete_and_tx(0, 0, 0, 0);
#endif /* !CONFIG_BT_CTLR_PHY */

		rx_packet_set(_radio.conn_curr,	(void *)
			_radio.packet_rx[_radio.packet_rx_last]->pdu_data);

		/* assert if radio packet ptr is not set and radio started rx */
		LL_ASSERT(!radio_is_ready());

#if defined(CONFIG_BT_CTLR_PHY)
		hcto += radio_rx_chain_delay_get(_radio.conn_curr->phy_rx, 1);
		hcto += addr_us_get(_radio.conn_curr->phy_rx);
		hcto -= radio_tx_chain_delay_get(_radio.conn_curr->phy_tx,
						 _radio.conn_curr->phy_flags);
#else /* !CONFIG_BT_CTLR_PHY */
		hcto += radio_rx_chain_delay_get(0, 0);
		hcto += addr_us_get(0);
		hcto -= radio_tx_chain_delay_get(0, 0);
#endif /* !CONFIG_BT_CTLR_PHY */

		radio_tmr_hcto_configure(hcto);

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
#if defined(CONFIG_BT_CTLR_PHY)
		radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() +
					 RADIO_TIFS - 4 -
					 radio_tx_chain_delay_get(
						 _radio.conn_curr->phy_tx,
						 _radio.conn_curr->phy_flags) -
					 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#else /* !CONFIG_BT_CTLR_PHY */
		radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() +
					 RADIO_TIFS - 4 -
					 radio_tx_chain_delay_get(0, 0) -
					 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#endif /* !CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CTLR_GPIO_LNA_PIN */

#if defined(CONFIG_BT_CTLR_PROFILE_ISR) || \
    defined(CONFIG_BT_CTLR_GPIO_PA_PIN)
		radio_tmr_end_capture();
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

		/* Route the tx packet to respective connections */
		/* TODO: use timebox for tx enqueue (instead of 1 packet
		 * that is routed, which may not be for the current connection)
		 * try to route as much tx packet in queue into corresponding
		 * connection's tx list.
		 */
		packet_tx_enqueue(1);

		break;

	case ROLE_NONE:
	default:
		LL_ASSERT(0);
		break;
	}
}

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
static u32_t isr_rx_adv_sr_report(struct pdu_adv *pdu_adv_rx, u8_t rssi_ready)
{
	struct radio_pdu_node_rx *node_rx;
	struct pdu_adv *pdu_adv;
	u8_t pdu_len;

	node_rx = packet_rx_reserve_get(3);
	if (node_rx == 0) {
		return 1;
	}

	/* Prepare the report (scan req) */
	node_rx->hdr.handle = 0xffff;
	node_rx->hdr.type = NODE_RX_TYPE_SCAN_REQ;

	/* Make a copy of PDU into Rx node (as the received PDU is in the
	 * scratch buffer), and save the RSSI value.
	 */
	pdu_adv = (void *)node_rx->pdu_data;
	pdu_len = offsetof(struct pdu_adv, payload) + pdu_adv_rx->len;
	memcpy(pdu_adv, pdu_adv_rx, pdu_len);
	((u8_t *)pdu_adv)[pdu_len] =
		(rssi_ready) ? (radio_rssi_get() & 0x7f) : 0x7f;

	packet_rx_enqueue();

	return 0;
}
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

static inline bool isr_adv_sr_adva_check(struct pdu_adv *adv,
					 struct pdu_adv *sr)
{
	return (adv->tx_addr == sr->rx_addr) &&
		!memcmp(adv->adv_ind.addr, sr->scan_req.adv_addr, BDADDR_SIZE);
}

static inline bool isr_adv_sr_check(struct pdu_adv *adv, struct pdu_adv *sr,
				    u8_t devmatch_ok, u8_t *rl_idx)
{
#if defined(CONFIG_BT_CTLR_PRIVACY)
	return ((((_radio.advertiser.filter_policy & 0x01) == 0) &&
		 ctrl_rl_addr_allowed(sr->tx_addr, sr->scan_req.scan_addr,
				      rl_idx)) ||
		(((_radio.advertiser.filter_policy & 0x01) != 0) &&
		 (devmatch_ok || ctrl_irk_whitelisted(*rl_idx)))) &&
		isr_adv_sr_adva_check(adv, sr);
#else
	return (((_radio.advertiser.filter_policy & 0x01) == 0) ||
		(devmatch_ok)) &&
		isr_adv_sr_adva_check(adv, sr);
#endif /* CONFIG_BT_CTLR_PRIVACY */
}

static inline bool isr_adv_ci_tgta_check(struct pdu_adv *adv, struct pdu_adv *ci,
					 u8_t rl_idx)
{
#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (rl_idx != FILTER_IDX_NONE) {
		return rl_idx == _radio.advertiser.rl_idx;
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */
	return (adv->rx_addr == ci->tx_addr) &&
	       !memcmp(adv->direct_ind.tgt_addr, ci->connect_ind.init_addr,
		       BDADDR_SIZE);
}

static inline bool isr_adv_ci_adva_check(struct pdu_adv *adv,
					 struct pdu_adv *ci)
{
	return (adv->tx_addr == ci->rx_addr) &&
		(((adv->type == PDU_ADV_TYPE_DIRECT_IND) &&
		 !memcmp(adv->direct_ind.adv_addr, ci->connect_ind.adv_addr,
			 BDADDR_SIZE)) ||
		 (!memcmp(adv->adv_ind.addr, ci->connect_ind.adv_addr,
			  BDADDR_SIZE)));
}

static inline bool isr_adv_ci_check(struct pdu_adv *adv, struct pdu_adv *ci,
				    u8_t devmatch_ok, u8_t *rl_idx)
{
	/* LL 4.3.2: filter policy shall be ignored for directed adv */
	if (adv->type == PDU_ADV_TYPE_DIRECT_IND) {
#if defined(CONFIG_BT_CTLR_PRIVACY)
		return ctrl_rl_addr_allowed(ci->tx_addr,
					    ci->connect_ind.init_addr,
					    rl_idx) &&
#else
		return (1) &&
#endif
		       isr_adv_ci_adva_check(adv, ci) &&
		       isr_adv_ci_tgta_check(adv, ci, *rl_idx);
	}

#if defined(CONFIG_BT_CTLR_PRIVACY)
	return ((((_radio.advertiser.filter_policy & 0x02) == 0) &&
		 ctrl_rl_addr_allowed(ci->tx_addr, ci->connect_ind.init_addr,
				      rl_idx)) ||
		(((_radio.advertiser.filter_policy & 0x02) != 0) &&
		 (devmatch_ok || ctrl_irk_whitelisted(*rl_idx)))) &&
	       isr_adv_ci_adva_check(adv, ci);
#else
	return (((_radio.advertiser.filter_policy & 0x02) == 0) ||
		(devmatch_ok)) &&
	       isr_adv_ci_adva_check(adv, ci);
#endif /* CONFIG_BT_CTLR_PRIVACY */
}

static inline u32_t isr_rx_adv(u8_t devmatch_ok, u8_t devmatch_id,
			       u8_t irkmatch_ok, u8_t irkmatch_id,
			       u8_t rssi_ready)
{
	struct pdu_adv *pdu_adv, *_pdu_adv;
	struct radio_pdu_node_rx *node_rx;
#if defined(CONFIG_BT_CTLR_PRIVACY)
	/* An IRK match implies address resolution enabled */
	u8_t rl_idx = irkmatch_ok ? ctrl_rl_irk_idx(irkmatch_id) :
				    FILTER_IDX_NONE;
#else
	u8_t rl_idx = FILTER_IDX_NONE;
#endif /* CONFIG_BT_CTLR_PRIVACY */

	pdu_adv = (void *)radio_pkt_scratch_get();
	_pdu_adv = (void *)&_radio.advertiser.adv_data.data
					[_radio.advertiser.adv_data.first][0];

	if ((pdu_adv->type == PDU_ADV_TYPE_SCAN_REQ) &&
	    (pdu_adv->len == sizeof(struct pdu_adv_scan_req)) &&
	    isr_adv_sr_check(_pdu_adv, pdu_adv, devmatch_ok, &rl_idx)) {

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
		if (!IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT) ||
		    0 /* TODO: extended adv. scan req notification enabled */) {
			u32_t err;

			/* Generate the scan request event */
			err = isr_rx_adv_sr_report(pdu_adv, rssi_ready);
			if (err) {
				/* Scan Response will not be transmitted */
				return err;
			}
		}
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

		_radio.state = STATE_CLOSE;

		radio_switch_complete_and_disable();

		radio_pkt_tx_set(&_radio.advertiser.scan_data.data
					[_radio.advertiser.scan_data.first][0]);

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
	} else if ((pdu_adv->type == PDU_ADV_TYPE_CONNECT_IND) &&
		   (pdu_adv->len == sizeof(struct pdu_adv_connect_ind)) &&
		   isr_adv_ci_check(_pdu_adv, pdu_adv, devmatch_ok, &rl_idx) &&
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
		       &pdu_adv->connect_ind.crc_init[0],
		       3);
		memcpy(&conn->access_addr[0],
		       &pdu_adv->connect_ind.access_addr[0],
		       4);
		memcpy(&conn->data_chan_map[0],
		       &pdu_adv->connect_ind.chan_map[0],
		       sizeof(conn->data_chan_map));
		conn->data_chan_count =
			util_ones_count_get(&conn->data_chan_map[0],
					    sizeof(conn->data_chan_map));
		conn->data_chan_hop = pdu_adv->connect_ind.hop;
		conn->conn_interval =
			pdu_adv->connect_ind.interval;
		conn_interval_us =
			pdu_adv->connect_ind.interval * 1250;
		conn->latency = pdu_adv->connect_ind.latency;
		memcpy((void *)&conn->slave.force, &conn->access_addr[0],
		       sizeof(conn->slave.force));
		conn->supervision_reload =
			RADIO_CONN_EVENTS((pdu_adv->connect_ind.timeout
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
		radio_le_conn_cmplt->own_addr_type = pdu_adv->rx_addr;
		memcpy(&radio_le_conn_cmplt->own_addr[0],
		       &pdu_adv->connect_ind.adv_addr[0], BDADDR_SIZE);
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
			       &pdu_adv->connect_ind.init_addr[0],
			       BDADDR_SIZE);
		} else {
			memset(&radio_le_conn_cmplt->peer_rpa[0], 0x0,
			       BDADDR_SIZE);
#else
		if (1) {
#endif /* CONFIG_BT_CTLR_PRIVACY */
			radio_le_conn_cmplt->peer_addr_type = pdu_adv->tx_addr;
			memcpy(&radio_le_conn_cmplt->peer_addr[0],
			       &pdu_adv->connect_ind.init_addr[0],
			       BDADDR_SIZE);
		}

		radio_le_conn_cmplt->interval =
			pdu_adv->connect_ind.interval;
		radio_le_conn_cmplt->latency =
			pdu_adv->connect_ind.latency;
		radio_le_conn_cmplt->timeout =
			pdu_adv->connect_ind.timeout;
		radio_le_conn_cmplt->mca =
			pdu_adv->connect_ind.sca;

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

			if (pdu_adv->chan_sel) {
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
		conn->slave.sca = pdu_adv->connect_ind.sca;
		conn->slave.window_widening_periodic_us =
			(((gc_lookup_ppm[_radio.sca] +
			   gc_lookup_ppm[conn->slave.sca]) *
			  conn_interval_us) + (1000000 - 1)) / 1000000;
		conn->slave.window_widening_max_us =
			(conn_interval_us >> 1) - RADIO_TIFS;
		conn->slave.window_size_event_us =
			pdu_adv->connect_ind.win_size * 1250;
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
		ticks_slot_offset = max(conn->hdr.ticks_active_to_start,
					conn->hdr.ticks_xtal_to_start);
		conn_interval_us -=
			conn->slave.window_widening_periodic_us;

		conn_offset_us = radio_tmr_end_get();
		conn_offset_us +=
			((u64_t)pdu_adv->connect_ind.win_offset +
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
		if (_pdu_adv->type == PDU_ADV_TYPE_DIRECT_IND) {
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
	}

	return 1;
}

static u32_t isr_rx_scan_report(u8_t rssi_ready, u8_t rl_idx, bool dir_report)
{
	struct radio_pdu_node_rx *node_rx;
	struct pdu_adv *pdu_adv_rx;

	node_rx = packet_rx_reserve_get(3);
	if (node_rx == 0) {
		return 1;
	}

	/* Prepare the report (adv or scan resp) */
	node_rx->hdr.handle = 0xffff;
	if (0) {
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	} else if (_radio.scanner.phy) {
		switch (_radio.scanner.phy) {
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

	/* save the RSSI value */
	pdu_adv_rx = (void *)node_rx->pdu_data;
	((u8_t *)pdu_adv_rx)[offsetof(struct pdu_adv, payload) +
			     pdu_adv_rx->len] =
		(rssi_ready) ? (radio_rssi_get() & 0x7f) : 0x7f;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	/* save the resolving list index. */
	((u8_t *)pdu_adv_rx)[offsetof(struct pdu_adv, payload) +
			     pdu_adv_rx->len + 1] = rl_idx;
#endif /* CONFIG_BT_CTLR_PRIVACY */
#if defined(CONFIG_BT_CTLR_EXT_SCAN_FP)
	/* save the directed adv report flag */
	((u8_t *)pdu_adv_rx)[offsetof(struct pdu_adv, payload) +
			     pdu_adv_rx->len + 2] = dir_report ? 1 : 0;
#endif /* CONFIG_BT_CTLR_EXT_SCAN_FP */

	packet_rx_enqueue();

	return 0;
}

static inline bool isr_rx_scan_check(u8_t irkmatch_ok, u8_t devmatch_ok,
				     u8_t rl_idx)
{
#if defined(CONFIG_BT_CTLR_PRIVACY)
	return (((_radio.scanner.filter_policy & 0x01) == 0) &&
		 (!devmatch_ok || ctrl_rl_idx_allowed(irkmatch_ok, rl_idx))) ||
		(((_radio.scanner.filter_policy & 0x01) != 0) &&
		 (devmatch_ok || ctrl_irk_whitelisted(rl_idx)));
#else
	return ((_radio.scanner.filter_policy & 0x01) == 0) ||
		devmatch_ok;
#endif /* CONFIG_BT_CTLR_PRIVACY */
}

static inline bool isr_scan_rsp_adva_matches(struct pdu_adv *srsp)
{
	struct pdu_adv *sreq = (void *)radio_pkt_scratch_get();

	return ((sreq->rx_addr == srsp->tx_addr) &&
		(memcmp(&sreq->scan_req.adv_addr[0],
			&srsp->scan_rsp.addr[0], BDADDR_SIZE) == 0));
}

static inline bool isr_scan_init_adva_check(struct pdu_adv *pdu,
					    u8_t rl_idx)
{
#if defined(CONFIG_BT_CTLR_PRIVACY)
	/* Only applies to initiator with no whitelist */
	if (rl_idx != FILTER_IDX_NONE) {
		return (rl_idx == _radio.scanner.rl_idx);
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */
	return ((_radio.scanner.adv_addr_type == pdu->tx_addr) &&
		(memcmp(&_radio.scanner.adv_addr[0],
			&pdu->adv_ind.addr[0], BDADDR_SIZE) == 0));
}

static inline bool isr_scan_tgta_rpa_check(struct pdu_adv *pdu,
					   bool *dir_report)
{
	if (((_radio.scanner.filter_policy & 0x02) != 0) &&
	    (pdu->rx_addr != 0) &&
	    ((pdu->direct_ind.tgt_addr[5] & 0xc0) == 0x40)) {

		if (dir_report) {
			*dir_report = true;
		}

		return true;
	}

	return false;
}

static inline bool isr_scan_tgta_check(bool init, struct pdu_adv *pdu,
				       u8_t rl_idx, bool *dir_report)
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

	return (((_radio.scanner.init_addr_type == pdu->rx_addr) &&
		(memcmp(&_radio.scanner.init_addr[0],
			&pdu->direct_ind.tgt_addr[0],
			BDADDR_SIZE) == 0))) ||
		  /* allow directed adv packets where TargetA address
		   * is resolvable private address (scanner only)
		   */
	       isr_scan_tgta_rpa_check(pdu, dir_report);
}

static inline bool isr_scan_init_check(struct pdu_adv *pdu, u8_t rl_idx)
{
	return ((((_radio.scanner.filter_policy & 0x01) != 0) ||
		isr_scan_init_adva_check(pdu, rl_idx)) &&
		((pdu->type == PDU_ADV_TYPE_ADV_IND) ||
		((pdu->type == PDU_ADV_TYPE_DIRECT_IND) &&
		 (/* allow directed adv packets addressed to this device */
		  isr_scan_tgta_check(true, pdu, rl_idx, NULL)))));
}

static inline u32_t isr_rx_scan(u8_t devmatch_ok, u8_t devmatch_id,
				u8_t irkmatch_ok, u8_t irkmatch_id, u8_t rl_idx,
				u8_t rssi_ready)
{
	struct pdu_adv *pdu_adv_rx;
	/* Directed Adverising Report */
	bool dir_report = false;

	pdu_adv_rx = (void *)_radio.packet_rx[_radio.packet_rx_last]->pdu_data;

	/* Initiator */
	if ((_radio.scanner.conn) && ((_radio.fc_ena == 0) ||
				      (_radio.fc_req == _radio.fc_ack)) &&
	    isr_scan_init_check(pdu_adv_rx, rl_idx) &&
	    ((radio_tmr_end_get() + 502 + (RADIO_TICKER_JITTER_US << 1)) <
	     (HAL_TICKER_TICKS_TO_US(_radio.scanner.hdr.ticks_slot) -
	      RADIO_TICKER_START_PART_US))) {
		struct radio_le_conn_cmplt *radio_le_conn_cmplt;
		struct radio_pdu_node_rx *node_rx;
		struct pdu_adv *pdu_adv_tx;
		struct pdu_data *pdu_data;
		struct connection *conn;
		u32_t ticks_slot_offset;
		u32_t conn_interval_us;
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
		pdu_data = (void *)node_rx->pdu_data;
		radio_le_conn_cmplt = (void *)pdu_data->lldata;
		radio_le_conn_cmplt->status = 0x00;
		radio_le_conn_cmplt->role = 0x00;
#if defined(CONFIG_BT_CTLR_PRIVACY)
		radio_le_conn_cmplt->own_addr_type = pdu_adv_tx->tx_addr;
		memcpy(&radio_le_conn_cmplt->own_addr[0],
		       &pdu_adv_tx->connect_ind.init_addr[0],
		       BDADDR_SIZE);

		if (irkmatch_ok && rl_idx != FILTER_IDX_NONE) {
			/* TODO: store rl_idx instead if safe */
			/* Store identity address */
			ll_rl_id_addr_get(rl_idx,
					  &radio_le_conn_cmplt->peer_addr_type,
					  &radio_le_conn_cmplt->peer_addr[0]);
			/* Mark it as identity address from RPA (0x02, 0x03) */
			radio_le_conn_cmplt->peer_addr_type += 2;

			/* Store peer RPA */
			memcpy(&radio_le_conn_cmplt->peer_rpa[0],
			       &pdu_adv_tx->connect_ind.adv_addr[0],
			       BDADDR_SIZE);
		} else {
			memset(&radio_le_conn_cmplt->peer_rpa[0], 0x0,
			       BDADDR_SIZE);
#else
		if (1) {
#endif /* CONFIG_BT_CTLR_PRIVACY */
			radio_le_conn_cmplt->peer_addr_type =
				pdu_adv_tx->rx_addr;
			memcpy(&radio_le_conn_cmplt->peer_addr[0],
			       &pdu_adv_tx->connect_ind.adv_addr[0],
			       BDADDR_SIZE);
		}

		radio_le_conn_cmplt->interval = _radio.scanner.conn_interval;
		radio_le_conn_cmplt->latency = _radio.scanner.conn_latency;
		radio_le_conn_cmplt->timeout = _radio.scanner.conn_timeout;
		radio_le_conn_cmplt->mca =
			pdu_adv_tx->connect_ind.sca;

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

			if (pdu_adv_rx->chan_sel) {
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

		/* Calculate master slot */
		conn->hdr.ticks_active_to_start = _radio.ticks_active_to_start;
		conn->hdr.ticks_xtal_to_start =	HAL_TICKER_US_TO_TICKS(
			RADIO_TICKER_XTAL_OFFSET_US);
		conn->hdr.ticks_preempt_to_start = HAL_TICKER_US_TO_TICKS(
			RADIO_TICKER_PREEMPT_PART_MIN_US);
		conn->hdr.ticks_slot = _radio.scanner.ticks_conn_slot;
		ticks_slot_offset = max(conn->hdr.ticks_active_to_start,
					conn->hdr.ticks_xtal_to_start);

		/* Stop Scanner */
		ticker_status = ticker_stop(RADIO_TICKER_INSTANCE_ID_RADIO,
					    RADIO_TICKER_USER_ID_WORKER,
					    RADIO_TICKER_ID_SCAN,
					    ticker_stop_scan_assert,
					    (void *)__LINE__);
		ticker_stop_scan_assert(ticker_status, (void *)__LINE__);

		/* Scanner stop can expire while here in this ISR.
		 * Deferred attempt to stop can fail as it would have
		 * expired, hence ignore failure.
		 */
		ticker_stop(RADIO_TICKER_INSTANCE_ID_RADIO,
			    RADIO_TICKER_USER_ID_WORKER,
			    RADIO_TICKER_ID_SCAN_STOP, NULL, NULL);

		/* Start master */
		ticker_status =
			ticker_start(RADIO_TICKER_INSTANCE_ID_RADIO,
				     RADIO_TICKER_USER_ID_WORKER,
				     RADIO_TICKER_ID_FIRST_CONNECTION +
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
	}

	/* Active scanner */
	else if (((pdu_adv_rx->type == PDU_ADV_TYPE_ADV_IND) ||
		  (pdu_adv_rx->type == PDU_ADV_TYPE_SCAN_IND)) &&
		 (_radio.scanner.type != 0) &&
		 (_radio.scanner.conn == 0)) {
		struct pdu_adv *pdu_adv_tx;
#if defined(CONFIG_BT_CTLR_PRIVACY)
		bt_addr_t *lrpa;
#endif /* CONFIG_BT_CTLR_PRIVACY */
		u32_t err;

		/* save the adv packet */
		err = isr_rx_scan_report(rssi_ready,
					 irkmatch_ok ? rl_idx :
						       FILTER_IDX_NONE,
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
			pdu_adv_tx->tx_addr = _radio.scanner.init_addr_type;
			memcpy(&pdu_adv_tx->scan_req.scan_addr[0],
			       &_radio.scanner.init_addr[0], BDADDR_SIZE);
		}
		memcpy(&pdu_adv_tx->scan_req.adv_addr[0],
		       &pdu_adv_rx->adv_ind.addr[0], BDADDR_SIZE);

		/* switch scanner state to active */
		_radio.scanner.state = 1;
		_radio.state = STATE_TX;

		radio_tmr_tifs_set(RADIO_TIFS);
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
		    isr_scan_tgta_check(false, pdu_adv_rx, rl_idx,
					&dir_report))) ||
		  (pdu_adv_rx->type == PDU_ADV_TYPE_NONCONN_IND) ||
		  (pdu_adv_rx->type == PDU_ADV_TYPE_SCAN_IND) ||
#if defined(CONFIG_BT_CTLR_ADV_EXT)
		  ((pdu_adv_rx->type == PDU_ADV_TYPE_EXT_IND) &&
		   (_radio.scanner.phy)) ||
#endif /* CONFIG_BT_CTLR_ADV_EXT */
		  ((pdu_adv_rx->type == PDU_ADV_TYPE_SCAN_RSP) &&
		   (_radio.scanner.state != 0) &&
		   isr_scan_rsp_adva_matches(pdu_adv_rx))) &&
		 (pdu_adv_rx->len != 0) && (!_radio.scanner.conn)) {
		u32_t err;

		/* save the scan response packet */
		err = isr_rx_scan_report(rssi_ready,
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

#if defined(CONFIG_BT_CTLR_PHY)
static inline void isr_rx_conn_phy_tx_time_set(void)
{
	/* select the probable PHY with longest Tx time, which will be
	 * restricted to fit current connEffectiveMaxTxTime.
	 */
	u8_t phy_tx_time[8] = {BIT(0), BIT(0), BIT(1), BIT(0),
			       BIT(2), BIT(2), BIT(2), BIT(2)};
	struct connection *conn = _radio.conn_curr;
	u8_t phys = conn->llcp_phy.tx | conn->phy_tx;

	conn->phy_tx_time = phy_tx_time[phys];
}
#endif /* CONFIG_BT_CTLR_PHY */

static inline u8_t isr_rx_conn_pkt_ack(struct pdu_data *pdu_data_tx,
				       struct radio_pdu_node_tx **node_tx)
{
	u8_t terminate = 0;

	switch (pdu_data_tx->llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_TERMINATE_IND:
		_radio.state = STATE_CLOSE;
		radio_disable();

		/* assert if radio packet ptr is not set and radio started tx */
		LL_ASSERT(!radio_is_ready());

		terminate_ind_rx_enqueue(_radio.conn_curr,
		     (pdu_data_tx->llctrl.terminate_ind.error_code ==
		      BT_HCI_ERR_REMOTE_USER_TERM_CONN) ?
		     BT_HCI_ERR_LOCALHOST_TERM_CONN :
		     pdu_data_tx->llctrl.terminate_ind.error_code);

		/* Ack received, hence terminate */
		terminate = 1;
		break;

#if defined(CONFIG_BT_CTLR_LE_ENC)
	case PDU_DATA_LLCTRL_TYPE_ENC_REQ:
		/* things from master stored for session key calculation */
		memcpy(&_radio.conn_curr->llcp.encryption.skd[0],
		       &pdu_data_tx->llctrl.enc_req.skdm[0], 8);
		memcpy(&_radio.conn_curr->ccm_rx.iv[0],
		       &pdu_data_tx->llctrl.enc_req.ivm[0], 4);

		/* pause data packet tx */
		_radio.conn_curr->pause_tx = 1;

		/* Start Procedure Timeout (this will not replace terminate
		 * procedure which always gets place before any packets
		 * going out, hence safe by design).
		 */
		_radio.conn_curr->procedure_expire =
			_radio.conn_curr->procedure_reload;
		break;

	case PDU_DATA_LLCTRL_TYPE_ENC_RSP:
		/* pause data packet tx */
		_radio.conn_curr->pause_tx = 1;
		break;

	case PDU_DATA_LLCTRL_TYPE_START_ENC_REQ:
		/* Nothing to do.
		 * Remember that we may have received encrypted START_ENC_RSP
		 * alongwith this tx ack at this point in time.
		 */
		break;

	case PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_REQ:
		/* pause data packet tx */
		_radio.conn_curr->pause_tx = 1;

		/* key refresh */
		_radio.conn_curr->refresh = 1;

		/* Start Procedure Timeout (this will not replace terminate
		 * procedure which always gets place before any packets
		 * going out, hence safe by design).
		 */
		_radio.conn_curr->procedure_expire =
			_radio.conn_curr->procedure_reload;
		break;

	case PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP:
		if (_radio.role == ROLE_MASTER) {
			/* reused tx-ed PDU and send enc req */
			enc_req_reused_send(_radio.conn_curr, *node_tx);

			/* dont release ctrl PDU memory */
			*node_tx = NULL;
		} else {
			/* pause data packet tx */
			_radio.conn_curr->pause_tx = 1;
		}
		break;

	case PDU_DATA_LLCTRL_TYPE_REJECT_IND:
		/* resume data packet rx and tx */
		_radio.conn_curr->pause_rx = 0;
		_radio.conn_curr->pause_tx = 0;

		/* Procedure complete */
		_radio.conn_curr->procedure_expire = 0;
		break;
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PDU_DATA_LLCTRL_TYPE_LENGTH_REQ:
		if ((_radio.conn_curr->llcp_length.req !=
		     _radio.conn_curr->llcp_length.ack) &&
		    (_radio.conn_curr->llcp_length.state ==
		     LLCP_LENGTH_STATE_ACK_WAIT)){
			/* pause data packet tx */
			_radio.conn_curr->pause_tx = 1;

			/* wait for response */
			_radio.conn_curr->llcp_length.state =
				LLCP_LENGTH_STATE_RSP_WAIT;
		}
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
	case PDU_DATA_LLCTRL_TYPE_PHY_REQ:
		_radio.conn_curr->llcp_phy.state = LLCP_PHY_STATE_RSP_WAIT;
		/* fall through */

	case PDU_DATA_LLCTRL_TYPE_PHY_RSP:
		if (_radio.role == ROLE_SLAVE) {
			isr_rx_conn_phy_tx_time_set();
		}
		break;

	case PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND:
		_radio.conn_curr->phy_tx_time =
			_radio.conn_curr->llcp.phy_upd_ind.tx;
		break;
#endif /* CONFIG_BT_CTLR_PHY */

	default:
		/* Do nothing for other ctrl packet ack */
		break;
	}

	return terminate;
}

static inline struct radio_pdu_node_tx *
isr_rx_conn_pkt_release(struct radio_pdu_node_tx *node_tx)
{
	struct connection *conn = _radio.conn_curr;

	conn->packet_tx_head_len = 0;
	conn->packet_tx_head_offset = 0;

	/* release */
	if (conn->pkt_tx_head == conn->pkt_tx_ctrl) {
		if (node_tx) {
			conn->pkt_tx_head = conn->pkt_tx_head->next;
			if (conn->pkt_tx_ctrl == conn->pkt_tx_ctrl_last) {
				conn->pkt_tx_ctrl = NULL;
				conn->pkt_tx_ctrl_last = NULL;
			} else {
				conn->pkt_tx_ctrl = conn->pkt_tx_head;
			}

			mem_release(node_tx, &_radio.pkt_tx_ctrl_free);
		}
	} else {
		if (conn->pkt_tx_head == conn->pkt_tx_data) {
			conn->pkt_tx_data = conn->pkt_tx_data->next;
		}
		conn->pkt_tx_head = conn->pkt_tx_head->next;

		return node_tx;
	}

	return NULL;
}

static inline u32_t feat_get(u8_t *features)
{
	u32_t feat;

	feat = ~RADIO_BLE_FEAT_BIT_MASK_VALID | features[0] |
	       (features[1] << 8) | (features[2] << 16);
	feat &= RADIO_BLE_FEAT_BIT_MASK;

	return feat;
}

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
static inline void
isr_rx_conn_pkt_ctrl_rej_conn_upd(struct radio_pdu_node_rx *node_rx,
				  u8_t *rx_enqueue)
{
	struct pdu_data_llctrl_reject_ext_ind *rej_ext_ind;
	struct radio_le_conn_update_cmplt *cp;
	struct pdu_data *pdu_data_rx;
	struct connection *conn;

	pdu_data_rx = (void *)node_rx->pdu_data;
	rej_ext_ind = (void *)&pdu_data_rx->llctrl.reject_ext_ind;
	if (rej_ext_ind->reject_opcode != PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ) {
		return;
	}

	conn = _radio.conn_curr;

	/* Unsupported remote feature */
	if (!conn->role && (rej_ext_ind->error_code ==
			    BT_HCI_ERR_UNSUPP_REMOTE_FEATURE)) {
		LL_ASSERT(conn->llcp_req == conn->llcp_ack);

		conn->llcp_conn_param.state = LLCP_CPR_STATE_UPD;

		conn->llcp.conn_upd.win_size = 1;
		conn->llcp.conn_upd.win_offset_us = 0;
		conn->llcp.conn_upd.interval = conn->llcp_conn_param.interval;
		conn->llcp.conn_upd.latency = conn->llcp_conn_param.latency;
		conn->llcp.conn_upd.timeout = conn->llcp_conn_param.timeout;
		/* conn->llcp.conn_upd.instant     = 0; */
		conn->llcp.conn_upd.state = LLCP_CUI_STATE_USE;
		conn->llcp.conn_upd.is_internal = !conn->llcp_conn_param.cmd;
		conn->llcp_type = LLCP_CONN_UPD;
		conn->llcp_ack--;

		return;
	}
	/* Same Procedure or Different Procedure Collision */

	/* If not same procedure, stop procedure timeout, else
	 * continue timer until phy upd ind is received.
	 */
	else if (rej_ext_ind->error_code != BT_HCI_ERR_LL_PROC_COLLISION) {
		LL_ASSERT(_radio.conn_upd == conn);

		/* reset mutex */
		_radio.conn_upd = NULL;

		/* Procedure complete */
		conn->llcp_conn_param.ack =
			conn->llcp_conn_param.req;

		/* Stop procedure timeout */
		conn->procedure_expire = 0;

		/* update to next ticks offsets */
		if (conn->role) {
			conn->slave.ticks_to_offset =
			    conn->llcp_conn_param.ticks_to_offset_next;
		}
	}

	/* skip event generation if not cmd initiated */
	if (!conn->llcp_conn_param.cmd) {
		return;
	}

	/* generate conn update complete event with error code */
	node_rx->hdr.type = NODE_RX_TYPE_CONN_UPDATE;

	/* prepare connection update complete structure */
	pdu_data_rx = (void *)node_rx->pdu_data;
	cp = (void *)pdu_data_rx->lldata;
	cp->status = rej_ext_ind->error_code;
	cp->interval = conn->conn_interval;
	cp->latency = conn->latency;
	cp->timeout = conn->supervision_reload *
		      conn->conn_interval * 125 / 1000;

	*rx_enqueue = 1;
}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
static inline void
isr_rx_conn_pkt_ctrl_rej_dle(struct radio_pdu_node_rx *node_rx,
			     u8_t *rx_enqueue)
{
	struct pdu_data_llctrl_reject_ext_ind *rej_ext_ind;
	struct pdu_data *pdu_data_rx;

	pdu_data_rx = (void *)node_rx->pdu_data;
	rej_ext_ind = (void *)&pdu_data_rx->llctrl.reject_ext_ind;
	if (rej_ext_ind->reject_opcode == PDU_DATA_LLCTRL_TYPE_LENGTH_REQ) {
		struct connection *conn = _radio.conn_curr;
		struct pdu_data_llctrl_length_req *lr;

		/* Procedure complete */
		conn->llcp_length.ack = conn->llcp_length.req;
		conn->procedure_expire = 0;

		/* Resume data packet tx */
		conn->pause_tx = 0;

		/* prepare length rsp structure */
		pdu_data_rx->len = offsetof(struct pdu_data_llctrl,
					    length_rsp) +
				   sizeof(struct pdu_data_llctrl_length_rsp);
		pdu_data_rx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_LENGTH_RSP;

		lr = (void *)&pdu_data_rx->llctrl.length_req;
		lr->max_rx_octets = conn->max_rx_octets;
		lr->max_tx_octets = conn->max_tx_octets;
#if !defined(CONFIG_BT_CTLR_PHY)
		lr->max_rx_time = RADIO_PKT_TIME(conn->max_rx_octets, 0);
		lr->max_tx_time = RADIO_PKT_TIME(conn->max_tx_octets, 0);
#else /* CONFIG_BT_CTLR_PHY */
		lr->max_rx_time = conn->max_rx_time;
		lr->max_tx_time = conn->max_tx_time;
#endif /* CONFIG_BT_CTLR_PHY */

		/* enqueue a length rsp */
		*rx_enqueue = 1;
	}
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
static inline void
isr_rx_conn_pkt_ctrl_rej_phy_upd(struct radio_pdu_node_rx *node_rx,
				 u8_t *rx_enqueue)
{
	struct pdu_data_llctrl_reject_ext_ind *rej_ext_ind;
	struct pdu_data *pdu_data_rx;

	pdu_data_rx = (void *)node_rx->pdu_data;
	rej_ext_ind = (void *)&pdu_data_rx->llctrl.reject_ext_ind;
	if (rej_ext_ind->reject_opcode == PDU_DATA_LLCTRL_TYPE_PHY_REQ) {
		struct radio_le_phy_upd_cmplt *p;

		/* Same Procedure or Different Procedure Collision */

		/* If not same procedure, stop procedure timeout, else
		 * continue timer until phy upd ind is received.
		 */
		if (rej_ext_ind->error_code != BT_HCI_ERR_LL_PROC_COLLISION) {
			/* Procedure complete */
			_radio.conn_curr->llcp_phy.ack =
				_radio.conn_curr->llcp_phy.req;

			/* Reset packet timing restrictions */
			_radio.conn_curr->phy_tx_time =
				_radio.conn_curr->phy_tx;

			/* Stop procedure timeout */
			_radio.conn_curr->procedure_expire = 0;
		}

		/* skip event generation if not cmd initiated */
		if (!_radio.conn_curr->llcp_phy.cmd) {
			return;
		}

		/* generate phy update complete event with error code */
		node_rx->hdr.type = NODE_RX_TYPE_PHY_UPDATE;

		p = (void *)pdu_data_rx->lldata;
		p->status = rej_ext_ind->error_code;
		p->tx = _radio.conn_curr->phy_tx;
		p->rx = _radio.conn_curr->phy_rx;

		/* enqueue the phy update complete */
		*rx_enqueue = 1;
	}
}
#endif /* CONFIG_BT_CTLR_PHY */

static inline void
isr_rx_conn_pkt_ctrl_rej(struct radio_pdu_node_rx *node_rx, u8_t *rx_enqueue)
{
	if (0) {
#if defined(CONFIG_BT_CTLR_PHY)
	} else if (_radio.conn_curr->llcp_phy.ack !=
		   _radio.conn_curr->llcp_phy.req) {
		isr_rx_conn_pkt_ctrl_rej_phy_upd(node_rx, rx_enqueue);
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	} else if (_radio.conn_curr->llcp_conn_param.ack !=
		   _radio.conn_curr->llcp_conn_param.req) {
		isr_rx_conn_pkt_ctrl_rej_conn_upd(node_rx, rx_enqueue);
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	} else if (_radio.conn_curr->llcp_length.ack !=
		   _radio.conn_curr->llcp_length.req) {
		isr_rx_conn_pkt_ctrl_rej_dle(node_rx, rx_enqueue);
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_LE_ENC)
	} else {
		struct pdu_data_llctrl_reject_ext_ind *rej_ext_ind;
		struct pdu_data *pdu_rx;

		pdu_rx = (void *)node_rx->pdu_data;
		rej_ext_ind = (void *)&pdu_rx->llctrl.reject_ext_ind;

		switch (rej_ext_ind->reject_opcode) {
		case PDU_DATA_LLCTRL_TYPE_ENC_REQ:
			/* resume data packet rx and tx */
			_radio.conn_curr->pause_rx = 0;
			_radio.conn_curr->pause_tx = 0;

			/* Procedure complete */
			_radio.conn_curr->procedure_expire = 0;

			/* enqueue as if it were a reject ind */
			pdu_rx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_REJECT_IND;
			pdu_rx->llctrl.reject_ind.error_code =
				rej_ext_ind->error_code;
			*rx_enqueue = 1;
			break;

		default:
			/* Ignore */
			break;
		}
#endif /* CONFIG_BT_CTLR_LE_ENC */
	}
}

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
static inline u8_t isr_rx_conn_pkt_ctrl_dle(struct pdu_data *pdu_data_rx,
					    u8_t *rx_enqueue)
{
	struct radio_pdu_node_tx *node_tx = NULL;
	u16_t eff_rx_octets;
	u16_t eff_tx_octets;
#if defined(CONFIG_BT_CTLR_PHY)
	u16_t eff_rx_time;
	u16_t eff_tx_time;
#endif /* CONFIG_BT_CTLR_PHY */
	u8_t nack = 0;

	/* Check for free ctrl tx PDU */
	if (pdu_data_rx->llctrl.opcode == PDU_DATA_LLCTRL_TYPE_LENGTH_REQ) {
		node_tx = mem_acquire(&_radio.pkt_tx_ctrl_free);
		if (!node_tx) {
			return 1;
		}
	}

	eff_rx_octets = _radio.conn_curr->max_rx_octets;
	eff_tx_octets = _radio.conn_curr->max_tx_octets;

#if defined(CONFIG_BT_CTLR_PHY)
	eff_rx_time = _radio.conn_curr->max_rx_time;
	eff_tx_time = _radio.conn_curr->max_tx_time;
#endif /* CONFIG_BT_CTLR_PHY */

	if (/* Local idle, and Peer request then complete the Peer procedure
	     * with response.
	     */
	    ((_radio.conn_curr->llcp_length.req ==
	      _radio.conn_curr->llcp_length.ack) &&
	     (pdu_data_rx->llctrl.opcode ==
	      PDU_DATA_LLCTRL_TYPE_LENGTH_REQ)) ||
	    /* or Local has active... */
	    ((_radio.conn_curr->llcp_length.req !=
	      _radio.conn_curr->llcp_length.ack) &&
	     /* with Local requested and Peer request then complete the
	      * Peer procedure with response.
	      */
	     ((((_radio.conn_curr->llcp_length.state ==
		 LLCP_LENGTH_STATE_REQ) ||
		(_radio.conn_curr->llcp_length.state ==
		 LLCP_LENGTH_STATE_ACK_WAIT)) &&
	       (pdu_data_rx->llctrl.opcode ==
		PDU_DATA_LLCTRL_TYPE_LENGTH_REQ)) ||
	      /* with Local waiting for response, and Peer response then
	       * complete the Local procedure or Peer request then complete the
	       * Peer procedure with response.
	       */
	      ((_radio.conn_curr->llcp_length.state ==
		LLCP_LENGTH_STATE_RSP_WAIT) &&
	       ((pdu_data_rx->llctrl.opcode ==
		 PDU_DATA_LLCTRL_TYPE_LENGTH_RSP) ||
		(pdu_data_rx->llctrl.opcode ==
		 PDU_DATA_LLCTRL_TYPE_LENGTH_REQ)))))) {
		struct pdu_data_llctrl_length_req *lr;

		lr = &pdu_data_rx->llctrl.length_req;

		/* use the minimal of our default_tx_octets and
		 * peer max_rx_octets
		 */
		if (lr->max_rx_octets >= RADIO_LL_LENGTH_OCTETS_RX_MIN) {
			eff_tx_octets = min(lr->max_rx_octets,
					    _radio.conn_curr->default_tx_octets);
		}

		/* use the minimal of our max supported and
		 * peer max_tx_octets
		 */
		if (lr->max_tx_octets >= RADIO_LL_LENGTH_OCTETS_RX_MIN) {
			eff_rx_octets = min(lr->max_tx_octets,
					    RADIO_LL_LENGTH_OCTETS_RX_MAX);
		}

#if defined(CONFIG_BT_CTLR_PHY)
		/* use the minimal of our default_tx_time and
		 * peer max_rx_time
		 */
		if (lr->max_rx_time >=
		    RADIO_PKT_TIME(RADIO_LL_LENGTH_OCTETS_RX_MIN, 0)) {
			eff_tx_time = min(lr->max_rx_time,
					    _radio.conn_curr->default_tx_time);
		}

		/* use the minimal of our max supported and
		 * peer max_tx_time
		 */
		if (lr->max_tx_time >=
		    RADIO_PKT_TIME(RADIO_LL_LENGTH_OCTETS_RX_MIN, 0)) {
			eff_rx_time = min(lr->max_tx_time,
				RADIO_PKT_TIME(RADIO_LL_LENGTH_OCTETS_RX_MAX,
					       BIT(2)));
		}
#endif /* CONFIG_BT_CTLR_PHY */

		/* check if change in rx octets */
		if (eff_rx_octets != _radio.conn_curr->max_rx_octets) {
			u16_t free_count_rx;

			free_count_rx = packet_rx_acquired_count_get() +
				mem_free_count_get(_radio.pkt_rx_data_free);
			LL_ASSERT(free_count_rx <= 0xFF);

			if (_radio.packet_rx_data_count == free_count_rx) {

				/* accept the effective tx */
				_radio.conn_curr->max_tx_octets = eff_tx_octets;

				/* trigger or retain the ctrl procedure so as
				 * to resize the rx buffers.
				 */
				_radio.conn_curr->llcp_length.rx_octets =
					eff_rx_octets;
				_radio.conn_curr->llcp_length.tx_octets =
					eff_tx_octets;

#if defined(CONFIG_BT_CTLR_PHY)
				/* accept the effective tx time */
				_radio.conn_curr->max_tx_time = eff_tx_time;

				_radio.conn_curr->llcp_length.rx_time =
					eff_rx_time;
				_radio.conn_curr->llcp_length.tx_time =
					eff_tx_time;
#endif /* CONFIG_BT_CTLR_PHY */

				_radio.conn_curr->llcp_length.ack =
					(_radio.conn_curr->llcp_length.req - 1);
				_radio.conn_curr->llcp_length.state =
					LLCP_LENGTH_STATE_RESIZE;

				/* close the current connection event, so as
				 * to perform rx octet change.
				 */
				_radio.state = STATE_CLOSE;
			} else {
				nack = 1;
			}
		} else {
			/* Procedure complete */
			_radio.conn_curr->llcp_length.ack =
				_radio.conn_curr->llcp_length.req;
			_radio.conn_curr->procedure_expire = 0;

			/* resume data packet tx */
			_radio.conn_curr->pause_tx = 0;

			/* No change in effective octets or time */
			if (eff_tx_octets == _radio.conn_curr->max_tx_octets &&
#if defined(CONFIG_BT_CTLR_PHY)
			    eff_tx_time == _radio.conn_curr->max_tx_time &&
			    eff_rx_time == _radio.conn_curr->max_rx_time &&
#endif /* CONFIG_BT_CTLR_PHY */
			    (1)) {
				goto send_length_resp;
			}

			/* accept the effective tx */
			_radio.conn_curr->max_tx_octets = eff_tx_octets;

#if defined(CONFIG_BT_CTLR_PHY)
			/* accept the effective rx time */
			_radio.conn_curr->max_rx_time = eff_rx_time;

			/* accept the effective tx time */
			_radio.conn_curr->max_tx_time = eff_tx_time;
#endif /* CONFIG_BT_CTLR_PHY */

			/* prepare event params */
			lr->max_rx_octets = eff_rx_octets;
			lr->max_tx_octets = eff_tx_octets;

#if !defined(CONFIG_BT_CTLR_PHY)
			lr->max_rx_time = RADIO_PKT_TIME(eff_rx_octets, 0);
			lr->max_tx_time = RADIO_PKT_TIME(eff_tx_octets, 0);
#else /* CONFIG_BT_CTLR_PHY */
			lr->max_rx_time = eff_rx_time;
			lr->max_tx_time = eff_tx_time;
#endif /* CONFIG_BT_CTLR_PHY */

			/* Enqueue data length change event (with no change in
			 * rx length happened), safe to enqueue rx.
			 */
			*rx_enqueue = 1;
		}
	} else {
		/* Drop response with no Local initiated request. */
		LL_ASSERT(pdu_data_rx->llctrl.opcode ==
			  PDU_DATA_LLCTRL_TYPE_LENGTH_RSP);
	}

send_length_resp:
	if (node_tx) {
		if (nack) {
			mem_release(node_tx, &_radio.pkt_tx_ctrl_free);
		} else {
#if !defined(CONFIG_BT_CTLR_PHY)
			length_resp_send(_radio.conn_curr, node_tx,
					 eff_rx_octets, eff_tx_octets);
#else /* CONFIG_BT_CTLR_PHY */
			length_resp_send(_radio.conn_curr, node_tx,
					 eff_rx_octets, eff_rx_time,
					 eff_tx_octets, eff_tx_time);
#endif /* CONFIG_BT_CTLR_PHY */
		}
	}

	return nack;
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

static inline bool pdu_len_cmp(u8_t opcode, u8_t len)
{
	const u8_t ctrl_len_lut[] = {
		(offsetof(struct pdu_data_llctrl, conn_update_ind) +
		 sizeof(struct pdu_data_llctrl_conn_update_ind)),
		(offsetof(struct pdu_data_llctrl, chan_map_ind) +
		 sizeof(struct pdu_data_llctrl_chan_map_ind)),
		(offsetof(struct pdu_data_llctrl, terminate_ind) +
		 sizeof(struct pdu_data_llctrl_terminate_ind)),
		(offsetof(struct pdu_data_llctrl, enc_req) +
		 sizeof(struct pdu_data_llctrl_enc_req)),
		(offsetof(struct pdu_data_llctrl, enc_rsp) +
		 sizeof(struct pdu_data_llctrl_enc_rsp)),
		(offsetof(struct pdu_data_llctrl, start_enc_req) +
		 sizeof(struct pdu_data_llctrl_start_enc_req)),
		(offsetof(struct pdu_data_llctrl, start_enc_rsp) +
		 sizeof(struct pdu_data_llctrl_start_enc_rsp)),
		(offsetof(struct pdu_data_llctrl, unknown_rsp) +
		 sizeof(struct pdu_data_llctrl_unknown_rsp)),
		(offsetof(struct pdu_data_llctrl, feature_req) +
		 sizeof(struct pdu_data_llctrl_feature_req)),
		(offsetof(struct pdu_data_llctrl, feature_rsp) +
		 sizeof(struct pdu_data_llctrl_feature_rsp)),
		(offsetof(struct pdu_data_llctrl, pause_enc_req) +
		 sizeof(struct pdu_data_llctrl_pause_enc_req)),
		(offsetof(struct pdu_data_llctrl, pause_enc_rsp) +
		 sizeof(struct pdu_data_llctrl_pause_enc_rsp)),
		(offsetof(struct pdu_data_llctrl, version_ind) +
		 sizeof(struct pdu_data_llctrl_version_ind)),
		(offsetof(struct pdu_data_llctrl, reject_ind) +
		 sizeof(struct pdu_data_llctrl_reject_ind)),
		(offsetof(struct pdu_data_llctrl, slave_feature_req) +
		 sizeof(struct pdu_data_llctrl_slave_feature_req)),
		(offsetof(struct pdu_data_llctrl, conn_param_req) +
		 sizeof(struct pdu_data_llctrl_conn_param_req)),
		(offsetof(struct pdu_data_llctrl, conn_param_rsp) +
		 sizeof(struct pdu_data_llctrl_conn_param_rsp)),
		(offsetof(struct pdu_data_llctrl, reject_ext_ind) +
		 sizeof(struct pdu_data_llctrl_reject_ext_ind)),
		(offsetof(struct pdu_data_llctrl, ping_req) +
		 sizeof(struct pdu_data_llctrl_ping_req)),
		(offsetof(struct pdu_data_llctrl, ping_rsp) +
		 sizeof(struct pdu_data_llctrl_ping_rsp)),
		(offsetof(struct pdu_data_llctrl, length_req) +
		 sizeof(struct pdu_data_llctrl_length_req)),
		(offsetof(struct pdu_data_llctrl, length_rsp) +
		 sizeof(struct pdu_data_llctrl_length_rsp)),
		(offsetof(struct pdu_data_llctrl, phy_req) +
		 sizeof(struct pdu_data_llctrl_phy_req)),
		(offsetof(struct pdu_data_llctrl, phy_rsp) +
		 sizeof(struct pdu_data_llctrl_phy_rsp)),
		(offsetof(struct pdu_data_llctrl, phy_upd_ind) +
		 sizeof(struct pdu_data_llctrl_phy_upd_ind)),
		(offsetof(struct pdu_data_llctrl, min_used_chans_ind) +
		 sizeof(struct pdu_data_llctrl_min_used_chans_ind)),
	};

	return ctrl_len_lut[opcode] == len;
}

static inline u8_t
isr_rx_conn_pkt_ctrl(struct radio_pdu_node_rx *node_rx, u8_t *rx_enqueue)
{
	struct pdu_data *pdu_data_rx;
	u8_t nack = 0;

	pdu_data_rx = (void *)node_rx->pdu_data;
	switch (pdu_data_rx->llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND:
	{
		u8_t err;

		if (!_radio.conn_curr->role ||
		    !pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND,
				 pdu_data_rx->len)) {
			goto isr_rx_conn_unknown_rsp_send;
		}

		err = conn_update(_radio.conn_curr, pdu_data_rx);
		if (err) {
			_radio.conn_curr->llcp_terminate.reason_peer = err;
		} else {
			/* conn param req procedure, if any, is complete */
			_radio.conn_curr->procedure_expire = 0;
		}
	}
	break;

	case PDU_DATA_LLCTRL_TYPE_CHAN_MAP_IND:
		if (!_radio.conn_curr->role ||
		    !pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_CHAN_MAP_IND,
				 pdu_data_rx->len)) {
			goto isr_rx_conn_unknown_rsp_send;
		}

		if (chan_map_update(_radio.conn_curr, pdu_data_rx)) {
			_radio.conn_curr->llcp_terminate.reason_peer =
				BT_HCI_ERR_INSTANT_PASSED;
		}
		break;

	case PDU_DATA_LLCTRL_TYPE_TERMINATE_IND:
		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_TERMINATE_IND,
				 pdu_data_rx->len)) {
			goto isr_rx_conn_unknown_rsp_send;
		}

		/* Ack and then terminate */
		_radio.conn_curr->llcp_terminate.reason_peer =
			pdu_data_rx->llctrl.terminate_ind.error_code;
		break;

#if defined(CONFIG_BT_CTLR_LE_ENC)
	case PDU_DATA_LLCTRL_TYPE_ENC_REQ:
		if (!_radio.conn_curr->role ||
		    !pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_ENC_REQ,
				 pdu_data_rx->len)) {
			goto isr_rx_conn_unknown_rsp_send;
		}

#if defined(CONFIG_BT_CTLR_FAST_ENC)
		/* TODO: BT Spec. text: may finalize the sending of additional
		 * data channel PDUs queued in the controller.
		 */
		nack = enc_rsp_send(_radio.conn_curr);
		if (nack) {
			break;
		}
#endif /* CONFIG_BT_CTLR_FAST_ENC */

		/* things from master stored for session key calculation */
		memcpy(&_radio.conn_curr->llcp.encryption.skd[0],
		       &pdu_data_rx->llctrl.enc_req.skdm[0], 8);
		memcpy(&_radio.conn_curr->ccm_rx.iv[0],
		       &pdu_data_rx->llctrl.enc_req.ivm[0], 4);

		/* pause rx data packets */
		_radio.conn_curr->pause_rx = 1;

		/* Start Procedure Timeout (TODO: this shall not replace
		 * terminate procedure).
		 */
		_radio.conn_curr->procedure_expire =
			_radio.conn_curr->procedure_reload;

		/* enqueue the enc req */
		*rx_enqueue = 1;
		break;

	case PDU_DATA_LLCTRL_TYPE_ENC_RSP:
		if (_radio.conn_curr->role ||
		    !pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_ENC_RSP,
				 pdu_data_rx->len)) {
			goto isr_rx_conn_unknown_rsp_send;
		}

		/* things sent by slave stored for session key calculation */
		memcpy(&_radio.conn_curr->llcp.encryption.skd[8],
		       &pdu_data_rx->llctrl.enc_rsp.skds[0], 8);
		memcpy(&_radio.conn_curr->ccm_rx.iv[4],
		       &pdu_data_rx->llctrl.enc_rsp.ivs[0], 4);

		/* pause rx data packets */
		_radio.conn_curr->pause_rx = 1;
		break;

	case PDU_DATA_LLCTRL_TYPE_START_ENC_REQ:
		LL_ASSERT((_radio.conn_curr->llcp_req ==
			   _radio.conn_curr->llcp_ack) ||
			  (_radio.conn_curr->llcp_type == LLCP_ENCRYPTION));

		if (_radio.conn_curr->role ||
		    !pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_START_ENC_REQ,
				 pdu_data_rx->len)) {
			goto isr_rx_conn_unknown_rsp_send;
		}

		/* start enc rsp to be scheduled in master prepare */
		_radio.conn_curr->llcp.encryption.initiate = 0;
		_radio.conn_curr->llcp_type = LLCP_ENCRYPTION;
		_radio.conn_curr->llcp_ack--;
		break;

	case PDU_DATA_LLCTRL_TYPE_START_ENC_RSP:
		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_START_ENC_RSP,
				 pdu_data_rx->len)) {
			goto isr_rx_conn_unknown_rsp_send;
		}

		if (_radio.role == ROLE_SLAVE) {
#if !defined(CONFIG_BT_CTLR_FAST_ENC)
			LL_ASSERT((_radio.conn_curr->llcp_req ==
				   _radio.conn_curr->llcp_ack) ||
				  (_radio.conn_curr->llcp_type ==
				   LLCP_ENCRYPTION));

			/* start enc rsp to be scheduled in slave  prepare */
			_radio.conn_curr->llcp.encryption.initiate = 0;
			_radio.conn_curr->llcp_type = LLCP_ENCRYPTION;
			_radio.conn_curr->llcp_ack--;
#else /* CONFIG_BT_CTLR_FAST_ENC */
			nack = start_enc_rsp_send(_radio.conn_curr, NULL);
			if (nack) {
				break;
			}

			/* resume data packet rx and tx */
			_radio.conn_curr->pause_rx = 0;
			_radio.conn_curr->pause_tx = 0;
#endif /* CONFIG_BT_CTLR_FAST_ENC */

		} else {
			/* resume data packet rx and tx */
			_radio.conn_curr->pause_rx = 0;
			_radio.conn_curr->pause_tx = 0;
		}

		/* enqueue the start enc resp (encryption change/refresh) */
		if (_radio.conn_curr->refresh) {
			_radio.conn_curr->refresh = 0;

			/* key refresh event */
			node_rx->hdr.type = NODE_RX_TYPE_ENC_REFRESH;
		}
		*rx_enqueue = 1;

		/* Procedure complete */
		_radio.conn_curr->procedure_expire = 0;
		break;
#endif /* CONFIG_BT_CTLR_LE_ENC */

	case PDU_DATA_LLCTRL_TYPE_FEATURE_REQ:
		if (!_radio.conn_curr->role ||
		    !pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_FEATURE_REQ,
				 pdu_data_rx->len)) {
			goto isr_rx_conn_unknown_rsp_send;
		}

		nack = feature_rsp_send(_radio.conn_curr, pdu_data_rx);
		break;

	case PDU_DATA_LLCTRL_TYPE_SLAVE_FEATURE_REQ:
		if (_radio.conn_curr->role ||
		    !pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_SLAVE_FEATURE_REQ,
				 pdu_data_rx->len)) {
			goto isr_rx_conn_unknown_rsp_send;
		}

		nack = feature_rsp_send(_radio.conn_curr, pdu_data_rx);
		break;

	case PDU_DATA_LLCTRL_TYPE_FEATURE_RSP:
	{
		struct pdu_data_llctrl_feature_rsp *rsp;

		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_FEATURE_RSP,
				 pdu_data_rx->len)) {
			goto isr_rx_conn_unknown_rsp_send;
		}

		rsp = &pdu_data_rx->llctrl.feature_rsp;

		/* AND the feature set to get Feature USED */
		_radio.conn_curr->llcp_features &= feat_get(&rsp->features[0]);

		/* features exchanged */
		_radio.conn_curr->common.fex_valid = 1;

		/* enqueue the feature resp */
		*rx_enqueue = 1;

		/* Procedure complete */
		_radio.conn_curr->procedure_expire = 0;
	}
	break;

#if defined(CONFIG_BT_CTLR_LE_ENC)
	case PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_REQ:
		if (!_radio.conn_curr->role ||
		    !pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_REQ,
				 pdu_data_rx->len)) {
			goto isr_rx_conn_unknown_rsp_send;
		}

		nack = pause_enc_rsp_send(_radio.conn_curr, 1);
		break;

	case PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP:
		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP,
				 pdu_data_rx->len)) {
			goto isr_rx_conn_unknown_rsp_send;
		}

		nack = pause_enc_rsp_send(_radio.conn_curr, 0);
		break;
#endif /* CONFIG_BT_CTLR_LE_ENC */

	case PDU_DATA_LLCTRL_TYPE_VERSION_IND:
		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_VERSION_IND,
				 pdu_data_rx->len)) {
			goto isr_rx_conn_unknown_rsp_send;
		}

		nack = version_ind_send(_radio.conn_curr, pdu_data_rx,
					rx_enqueue);
		break;

#if defined(CONFIG_BT_CTLR_LE_ENC)
	case PDU_DATA_LLCTRL_TYPE_REJECT_IND:
		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_REJECT_IND,
				 pdu_data_rx->len)) {
			goto isr_rx_conn_unknown_rsp_send;
		}

		/* resume data packet rx and tx */
		_radio.conn_curr->pause_rx = 0;
		_radio.conn_curr->pause_tx = 0;

		/* Procedure complete */
		_radio.conn_curr->procedure_expire = 0;

		/* enqueue the reject ind */
		*rx_enqueue = 1;
		break;
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	case PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ:
		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
				 pdu_data_rx->len)) {
			goto isr_rx_conn_unknown_rsp_send;
		}


		/* check CUI/CPR mutex for other connections having CPR in
		 * progress.
		 */
		if (_radio.conn_upd && (_radio.conn_upd != _radio.conn_curr)) {
			/* Unsupported LL Parameter Value */
			nack = reject_ext_ind_send(_radio.conn_curr,
					PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
					BT_HCI_ERR_UNSUPP_LL_PARAM_VAL);
			break;
		}

		if (!_radio.conn_curr->role) {
			struct connection *conn = _radio.conn_curr;

			if ((conn->llcp_conn_param.req !=
					conn->llcp_conn_param.ack) &&
			    ((conn->llcp_conn_param.state ==
			      LLCP_CPR_STATE_REQ) ||
			     (conn->llcp_conn_param.state ==
			      LLCP_CPR_STATE_RSP_WAIT) ||
			     (conn->llcp_conn_param.state ==
			      LLCP_CPR_STATE_UPD))) {
				/* Same procedure collision  */
				nack = reject_ext_ind_send(_radio.conn_curr,
					PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
					BT_HCI_ERR_LL_PROC_COLLISION);
#if defined(CONFIG_BT_CTLR_PHY)
#if defined(CONFIG_BT_CTLR_LE_ENC)
			} else if (((conn->llcp_req != conn->llcp_ack) &&
				    (conn->llcp_type != LLCP_ENCRYPTION)) ||
				   (conn->llcp_phy.req != conn->llcp_phy.ack)) {
#else /* !CONFIG_BT_CTLR_LE_ENC */
			} else if ((conn->llcp_req != conn->llcp_ack) ||
				   (conn->llcp_phy.req != conn->llcp_phy.ack)) {
#endif /* !CONFIG_BT_CTLR_LE_ENC */
#else /* !CONFIG_BT_CTLR_PHY */
#if defined(CONFIG_BT_CTLR_LE_ENC)
			} else if ((conn->llcp_req != conn->llcp_ack) &&
				   (conn->llcp_type != LLCP_ENCRYPTION)) {
#else /* !CONFIG_BT_CTLR_LE_ENC */
			} else if (conn->llcp_req != conn->llcp_ack) {
#endif /* !CONFIG_BT_CTLR_LE_ENC */
#endif /* !CONFIG_BT_CTLR_PHY */
				/* Different procedure collision */
				nack = reject_ext_ind_send(_radio.conn_curr,
					PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
					BT_HCI_ERR_DIFF_TRANS_COLLISION);
			} else {
				struct pdu_data_llctrl_conn_param_req *cpr = (void *)
					&pdu_data_rx->llctrl.conn_param_req;

				/* Invalid parameters */
				if ((cpr->interval_min < 6) ||
				    (cpr->interval_max > 3200) ||
				    (cpr->interval_min > cpr->interval_max) ||
				    (cpr->latency > 499) ||
				    (cpr->timeout < 10) ||
				    (cpr->timeout > 3200) ||
				    ((cpr->timeout * 4) <=
				     ((cpr->latency + 1) *
				      cpr->interval_max)) ||
				    (cpr->preferred_periodicity >
				     cpr->interval_max)) {
					nack = reject_ext_ind_send(conn,
						PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
						BT_HCI_ERR_INVALID_LL_PARAM);
					break;
				}

				/* save parameters to be used to select offset
				 */
				conn->llcp_conn_param.interval = cpr->interval_min;
				conn->llcp_conn_param.latency =	cpr->latency;
				conn->llcp_conn_param.timeout =	cpr->timeout;
				conn->llcp_conn_param.preferred_periodicity =
					cpr->preferred_periodicity;
				conn->llcp_conn_param.reference_conn_event_count =
					cpr->reference_conn_event_count;
				conn->llcp_conn_param.offset0 =	cpr->offset0;
				conn->llcp_conn_param.offset1 =	cpr->offset1;
				conn->llcp_conn_param.offset2 =	cpr->offset2;
				conn->llcp_conn_param.offset3 =	cpr->offset3;
				conn->llcp_conn_param.offset4 =	cpr->offset4;
				conn->llcp_conn_param.offset5 =	cpr->offset5;

				/* enqueue the conn param req, if parameters
				 * changed, else respond.
				 */
				if ((conn->llcp_conn_param.interval !=
				     conn->conn_interval) ||
				    (conn->llcp_conn_param.latency !=
				     conn->latency) ||
				    (RADIO_CONN_EVENTS(conn->llcp_conn_param.timeout *
						       10000,
						       conn->conn_interval *
						       1250) !=
				     conn->supervision_reload)) {
					/* postpone CP request event if under
					 * encryption setup
					 */
					if (conn->pause_tx) {
						conn->llcp_conn_param.state =
							LLCP_CPR_STATE_APP_REQ;
					} else {
						*rx_enqueue = 1;

						conn->llcp_conn_param.state =
							LLCP_CPR_STATE_APP_WAIT;
					}
				} else {
					conn->llcp_conn_param.status = 0;
					conn->llcp_conn_param.cmd = 0;
					conn->llcp_conn_param.state =
						LLCP_CPR_STATE_RSP;
				}

				conn->llcp_conn_param.ack--;

				/* set mutex */
				if (_radio.conn_upd == 0) {
					_radio.conn_upd = conn;
				}
			}
		} else if ((_radio.conn_curr->llcp_conn_param.req ==
			    _radio.conn_curr->llcp_conn_param.ack) ||
			   (_radio.conn_curr->llcp_conn_param.state ==
			    LLCP_CPR_STATE_REQ) ||
			   (_radio.conn_curr->llcp_conn_param.state ==
			    LLCP_CPR_STATE_RSP_WAIT)) {
			struct connection *conn = _radio.conn_curr;
			struct pdu_data_llctrl_conn_param_req *cpr = (void *)
				&pdu_data_rx->llctrl.conn_param_req;

			/* Invalid parameters */
			if ((cpr->interval_min < 6) ||
			    (cpr->interval_max > 3200) ||
			    (cpr->interval_min > cpr->interval_max) ||
			    (cpr->latency > 499) ||
			    (cpr->timeout < 10) || (cpr->timeout > 3200) ||
			    ((cpr->timeout * 4) <= ((cpr->latency + 1) *
						    cpr->interval_max)) ||
			    (cpr->preferred_periodicity > cpr->interval_max)) {
				nack = reject_ext_ind_send(conn,
					PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
					BT_HCI_ERR_INVALID_LL_PARAM);
				break;
			}

			/* resp to be generated by app, for now save
			 * parameters
			 */
			conn->llcp_conn_param.interval = cpr->interval_min;
			conn->llcp_conn_param.latency =	cpr->latency;
			conn->llcp_conn_param.timeout =	cpr->timeout;
			conn->llcp_conn_param.preferred_periodicity =
				cpr->preferred_periodicity;
			conn->llcp_conn_param.reference_conn_event_count =
				cpr->reference_conn_event_count;
			conn->llcp_conn_param.offset0 =	cpr->offset0;
			conn->llcp_conn_param.offset1 =	cpr->offset1;
			conn->llcp_conn_param.offset2 =	cpr->offset2;
			conn->llcp_conn_param.offset3 =	cpr->offset3;
			conn->llcp_conn_param.offset4 =	cpr->offset4;
			conn->llcp_conn_param.offset5 =	cpr->offset5;

			/* enqueue the conn param req, if parameters changed,
			 * else respond
			 */
			if ((conn->llcp_conn_param.interval !=
			     conn->conn_interval) ||
			    (conn->llcp_conn_param.latency != conn->latency) ||
			    (RADIO_CONN_EVENTS(conn->llcp_conn_param.timeout *
					       10000,
					       conn->conn_interval *
					       1250) !=
			     conn->supervision_reload)) {
				*rx_enqueue = 1;

				conn->llcp_conn_param.state =
					LLCP_CPR_STATE_APP_WAIT;
			} else {
				conn->llcp_conn_param.status = 0;
				conn->llcp_conn_param.cmd = 0;
				conn->llcp_conn_param.state =
					LLCP_CPR_STATE_RSP;
			}

			conn->llcp_conn_param.ack--;

			/* set mutex */
			if (_radio.conn_upd == 0) {
				_radio.conn_upd = conn;
			}
		} else {
			LL_ASSERT(0);
		}
		break;

	case PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP:
		if (_radio.conn_curr->role ||
		    !pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP,
				 pdu_data_rx->len)) {
			goto isr_rx_conn_unknown_rsp_send;
		}

		if (!_radio.conn_curr->role &&
		    (_radio.conn_curr->llcp_conn_param.req !=
		     _radio.conn_curr->llcp_conn_param.ack) &&
		    (_radio.conn_curr->llcp_conn_param.state ==
		     LLCP_CPR_STATE_RSP_WAIT)) {
			struct connection *conn = _radio.conn_curr;
			struct pdu_data_llctrl_conn_param_req *cpr = (void *)
				&pdu_data_rx->llctrl.conn_param_req;

			/* Invalid parameters */
			if ((cpr->interval_min < 6) ||
			    (cpr->interval_max > 3200) ||
			    (cpr->interval_min > cpr->interval_max) ||
			    (cpr->latency > 499) ||
			    (cpr->timeout < 10) || (cpr->timeout > 3200) ||
			    ((cpr->timeout * 4) <= ((cpr->latency + 1) *
						    cpr->interval_max)) ||
			    (cpr->preferred_periodicity > cpr->interval_max)) {
				nack = reject_ext_ind_send(conn,
					PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP,
					BT_HCI_ERR_INVALID_LL_PARAM);
				break;
			}

			/* Stop procedure timeout */
			_radio.conn_curr->procedure_expire = 0;

			/* save parameters to be used to select offset
			 */
			conn->llcp_conn_param.interval = cpr->interval_min;
			conn->llcp_conn_param.latency =	cpr->latency;
			conn->llcp_conn_param.timeout =	cpr->timeout;
			conn->llcp_conn_param.preferred_periodicity =
				cpr->preferred_periodicity;
			conn->llcp_conn_param.reference_conn_event_count =
				cpr->reference_conn_event_count;
			conn->llcp_conn_param.offset0 =	cpr->offset0;
			conn->llcp_conn_param.offset1 =	cpr->offset1;
			conn->llcp_conn_param.offset2 =	cpr->offset2;
			conn->llcp_conn_param.offset3 =	cpr->offset3;
			conn->llcp_conn_param.offset4 =	cpr->offset4;
			conn->llcp_conn_param.offset5 =	cpr->offset5;

			/* Perform connection update */
			conn->llcp_conn_param.state = LLCP_CPR_STATE_RSP;
		}
		break;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

	case PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND:
		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND,
				 pdu_data_rx->len)) {
			goto isr_rx_conn_unknown_rsp_send;
		}

		isr_rx_conn_pkt_ctrl_rej(node_rx, rx_enqueue);
		break;

#if defined(CONFIG_BT_CTLR_LE_PING)
	case PDU_DATA_LLCTRL_TYPE_PING_REQ:
		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_PING_REQ,
				 pdu_data_rx->len)) {
			goto isr_rx_conn_unknown_rsp_send;
		}

		nack = ping_resp_send(_radio.conn_curr);
		break;

	case PDU_DATA_LLCTRL_TYPE_PING_RSP:
		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_PING_RSP,
				 pdu_data_rx->len)) {
			goto isr_rx_conn_unknown_rsp_send;
		}

		/* Procedure complete */
		_radio.conn_curr->procedure_expire = 0;
		break;
#endif /* CONFIG_BT_CTLR_LE_PING */

	case PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP:
		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP,
				 pdu_data_rx->len)) {
			goto isr_rx_conn_unknown_rsp_send;
		}

		if (0) {
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		} else if (_radio.conn_curr->llcp_conn_param.ack !=
			   _radio.conn_curr->llcp_conn_param.req) {
			struct connection *conn = _radio.conn_curr;
			struct radio_le_conn_update_cmplt *cp;

			/* Mark CPR as unsupported */
			conn->llcp_conn_param.disabled = 1;

			/* TODO: check for unsupported remote feature reason */
			if (!conn->role) {
				LL_ASSERT(conn->llcp_req == conn->llcp_ack);

				conn->llcp_conn_param.state =
					LLCP_CPR_STATE_UPD;

				conn->llcp.conn_upd.win_size = 1;
				conn->llcp.conn_upd.win_offset_us = 0;
				conn->llcp.conn_upd.interval =
					conn->llcp_conn_param.interval;
				conn->llcp.conn_upd.latency =
					conn->llcp_conn_param.latency;
				conn->llcp.conn_upd.timeout =
					conn->llcp_conn_param.timeout;
				/* conn->llcp.conn_upd.instant     = 0; */
				conn->llcp.conn_upd.state = LLCP_CUI_STATE_USE;
				conn->llcp.conn_upd.is_internal =
					!conn->llcp_conn_param.cmd;
				conn->llcp_type = LLCP_CONN_UPD;
				conn->llcp_ack--;

				break;
			}

			LL_ASSERT(_radio.conn_upd == conn);

			/* reset mutex */
			_radio.conn_upd = NULL;

			/* Procedure complete */
			conn->llcp_conn_param.ack = conn->llcp_conn_param.req;

			/* skip event generation if not cmd initiated */
			if (!conn->llcp_conn_param.cmd) {
				break;
			}

			/* generate conn upd complete event with error code */
			node_rx->hdr.type = NODE_RX_TYPE_CONN_UPDATE;

			/* prepare connection update complete structure */
			pdu_data_rx = (void *)node_rx->pdu_data;
			cp = (void *)pdu_data_rx->lldata;
			cp->status = BT_HCI_ERR_UNSUPP_REMOTE_FEATURE;
			cp->interval = conn->conn_interval;
			cp->latency = conn->latency;
			cp->timeout = conn->supervision_reload *
				      conn->conn_interval * 125 / 1000;

			*rx_enqueue = 1;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
		} else if (_radio.conn_curr->llcp_length.req !=
			   _radio.conn_curr->llcp_length.ack) {
			/* Procedure complete */
			_radio.conn_curr->llcp_length.ack =
				_radio.conn_curr->llcp_length.req;

			/* resume data packet tx */
			_radio.conn_curr->pause_tx = 0;

			/* propagate the data length procedure to
			 * host
			 */
			*rx_enqueue = 1;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
		} else if (_radio.conn_curr->llcp_phy.req !=
			   _radio.conn_curr->llcp_phy.ack) {
			struct radio_le_phy_upd_cmplt *p;

			/* Procedure complete */
			_radio.conn_curr->llcp_phy.ack =
				_radio.conn_curr->llcp_phy.req;

			/* Reset packet timing restrictions */
			_radio.conn_curr->phy_tx_time =
				_radio.conn_curr->phy_tx;

			/* skip event generation is not cmd initiated */
			if (_radio.conn_curr->llcp_phy.cmd) {
				/* generate phy update complete event */
				node_rx->hdr.type = NODE_RX_TYPE_PHY_UPDATE;

				p = (void *)pdu_data_rx->lldata;
				p->status = 0;
				p->tx = _radio.conn_curr->phy_tx;
				p->rx = _radio.conn_curr->phy_rx;

				/* enqueue the phy update complete */
				*rx_enqueue = 1;
			}
#endif /* CONFIG_BT_CTLR_PHY */

		} else {
			struct pdu_data_llctrl *llctrl;

			llctrl = (void *)&pdu_data_rx->llctrl;
			switch (llctrl->unknown_rsp.type) {

#if defined(CONFIG_BT_CTLR_LE_PING)
			case PDU_DATA_LLCTRL_TYPE_PING_REQ:
				/* unknown rsp to LE Ping Req completes the
				 * procedure; nothing to do here.
				 */
				break;
#endif /* CONFIG_BT_CTLR_LE_PING */

			default:
				/* enqueue the error and let HCI handle it */
				*rx_enqueue = 1;
				break;
			}
		}

		/* Procedure complete */
		_radio.conn_curr->procedure_expire = 0;
		break;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PDU_DATA_LLCTRL_TYPE_LENGTH_RSP:
	case PDU_DATA_LLCTRL_TYPE_LENGTH_REQ:
		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_LENGTH_REQ,
				 pdu_data_rx->len)) {
			goto isr_rx_conn_unknown_rsp_send;
		}

		nack = isr_rx_conn_pkt_ctrl_dle(pdu_data_rx, rx_enqueue);
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
	case PDU_DATA_LLCTRL_TYPE_PHY_REQ:
		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_PHY_REQ,
				 pdu_data_rx->len)) {
			goto isr_rx_conn_unknown_rsp_send;
		}

		if (_radio.role == ROLE_MASTER) {
			if ((_radio.conn_curr->llcp_phy.ack !=
			     _radio.conn_curr->llcp_phy.req) &&
			    ((_radio.conn_curr->llcp_phy.state ==
			      LLCP_PHY_STATE_ACK_WAIT) ||
			     (_radio.conn_curr->llcp_phy.state ==
			      LLCP_PHY_STATE_RSP_WAIT) ||
			     (_radio.conn_curr->llcp_phy.state ==
			      LLCP_PHY_STATE_UPD))) {
				/* Same procedure collision  */
				nack = reject_ext_ind_send(_radio.conn_curr,
					PDU_DATA_LLCTRL_TYPE_PHY_REQ,
					BT_HCI_ERR_LL_PROC_COLLISION);
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
#if defined(CONFIG_BT_CTLR_LE_ENC)
			} else if (((_radio.conn_curr->llcp_req !=
				     _radio.conn_curr->llcp_ack) &&
				    (_radio.conn_curr->llcp_type !=
				     LLCP_ENCRYPTION)) ||
				   (_radio.conn_curr->llcp_conn_param.req !=
				    _radio.conn_curr->llcp_conn_param.ack)) {
#else /* !CONFIG_BT_CTLR_LE_ENC */
			} else if ((_radio.conn_curr->llcp_req !=
				     _radio.conn_curr->llcp_ack) ||
				   (_radio.conn_curr->llcp_conn_param.req !=
				    _radio.conn_curr->llcp_conn_param.ack)) {
#endif /* !CONFIG_BT_CTLR_LE_ENC */
#else /* !CONFIG_BT_CTLR_CONN_PARAM_REQ */
#if defined(CONFIG_BT_CTLR_LE_ENC)
			} else if ((_radio.conn_curr->llcp_req !=
				    _radio.conn_curr->llcp_ack) &&
				   (_radio.conn_curr->llcp_type !=
				    LLCP_ENCRYPTION)) {
#else /* !CONFIG_BT_CTLR_LE_ENC */
			} else if (_radio.conn_curr->llcp_req !=
				    _radio.conn_curr->llcp_ack) {
#endif /* !CONFIG_BT_CTLR_LE_ENC */
#endif /* !CONFIG_BT_CTLR_CONN_PARAM_REQ */
				/* Different procedure collision */
				nack = reject_ext_ind_send(_radio.conn_curr,
					PDU_DATA_LLCTRL_TYPE_PHY_REQ,
					BT_HCI_ERR_DIFF_TRANS_COLLISION);
			} else {
				struct pdu_data_llctrl *c =
					&pdu_data_rx->llctrl;
				struct pdu_data_llctrl_phy_req *p =
					&c->phy_req;

				_radio.conn_curr->llcp_phy.state =
					LLCP_PHY_STATE_UPD;

				if (_radio.conn_curr->llcp_phy.ack ==
				    _radio.conn_curr->llcp_phy.req) {
					_radio.conn_curr->llcp_phy.ack--;

					_radio.conn_curr->llcp_phy.cmd = 0;

					_radio.conn_curr->llcp_phy.tx =
						_radio.conn_curr->phy_pref_tx;
					_radio.conn_curr->llcp_phy.rx =
						_radio.conn_curr->phy_pref_rx;
				}

				_radio.conn_curr->llcp_phy.tx &= p->rx_phys;
				_radio.conn_curr->llcp_phy.rx &= p->tx_phys;
			}
		} else {
			nack = phy_rsp_send(_radio.conn_curr, pdu_data_rx);
		}
		break;

	case PDU_DATA_LLCTRL_TYPE_PHY_RSP:
		if (_radio.conn_curr->role ||
		    !pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_PHY_RSP,
				 pdu_data_rx->len)) {
			goto isr_rx_conn_unknown_rsp_send;
		}

		if ((_radio.role == ROLE_MASTER) &&
		    (_radio.conn_curr->llcp_phy.ack !=
		     _radio.conn_curr->llcp_phy.req) &&
		    (_radio.conn_curr->llcp_phy.state ==
		     LLCP_PHY_STATE_RSP_WAIT)) {
			struct pdu_data_llctrl_phy_rsp *p =
				&pdu_data_rx->llctrl.phy_rsp;

			_radio.conn_curr->llcp_phy.state = LLCP_PHY_STATE_UPD;

			_radio.conn_curr->llcp_phy.tx &= p->rx_phys;
			_radio.conn_curr->llcp_phy.rx &= p->tx_phys;

			/* Procedure timeout is stopped */
			_radio.conn_curr->procedure_expire = 0;
		}
		break;

	case PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND:
	{
		u8_t err;

		if (!_radio.conn_curr->role ||
		    !pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND,
				 pdu_data_rx->len)) {
			goto isr_rx_conn_unknown_rsp_send;
		}

		err = phy_upd_ind_recv(node_rx, rx_enqueue);
		if (err) {
			_radio.conn_curr->llcp_terminate.reason_peer = err;
		}
	}
	break;
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN)
	case PDU_DATA_LLCTRL_TYPE_MIN_USED_CHAN_IND:
		if (_radio.conn_curr->role ||
		    !pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_MIN_USED_CHAN_IND,
				 pdu_data_rx->len)) {
			goto isr_rx_conn_unknown_rsp_send;
		}

		if (!_radio.conn_curr->role) {
			struct pdu_data_llctrl_min_used_chans_ind *p =
				&pdu_data_rx->llctrl.min_used_chans_ind;
			struct connection *conn = _radio.conn_curr;

#if defined(CONFIG_BT_CTLR_PHY)
			if (!(p->phys & (conn->phy_tx | conn->phy_rx))) {
#else /* !CONFIG_BT_CTLR_PHY */
			if (!(p->phys & 0x01)) {
#endif /* !CONFIG_BT_CTLR_PHY */
				break;
			}

			if (conn->llcp_req != conn->llcp_ack) {
				break;
			}

			memcpy(&conn->llcp.chan_map.chm[0],
			       &_radio.data_chan_map[0],
			       sizeof(conn->llcp.chan_map.chm));
			/* conn->llcp.chan_map.instant     = 0; */
			conn->llcp.chan_map.initiate = 1;

			conn->llcp_type = LLCP_CHAN_MAP;
			conn->llcp_ack--;
		}
		break;
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN */

	default:
isr_rx_conn_unknown_rsp_send:
		nack = unknown_rsp_send(_radio.conn_curr,
					pdu_data_rx->llctrl.opcode);
		break;
	}

	return nack;
}

static inline bool isr_rx_conn_enc_unexpected(struct connection *conn,
					      struct pdu_data *pdu_data)
{
	u8_t opcode = pdu_data->llctrl.opcode;

	return (pdu_data->ll_id != PDU_DATA_LLID_CTRL) ||
	       (!conn->role &&
		((!conn->refresh &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_TERMINATE_IND) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_START_ENC_REQ) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_START_ENC_RSP) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_REJECT_IND) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND)) ||
		 (conn->refresh &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_TERMINATE_IND) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_ENC_RSP) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_START_ENC_REQ) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_START_ENC_RSP) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_REJECT_IND) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND)))) ||
	       (conn->role &&
		((!conn->refresh &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_TERMINATE_IND) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_START_ENC_RSP) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_REJECT_IND) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND)) ||
		 (conn->refresh &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_TERMINATE_IND) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_ENC_REQ) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_START_ENC_RSP) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_REJECT_IND) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND))));
}

static inline u32_t
isr_rx_conn_pkt(struct radio_pdu_node_rx *node_rx,
		struct radio_pdu_node_tx **tx_release, u8_t *rx_enqueue)
{
	struct pdu_data *pdu_data_rx;
	struct pdu_data *pdu_data_tx;
	u8_t terminate = 0;
	u8_t nack = 0;

	/* Ack for transmitted data */
	pdu_data_rx = (void *)node_rx->pdu_data;
	if (pdu_data_rx->nesn != _radio.conn_curr->sn) {

		/* Increment serial number */
		_radio.conn_curr->sn++;

		/* First ack (and redundantly any other ack) enable use of
		 * slave latency.
		 */
		if (_radio.role == ROLE_SLAVE) {
			_radio.conn_curr->slave.latency_enabled = 1;
		}

		if (_radio.conn_curr->empty == 0) {
			struct radio_pdu_node_tx *node_tx;
			u8_t pdu_data_tx_len;

			node_tx = _radio.conn_curr->pkt_tx_head;
			pdu_data_tx = (void *)(node_tx->pdu_data +
				_radio.conn_curr->packet_tx_head_offset);

			pdu_data_tx_len = pdu_data_tx->len;
			if (pdu_data_tx_len != 0) {
				/* if encrypted increment tx counter */
				if (_radio.conn_curr->enc_tx) {
					_radio.conn_curr->ccm_tx.counter++;
				}

				/* process ctrl packet on tx cmplt */
				if (pdu_data_tx->ll_id == PDU_DATA_LLID_CTRL) {
					terminate =
						isr_rx_conn_pkt_ack(pdu_data_tx,
								    &node_tx);
				}
			}

			_radio.conn_curr->packet_tx_head_offset += pdu_data_tx_len;
			if (_radio.conn_curr->packet_tx_head_offset ==
			    _radio.conn_curr->packet_tx_head_len) {
				*tx_release = isr_rx_conn_pkt_release(node_tx);
			}
		} else {
			_radio.conn_curr->empty = 0;
		}
	}

	/* local initiated disconnect procedure completed */
	if (terminate) {
		connection_release(_radio.conn_curr);
		_radio.conn_curr = NULL;

		return terminate;
	}

	/* process received data */
	if ((pdu_data_rx->sn == _radio.conn_curr->nesn) &&
	/* check so that we will NEVER use the rx buffer reserved for empty
	 * packet and internal control enqueue
	 */
	    (packet_rx_reserve_get(3) != 0) &&
	    ((_radio.fc_ena == 0) ||
	     ((_radio.link_rx_head == _radio.link_rx_tail) &&
	      (_radio.fc_req == _radio.fc_ack)) ||
	     ((_radio.link_rx_head != _radio.link_rx_tail) &&
	      (_radio.fc_req != _radio.fc_ack) &&
		(((_radio.fc_req == 0) &&
		  (_radio.fc_handle[TRIPLE_BUFFER_SIZE - 1] ==
		   _radio.conn_curr->handle)) ||
		 ((_radio.fc_req != 0) &&
		  (_radio.fc_handle[_radio.fc_req - 1] ==
		   _radio.conn_curr->handle)))))) {
		u8_t ccm_rx_increment = 0;

		if (pdu_data_rx->len != 0) {
			/* If required, wait for CCM to finish
			 */
			if (_radio.conn_curr->enc_rx) {
				u32_t done;

				done = radio_ccm_is_done();
				LL_ASSERT(done);

				ccm_rx_increment = 1;
			}

			/* MIC Failure Check or data rx during pause */
			if ((_radio.conn_curr->enc_rx &&
			     !radio_ccm_mic_is_valid()) ||
			    (_radio.conn_curr->pause_rx &&
			     isr_rx_conn_enc_unexpected(_radio.conn_curr,
							pdu_data_rx))) {
				_radio.state = STATE_CLOSE;
				radio_disable();

				/* assert if radio packet ptr is not set and
				 * radio started tx */
				LL_ASSERT(!radio_is_ready());

				terminate_ind_rx_enqueue(_radio.conn_curr,
					BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL);

				connection_release(_radio.conn_curr);
				_radio.conn_curr = NULL;

				return 1; /* terminated */
			}

#if defined(CONFIG_BT_CTLR_LE_PING)
			/* stop authenticated payload (pre) timeout */
			_radio.conn_curr->appto_expire = 0;
			_radio.conn_curr->apto_expire = 0;
#endif /* CONFIG_BT_CTLR_LE_PING */

			switch (pdu_data_rx->ll_id) {
			case PDU_DATA_LLID_DATA_CONTINUE:
			case PDU_DATA_LLID_DATA_START:
				/* enqueue data packet */
				*rx_enqueue = 1;
				break;

			case PDU_DATA_LLID_CTRL:
				nack = isr_rx_conn_pkt_ctrl(node_rx,
							    rx_enqueue);
				break;
			case PDU_DATA_LLID_RESV:
			default:
				/* Invalid LL id, drop it. */
				break;
			}

#if defined(CONFIG_BT_CTLR_LE_PING)
		} else if ((_radio.conn_curr->enc_rx) ||
			   (_radio.conn_curr->pause_rx)) {
			struct connection *conn = _radio.conn_curr;
			u16_t appto_reload_new;

			/* check for change in apto */
			appto_reload_new = (conn->apto_reload >
					    (conn->latency + 6)) ?
					   (conn->apto_reload -
					    (conn->latency + 6)) :
					   conn->apto_reload;
			if (conn->appto_reload != appto_reload_new) {
				conn->appto_reload = appto_reload_new;
				conn->apto_expire = 0;
			}

			/* start authenticated payload (pre) timeout */
			if (conn->apto_expire == 0) {
				conn->appto_expire = conn->appto_reload;
				conn->apto_expire = conn->apto_reload;
			}
#endif /* CONFIG_BT_CTLR_LE_PING */

		}

		if (!nack) {
			_radio.conn_curr->nesn++;

			if (ccm_rx_increment) {
				_radio.conn_curr->ccm_rx.counter++;
			}
		}
	}

	return 0;
}

static inline void isr_rx_conn(u8_t crc_ok, u8_t trx_done,
			       u8_t rssi_ready)
{
	struct radio_pdu_node_rx *node_rx;
	struct radio_pdu_node_tx *tx_release = NULL;
	u8_t is_empty_pdu_tx_retry;
	struct pdu_data *pdu_data_rx;
	struct pdu_data *pdu_data_tx;
	u8_t rx_enqueue = 0;
	u8_t crc_close = 0;

#if defined(CONFIG_BT_CTLR_PROFILE_ISR)
	static u8_t s_lmin = (u8_t) -1;
	static u8_t s_min = (u8_t) -1;
	static u8_t s_lmax;
	static u8_t s_lprv;
	static u8_t s_max;
	static u8_t s_prv;

	u8_t latency, elapsed, prv;
	u32_t radio_tmr_end = 0;
	u32_t sample;
	u8_t chg = 0;
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

	/* Increment packet counter for this connection event */
	_radio.packet_counter++;

	/* received data packet */
	node_rx = _radio.packet_rx[_radio.packet_rx_last];
	node_rx->hdr.type = NODE_RX_TYPE_DC_PDU;

	if (crc_ok) {
		u32_t terminate;

		terminate = isr_rx_conn_pkt(node_rx, &tx_release, &rx_enqueue);
		if (terminate) {
			goto isr_rx_conn_exit;
		}

		/* Reset CRC expiry counter */
		_radio.crc_expire = 0;

		/* Reset supervision counter */
		_radio.conn_curr->supervision_expire = 0;
	} else {
		/* Start CRC error countdown, if not already started */
		if (_radio.crc_expire == 0) {
			_radio.crc_expire = 2;
		}

		/* Check crc error countdown expiry */
		_radio.crc_expire--;
		crc_close = (_radio.crc_expire == 0);

		/* Start supervision timeout, if not started already */
		if (!_radio.conn_curr->supervision_expire) {
			_radio.conn_curr->supervision_expire =
				_radio.conn_curr->supervision_reload;
		}
	}

	/* prepare transmit packet */
	is_empty_pdu_tx_retry = _radio.conn_curr->empty;
	prepare_pdu_data_tx(_radio.conn_curr, &pdu_data_tx);

	/* silent connection */
	if (SILENT_CONNECTION) {
		/* slave silent, enter/be in supervision timeout */
		if (_radio.packet_counter == 0) {
			_radio.packet_counter = 0xFF;
		}

		/* master silent, hence avoid slave drift compensation, and
		 * close slave if no tx packets
		 */
		if (!trx_done) {
			/* avoid slave drift compensation if first packet
			 * missed
			 */
			if (_radio.packet_counter == 1) {
				_radio.packet_counter = 0xFF;
			}

			/* no Rx-ed packet and none to Tx, close event */
			if ((_radio.conn_curr->empty) &&
			    (pdu_data_tx->md == 0)) {
				_radio.state = STATE_CLOSE;
				radio_disable();

				goto isr_rx_conn_exit;
			}
		}
	}

	/* Decide on event continuation and hence Radio Shorts to use */
	pdu_data_rx = (void *)node_rx->pdu_data;
	_radio.state = ((_radio.state == STATE_CLOSE) || (crc_close) ||
			((crc_ok) && (pdu_data_rx->md == 0) &&
			 (pdu_data_tx->len == 0)) ||
			_radio.conn_curr->llcp_terminate.reason_peer) ?
			STATE_CLOSE : STATE_TX;

	if (_radio.state == STATE_CLOSE) {
		/* Event close for master */
		if (_radio.role == ROLE_MASTER) {
			_radio.conn_curr->empty = is_empty_pdu_tx_retry;

			radio_disable();

			goto isr_rx_conn_exit;
		}
		/* Event close for slave */
		else {
			radio_switch_complete_and_disable();
		}
	} else {	/* if (_radio.state == STATE_TX) */
		radio_tmr_tifs_set(RADIO_TIFS);

#if defined(CONFIG_BT_CTLR_PHY)
		radio_switch_complete_and_rx(_radio.conn_curr->phy_rx);
#else /* !CONFIG_BT_CTLR_PHY */
		radio_switch_complete_and_rx(0);
#endif /* !CONFIG_BT_CTLR_PHY */

		/* capture end of Tx-ed PDU, used to calculate HCTO. */
		radio_tmr_end_capture();
	}

	/* fill sn and nesn */
	pdu_data_tx->sn = _radio.conn_curr->sn;
	pdu_data_tx->nesn = _radio.conn_curr->nesn;

	/* setup the radio tx packet buffer */
	tx_packet_set(_radio.conn_curr, pdu_data_tx);

#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN)

#if defined(CONFIG_BT_CTLR_PROFILE_ISR)
	/* PA enable is overwriting packet end used in ISR profiling, hence
	 * back it up for later use.
	 */
	radio_tmr_end = radio_tmr_end_get();
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

	radio_gpio_pa_setup();

#if defined(CONFIG_BT_CTLR_PHY)
	radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() + RADIO_TIFS -
				 radio_rx_chain_delay_get(
					 _radio.conn_curr->phy_rx, 1) -
				 CONFIG_BT_CTLR_GPIO_PA_OFFSET);
#else /* !CONFIG_BT_CTLR_PHY */
	radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() + RADIO_TIFS -
				 radio_rx_chain_delay_get(0, 0) -
				 CONFIG_BT_CTLR_GPIO_PA_OFFSET);
#endif /* !CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CTLR_GPIO_PA_PIN */

	/* assert if radio packet ptr is not set and radio started tx */
	LL_ASSERT(!radio_is_ready());

isr_rx_conn_exit:

	/* Save the AA captured for the first Rx in connection event */
	if (!radio_tmr_aa_restore()) {
		radio_tmr_aa_save(radio_tmr_aa_get());
	}

#if defined(CONFIG_BT_CTLR_PROFILE_ISR)
	/* get the ISR latency sample */
	sample = radio_tmr_sample_get();

	/* sample the packet timer again, use it to calculate ISR execution time
	 * and use it in profiling event
	 */
	radio_tmr_sample();

#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

	/* NOTE: Check for connection termination and skip accessing connection
	 * context.
	 */
	if (!_radio.conn_curr) {
		goto isr_rx_conn_terminate_exit;
	}

	/* release tx node and generate event for num complete */
	if (tx_release) {
		pdu_node_tx_release(_radio.conn_curr->handle, tx_release);
	}

	/* enqueue any rx packet/event towards application */
	if (rx_enqueue) {
		/* set data flow control lock on currently rx-ed connection */
		rx_fc_lock(_radio.conn_curr->handle);

		/* set the connection handle and enqueue */
		node_rx->hdr.handle = _radio.conn_curr->handle;
		packet_rx_enqueue();
	}

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
	/* Collect RSSI for connection */
	if (rssi_ready) {
		u8_t rssi = radio_rssi_get();

		_radio.conn_curr->rssi_latest = rssi;

		if (((_radio.conn_curr->rssi_reported - rssi) & 0xFF) >
		    RADIO_RSSI_THRESHOLD) {
			if (_radio.conn_curr->rssi_sample_count) {
				_radio.conn_curr->rssi_sample_count--;
			}
		} else {
			_radio.conn_curr->rssi_sample_count =
				RADIO_RSSI_SAMPLE_COUNT;
		}
	}
#else /* !CONFIG_BT_CTLR_CONN_RSSI */
	ARG_UNUSED(rssi_ready);
#endif /* !CONFIG_BT_CTLR_CONN_RSSI */

isr_rx_conn_terminate_exit:

#if defined(CONFIG_BT_CTLR_PROFILE_ISR)
	/* calculate the elapsed time in us since on-air radio packet end
	 * to ISR entry
	 */
#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN)
	latency = sample - radio_tmr_end;
#else /* !CONFIG_BT_CTLR_GPIO_PA_PIN */
	ARG_UNUSED(radio_tmr_end);

	latency = sample - radio_tmr_end_get();
#endif /* !CONFIG_BT_CTLR_GPIO_PA_PIN */

	/* check changes in min, avg and max of latency */
	if (latency > s_lmax) {
		s_lmax = latency;
		chg = 1;
	}
	if (latency < s_lmin) {
		s_lmin = latency;
		chg = 1;
	}

	/* check for +/- 1us change */
	prv = ((u16_t)s_lprv + latency) >> 1;
	if (prv != s_lprv) {
		s_lprv = latency;
		chg = 1;
	}

	/* calculate the elapsed time in us since ISR entry */
	elapsed = radio_tmr_sample_get() - sample;

	/* check changes in min, avg and max */
	if (elapsed > s_max) {
		s_max = elapsed;
		chg = 1;
	}

	if (elapsed < s_min) {
		s_min = elapsed;
		chg = 1;
	}

	/* check for +/- 1us change */
	prv = ((u16_t)s_prv + elapsed) >> 1;
	if (prv != s_prv) {
		s_prv = elapsed;
		chg = 1;
	}

	/* generate event if any change */
	if (chg) {
		/* NOTE: enqueue only if rx buffer available, else ignore */
		node_rx = packet_rx_reserve_get(2);
		if (node_rx) {
			node_rx->hdr.handle = 0xFFFF;
			node_rx->hdr.type = NODE_RX_TYPE_PROFILE;
			pdu_data_rx = (void *)node_rx->pdu_data;
			pdu_data_rx->profile.lcur = latency;
			pdu_data_rx->profile.lmin = s_lmin;
			pdu_data_rx->profile.lmax = s_lmax;
			pdu_data_rx->profile.cur = elapsed;
			pdu_data_rx->profile.min = s_min;
			pdu_data_rx->profile.max = s_max;
			packet_rx_enqueue();
		}
	}
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

	return;
}

static inline void isr_radio_state_rx(u8_t trx_done, u8_t crc_ok,
				      u8_t devmatch_ok, u8_t devmatch_id,
				      u8_t irkmatch_ok, u8_t irkmatch_id,
				      u8_t rssi_ready)
{
	u32_t err;
	u8_t  rl_idx;

	if (!((trx_done) || ((SILENT_CONNECTION) &&
			     (_radio.role == ROLE_SLAVE)))) {
		_radio.state = STATE_CLOSE;
		radio_disable();

		return;
	}

	switch (_radio.role) {
	case ROLE_ADV:
		if (crc_ok) {
			err = isr_rx_adv(devmatch_ok, devmatch_id, irkmatch_ok,
					 irkmatch_id, rssi_ready);
		} else {
			err = 1;
		}
		if (err) {
			_radio.state = STATE_CLOSE;
			radio_disable();
		}
		break;

	case ROLE_SCAN:
#if defined(CONFIG_BT_CTLR_PRIVACY)
		rl_idx = devmatch_ok ?
			 ctrl_rl_idx(!!(_radio.scanner.filter_policy & 0x01),
					devmatch_id) :
			 irkmatch_ok ? ctrl_rl_irk_idx(irkmatch_id) :
				       FILTER_IDX_NONE;
#else
		rl_idx = FILTER_IDX_NONE;
#endif
		if (crc_ok &&
		    isr_rx_scan_check(irkmatch_ok, devmatch_ok, rl_idx)) {
			err = isr_rx_scan(devmatch_ok, devmatch_id, irkmatch_ok,
					  irkmatch_id, rl_idx, rssi_ready);
		} else {
			err = 1;
		}
		if (err) {
			_radio.state = STATE_CLOSE;
			radio_disable();
			/* switch scanner state to idle */
			_radio.scanner.state = 0;
		}
		break;

	case ROLE_SLAVE:
	case ROLE_MASTER:
		isr_rx_conn(crc_ok, trx_done, rssi_ready);
		break;

	case ROLE_NONE:
	default:
		LL_ASSERT(0);
		break;
	}
}

static inline u32_t isr_close_adv(void)
{
	u32_t dont_close = 0;

	if ((_radio.state == STATE_CLOSE) &&
	    (_radio.advertiser.chan_map_current != 0)) {
		u32_t start_us;

		dont_close = 1;

		adv_setup();

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
	} else {
		radio_filter_disable();

		if ((_radio.state == STATE_CLOSE) &&
		    (!_radio.advertiser.is_hdcd)) {
			u32_t ticker_status;
			u8_t random_delay;

			entropy_nrf_get_entropy_isr(_radio.entropy,
						    &random_delay,
						    sizeof(random_delay));
			random_delay %= 10;
			random_delay += 1;

			/* Call to ticker_update can fail under the race
			 * condition where in the Adv role is being stopped but
			 * at the same time it is preempted by Adv event that
			 * gets into close state. Accept failure when Adv role
			 * is being stopped.
			 */
			ticker_status =
				ticker_update(RADIO_TICKER_INSTANCE_ID_RADIO,
					RADIO_TICKER_USER_ID_WORKER,
					RADIO_TICKER_ID_ADV,
					HAL_TICKER_US_TO_TICKS(random_delay *
							       1000),
					0, 0, 0, 0, 0, ticker_update_adv_assert,
					(void *)__LINE__);
			LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
				  (ticker_status == TICKER_STATUS_BUSY) ||
				  (_radio.ticker_id_stop ==
				   RADIO_TICKER_ID_ADV));
		}

#if defined(CONFIG_BT_CTLR_ADV_INDICATION)
		if (1) {
			struct radio_pdu_node_rx *node_rx;

			node_rx = packet_rx_reserve_get(3);
			if (node_rx) {
				node_rx->hdr.type = NODE_RX_TYPE_ADV_INDICATION;
				node_rx->hdr.handle = 0xFFFF;
				/* TODO: add other info by defining a payload
				 * structure.
				 */

				packet_rx_enqueue();
			}
		}
#endif /* CONFIG_BT_CTLR_ADV_INDICATION */
	}

	return dont_close;
}

static inline u32_t isr_close_scan(void)
{
	u32_t dont_close = 0;

	if (_radio.state == STATE_CLOSE) {
		u32_t start_us;

		dont_close = 1;

		radio_tmr_tifs_set(RADIO_TIFS);
		radio_switch_complete_and_tx(0, 0, 0, 0);
		radio_pkt_rx_set(_radio.packet_rx
					[_radio.packet_rx_last]->pdu_data);
		radio_rssi_measure();
#if defined(CONFIG_BT_CTLR_PRIVACY)
		if (ctrl_rl_enabled()) {
			u8_t count, *irks = ctrl_irks_get(&count);

			radio_ar_configure(count, irks);
		}
#endif /* CONFIG_BT_CTLR_PRIVACY */
		_radio.state = STATE_RX;

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
	} else {
		radio_filter_disable();

		if (_radio.state == STATE_ABORT) {
			/* Scanner stop can expire while here in this ISR.
			 * Deferred attempt to stop can fail as it would have
			 * expired, hence ignore failure.
			 */
			ticker_stop(RADIO_TICKER_INSTANCE_ID_RADIO,
				    RADIO_TICKER_USER_ID_WORKER,
				    RADIO_TICKER_ID_SCAN_STOP, NULL, NULL);
		}
	}

	return dont_close;
}

static inline void isr_close_conn(void)
{
	u16_t ticks_drift_plus;
	u16_t ticks_drift_minus;
	u16_t latency_event;
	u16_t elapsed_event;
	u8_t reason_peer;
	u16_t lazy;
	u8_t force;

	/* Local initiated terminate happened */
	if (_radio.conn_curr == 0) {
		return;
	}

	/* Master transmitted ack for the received terminate ind or
	 * Slave received terminate ind.
	 */
	reason_peer = _radio.conn_curr->llcp_terminate.reason_peer;
	if (reason_peer && ((_radio.role == ROLE_SLAVE) ||
			    _radio.conn_curr->master.terminate_ack)) {
		terminate_ind_rx_enqueue(_radio.conn_curr, reason_peer);

		connection_release(_radio.conn_curr);
		_radio.conn_curr = NULL;

		return;
	}

	ticks_drift_plus = 0;
	ticks_drift_minus = 0;
	latency_event = _radio.conn_curr->latency_event;
	elapsed_event = latency_event + 1;

	/* calculate drift if anchor point sync-ed */
	if (_radio.packet_counter &&
	    (!SILENT_CONNECTION || (_radio.packet_counter != 0xFF))) {
		if (_radio.role == ROLE_SLAVE) {
			u32_t start_to_address_expected_us;
			u32_t start_to_address_actual_us;
			u32_t window_widening_event_us;
			u32_t preamble_to_addr_us;

			/* calculate the drift in ticks */
			start_to_address_actual_us = radio_tmr_aa_restore() -
						     radio_tmr_ready_get();
			window_widening_event_us =
				_radio.conn_curr->slave.window_widening_event_us;
#if defined(CONFIG_BT_CTLR_PHY)
			preamble_to_addr_us =
				addr_us_get(_radio.conn_curr->phy_rx);
#else /* !CONFIG_BT_CTLR_PHY */
			preamble_to_addr_us = addr_us_get(0);
#endif /* !CONFIG_BT_CTLR_PHY */
			start_to_address_expected_us = RADIO_TICKER_JITTER_US +
				(RADIO_TICKER_JITTER_US << 1) +
				preamble_to_addr_us +
				window_widening_event_us;
			if (start_to_address_actual_us <=
			    start_to_address_expected_us) {
				ticks_drift_plus = HAL_TICKER_US_TO_TICKS(
					window_widening_event_us);
				ticks_drift_minus = HAL_TICKER_US_TO_TICKS(
					(start_to_address_expected_us -
					 start_to_address_actual_us));
			} else {
				ticks_drift_plus = HAL_TICKER_US_TO_TICKS(
					start_to_address_actual_us);
				ticks_drift_minus = HAL_TICKER_US_TO_TICKS(
					RADIO_TICKER_JITTER_US +
					(RADIO_TICKER_JITTER_US << 1) +
					preamble_to_addr_us);
			}

			/* Reset window widening, as anchor point sync-ed */
			_radio.conn_curr->slave.window_widening_event_us = 0;
			_radio.conn_curr->slave.window_size_event_us = 0;

			/* apply latency if no more data */
			if (_radio.conn_curr->pkt_tx_head) {
				struct pdu_data *pdu_data_tx;

				pdu_data_tx = (void *)
					_radio.conn_curr->pkt_tx_head->pdu_data;
				if (pdu_data_tx->len ||
				    _radio.conn_curr->packet_tx_head_offset) {
					_radio.conn_curr->latency_event = 0;
				}
			} else if (_radio.conn_curr->slave.latency_enabled) {
				_radio.conn_curr->latency_event =
					_radio.conn_curr->latency;
			}
		} else if (reason_peer) {
			_radio.conn_curr->master.terminate_ack = 1;
		}

		/* Reset connection failed to establish procedure */
		_radio.conn_curr->connect_expire = 0;
	}

	/* check connection failed to establish */
	else if (_radio.conn_curr->connect_expire) {
		if (_radio.conn_curr->connect_expire > elapsed_event) {
			_radio.conn_curr->connect_expire -= elapsed_event;
		} else {
			terminate_ind_rx_enqueue(_radio.conn_curr,
						 BT_HCI_ERR_CONN_FAIL_TO_ESTAB);

			connection_release(_radio.conn_curr);
			_radio.conn_curr = NULL;

			return;
		}
	}

	/* if anchor point not sync-ed, start supervision timeout, and break
	 * latency if any.
	 */
	else {
		/* Start supervision timeout, if not started already */
		if (!_radio.conn_curr->supervision_expire) {
			_radio.conn_curr->supervision_expire =
				_radio.conn_curr->supervision_reload;
		}
	}

	/* check supervision timeout */
	force = 0;
	if (_radio.conn_curr->supervision_expire) {
		if (_radio.conn_curr->supervision_expire > elapsed_event) {
			_radio.conn_curr->supervision_expire -= elapsed_event;

			/* break latency */
			_radio.conn_curr->latency_event = 0;

			/* Force both master and slave when close to
			 * supervision timeout.
			 */
			if (_radio.conn_curr->supervision_expire <= 6) {
				force = 1;
			}
			/* use randomness to force slave role when anchor
			 * points are being missed.
			 */
			else if (_radio.role == ROLE_SLAVE) {
				if (latency_event != 0) {
					force = 1;
				} else {
					force = _radio.conn_curr->slave.force & 0x01;

					/* rotate force bits */
					_radio.conn_curr->slave.force >>= 1;
					if (force) {
						_radio.conn_curr->slave.force |=
							((u32_t)1 << 31);
					}
				}
			}
		} else {
			terminate_ind_rx_enqueue(_radio.conn_curr,
						 BT_HCI_ERR_CONN_TIMEOUT);

			connection_release(_radio.conn_curr);
			_radio.conn_curr = NULL;

			return;
		}
	}

	/* check procedure timeout */
	if (_radio.conn_curr->procedure_expire != 0) {
		if (_radio.conn_curr->procedure_expire > elapsed_event) {
			_radio.conn_curr->procedure_expire -= elapsed_event;
		} else {
			terminate_ind_rx_enqueue(_radio.conn_curr,
						 BT_HCI_ERR_LL_RESP_TIMEOUT);

			connection_release(_radio.conn_curr);
			_radio.conn_curr = NULL;

			return;
		}
	}

#if defined(CONFIG_BT_CTLR_LE_PING)
	/* check apto */
	if (_radio.conn_curr->apto_expire != 0) {
		if (_radio.conn_curr->apto_expire > elapsed_event) {
			_radio.conn_curr->apto_expire -= elapsed_event;
		} else {
			struct radio_pdu_node_rx *node_rx;

			_radio.conn_curr->apto_expire = 0;

			/* Prepare the rx packet structure */
			node_rx = packet_rx_reserve_get(2);
			LL_ASSERT(node_rx);

			node_rx->hdr.handle = _radio.conn_curr->handle;
			node_rx->hdr.type = NODE_RX_TYPE_APTO;

			/* enqueue apto event into rx queue */
			packet_rx_enqueue();
		}
	}

	/* check appto */
	if (_radio.conn_curr->appto_expire != 0) {
		if (_radio.conn_curr->appto_expire > elapsed_event) {
			_radio.conn_curr->appto_expire -= elapsed_event;
		} else {
			_radio.conn_curr->appto_expire = 0;

			if ((_radio.conn_curr->procedure_expire == 0) &&
			    (_radio.conn_curr->llcp_req ==
			     _radio.conn_curr->llcp_ack)) {
				_radio.conn_curr->llcp_type = LLCP_PING;
				_radio.conn_curr->llcp_ack--;
			}
		}
	}
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
	/* generate RSSI event */
	if (_radio.conn_curr->rssi_sample_count == 0) {
		struct radio_pdu_node_rx *node_rx;
		struct pdu_data *pdu_data_rx;

		node_rx = packet_rx_reserve_get(2);
		if (node_rx) {
			_radio.conn_curr->rssi_reported =
				_radio.conn_curr->rssi_latest;
			_radio.conn_curr->rssi_sample_count =
				RADIO_RSSI_SAMPLE_COUNT;

			/* Prepare the rx packet structure */
			node_rx->hdr.handle = _radio.conn_curr->handle;
			node_rx->hdr.type = NODE_RX_TYPE_RSSI;

			/* prepare connection RSSI structure */
			pdu_data_rx = (void *)node_rx->pdu_data;
			pdu_data_rx->rssi = _radio.conn_curr->rssi_reported;

			/* enqueue connection RSSI structure into queue */
			packet_rx_enqueue();
		}
	}
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

	/* break latency based on ctrl procedure pending */
	if ((_radio.conn_curr->llcp_ack != _radio.conn_curr->llcp_req) &&
	    ((_radio.conn_curr->llcp_type == LLCP_CONN_UPD) ||
	     (_radio.conn_curr->llcp_type == LLCP_CHAN_MAP))) {
		_radio.conn_curr->latency_event = 0;
	}

	/* check if latency needs update */
	lazy = 0;
	if ((force) || (latency_event != _radio.conn_curr->latency_event)) {
		lazy = _radio.conn_curr->latency_event + 1;
	}

	if ((ticks_drift_plus != 0) || (ticks_drift_minus != 0) ||
	    (lazy != 0) || (force != 0)) {
		u32_t ticker_status;
		u8_t ticker_id = RADIO_TICKER_ID_FIRST_CONNECTION +
				    _radio.conn_curr->handle;

		/* Call to ticker_update can fail under the race
		 * condition where in the Slave role is being stopped but
		 * at the same time it is preempted by Slave event that
		 * gets into close state. Accept failure when Slave role
		 * is being stopped.
		 */
		ticker_status =
			ticker_update(RADIO_TICKER_INSTANCE_ID_RADIO,
				      RADIO_TICKER_USER_ID_WORKER,
				      ticker_id,
				      ticks_drift_plus, ticks_drift_minus, 0, 0,
				      lazy, force, ticker_update_slave_assert,
				      (void *)(u32_t)ticker_id);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY) ||
			  (_radio.ticker_id_stop == ticker_id));
	}
}

static inline void isr_radio_state_close(void)
{
	u32_t dont_close = 0;

	switch (_radio.role) {
	case ROLE_ADV:
		dont_close = isr_close_adv();
		break;

	case ROLE_SCAN:
		dont_close = isr_close_scan();
		break;

	case ROLE_SLAVE:
	case ROLE_MASTER:
		isr_close_conn();
		break;

	case ROLE_NONE:
		/* If a role closes graceful while it is being stopped, then
		 * Radio ISR will be triggered to process the stop state with
		 * no active role at that instance in time.
		 * Just reset the state to none. The role has gracefully closed
		 * before this ISR run.
		 * The above applies to aborting a role event too.
		 */
		LL_ASSERT((_radio.state == STATE_STOP) ||
			  (_radio.state == STATE_ABORT));

		_radio.state = STATE_NONE;

		return;

	default:
		LL_ASSERT(0);
		break;
	}

	if (dont_close) {
		return;
	}

	_radio.role = ROLE_NONE;
	_radio.state = STATE_NONE;
	_radio.ticker_id_event = 0;

	radio_tmr_stop();

	event_inactive(0, 0, 0, NULL);

	clock_control_off(_radio.hf_clock, NULL);

	mayfly_enable(RADIO_TICKER_USER_ID_WORKER, RADIO_TICKER_USER_ID_JOB, 1);

	DEBUG_RADIO_CLOSE(0);
}

static void isr(void)
{
	u8_t trx_done;
	u8_t crc_ok;
	u8_t devmatch_ok;
	u8_t devmatch_id;
	u8_t irkmatch_ok;
	u8_t irkmatch_id;
	u8_t rssi_ready;

	DEBUG_RADIO_ISR(1);

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

	switch (_radio.state) {
	case STATE_TX:
		isr_radio_state_tx();
		break;

	case STATE_RX:
		isr_radio_state_rx(trx_done, crc_ok, devmatch_ok, devmatch_id,
				   irkmatch_ok, irkmatch_id, rssi_ready);
		break;

	case STATE_ABORT:
	case STATE_STOP:
	case STATE_CLOSE:
		isr_radio_state_close();
		break;

	case STATE_NONE:
		/* Ignore Duplicate Radio Disabled IRQ due to forced stop
		 * using Radio Disable task.
		 */
		break;

	default:
		LL_ASSERT(0);
		break;
	}

	DEBUG_RADIO_ISR(0);
}

#if (RADIO_TICKER_USER_ID_WORKER_PRIO == RADIO_TICKER_USER_ID_JOB_PRIO)
static void ticker_job_disable(u32_t status, void *op_context)
{
	ARG_UNUSED(status);
	ARG_UNUSED(op_context);

	if (_radio.state != STATE_NONE) {
		mayfly_enable(RADIO_TICKER_USER_ID_JOB,
			      RADIO_TICKER_USER_ID_JOB, 0);
	}
}
#endif

static void ticker_if_done(u32_t status, void *ops_context)
{
	*((u32_t volatile *)ops_context) = status;
}

static void ticker_success_assert(u32_t status, void *params)
{
	ARG_UNUSED(params);

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);
}

static void ticker_stop_adv_assert(u32_t status, void *params)
{
	ARG_UNUSED(params);

	if (status == TICKER_STATUS_FAILURE) {
		if (_radio.ticker_id_stop == RADIO_TICKER_ID_ADV) {
			/* ticker_stop failed due to race condition
			 * while in role_disable. Let the role_disable
			 * be made aware of, so it can return failure
			 * (to stop Adv role as it is now transitioned
			 * to Slave role).
			 */
			_radio.ticker_id_stop = 0;
		} else {
			LL_ASSERT(0);
		}
	} else {
		/* This assert shall not happen if advertiser role's slot
		 * calculation is correct, and next event shall not
		 * overlap/pre-empt the current advertise role event.
		 */
		LL_ASSERT(_radio.ticker_id_prepare != RADIO_TICKER_ID_ADV);
	}
}

static void ticker_stop_scan_assert(u32_t status, void *params)
{
	ARG_UNUSED(params);

	if (status == TICKER_STATUS_FAILURE) {
		if (_radio.ticker_id_stop == RADIO_TICKER_ID_SCAN) {
			/* ticker_stop failed due to race condition
			 * while in role_disable. Let the role_disable
			 * be made aware of, so it can return failure
			 * (to stop Scan role as it is now transitioned
			 * to Master role).
			 */
			_radio.ticker_id_stop = 0;
		} else {
			LL_ASSERT(0);
		}
	} else {
		/* This assert shall not happen if scanner role's slot
		 * calculation is correct, and next event shall not
		 * overlap/pre-empt the current scanner role event.
		 */
		LL_ASSERT(_radio.ticker_id_prepare != RADIO_TICKER_ID_SCAN);
	}
}

static void ticker_update_adv_assert(u32_t status, void *params)
{
	ARG_UNUSED(params);

	LL_ASSERT((status == TICKER_STATUS_SUCCESS) ||
		  (_radio.ticker_id_stop == RADIO_TICKER_ID_ADV));
}

static void ticker_update_slave_assert(u32_t status, void *params)
{
	u8_t ticker_id = (u32_t)params & 0xFF;

	LL_ASSERT((status == TICKER_STATUS_SUCCESS) ||
		  (_radio.ticker_id_stop == ticker_id));
}

static void mayfly_radio_active(void *params)
{
	static u8_t s_active;

	if ((u32_t)params) {
		if (s_active++) {
			return;
		}

		DEBUG_RADIO_ACTIVE(1);

		radio_active_callback(1);
	} else {
		LL_ASSERT(s_active);

		if (--s_active) {
			return;
		}

		DEBUG_RADIO_ACTIVE(0);

		radio_active_callback(0);
	}
}

static void event_active(u32_t ticks_at_expire, u32_t remainder,
			 u16_t lazy, void *context)
{
	static memq_link_t s_link;
	static struct mayfly s_mfy_radio_active = {0, 0, &s_link, (void *)1,
						   mayfly_radio_active};
	u32_t retval;

	ARG_UNUSED(ticks_at_expire);
	ARG_UNUSED(remainder);
	ARG_UNUSED(lazy);
	ARG_UNUSED(context);

	retval = mayfly_enqueue(RADIO_TICKER_USER_ID_WORKER,
				RADIO_TICKER_USER_ID_WORKER, 0,
				&s_mfy_radio_active);
	LL_ASSERT(!retval);
}

static void mayfly_radio_inactive(void *params)
{
	ARG_UNUSED(params);

	mayfly_radio_active(0);

	DEBUG_RADIO_CLOSE(0);
}

static void event_inactive(u32_t ticks_at_expire, u32_t remainder,
			   u16_t lazy, void *context)
{
	static memq_link_t s_link;
	static struct mayfly s_mfy_radio_inactive = {0, 0, &s_link, NULL,
						     mayfly_radio_inactive};
	u32_t retval;

	ARG_UNUSED(ticks_at_expire);
	ARG_UNUSED(remainder);
	ARG_UNUSED(lazy);
	ARG_UNUSED(context);

	retval = mayfly_enqueue(RADIO_TICKER_USER_ID_WORKER,
				RADIO_TICKER_USER_ID_WORKER, 0,
				&s_mfy_radio_inactive);
	LL_ASSERT(!retval);
}

static void mayfly_xtal_start(void *params)
{
	ARG_UNUSED(params);

	/* turn on 16MHz clock, non-blocking mode. */
	clock_control_on(_radio.hf_clock, NULL);
}

static void event_xtal(u32_t ticks_at_expire, u32_t remainder,
		       u16_t lazy, void *context)
{
	static memq_link_t s_link;
	static struct mayfly s_mfy_xtal_start = {0, 0, &s_link, NULL,
						 mayfly_xtal_start};
	u32_t retval;

	ARG_UNUSED(ticks_at_expire);
	ARG_UNUSED(remainder);
	ARG_UNUSED(lazy);
	ARG_UNUSED(context);

	retval = mayfly_enqueue(RADIO_TICKER_USER_ID_WORKER,
				RADIO_TICKER_USER_ID_WORKER, 0,
				&s_mfy_xtal_start);
	LL_ASSERT(!retval);
}

static void mayfly_xtal_stop(void *params)
{
	ARG_UNUSED(params);

	clock_control_off(_radio.hf_clock, NULL);

	DEBUG_RADIO_CLOSE(0);
}

#define DRV_NAME CONFIG_CLOCK_CONTROL_NRF5_K32SRC_DRV_NAME
#define K32SRC   CLOCK_CONTROL_NRF5_K32SRC
static void k32src_wait(void)
{
	if (!_radio.is_k32src_stable) {
		struct device *clk_k32;

		_radio.is_k32src_stable = 1;

		clk_k32 = device_get_binding(DRV_NAME);
		LL_ASSERT(clk_k32);

		while (clock_control_on(clk_k32, (void *)K32SRC)) {
			DEBUG_CPU_SLEEP(1);
			cpu_sleep();
			DEBUG_CPU_SLEEP(0);
		}
	}
}
#undef K32SRC
#undef DRV_NAME

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
#define XON_BITMASK BIT(31) /* XTAL has been retained from previous prepare */

static void mayfly_xtal_retain(u8_t caller_id, u8_t retain)
{
	static u8_t s_xtal_retained;

	if (retain) {
		if (!s_xtal_retained) {
			static memq_link_t s_link;
			static struct mayfly s_mfy_xtal_start = {0, 0, &s_link,
				NULL, mayfly_xtal_start};
			u32_t retval;

			/* Only user id job will try to retain the XTAL. */
			LL_ASSERT(caller_id == RADIO_TICKER_USER_ID_JOB);

			s_xtal_retained = 1;

			retval = mayfly_enqueue(caller_id,
						RADIO_TICKER_USER_ID_WORKER, 0,
						&s_mfy_xtal_start);
			LL_ASSERT(!retval);
		}
	} else {
		if (s_xtal_retained) {
			static memq_link_t s_link[2];
			static struct mayfly s_mfy_xtal_stop[2] = {
				{0, 0, &s_link[0], NULL, mayfly_xtal_stop},
				{0, 0, &s_link[1], NULL, mayfly_xtal_stop}
			};
			struct mayfly *p_mfy_xtal_stop = NULL;
			u32_t retval;

			s_xtal_retained = 0;

			switch (caller_id) {
			case RADIO_TICKER_USER_ID_WORKER:
				p_mfy_xtal_stop = &s_mfy_xtal_stop[0];
				break;

			case RADIO_TICKER_USER_ID_JOB:
				p_mfy_xtal_stop = &s_mfy_xtal_stop[1];
				break;

			default:
				LL_ASSERT(0);
				break;
			}

			retval = mayfly_enqueue(caller_id,
						RADIO_TICKER_USER_ID_WORKER, 0,
						p_mfy_xtal_stop);
			LL_ASSERT(!retval);
		}
	}
}

static void prepare_reduced(u32_t status, void *op_context)
{
	/* It is acceptable that ticker_update will fail, if ticker is stopped;
	 * for example, scan ticker is stopped on connection estblishment but
	 * is also preempted.
	 */
	if (status == 0) {
		struct shdr *hdr = (void *)op_context;

		hdr->ticks_xtal_to_start |= XON_BITMASK;
	}
}

static void prepare_normal(u32_t status, void *op_context)
{
	/* It is acceptable that ticker_update will fail, if ticker is stopped;
	 * for example, scan ticker is stopped on connection estblishment but
	 * is also preempted.
	 */
	if (status == 0) {
		struct shdr *hdr = (void *)op_context;

		hdr->ticks_xtal_to_start &= ~XON_BITMASK;
	}
}

static void prepare_normal_set(struct shdr *hdr, u8_t ticker_user_id,
			       u8_t ticker_id)
{
	if (hdr->ticks_xtal_to_start & XON_BITMASK) {
		u32_t ticker_status;
		u32_t ticks_prepare_to_start =
			max(hdr->ticks_active_to_start,
			    hdr->ticks_preempt_to_start);
		u32_t ticks_drift_minus = (hdr->ticks_xtal_to_start &
					   ~XON_BITMASK) -
					  ticks_prepare_to_start;

		ticker_status =
			ticker_update(RADIO_TICKER_INSTANCE_ID_RADIO,
				      ticker_user_id,
				      ticker_id, 0, ticks_drift_minus,
				      ticks_drift_minus, 0, 0, 0,
				      prepare_normal, hdr);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY));
	}
}

#if (RADIO_TICKER_PREEMPT_PART_US <= RADIO_TICKER_PREEMPT_PART_MIN_US)
static u32_t preempt_calc(struct shdr *hdr, u8_t ticker_id,
			  u32_t ticks_at_expire)
{
	u32_t diff =
		ticker_ticks_diff_get(ticker_ticks_now_get(), ticks_at_expire);

	diff += 3;
	if (diff > HAL_TICKER_US_TO_TICKS(RADIO_TICKER_START_PART_US)) {
		mayfly_xtal_retain(RADIO_TICKER_USER_ID_WORKER, 0);

		prepare_normal_set(hdr, RADIO_TICKER_USER_ID_WORKER, ticker_id);

		diff += hdr->ticks_preempt_to_start;
		if (diff <
		    HAL_TICKER_US_TO_TICKS(RADIO_TICKER_PREEMPT_PART_MAX_US)) {
			hdr->ticks_preempt_to_start = diff;
		}

		return 1;
	}

	return 0;
}
#endif

static inline struct shdr *hdr_conn_get(u8_t ticker_id,
					struct connection **conn)
{
	if (ticker_id >= RADIO_TICKER_ID_FIRST_CONNECTION) {
		*conn = mem_get(_radio.conn_pool, CONNECTION_T_SIZE,
				(ticker_id -
				 RADIO_TICKER_ID_FIRST_CONNECTION));
		return &(*conn)->hdr;
	} else if (ticker_id == RADIO_TICKER_ID_ADV) {
		return &_radio.advertiser.hdr;
	} else if (ticker_id == RADIO_TICKER_ID_SCAN) {
		return &_radio.scanner.hdr;
	}

	return NULL;
}

/* This function decides to start (additional call) xtal ahead of next ticker,
 * if next ticker is close to current ticker expire.
 *
 * NOTE: This function also detects if two tickers of same interval are drifting
 * close and issues a conn param req or does a conn update.
 *
 * TODO: Detect drift for already overlapping tickers.
 */
static void mayfly_xtal_stop_calc(void *params)
{
	u8_t ticker_id_curr = ((u32_t)params & 0xff);
	u32_t volatile ret_cb = TICKER_STATUS_BUSY;
	struct connection *conn_curr = NULL;
	struct connection *conn_next = NULL;
	u32_t ticks_prepare_to_start_next;
	struct shdr *hdr_curr = NULL;
	struct shdr *hdr_next = NULL;
	u32_t ticks_to_expire;
	u32_t ticks_slot_abs;
	u32_t ticks_current;
	u8_t ticker_id_next;
	u32_t ret;

	ticker_id_next = 0xff;
	ticks_to_expire = 0;
	do {
		ret = ticker_next_slot_get(RADIO_TICKER_INSTANCE_ID_RADIO,
					   RADIO_TICKER_USER_ID_JOB,
					   &ticker_id_next, &ticks_current,
					   &ticks_to_expire, ticker_if_done,
					   (void *)&ret_cb);

		if (ret == TICKER_STATUS_BUSY) {
			while (ret_cb == TICKER_STATUS_BUSY) {
				ticker_job_sched(RADIO_TICKER_INSTANCE_ID_RADIO,
						 RADIO_TICKER_USER_ID_JOB);
			}
		}

		LL_ASSERT(ret_cb == TICKER_STATUS_SUCCESS);
	} while (ticker_id_next != TICKER_NULL &&
		 ticker_id_next >= (RADIO_TICKER_ID_FIRST_CONNECTION +
				    _radio.connection_count));

	if ((ticker_id_next == TICKER_NULL) ||
	    (ticker_id_next < RADIO_TICKER_ID_ADV) ||
	    (ticker_id_next >= (RADIO_TICKER_ID_FIRST_CONNECTION +
				_radio.connection_count))) {
		mayfly_xtal_retain(RADIO_TICKER_USER_ID_JOB, 0);

		return;
	}

	/* Select the current role's scheduling header */
	hdr_curr = hdr_conn_get(ticker_id_curr, &conn_curr);
	LL_ASSERT(hdr_curr);

	/* Compensate for current ticker in reduced prepare */
	if (hdr_curr->ticks_xtal_to_start & XON_BITMASK) {
		ticks_slot_abs = max(hdr_curr->ticks_active_to_start,
				     hdr_curr->ticks_preempt_to_start);
	} else {
		ticks_slot_abs = max(hdr_curr->ticks_active_to_start,
				     hdr_curr->ticks_xtal_to_start);
	}
	ticks_slot_abs += hdr_curr->ticks_slot;

	/* Select the next role's scheduling header */
	hdr_next = hdr_conn_get(ticker_id_next, &conn_next);
	LL_ASSERT(hdr_next);

	ticks_prepare_to_start_next =
		max(hdr_next->ticks_active_to_start,
		    hdr_next->ticks_preempt_to_start);

	/* Compensate for next ticker in reduced prepare */
	if (hdr_next->ticks_xtal_to_start & XON_BITMASK) {
		ticks_to_expire -=
			(hdr_next->ticks_xtal_to_start &
			 ~XON_BITMASK) - ticks_prepare_to_start_next;
	}

	/* If beyond the xtal threshold reset to normal the next prepare,
	 * else retain xtal and reduce the next prepare.
	 */
	if (ticks_to_expire >
	    (ticks_slot_abs +
	     HAL_TICKER_US_TO_TICKS(CONFIG_BT_CTLR_XTAL_THRESHOLD))) {
		mayfly_xtal_retain(RADIO_TICKER_USER_ID_JOB, 0);
		prepare_normal_set(hdr_next, RADIO_TICKER_USER_ID_JOB,
				   ticker_id_next);
	} else {
		mayfly_xtal_retain(RADIO_TICKER_USER_ID_JOB, 1);

		/* Reduce the next prepare if not already and, active to start
		 * and preempt to start both are less than xtal to start
		 */
		if (!(hdr_next->ticks_xtal_to_start & XON_BITMASK) &&
		    (hdr_next->ticks_xtal_to_start >
		     ticks_prepare_to_start_next)) {
			u32_t ticks_drift_plus = hdr_next->ticks_xtal_to_start -
						 ticks_prepare_to_start_next;
			u32_t ticker_status;

			ticker_status =
				ticker_update(RADIO_TICKER_INSTANCE_ID_RADIO,
					      RADIO_TICKER_USER_ID_JOB,
					      ticker_id_next,
					      ticks_drift_plus, 0,
					      0, ticks_drift_plus,
					      0, 0,
					      prepare_reduced, hdr_next);
			LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
				  (ticker_status == TICKER_STATUS_BUSY));
		}

#if defined(CONFIG_BT_CTLR_SCHED_ADVANCED)
		if (!conn_curr || !conn_next) {
			return;
		}

		/* auto conn param req or conn update procedure to
		 * avoid connection collisions due to drifting roles.
		 */
		if (conn_curr->conn_interval == conn_next->conn_interval) {
			u32_t ticks_conn_interval = HAL_TICKER_US_TO_TICKS(
				conn_curr->conn_interval * 1250);

			/* remove laziness, if any, from
			 * ticks_to_expire.
			 */
			while (ticks_to_expire > ticks_conn_interval) {
				ticks_to_expire -= ticks_conn_interval;
			}

			/* if next ticker close to this ticker, send
			 * conn param req.
			 */
			if (conn_curr->role && !conn_next->role &&
			    (ticks_to_expire <
			     (ticks_slot_abs +
			      HAL_TICKER_US_TO_TICKS(RADIO_TIES_US)))) {
				u32_t status;

				status = conn_update_req(conn_curr);
				if (status == 2) {
					conn_update_req(conn_next);
				}
			} else if (!conn_curr->role && conn_next->role &&
				   (ticks_to_expire <
				    (ticks_slot_abs +
				     HAL_TICKER_US_TO_TICKS(RADIO_TIES_US)))) {
				u32_t status;

				status = conn_update_req(conn_next);
				if (status == 2) {
					conn_update_req(conn_curr);
				}
			}
		}
#endif /* CONFIG_BT_CTLR_SCHED_ADVANCED */
	}
}
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */

#if defined(CONFIG_BT_CTLR_SCHED_ADVANCED)
static void sched_after_mstr_free_slot_get(u8_t user_id,
					   u32_t ticks_slot_abs,
					   u32_t *ticks_anchor,
					   u32_t *us_offset)
{
	u32_t ticks_to_expire_prev;
	u32_t ticks_slot_abs_prev;
	u32_t ticks_to_expire;
	u8_t ticker_id_prev;
	u8_t ticker_id;

	ticks_slot_abs += HAL_TICKER_US_TO_TICKS(RADIO_TICKER_JITTER_US << 3);

	ticker_id = ticker_id_prev = 0xff;
	ticks_to_expire = ticks_to_expire_prev = *us_offset = 0;
	ticks_slot_abs_prev = 0;
	while (1) {
		u32_t volatile ret_cb = TICKER_STATUS_BUSY;
		struct connection *conn;
		u32_t ret;

		ret = ticker_next_slot_get(RADIO_TICKER_INSTANCE_ID_RADIO,
					   user_id, &ticker_id, ticks_anchor,
					   &ticks_to_expire, ticker_if_done,
					   (void *)&ret_cb);

		if (ret == TICKER_STATUS_BUSY) {
			while (ret_cb == TICKER_STATUS_BUSY) {
				ticker_job_sched(RADIO_TICKER_INSTANCE_ID_RADIO,
						 user_id);
			}
		}

		LL_ASSERT(ret_cb == TICKER_STATUS_SUCCESS);

		if (ticker_id == 0xff) {
			break;
		}

		if (ticker_id < RADIO_TICKER_ID_FIRST_CONNECTION ||
		    ticker_id >= (RADIO_TICKER_ID_FIRST_CONNECTION +
				  _radio.connection_count)) {
			continue;
		}

		conn = mem_get(_radio.conn_pool, CONNECTION_T_SIZE,
			       (ticker_id - RADIO_TICKER_ID_FIRST_CONNECTION));
		if (conn && !conn->role) {
			u32_t ticks_to_expire_normal = ticks_to_expire;
			u32_t ticks_slot_abs_curr;

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
			if (conn->hdr.ticks_xtal_to_start & XON_BITMASK) {
				u32_t ticks_prepare_to_start =
					max(conn->hdr.ticks_active_to_start,
					    conn->hdr.ticks_preempt_to_start);

				ticks_slot_abs_curr =
					conn->hdr.ticks_xtal_to_start &
					~XON_BITMASK;
				ticks_to_expire_normal -=
					ticks_slot_abs_curr -
					ticks_prepare_to_start;
			} else
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
			{
				u32_t ticks_prepare_to_start =
					max(conn->hdr.ticks_active_to_start,
					    conn->hdr.ticks_xtal_to_start);

				ticks_slot_abs_curr = ticks_prepare_to_start;
			}

			ticks_slot_abs_curr +=
				conn->hdr.ticks_slot +
				HAL_TICKER_US_TO_TICKS(RADIO_TICKER_JITTER_US <<
						       3);

			if ((ticker_id_prev != 0xFF) &&
			    (ticker_ticks_diff_get(ticks_to_expire_normal,
						   ticks_to_expire_prev) >
			     (ticks_slot_abs_prev + ticks_slot_abs))) {
				break;
			}

			ticker_id_prev = ticker_id;
			ticks_to_expire_prev = ticks_to_expire_normal;
			ticks_slot_abs_prev = ticks_slot_abs_curr;
		}
	}

	if (ticker_id_prev != 0xff) {
		*us_offset = HAL_TICKER_TICKS_TO_US(ticks_to_expire_prev +
						    ticks_slot_abs_prev) +
						    (RADIO_TICKER_JITTER_US <<
						     3);
	}
}

static void sched_after_mstr_free_offset_get(u16_t conn_interval,
					     u32_t ticks_slot,
					     u32_t ticks_anchor,
					     u32_t *win_offset_us)
{
	u32_t ticks_anchor_offset = ticks_anchor;

	sched_after_mstr_free_slot_get(RADIO_TICKER_USER_ID_JOB,
				       (HAL_TICKER_US_TO_TICKS(
						RADIO_TICKER_XTAL_OFFSET_US) +
					ticks_slot), &ticks_anchor_offset,
				       win_offset_us);

	if (!*win_offset_us) {
		return;
	}

	LL_ASSERT(!((ticks_anchor_offset - ticks_anchor) &
		    BIT(HAL_TICKER_MSBIT)));

	*win_offset_us += HAL_TICKER_TICKS_TO_US(
		ticker_ticks_diff_get(ticks_anchor_offset,
				      ticks_anchor));

	if ((*win_offset_us & BIT(31)) == 0) {
		u32_t conn_interval_us = conn_interval * 1250;

		while (*win_offset_us > conn_interval_us) {
			*win_offset_us -= conn_interval_us;
		}
	}
}

static void mayfly_sched_after_mstr_free_offset_get(void *params)
{
	sched_after_mstr_free_offset_get(_radio.scanner.conn_interval,
					 _radio.scanner.ticks_conn_slot,
					 (u32_t)params,
					 &_radio.scanner.win_offset_us);
}

static void mayfly_sched_win_offset_use(void *params)
{
	struct connection *conn = params;
	u16_t win_offset;

	sched_after_mstr_free_offset_get(conn->conn_interval,
				conn->hdr.ticks_slot,
				conn->llcp.conn_upd.ticks_anchor,
				&conn->llcp.conn_upd.win_offset_us);

	win_offset = conn->llcp.conn_upd.win_offset_us / 1250;
	memcpy(conn->llcp.conn_upd.pdu_win_offset, &win_offset,
	       sizeof(u16_t));
}

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
static void sched_free_win_offset_calc(struct connection *conn_curr,
				       u8_t is_select,
				       u32_t *ticks_to_offset_next,
				       u16_t conn_interval,
				       u8_t *offset_max,
				       u8_t *win_offset)
{
	u32_t ticks_prepare_reduced = 0;
	u32_t ticks_to_expire_prev;
	u32_t ticks_slot_abs_prev;
	u32_t ticks_anchor_prev;
	u32_t ticks_to_expire;
	u32_t ticks_slot_abs;
	u8_t ticker_id_other;
	u8_t ticker_id_prev;
	u32_t ticks_anchor;
	u8_t offset_index;
	u16_t _win_offset;
	u8_t ticker_id;

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
	if (conn_curr->hdr.ticks_xtal_to_start & XON_BITMASK) {
		u32_t ticks_prepare_to_start =
			max(conn_curr->hdr.ticks_active_to_start,
			    conn_curr->hdr.ticks_preempt_to_start);

		ticks_slot_abs = conn_curr->hdr.ticks_xtal_to_start &
				 ~XON_BITMASK;
		ticks_prepare_reduced = ticks_slot_abs - ticks_prepare_to_start;
	} else
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
	{
		u32_t ticks_prepare_to_start =
			max(conn_curr->hdr.ticks_active_to_start,
			    conn_curr->hdr.ticks_xtal_to_start);

		ticks_slot_abs = ticks_prepare_to_start;
	}

	ticks_slot_abs += conn_curr->hdr.ticks_slot +
			  HAL_TICKER_US_TO_TICKS(RADIO_TIES_US + 1250);

	ticker_id = ticker_id_prev = ticker_id_other = 0xFF;
	ticks_to_expire = ticks_to_expire_prev = ticks_anchor =
		ticks_anchor_prev = offset_index = _win_offset = 0;
	ticks_slot_abs_prev = 0;
	do {
		u32_t volatile ret_cb = TICKER_STATUS_BUSY;
		struct connection *conn;
		u32_t ret;

		ret = ticker_next_slot_get(RADIO_TICKER_INSTANCE_ID_RADIO,
					   RADIO_TICKER_USER_ID_JOB,
					   &ticker_id, &ticks_anchor,
					   &ticks_to_expire, ticker_if_done,
					   (void *)&ret_cb);

		if (ret == TICKER_STATUS_BUSY) {
			while (ret_cb == TICKER_STATUS_BUSY) {
				ticker_job_sched(RADIO_TICKER_INSTANCE_ID_RADIO,
						 RADIO_TICKER_USER_ID_JOB);
			}
		}

		LL_ASSERT(ret_cb == TICKER_STATUS_SUCCESS);

		if (ticker_id == 0xff) {
			break;
		}

		/* ticks_anchor shall not change during this loop */
		if ((ticker_id_prev != 0xff) &&
		    (ticks_anchor != ticks_anchor_prev)) {
			LL_ASSERT(0);
		}

		/* consider advertiser time as available. Any other time used by
		 * tickers declared outside the controller is also available.
		 */
		if (ticker_id <= RADIO_TICKER_ID_ADV ||
		    ticker_id >= (RADIO_TICKER_ID_FIRST_CONNECTION +
				  _radio.connection_count)) {
			continue;
		}

		if (ticker_id < RADIO_TICKER_ID_FIRST_CONNECTION) {
			/* non conn role found which could have preempted a
			 * conn role, hence do not consider this free space
			 * and any further as free slot for offset,
			 */
			ticker_id_other = ticker_id;
			continue;
		}

		/* TODO: handle scanner; for now we exit with as much we
		 * where able to fill (offsets).
		 */
		if (ticker_id_other != 0xFF) {
			break;
		}

		conn = mem_get(_radio.conn_pool, CONNECTION_T_SIZE,
			       (ticker_id - RADIO_TICKER_ID_FIRST_CONNECTION));

		if ((conn != conn_curr) && (is_select || !conn->role)) {
			u32_t ticks_to_expire_normal =
				ticks_to_expire + ticks_prepare_reduced;
			u32_t ticks_slot_abs_curr;

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
			if (conn->hdr.ticks_xtal_to_start & XON_BITMASK) {
				u32_t ticks_prepare_to_start =
					max(conn->hdr.ticks_active_to_start,
					    conn->hdr.ticks_preempt_to_start);

				ticks_slot_abs_curr =
					conn->hdr.ticks_xtal_to_start &
					~XON_BITMASK;
				ticks_to_expire_normal -=
					ticks_slot_abs_curr -
					ticks_prepare_to_start;
			} else
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
			{
				u32_t ticks_prepare_to_start =
					max(conn->hdr.ticks_active_to_start,
					    conn->hdr.ticks_xtal_to_start);

				ticks_slot_abs_curr = ticks_prepare_to_start;
			}

			ticks_slot_abs_curr +=
				conn->hdr.ticks_slot +
				HAL_TICKER_US_TO_TICKS(RADIO_TIES_US + 1250);

			if (*ticks_to_offset_next < ticks_to_expire_normal) {
				if (ticks_to_expire_prev < *ticks_to_offset_next) {
					ticks_to_expire_prev =
						*ticks_to_offset_next;
				}

				while ((offset_index < *offset_max) &&
				       (ticker_ticks_diff_get(
							ticks_to_expire_normal,
							ticks_to_expire_prev) >=
					(ticks_slot_abs_prev +
					 ticks_slot_abs))) {
					_win_offset = HAL_TICKER_TICKS_TO_US(
						ticks_to_expire_prev +
						ticks_slot_abs_prev) / 1250;
					if (_win_offset >= conn_interval) {
						ticks_to_expire_prev = 0;

						break;
					}

					memcpy(win_offset +
					       (sizeof(u16_t) * offset_index),
					       &_win_offset, sizeof(u16_t));
					offset_index++;

					ticks_to_expire_prev +=
						HAL_TICKER_US_TO_TICKS(1250);
				}

				*ticks_to_offset_next = ticks_to_expire_prev;

				if (_win_offset >= conn_interval) {
					break;
				}
			}

			ticks_anchor_prev = ticks_anchor;
			ticker_id_prev = ticker_id;
			ticks_to_expire_prev = ticks_to_expire_normal;
			ticks_slot_abs_prev = ticks_slot_abs_curr;
		}
	} while (offset_index < *offset_max);

	if (ticker_id == 0xFF) {
		if (ticks_to_expire_prev < *ticks_to_offset_next) {
			ticks_to_expire_prev = *ticks_to_offset_next;
		}

		while (offset_index < *offset_max) {
			_win_offset = HAL_TICKER_TICKS_TO_US(
					ticks_to_expire_prev +
					ticks_slot_abs_prev) / 1250;
			if (_win_offset >= conn_interval) {
				ticks_to_expire_prev = 0;

				break;
			}

			memcpy(win_offset + (sizeof(u16_t) * offset_index),
			       &_win_offset, sizeof(u16_t));
			offset_index++;

			ticks_to_expire_prev += HAL_TICKER_US_TO_TICKS(1250);
		}

		*ticks_to_offset_next = ticks_to_expire_prev;
	}

	*offset_max = offset_index;
}

static void mayfly_sched_free_win_offset_calc(void *params)
{
	struct connection *conn = params;
	u32_t ticks_to_offset_default = 0;
	u32_t *ticks_to_offset_next;
	u8_t offset_max = 6;

	ticks_to_offset_next = &ticks_to_offset_default;

	if (conn->role) {
		conn->llcp_conn_param.ticks_to_offset_next =
			conn->slave.ticks_to_offset;

		ticks_to_offset_next =
			&conn->llcp_conn_param.ticks_to_offset_next;
	}

	sched_free_win_offset_calc(conn, 0, ticks_to_offset_next,
				   conn->llcp_conn_param.interval, &offset_max,
				   (u8_t *)conn->llcp_conn_param.pdu_win_offset0);
}

static void mayfly_sched_win_offset_select(void *params)
{
#define OFFSET_S_MAX 6
#define OFFSET_M_MAX 6
	struct connection *conn = params;
	u16_t win_offset_m[OFFSET_M_MAX] = {0, };
	u8_t offset_m_max = OFFSET_M_MAX;
	u8_t offset_index_s = 0;
	u8_t has_offset_s = 0;
	u32_t ticks_to_offset;
	u16_t win_offset_s;

	ticks_to_offset = HAL_TICKER_US_TO_TICKS(conn->llcp_conn_param.offset0 *
						 1250);

	sched_free_win_offset_calc(conn, 1, &ticks_to_offset,
				   conn->llcp_conn_param.interval,
				   &offset_m_max, (u8_t *)&win_offset_m[0]);

	while (offset_index_s < OFFSET_S_MAX) {
		u8_t offset_index_m = 0;

		memcpy((u8_t *)&win_offset_s,
		       ((u8_t *)&conn->llcp_conn_param.offset0 +
			(sizeof(u16_t) * offset_index_s)), sizeof(u16_t));

		while (offset_index_m < offset_m_max) {
			if (win_offset_s != 0xffff) {
				if (win_offset_s ==
				    win_offset_m[offset_index_m]) {
					break;
				}

				has_offset_s = 1;
			}

			offset_index_m++;
		}

		if (offset_index_m < offset_m_max) {
			break;
		}

		offset_index_s++;
	}

	if (offset_index_s < OFFSET_S_MAX) {
		conn->llcp.conn_upd.win_offset_us =
			win_offset_s * 1250;
		memcpy(conn->llcp.conn_upd.pdu_win_offset,
		       &win_offset_s, sizeof(u16_t));
	} else if (!has_offset_s) {
		conn->llcp.conn_upd.win_offset_us =
			win_offset_m[0] * 1250;
		memcpy(conn->llcp.conn_upd.pdu_win_offset,
		       &win_offset_m[0], sizeof(u16_t));
	} else {
		struct pdu_data *pdu_ctrl_tx;

		/* procedure request acked */
		conn->llcp_ack = conn->llcp_req;

		/* CPR request acked */
		conn->llcp_conn_param.ack = conn->llcp_conn_param.req;

		/* reset mutex */
		_radio.conn_upd = NULL;

		/* send reject_ind_ext */
		pdu_ctrl_tx = (void *)
			((u8_t *)conn->llcp.conn_upd.pdu_win_offset -
			 offsetof(struct pdu_data,
				  llctrl.conn_update_ind.win_offset));
		pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
		pdu_ctrl_tx->len =
			offsetof(struct pdu_data_llctrl, reject_ext_ind) +
			sizeof(struct pdu_data_llctrl_reject_ext_ind);
		pdu_ctrl_tx->llctrl.opcode =
			PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND;
		pdu_ctrl_tx->llctrl.reject_ext_ind.reject_opcode =
			PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ;
		pdu_ctrl_tx->llctrl.reject_ext_ind.error_code =
			BT_HCI_ERR_UNSUPP_LL_PARAM_VAL;
	}
}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */
#endif /* CONFIG_BT_CTLR_SCHED_ADVANCED */

static void mayfly_radio_stop(void *params)
{
	enum state state = (enum state)((u32_t)params & 0xff);
	u32_t radio_used;

	LL_ASSERT((state == STATE_STOP) || (state == STATE_ABORT));

	radio_used = ((_radio.state != STATE_NONE) &&
		      (_radio.state != STATE_STOP) &&
		      (_radio.state != STATE_ABORT));
	if (radio_used || !radio_is_idle()) {
		if (radio_used) {
			_radio.state = state;
		}

		/** @todo try designing so as to not to abort tx packet */
		radio_disable();
	}
}

static void event_stop(u32_t ticks_at_expire, u32_t remainder,
		       u16_t lazy, void *context)
{
	static memq_link_t s_link;
	static struct mayfly s_mfy_radio_stop = {0, 0, &s_link, NULL,
						 mayfly_radio_stop};
	u32_t retval;

	ARG_UNUSED(ticks_at_expire);
	ARG_UNUSED(remainder);
	ARG_UNUSED(lazy);

	/* Radio state requested (stop or abort) stored in context is supplied
	 * in params.
	 */
	s_mfy_radio_stop.param = context;

	/* Stop Radio Tx/Rx */
	retval = mayfly_enqueue(RADIO_TICKER_USER_ID_WORKER,
				RADIO_TICKER_USER_ID_WORKER, 0,
				&s_mfy_radio_stop);
	LL_ASSERT(!retval);
}

static void event_common_prepare(u32_t ticks_at_expire,
				 u32_t remainder,
				 u32_t *ticks_xtal_to_start,
				 u32_t *ticks_active_to_start,
				 u32_t ticks_preempt_to_start,
				 u8_t ticker_id,
				 ticker_timeout_func ticker_timeout_fp,
				 void *context)
{
	u32_t ticker_status;
	u32_t _ticks_xtal_to_start = *ticks_xtal_to_start;
	u32_t _ticks_active_to_start = *ticks_active_to_start;
	u32_t ticks_to_start;

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
	/* in case this event is short prepare, xtal to start duration will be
	 * active to start duration.
	 */
	if (_ticks_xtal_to_start & XON_BITMASK) {
		_ticks_xtal_to_start = max(_ticks_active_to_start,
					   ticks_preempt_to_start);
	}
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */

	/* decide whether its XTAL start or active event that is the current
	 * execution context and accordingly setup the ticker for the other
	 * event (XTAL or active event). These are oneshot ticker.
	 */
	if (_ticks_active_to_start < _ticks_xtal_to_start) {
		u32_t ticks_to_active;

		/* XTAL is before Active */
		ticks_to_active = _ticks_xtal_to_start - _ticks_active_to_start;
		ticks_to_start = _ticks_xtal_to_start;

		ticker_status =
			ticker_start(RADIO_TICKER_INSTANCE_ID_RADIO,
				     RADIO_TICKER_USER_ID_WORKER,
				     RADIO_TICKER_ID_MARKER_0, ticks_at_expire,
				     ticks_to_active, TICKER_NULL_PERIOD,
				     TICKER_NULL_REMAINDER, TICKER_NULL_LAZY,
				     TICKER_NULL_SLOT, event_active, NULL,
				     ticker_success_assert, (void *)__LINE__);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY));

		event_xtal(0, 0, 0, NULL);
	} else if (_ticks_active_to_start > _ticks_xtal_to_start) {
		u32_t ticks_to_xtal;

		/* Active is before XTAL */
		ticks_to_xtal = _ticks_active_to_start - _ticks_xtal_to_start;
		ticks_to_start = _ticks_active_to_start;

		event_active(0, 0, 0, NULL);

		ticker_status =
			ticker_start(RADIO_TICKER_INSTANCE_ID_RADIO,
				     RADIO_TICKER_USER_ID_WORKER,
				     RADIO_TICKER_ID_MARKER_0, ticks_at_expire,
				     ticks_to_xtal, TICKER_NULL_PERIOD,
				     TICKER_NULL_REMAINDER, TICKER_NULL_LAZY,
				     TICKER_NULL_SLOT, event_xtal, NULL,
				     ticker_success_assert, (void *)__LINE__);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY));
	} else {
		/* Active and XTAL are at the same time,
		 * no ticker required to be setup.
		 */
		ticks_to_start = _ticks_xtal_to_start;

		event_active(0, 0, 0, NULL);
		event_xtal(0, 0, 0, NULL);
	}

	/* remember the remainder to be used in pkticker */
	_radio.remainder_anchor = remainder;

	/* setup the start ticker */
	ticker_status =
		ticker_start(RADIO_TICKER_INSTANCE_ID_RADIO,
			     RADIO_TICKER_USER_ID_WORKER, RADIO_TICKER_ID_EVENT,
			     ticks_at_expire, ticks_to_start,
			     TICKER_NULL_PERIOD, TICKER_NULL_REMAINDER,
			     TICKER_NULL_LAZY, TICKER_NULL_SLOT,
			     ticker_timeout_fp, context, ticker_success_assert,
			     (void *)__LINE__);
	LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
		  (ticker_status == TICKER_STATUS_BUSY));

#define RADIO_DEFERRED_PREEMPT 0
#if RADIO_DEFERRED_PREEMPT
	/* setup pre-empt ticker if any running state present */
	if (_radio.state != STATE_NONE) {
		ticker_status =
			ticker_start(RADIO_TICKER_INSTANCE_ID_RADIO,
				     RADIO_TICKER_USER_ID_WORKER,
				     RADIO_TICKER_ID_PRE_EMPT, ticks_at_expire,
				     (ticks_to_start - conn->hdr.ticks_preempt_to_start),
				     TICKER_NULL_PERIOD, TICKER_NULL_REMAINDER,
				     TICKER_NULL_LAZY, TICKER_NULL_SLOT,
				     event_stop, (void *)STATE_ABORT,
				     ticker_success_assert, (void *)__LINE__);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY));
	}
#else
	event_stop(0, 0, 0, (void *)STATE_ABORT);
#endif
#undef RADIO_DEFERRED_PREEMPT

	/** Handle change in _ticks_active_to_start */
	if (_radio.ticks_active_to_start != _ticks_active_to_start) {
		u32_t ticks_to_start_new;

		*ticks_active_to_start = _radio.ticks_active_to_start;

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
		if (*ticks_xtal_to_start & XON_BITMASK) {
			*ticks_xtal_to_start &= ~XON_BITMASK;
		}
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */

		ticks_to_start_new = max(_radio.ticks_active_to_start,
					 *ticks_xtal_to_start);

		/* drift the primary as required due to active line change */
		ticker_status =
			ticker_update(RADIO_TICKER_INSTANCE_ID_RADIO,
				      RADIO_TICKER_USER_ID_WORKER, ticker_id,
				      ticks_to_start, ticks_to_start_new,
				      ticks_to_start_new, ticks_to_start, 0, 0,
				      ticker_success_assert, (void *)__LINE__);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY));
	}

	/* route all packets queued for connections */
	packet_tx_enqueue(0xFF);

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
	/* calc whether xtal needs to be retained after this event */
	{
		static memq_link_t s_link;
		static struct mayfly s_mfy_xtal_stop_calc = {0, 0, &s_link, NULL,
			mayfly_xtal_stop_calc};
		u32_t retval;

		s_mfy_xtal_stop_calc.param = (void *)(u32_t)ticker_id;

		retval = mayfly_enqueue(RADIO_TICKER_USER_ID_WORKER,
					RADIO_TICKER_USER_ID_JOB, 1,
					&s_mfy_xtal_stop_calc);
		LL_ASSERT(!retval);
	}
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
}

static u8_t chan_sel_remap(u8_t *chan_map, u8_t chan_index)
{
	u8_t chan_next;
	u8_t byte_count;

	chan_next = 0;
	byte_count = 5;
	while (byte_count--) {
		u8_t bite;
		u8_t bit_count;

		bite = *chan_map;
		bit_count = 8;
		while (bit_count--) {
			if (bite & 0x01) {
				if (chan_index == 0) {
					break;
				}
				chan_index--;
			}
			chan_next++;
			bite >>= 1;
		}

		if (bit_count < 8) {
			break;
		}

		chan_map++;
	}

	return chan_next;
}

static u8_t chan_sel_1(u8_t *chan_use, u8_t hop, u16_t latency, u8_t *chan_map,
		       u8_t chan_count)
{
	u8_t chan_next;

	chan_next = ((*chan_use) + (hop * (1 + latency))) % 37;
	*chan_use = chan_next;

	if ((chan_map[chan_next >> 3] & (1 << (chan_next % 8))) == 0) {
		u8_t chan_index;

		chan_index = chan_next % chan_count;
		chan_next = chan_sel_remap(chan_map, chan_index);

	} else {
		/* channel can be used, return it */
	}

	return chan_next;
}

#if defined(CONFIG_BT_CTLR_CHAN_SEL_2)
static u8_t chan_rev_8(u8_t i)
{
	u8_t iterate;
	u8_t o;

	o = 0;
	for (iterate = 0; iterate < 8; iterate++) {
		o <<= 1;
		o |= (i & 1);
		i >>= 1;
	}

	return o;
}

static u16_t chan_perm(u16_t i)
{
	return (chan_rev_8((i >> 8) & 0xFF) << 8) | chan_rev_8(i & 0xFF);
}

static u16_t chan_mam(u16_t a, u16_t b)
{
	return ((u32_t)a * 17 + b) & 0xFFFF;
}

static u16_t chan_prn(u16_t counter, u16_t chan_id)
{
	u8_t iterate;
	u16_t prn_e;

	prn_e = counter ^ chan_id;

	for (iterate = 0; iterate < 3; iterate++) {
		prn_e = chan_perm(prn_e);
		prn_e = chan_mam(prn_e, chan_id);
	}

	prn_e ^= chan_id;

	return prn_e;
}

static u8_t chan_sel_2(u16_t counter, u16_t chan_id, u8_t *chan_map,
		       u8_t chan_count)
{
	u8_t chan_next;
	u16_t prn_e;

	prn_e = chan_prn(counter, chan_id);
	chan_next = prn_e % 37;

	if ((chan_map[chan_next >> 3] & (1 << (chan_next % 8))) == 0) {
		u8_t chan_index;

		chan_index = ((u32_t)chan_count * prn_e) >> 16;
		chan_next = chan_sel_remap(chan_map, chan_index);

	} else {
		/* channel can be used, return it */
	}

	return chan_next;
}

#if defined(RADIO_UNIT_TEST)
static void chan_sel_2_ut(void)
{
	u8_t chan_map_1[] = {0xFF, 0xFF, 0xFF, 0xFF, 0x1F};
	u8_t chan_map_2[] = {0x00, 0x06, 0xE0, 0x00, 0x1E};
	u8_t m;

	m = chan_sel_2(1, 0x305F, chan_map_1, 37);
	LL_ASSERT(m == 20);

	m = chan_sel_2(2, 0x305F, chan_map_1, 37);
	LL_ASSERT(m == 6);

	m = chan_sel_2(3, 0x305F, chan_map_1, 37);
	LL_ASSERT(m == 21);

	m = chan_sel_2(6, 0x305F, chan_map_2, 9);
	LL_ASSERT(m == 23);

	m = chan_sel_2(7, 0x305F, chan_map_2, 9);
	LL_ASSERT(m == 9);

	m = chan_sel_2(8, 0x305F, chan_map_2, 9);
	LL_ASSERT(m == 34);
}
#endif /* RADIO_UNIT_TEST */
#endif /* CONFIG_BT_CTLR_CHAN_SEL_2 */

static void chan_set(u32_t chan)
{
	switch (chan) {
	case 37:
		radio_freq_chan_set(2);
		break;

	case 38:
		radio_freq_chan_set(26);
		break;

	case 39:
		radio_freq_chan_set(80);
		break;

	default:
		if (chan < 11) {
			radio_freq_chan_set(4 + (2 * chan));
		} else if (chan < 40) {
			radio_freq_chan_set(28 + (2 * (chan - 11)));
		} else {
			LL_ASSERT(0);
		}
		break;
	}

	radio_whiten_iv_set(chan);
}

/** @brief Prepare access address as per BT Spec.
 *
 * - It shall have no more than six consecutive zeros or ones.
 * - It shall not be the advertising channel packets' Access Address.
 * - It shall not be a sequence that differs from the advertising channel
 *   packets Access Address by only one bit.
 * - It shall not have all four octets equal.
 * - It shall have no more than 24 transitions.
 * - It shall have a minimum of two transitions in the most significant six
 *   bits.
 *
 * LE Coded PHY requirements:
 * - It shall have at least three ones in the least significant 8 bits.
 * - It shall have no more than eleven transitions in the least significant 16
 *   bits.
 */
static u32_t access_addr_get(void)
{
#if defined(CONFIG_BT_CTLR_PHY_CODED)
	u8_t transitions_lsb16;
	u8_t ones_count_lsb8;
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	u8_t consecutive_cnt;
	u8_t consecutive_bit;
	u32_t adv_aa_check;
	u32_t access_addr;
	u8_t transitions;
	u8_t bit_idx;
	u8_t retry;

	retry = 3;
again:
	LL_ASSERT(retry);
	retry--;

	bt_rand(&access_addr, sizeof(u32_t));

	bit_idx = 31;
	transitions = 0;
	consecutive_cnt = 1;
#if defined(CONFIG_BT_CTLR_PHY_CODED)
	ones_count_lsb8 = 0;
	transitions_lsb16 = 0;
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	consecutive_bit = (access_addr >> bit_idx) & 0x01;
	while (bit_idx--) {
#if defined(CONFIG_BT_CTLR_PHY_CODED)
		u8_t transitions_lsb16_prev = transitions_lsb16;
#endif /* CONFIG_BT_CTLR_PHY_CODED */
		u8_t consecutive_cnt_prev = consecutive_cnt;
		u8_t transitions_prev = transitions;
		u8_t bit;

		bit = (access_addr >> bit_idx) & 0x01;
		if (bit == consecutive_bit) {
			consecutive_cnt++;
		} else {
			consecutive_cnt = 1;
			consecutive_bit = bit;
			transitions++;

#if defined(CONFIG_BT_CTLR_PHY_CODED)
			if (bit_idx < 15) {
				transitions_lsb16++;
			}
#endif /* CONFIG_BT_CTLR_PHY_CODED */
		}

#if defined(CONFIG_BT_CTLR_PHY_CODED)
		if ((bit_idx < 8) && bit) {
			ones_count_lsb8++;
		}
#endif /* CONFIG_BT_CTLR_PHY_CODED */

		/* It shall have no more than six consecutive zeros or ones. */
		/* It shall have a minimum of two transitions in the most
		 * significant six bits.
		 */
		if ((consecutive_cnt > 6) ||
#if defined(CONFIG_BT_CTLR_PHY_CODED)
		    (!bit && (((bit_idx < 6) && (ones_count_lsb8 < 1)) ||
			      ((bit_idx < 5) && (ones_count_lsb8 < 2)) ||
			      ((bit_idx < 4) && (ones_count_lsb8 < 3)))) ||
#endif /* CONFIG_BT_CTLR_PHY_CODED */
		    ((consecutive_cnt < 6) &&
		     (((bit_idx < 29) && (transitions < 1)) ||
		      ((bit_idx < 28) && (transitions < 2))))) {
			if (consecutive_bit) {
				consecutive_bit = 0;
				access_addr &= ~BIT(bit_idx);
#if defined(CONFIG_BT_CTLR_PHY_CODED)
				if (bit_idx < 8) {
					ones_count_lsb8--;
				}
#endif /* CONFIG_BT_CTLR_PHY_CODED */
			} else {
				consecutive_bit = 1;
				access_addr |= BIT(bit_idx);
#if defined(CONFIG_BT_CTLR_PHY_CODED)
				if (bit_idx < 8) {
					ones_count_lsb8++;
				}
#endif /* CONFIG_BT_CTLR_PHY_CODED */
			}

			if (transitions != transitions_prev) {
				consecutive_cnt = consecutive_cnt_prev;
				transitions = transitions_prev;
			} else {
				consecutive_cnt = 1;
				transitions++;
			}

#if defined(CONFIG_BT_CTLR_PHY_CODED)
			if (bit_idx < 15) {
				if (transitions_lsb16 !=
				    transitions_lsb16_prev) {
					transitions_lsb16 =
						transitions_lsb16_prev;
				} else {
					transitions_lsb16++;
				}
			}
#endif /* CONFIG_BT_CTLR_PHY_CODED */
		}

		/* It shall have no more than 24 transitions
		 * It shall have no more than eleven transitions in the least
		 * significant 16 bits.
		 */
		if ((transitions > 24) ||
#if defined(CONFIG_BT_CTLR_PHY_CODED)
		    (transitions_lsb16 > 11) ||
#endif /* CONFIG_BT_CTLR_PHY_CODED */
		    0) {
			if (consecutive_bit) {
				access_addr &= ~(BIT(bit_idx + 1) - 1);
			} else {
				access_addr |= (BIT(bit_idx + 1) - 1);
			}

			break;
		}
	}

	/* It shall not be the advertising channel packets Access Address.
	 * It shall not be a sequence that differs from the advertising channel
	 * packets Access Address by only one bit.
	 */
	adv_aa_check = access_addr ^ 0x8e89bed6;
	if (util_ones_count_get((u8_t *)&adv_aa_check,
				sizeof(adv_aa_check)) <= 1) {
		goto again;
	}

	/* It shall not have all four octets equal. */
	if (!((access_addr & 0xFFFF) ^ (access_addr >> 16)) &&
	    !((access_addr & 0xFF) ^ (access_addr >> 24))) {
		goto again;
	}

	return access_addr;
}

static void adv_scan_conn_configure(void)
{
	radio_reset();
	radio_tx_power_set(RADIO_TXP_DEFAULT);
	radio_isr_set(isr);
}

static void adv_scan_configure(u8_t phy, u8_t flags)
{
	u32_t aa = 0x8e89bed6;

	adv_scan_conn_configure();
	radio_phy_set(phy, flags);
	radio_aa_set((u8_t *)&aa);
	radio_pkt_configure(8, PDU_AC_PAYLOAD_SIZE_MAX, (phy << 1));
	radio_crc_configure(((0x5bUL) | ((0x06UL) << 8) | ((0x00UL) << 16)),
			    0x555555);
}

void radio_event_adv_prepare(u32_t ticks_at_expire, u32_t remainder,
			     u16_t lazy, void *context)
{
	ARG_UNUSED(lazy);
	ARG_UNUSED(context);

	DEBUG_RADIO_PREPARE_A(1);

	LL_ASSERT(!_radio.ticker_id_prepare);
	_radio.ticker_id_prepare = RADIO_TICKER_ID_ADV;

	event_common_prepare(ticks_at_expire, remainder,
			     &_radio.advertiser.hdr.ticks_xtal_to_start,
			     &_radio.advertiser.hdr.ticks_active_to_start,
			     _radio.advertiser.hdr.ticks_preempt_to_start,
			     RADIO_TICKER_ID_ADV, event_adv, NULL);

	DEBUG_RADIO_PREPARE_A(0);
}

static void adv_setup(void)
{
	struct pdu_adv *pdu;
	u8_t bitmap;
	u8_t chan;
	u8_t upd = 0;

	/* Use latest adv data PDU buffer */
	if (_radio.advertiser.adv_data.first !=
	    _radio.advertiser.adv_data.last) {
		u8_t first;

		first = _radio.advertiser.adv_data.first + 1;
		if (first == DOUBLE_BUFFER_SIZE) {
			first = 0;
		}
		_radio.advertiser.adv_data.first = first;
		upd = 1;
	}

	/* Use latest scan data PDU buffer */
	if (_radio.advertiser.scan_data.first != _radio.
	    advertiser.scan_data.last) {
		u8_t first;

		first = _radio.advertiser.scan_data.first + 1;
		if (first == DOUBLE_BUFFER_SIZE) {
			first = 0;
		}
		_radio.advertiser.scan_data.first = first;
		upd = 1;
	}

	pdu = (void *)_radio.advertiser.adv_data.data
					[_radio.advertiser.adv_data.first];
#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (upd) {
		struct pdu_adv *scan_pdu = (void *)
			_radio.advertiser.scan_data.data
					[_radio.advertiser.scan_data.first];

		/* Copy the address from the adv packet we will send into the
		 * scan response.
		 */
		memcpy(&scan_pdu->scan_rsp.addr[0],
		       &pdu->adv_ind.addr[0], BDADDR_SIZE);
	}
#else
	ARG_UNUSED(upd);
#endif /* !CONFIG_BT_CTLR_PRIVACY */

	radio_pkt_tx_set(pdu);

	if ((pdu->type != PDU_ADV_TYPE_NONCONN_IND) &&
	    (!IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT) ||
	     (pdu->type != PDU_ADV_TYPE_EXT_IND))) {
		_radio.state = STATE_TX;
		radio_tmr_tifs_set(RADIO_TIFS);
		radio_switch_complete_and_rx(0);
	} else {
		_radio.state = STATE_CLOSE;
		radio_switch_complete_and_disable();
	}

	bitmap = _radio.advertiser.chan_map_current;
	chan = 0;
	while ((bitmap & 0x01) == 0) {
		chan++;
		bitmap >>= 1;
	}
	_radio.advertiser.chan_map_current &=
		(_radio.advertiser.chan_map_current - 1);

	chan_set(37 + chan);
}

static void event_adv(u32_t ticks_at_expire, u32_t remainder,
		      u16_t lazy, void *context)
{
	u32_t remainder_us;

	ARG_UNUSED(remainder);
	ARG_UNUSED(lazy);
	ARG_UNUSED(context);

	DEBUG_RADIO_START_A(1);

	LL_ASSERT(_radio.role == ROLE_NONE);
	LL_ASSERT(_radio.ticker_id_prepare == RADIO_TICKER_ID_ADV);

	/** @todo check if XTAL is started,
	 * options 1: abort Radio Start,
	 * 2: wait for XTAL start.
	 */

	_radio.role = ROLE_ADV;
	_radio.ticker_id_prepare = 0;
	_radio.ticker_id_event = RADIO_TICKER_ID_ADV;
	_radio.ticks_anchor = ticks_at_expire;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	/* TODO: if coded we use S8? */
	adv_scan_configure(_radio.advertiser.phy_p, 1);
#else /* !CONFIG_BT_CTLR_ADV_EXT */
	adv_scan_configure(0, 0);
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

	_radio.advertiser.chan_map_current = _radio.advertiser.chan_map;
	adv_setup();

#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (ctrl_rl_enabled()) {
		struct ll_filter *filter =
			ctrl_filter_get(!!(_radio.advertiser.filter_policy));

		radio_filter_configure(filter->enable_bitmask,
				       filter->addr_type_bitmask,
				       (u8_t *)filter->bdaddr);
	} else
#endif /* CONFIG_BT_CTLR_PRIVACY */
	/* Setup Radio Filter */
	if (_radio.advertiser.filter_policy) {

		struct ll_filter *wl = ctrl_filter_get(true);

		radio_filter_configure(wl->enable_bitmask,
				       wl->addr_type_bitmask,
				       (u8_t *)wl->bdaddr);
	}

	remainder_us = radio_tmr_start(1, ticks_at_expire +
					  HAL_TICKER_US_TO_TICKS(
						RADIO_TICKER_START_PART_US),
				       _radio.remainder_anchor);

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

#if (defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
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
#if (RADIO_TICKER_USER_ID_WORKER_PRIO == RADIO_TICKER_USER_ID_JOB_PRIO)
		u32_t ticker_status;

		ticker_status =
		    ticker_job_idle_get(RADIO_TICKER_INSTANCE_ID_RADIO,
					RADIO_TICKER_USER_ID_WORKER,
					ticker_job_disable, NULL);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY));
#endif
	}

	DEBUG_RADIO_START_A(0);
}

static void mayfly_adv_stop(void *param)
{
	struct radio_le_conn_cmplt *radio_le_conn_cmplt;
	struct radio_pdu_node_rx *node_rx;
	struct pdu_data *pdu_data_rx;

	/* Prepare the rx packet structure */
	node_rx = packet_rx_reserve_get(1);
	LL_ASSERT(node_rx);

	/** Connection handle */
	node_rx->hdr.handle = 0xffff;
	node_rx->hdr.type = NODE_RX_TYPE_CONNECTION;

	/* prepare connection complete structure */
	pdu_data_rx = (void *)node_rx->pdu_data;
	radio_le_conn_cmplt = (void *)pdu_data_rx->lldata;
	(void)memset(radio_le_conn_cmplt, 0x00,
		     sizeof(struct radio_le_conn_cmplt));
	radio_le_conn_cmplt->status = BT_HCI_ERR_ADV_TIMEOUT;

	/* enqueue connection complete structure into queue */
	packet_rx_enqueue();
}

static inline void ticker_stop_adv_stop_active(void)
{
	static memq_link_t link_inact;
	static struct mayfly s_mfy_radio_inactive = {0, 0, &link_inact, NULL,
						     mayfly_radio_inactive};
	u32_t volatile ret_cb_evt = TICKER_STATUS_BUSY;
	u32_t ret;

	/* Step 2: Is caller before Event? Stop Event */
	ret = ticker_stop(RADIO_TICKER_INSTANCE_ID_RADIO,
			  RADIO_TICKER_USER_ID_JOB, RADIO_TICKER_ID_EVENT,
			  ticker_if_done, (void *)&ret_cb_evt);

	if (ret == TICKER_STATUS_BUSY) {
		mayfly_enable(RADIO_TICKER_USER_ID_JOB,
			      RADIO_TICKER_USER_ID_JOB, 1);

		while (ret_cb_evt == TICKER_STATUS_BUSY) {
			ticker_job_sched(RADIO_TICKER_INSTANCE_ID_RADIO,
					 RADIO_TICKER_USER_ID_JOB);
		}
	}

	if (ret_cb_evt == TICKER_STATUS_SUCCESS) {
		static memq_link_t link_xtal;
		static struct mayfly s_mfy_xtal_stop = {0, 0, &link_xtal, NULL,
							mayfly_xtal_stop};
		u32_t volatile ret_cb_m0 = TICKER_STATUS_BUSY;

		/* Reset the stored ticker id in prepare phase. */
		LL_ASSERT(_radio.ticker_id_prepare);
		_radio.ticker_id_prepare = 0;

		/* Step 2.1: Is caller between Primary and Marker0?
		 * Stop the Marker0 event
		 */
		ret = ticker_stop(RADIO_TICKER_INSTANCE_ID_RADIO,
				  RADIO_TICKER_USER_ID_JOB,
				  RADIO_TICKER_ID_MARKER_0,
				  ticker_if_done, (void *)&ret_cb_m0);

		if (ret == TICKER_STATUS_BUSY) {
			mayfly_enable(RADIO_TICKER_USER_ID_JOB,
				      RADIO_TICKER_USER_ID_JOB, 1);

			while (ret_cb_m0 == TICKER_STATUS_BUSY) {
				ticker_job_sched(RADIO_TICKER_INSTANCE_ID_RADIO,
						 RADIO_TICKER_USER_ID_JOB);
			}
		}

		if (ret_cb_m0 == TICKER_STATUS_SUCCESS) {
			u32_t ticks_xtal_to_start =
				_radio.advertiser.hdr.ticks_xtal_to_start;

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
			/* If in reduced prepare, use the absolute value */
			ticks_xtal_to_start &= ~XON_BITMASK;
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */

			/* Step 2.1.1: Check and deassert Radio Active or XTAL
			 * start
			 */
			if (_radio.advertiser.hdr.ticks_active_to_start >
			    ticks_xtal_to_start) {
				/* radio active asserted, handle deasserting
				 * here
				 */
				ret = mayfly_enqueue(
						RADIO_TICKER_USER_ID_JOB,
						RADIO_TICKER_USER_ID_WORKER, 0,
						&s_mfy_radio_inactive);
				LL_ASSERT(!ret);
			} else {
				/* XTAL started, handle XTAL stop here */
				ret = mayfly_enqueue(
						RADIO_TICKER_USER_ID_JOB,
						RADIO_TICKER_USER_ID_WORKER, 0,
						&s_mfy_xtal_stop);
				LL_ASSERT(!ret);
			}
		} else if (ret_cb_m0 == TICKER_STATUS_FAILURE) {
			/* Step 2.1.2: Deassert Radio Active and XTAL start */

			/* radio active asserted, handle deasserting here */
			ret = mayfly_enqueue(RADIO_TICKER_USER_ID_JOB,
					     RADIO_TICKER_USER_ID_WORKER, 0,
					     &s_mfy_radio_inactive);
			LL_ASSERT(!ret);

			/* XTAL started, handle XTAL stop here */
			ret = mayfly_enqueue(RADIO_TICKER_USER_ID_JOB,
					     RADIO_TICKER_USER_ID_WORKER, 0,
					     &s_mfy_xtal_stop);
			LL_ASSERT(!ret);
		} else {
			LL_ASSERT(0);
		}
	} else if (ret_cb_evt == TICKER_STATUS_FAILURE) {
		/* Step 3: Caller inside Event, handle graceful stop of Event
		 * (role dependent)
		 */
		if (_radio.role != ROLE_NONE) {
			static memq_link_t link_radio;
			static struct mayfly s_mfy_radio_stop = {
				0, 0, &link_radio, NULL, mayfly_radio_stop};

			/* Radio state STOP is supplied in params */
			s_mfy_radio_stop.param = (void *)STATE_STOP;

			/* Stop Radio Tx/Rx */
			ret = mayfly_enqueue(RADIO_TICKER_USER_ID_JOB,
						RADIO_TICKER_USER_ID_WORKER, 0,
						&s_mfy_radio_stop);
			LL_ASSERT(!ret);

			/* NOTE: Cannot wait here for the event to finish
			 * as we need to let radio ISR to execute if we are in
			 * the same priority.
			 */
		}
	} else {
		LL_ASSERT(0);
	}
}

static void ticker_stop_adv_stop(u32_t status, void *params)
{
	static memq_link_t s_link;
	static struct mayfly s_mfy_adv_stop = {0, 0, &s_link, NULL,
					       mayfly_adv_stop};
	u32_t retval;

	ARG_UNUSED(params);

	/* Ignore if being stopped from app/thread prio */
	if (status != TICKER_STATUS_SUCCESS) {
		LL_ASSERT(_radio.ticker_id_stop == RADIO_TICKER_ID_ADV);

		return;
	}

	/* Handle adv stop inside a prepare and/or event */
	if ((_radio.ticker_id_prepare == RADIO_TICKER_ID_ADV) ||
	    (_radio.ticker_id_event == RADIO_TICKER_ID_ADV)) {
		ticker_stop_adv_stop_active();
	}

	/* Generate the connection complete event in WORKER Prio */
	retval = mayfly_enqueue(RADIO_TICKER_USER_ID_JOB,
				RADIO_TICKER_USER_ID_WORKER, 0,
				&s_mfy_adv_stop);
	LL_ASSERT(!retval);
}

void event_adv_stop(u32_t ticks_at_expire, u32_t remainder, u16_t lazy,
		    void *context)
{
	u32_t ticker_status;

	ARG_UNUSED(ticks_at_expire);
	ARG_UNUSED(remainder);
	ARG_UNUSED(lazy);
	ARG_UNUSED(context);

	/* Abort an event, if any, to avoid Rx queue corruption used by Radio
	 * ISR.
	 */
	event_stop(0, 0, 0, (void *)STATE_ABORT);

	/* Stop Direct Adv */
	ticker_status =
	    ticker_stop(RADIO_TICKER_INSTANCE_ID_RADIO,
			RADIO_TICKER_USER_ID_WORKER, RADIO_TICKER_ID_ADV,
			ticker_stop_adv_stop, (void *)__LINE__);
	LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
		  (ticker_status == TICKER_STATUS_BUSY));
}

static void event_scan_prepare(u32_t ticks_at_expire, u32_t remainder,
			      u16_t lazy, void *context)
{
	ARG_UNUSED(lazy);
	ARG_UNUSED(context);

	DEBUG_RADIO_PREPARE_O(1);

	LL_ASSERT(!_radio.ticker_id_prepare);
	_radio.ticker_id_prepare = RADIO_TICKER_ID_SCAN;

	event_common_prepare(ticks_at_expire, remainder,
			     &_radio.scanner.hdr.ticks_xtal_to_start,
			     &_radio.scanner.hdr.ticks_active_to_start,
			     _radio.scanner.hdr.ticks_preempt_to_start,
			     RADIO_TICKER_ID_SCAN, event_scan, NULL);

#if defined(CONFIG_BT_CTLR_SCHED_ADVANCED)
	/* calc next group in us for the anchor where first connection event
	 * to be placed
	 */
	if (_radio.scanner.conn) {
		static memq_link_t s_link;
		static struct mayfly s_mfy_sched_after_mstr_free_offset_get = {
			0, 0, &s_link, NULL,
			mayfly_sched_after_mstr_free_offset_get};
		u32_t ticks_at_expire_normal = ticks_at_expire;
		u32_t retval;

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
		if (_radio.scanner.hdr.ticks_xtal_to_start & XON_BITMASK) {
			u32_t ticks_prepare_to_start =
				max(_radio.scanner.hdr.ticks_active_to_start,
				    _radio.scanner.hdr.ticks_preempt_to_start);

			ticks_at_expire_normal -=
				(_radio.scanner.hdr.ticks_xtal_to_start &
				 ~XON_BITMASK) - ticks_prepare_to_start;
		}
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */

		s_mfy_sched_after_mstr_free_offset_get.param =
			(void *)ticks_at_expire_normal;

		retval = mayfly_enqueue(RADIO_TICKER_USER_ID_WORKER,
				RADIO_TICKER_USER_ID_JOB, 1,
				&s_mfy_sched_after_mstr_free_offset_get);
		LL_ASSERT(!retval);
	}
#endif /* CONFIG_BT_CTLR_SCHED_ADVANCED */

	DEBUG_RADIO_PREPARE_O(0);
}

static void event_scan(u32_t ticks_at_expire, u32_t remainder, u16_t lazy,
		      void *context)
{
	u32_t remainder_us;
	u32_t ret;

	ARG_UNUSED(remainder);
	ARG_UNUSED(lazy);
	ARG_UNUSED(context);

	DEBUG_RADIO_START_O(1);

	LL_ASSERT(_radio.role == ROLE_NONE);
	LL_ASSERT(_radio.ticker_id_prepare == RADIO_TICKER_ID_SCAN);

	/** @todo check if XTAL is started, options 1: abort Radio Start,
	 * 2: wait for XTAL start
	 */
	_radio.role = ROLE_SCAN;
	_radio.state = STATE_RX;
	_radio.ticker_id_prepare = 0;
	_radio.ticker_id_event = RADIO_TICKER_ID_SCAN;
	_radio.ticks_anchor = ticks_at_expire;
	_radio.scanner.state = 0;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	adv_scan_configure(_radio.scanner.phy, 1); /* if coded then use S8. */
#else /* !CONFIG_BT_CTLR_ADV_EXT */
	adv_scan_configure(0, 0);
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

	chan_set(37 + _radio.scanner.chan++);
	if (_radio.scanner.chan == 3) {
		_radio.scanner.chan = 0;
	}

	radio_tmr_tifs_set(RADIO_TIFS);
	radio_switch_complete_and_tx(0, 0, 0, 0);
	radio_pkt_rx_set(_radio.packet_rx[_radio.packet_rx_last]->pdu_data);
	radio_rssi_measure();

#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (ctrl_rl_enabled()) {
		struct ll_filter *filter =
			ctrl_filter_get(!!(_radio.scanner.filter_policy & 0x1));
		u8_t count, *irks = ctrl_irks_get(&count);

		radio_filter_configure(filter->enable_bitmask,
				       filter->addr_type_bitmask,
				       (u8_t *)filter->bdaddr);

		radio_ar_configure(count, irks);
	} else
#endif /* CONFIG_BT_CTLR_PRIVACY */
	/* Setup Radio Filter */
	if (_radio.scanner.filter_policy) {

		struct ll_filter *wl = ctrl_filter_get(true);

		radio_filter_configure(wl->enable_bitmask,
				       wl->addr_type_bitmask,
				       (u8_t *)wl->bdaddr);
	}

	remainder_us = radio_tmr_start(0, ticks_at_expire +
					  HAL_TICKER_US_TO_TICKS(
						RADIO_TICKER_START_PART_US),
				       _radio.remainder_anchor);

	/* capture end of Rx-ed PDU, for initiator to calculate first
	 * master event.
	 */
	radio_tmr_end_capture();

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	radio_gpio_lna_setup();
	radio_gpio_pa_lna_enable(remainder_us +
				 radio_rx_ready_delay_get(0, 0) -
				 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#else /* !CONFIG_BT_CTLR_GPIO_LNA_PIN */
	ARG_UNUSED(remainder_us);
#endif /* !CONFIG_BT_CTLR_GPIO_LNA_PIN */

#if (defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
     (RADIO_TICKER_PREEMPT_PART_US <= RADIO_TICKER_PREEMPT_PART_MIN_US))
	/* check if preempt to start has changed */
	if (preempt_calc(&_radio.scanner.hdr, RADIO_TICKER_ID_SCAN,
			 ticks_at_expire) != 0) {
		_radio.state = STATE_STOP;
		radio_disable();
	} else
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
	{
		/* start window close timeout */
		ret = ticker_start(
				RADIO_TICKER_INSTANCE_ID_RADIO,
				RADIO_TICKER_USER_ID_WORKER,
				RADIO_TICKER_ID_SCAN_STOP, ticks_at_expire,
				_radio.scanner.ticks_window +
				HAL_TICKER_US_TO_TICKS(
					RADIO_TICKER_START_PART_US),
				TICKER_NULL_PERIOD, TICKER_NULL_REMAINDER,
				TICKER_NULL_LAZY, TICKER_NULL_SLOT,
				event_stop, (void *)STATE_STOP,
				ticker_success_assert, (void *)__LINE__);

		LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
			  (ret == TICKER_STATUS_BUSY));

		/* Ticker Job Silence */
#if (RADIO_TICKER_USER_ID_WORKER_PRIO == RADIO_TICKER_USER_ID_JOB_PRIO)
		ret = ticker_job_idle_get(RADIO_TICKER_INSTANCE_ID_RADIO,
					  RADIO_TICKER_USER_ID_WORKER,
					  ticker_job_disable, NULL);

		LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
			  (ret == TICKER_STATUS_BUSY));
#endif
	}

	DEBUG_RADIO_START_O(0);
}

static inline void event_conn_upd_init(struct connection *conn,
				       u16_t event_counter,
				       u32_t ticks_at_expire,
				       struct pdu_data *pdu_ctrl_tx,
				       struct mayfly *mayfly_sched_offset,
				       void (*fp_mayfly_select_or_use)(void *))
{
	/* move to in progress */
	conn->llcp.conn_upd.state = LLCP_CUI_STATE_INPROG;

	/* set instant */
	conn->llcp.conn_upd.instant =
		event_counter + conn->latency + 6;

	/* place the conn update req packet as next in tx queue */
	pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
	pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl, conn_update_ind) +
		sizeof(struct pdu_data_llctrl_conn_update_ind);
	pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND;
	pdu_ctrl_tx->llctrl.conn_update_ind.win_size =
		conn->llcp.conn_upd.win_size;
	pdu_ctrl_tx->llctrl.conn_update_ind.
		win_offset = conn->llcp.conn_upd.win_offset_us / 1250;
	pdu_ctrl_tx->llctrl.conn_update_ind.interval =
		conn->llcp.conn_upd.interval;
	pdu_ctrl_tx->llctrl.conn_update_ind.latency =
		conn->llcp.conn_upd.latency;
	pdu_ctrl_tx->llctrl.conn_update_ind.timeout =
		conn->llcp.conn_upd.timeout;
	pdu_ctrl_tx->llctrl.conn_update_ind.instant =
		conn->llcp.conn_upd.instant;

#if defined(CONFIG_BT_CTLR_SCHED_ADVANCED)
	{
		u32_t retval;

		/* calculate window offset that places the connection in the
		 * next available slot after existing masters.
		 */
		conn->llcp.conn_upd.ticks_anchor = ticks_at_expire;

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
		if (conn->hdr.ticks_xtal_to_start & XON_BITMASK) {
			u32_t ticks_prepare_to_start =
				max(conn->hdr.ticks_active_to_start,
				    conn->hdr.ticks_preempt_to_start);

			conn->llcp.conn_upd.ticks_anchor -=
				(conn->hdr.ticks_xtal_to_start &
				 ~XON_BITMASK) - ticks_prepare_to_start;
		}
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */

		conn->llcp.conn_upd.pdu_win_offset = (u16_t *)
			&pdu_ctrl_tx->llctrl.conn_update_ind.win_offset;

		mayfly_sched_offset->fp = fp_mayfly_select_or_use;
		mayfly_sched_offset->param = (void *)conn;

		retval = mayfly_enqueue(RADIO_TICKER_USER_ID_WORKER,
					RADIO_TICKER_USER_ID_JOB, 1,
					mayfly_sched_offset);
		LL_ASSERT(!retval);
	}
#else /* !CONFIG_BT_CTLR_SCHED_ADVANCED */
	ARG_UNUSED(ticks_at_expire);
	ARG_UNUSED(mayfly_sched_offset);
	ARG_UNUSED(fp_mayfly_select_or_use);
#endif /* !CONFIG_BT_CTLR_SCHED_ADVANCED */
}

static inline u32_t event_conn_upd_prep(struct connection *conn,
					u16_t event_counter,
					u32_t ticks_at_expire)
{
	struct connection *conn_upd;
	u16_t instant_latency;

	conn_upd = _radio.conn_upd;

	/* set mutex */
	if (!conn_upd) {
		_radio.conn_upd = conn;
	}

	instant_latency = (event_counter - conn->llcp.conn_upd.instant) &
			  0xffff;
	if (conn->llcp.conn_upd.state != LLCP_CUI_STATE_INPROG) {
#if defined(CONFIG_BT_CTLR_SCHED_ADVANCED)
		static memq_link_t s_link;
		static struct mayfly s_mfy_sched_offset = {0, 0,
			&s_link, 0, 0 };
		void (*fp_mayfly_select_or_use)(void *) = NULL;
#endif /* CONFIG_BT_CTLR_SCHED_ADVANCED */
		struct radio_pdu_node_tx *node_tx;
		struct pdu_data *pdu_ctrl_tx;

		node_tx = mem_acquire(&_radio.pkt_tx_ctrl_free);
		if (!node_tx) {
			return 1;
		}

		pdu_ctrl_tx = (void *)node_tx->pdu_data;

#if defined(CONFIG_BT_CTLR_SCHED_ADVANCED)
		switch (conn->llcp.conn_upd.state) {
		case LLCP_CUI_STATE_USE:
			fp_mayfly_select_or_use = mayfly_sched_win_offset_use;
			break;

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		case LLCP_CUI_STATE_SELECT:
			fp_mayfly_select_or_use =
				mayfly_sched_win_offset_select;
			break;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

		default:
			LL_ASSERT(0);
			break;
		}

		event_conn_upd_init(conn, event_counter, ticks_at_expire,
				    pdu_ctrl_tx, &s_mfy_sched_offset,
				    fp_mayfly_select_or_use);
#else /* !CONFIG_BT_CTLR_SCHED_ADVANCED */
		event_conn_upd_init(conn, event_counter, ticks_at_expire,
				    pdu_ctrl_tx, NULL, NULL);
#endif /* !CONFIG_BT_CTLR_SCHED_ADVANCED */

		ctrl_tx_enqueue(conn, node_tx);

	} else if (instant_latency <= 0x7FFF) {
		struct radio_pdu_node_rx *node_rx;
		struct pdu_data *pdu_data_rx;
		struct radio_le_conn_update_cmplt *radio_le_conn_update_cmplt;
		u32_t ticker_status;
		u32_t conn_interval_us;
		u32_t periodic_us;
		u32_t ticks_win_offset;
		u32_t ticks_slot_offset;
		u16_t conn_interval_old;
		u16_t conn_interval_new;
		u16_t latency;
		u32_t mayfly_was_enabled;

		/* procedure request acked */
		conn->llcp_ack = conn->llcp_req;

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		if ((conn->llcp_conn_param.req != conn->llcp_conn_param.ack) &&
		    (conn->llcp_conn_param.state == LLCP_CPR_STATE_UPD)) {
			conn->llcp_conn_param.ack = conn->llcp_conn_param.req;

			/* Stop procedure timeout */
			conn->procedure_expire = 0;
		}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */
		/* Reset ticker_id_prepare as role is not continued further
		 * due to conn update at this event.
		 */
		_radio.ticker_id_prepare = 0;

		/* reset mutex */
		if (_radio.conn_upd == conn) {
			_radio.conn_upd = NULL;
		}

		/* Prepare the rx packet structure */
		if ((conn->llcp.conn_upd.interval !=
		     conn->conn_interval) ||
		    (conn->llcp.conn_upd.latency != conn->latency) ||
		    (RADIO_CONN_EVENTS(conn->llcp.conn_upd.timeout *
				       10000,
				       conn->conn_interval *
				       1250) !=
		     conn->supervision_reload)) {
			node_rx = packet_rx_reserve_get(2);
			LL_ASSERT(node_rx);

			node_rx->hdr.handle = conn->handle;
			node_rx->hdr.type = NODE_RX_TYPE_CONN_UPDATE;

			/* prepare connection update complete structure */
			pdu_data_rx = (void *)node_rx->pdu_data;
			radio_le_conn_update_cmplt = (void *)
				pdu_data_rx->lldata;
			radio_le_conn_update_cmplt->status =
				0x00;
			radio_le_conn_update_cmplt->interval =
				conn->llcp.conn_upd.interval;
			radio_le_conn_update_cmplt->latency =
				conn->llcp.conn_upd.latency;
			radio_le_conn_update_cmplt->timeout =
				conn->llcp.conn_upd.timeout;

			/* enqueue connection update complete structure
			 * into queue.
			 */
			packet_rx_enqueue();
		}

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
		/* restore to normal prepare */
		if (conn->hdr.ticks_xtal_to_start & XON_BITMASK) {
			u32_t ticks_prepare_to_start =
				max(conn->hdr.ticks_active_to_start,
				    conn->hdr.ticks_preempt_to_start);

			conn->hdr.ticks_xtal_to_start &= ~XON_BITMASK;
			ticks_at_expire -= (conn->hdr.ticks_xtal_to_start -
					    ticks_prepare_to_start);
		}
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */

		/* compensate for instant_latency due to laziness */
		conn_interval_old = instant_latency * conn->conn_interval;
		latency = conn_interval_old /
			conn->llcp.conn_upd.interval;
		conn_interval_new = latency *
			conn->llcp.conn_upd.interval;
		if (conn_interval_new > conn_interval_old) {
			ticks_at_expire += HAL_TICKER_US_TO_TICKS(
				(conn_interval_new - conn_interval_old) * 1250);
		} else {
			ticks_at_expire -= HAL_TICKER_US_TO_TICKS(
				(conn_interval_old - conn_interval_new) * 1250);
		}
		conn->latency_prepare -= (instant_latency - latency);

		/* calculate the offset, window widening and interval */
		ticks_slot_offset = max(conn->hdr.ticks_active_to_start,
					conn->hdr.ticks_xtal_to_start);
		conn_interval_us = conn->llcp.conn_upd.interval * 1250;
		periodic_us = conn_interval_us;
		if (conn->role) {
			conn->slave.window_widening_prepare_us -=
				conn->slave.window_widening_periodic_us *
				instant_latency;

			conn->slave.window_widening_periodic_us =
				(((gc_lookup_ppm[_radio.sca] +
				   gc_lookup_ppm[conn->slave.sca]) *
				  conn_interval_us) + (1000000 - 1)) / 1000000;
			conn->slave.window_widening_max_us =
				(conn_interval_us >> 1) - RADIO_TIFS;
			conn->slave.window_size_prepare_us =
				conn->llcp.conn_upd.win_size * 1250;
			conn->slave.ticks_to_offset = 0;

			conn->slave.window_widening_prepare_us +=
				conn->slave.window_widening_periodic_us *
				latency;
			if (conn->slave.window_widening_prepare_us >
			    conn->slave.window_widening_max_us) {
				conn->slave.window_widening_prepare_us =
					conn->slave.window_widening_max_us;
			}

			ticks_at_expire -= HAL_TICKER_US_TO_TICKS(
				conn->slave.window_widening_periodic_us *
				latency);
			ticks_win_offset = HAL_TICKER_US_TO_TICKS(
				(conn->llcp.conn_upd.win_offset_us / 1250) *
				1250);
			periodic_us -= conn->slave.window_widening_periodic_us;
		} else {
			ticks_win_offset = HAL_TICKER_US_TO_TICKS(
				conn->llcp.conn_upd.win_offset_us);

			/* Workaround: Due to the missing remainder param in
			 * ticker_start function for first interval; add a
			 * tick so as to use the ceiled value.
			 */
			ticks_win_offset += 1;
		}
		conn->conn_interval = conn->llcp.conn_upd.interval;
		conn->latency = conn->llcp.conn_upd.latency;
		conn->supervision_reload =
			RADIO_CONN_EVENTS((conn->llcp.conn_upd.timeout
					   * 10 * 1000), conn_interval_us);
		conn->procedure_reload =
			RADIO_CONN_EVENTS((40 * 1000 * 1000), conn_interval_us);

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

		if (!conn->llcp.conn_upd.is_internal) {
			conn->supervision_expire = 0;
		}

		/* disable ticker job, in order to chain stop and start
		 * to avoid RTC being stopped if no tickers active.
		 */
		mayfly_was_enabled =
			mayfly_is_enabled(RADIO_TICKER_USER_ID_WORKER,
					  RADIO_TICKER_USER_ID_JOB);
		mayfly_enable(RADIO_TICKER_USER_ID_WORKER,
			      RADIO_TICKER_USER_ID_JOB, 0);

		/* start slave/master with new timings */
		ticker_status =
			ticker_stop(RADIO_TICKER_INSTANCE_ID_RADIO,
				    RADIO_TICKER_USER_ID_WORKER,
				    RADIO_TICKER_ID_FIRST_CONNECTION +
				    conn->handle, ticker_success_assert,
				    (void *)__LINE__);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY));
		ticker_status =
			ticker_start(RADIO_TICKER_INSTANCE_ID_RADIO,
				     RADIO_TICKER_USER_ID_WORKER,
				     RADIO_TICKER_ID_FIRST_CONNECTION +
				     conn->handle,
				     ticks_at_expire, ticks_win_offset,
				     HAL_TICKER_US_TO_TICKS(periodic_us),
				     HAL_TICKER_REMAINDER(periodic_us),
				     TICKER_NULL_LAZY,
				     (ticks_slot_offset + conn->hdr.ticks_slot),
				     conn->role ?
				     event_slave_prepare : event_master_prepare,
				     conn, ticker_success_assert,
				     (void *)__LINE__);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY));

		/* enable ticker job, if disabled in this function */
		if (mayfly_was_enabled) {
			mayfly_enable(RADIO_TICKER_USER_ID_WORKER,
				      RADIO_TICKER_USER_ID_JOB, 1);
		}

		return 0;
	}

	return 1;
}

static inline void event_ch_map_prep(struct connection *conn,
				     u16_t event_counter)
{
	if (conn->llcp.chan_map.initiate) {
		struct radio_pdu_node_tx *node_tx;

		node_tx = mem_acquire(&_radio.pkt_tx_ctrl_free);
		if (node_tx) {
			struct pdu_data *pdu_ctrl_tx = (void *)
						       node_tx->pdu_data;

			/* reset initiate flag */
			conn->llcp.chan_map.initiate = 0;

			/* set instant */
			conn->llcp.chan_map.instant =
				event_counter + conn->latency + 6;

			/* place the channel map req packet as next in
			 * tx queue
			 */
			pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
			pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl,
						    chan_map_ind) +
				sizeof(struct pdu_data_llctrl_chan_map_ind);
			pdu_ctrl_tx->llctrl.opcode =
				PDU_DATA_LLCTRL_TYPE_CHAN_MAP_IND;
			memcpy(&pdu_ctrl_tx->llctrl.chan_map_ind.chm[0],
			       &conn->llcp.chan_map.chm[0],
			       sizeof(pdu_ctrl_tx->llctrl.chan_map_ind.chm));
			pdu_ctrl_tx->llctrl.chan_map_ind.instant =
				conn->llcp.chan_map.instant;

			ctrl_tx_enqueue(conn, node_tx);
		}
	} else if (((event_counter - conn->llcp.chan_map.instant) & 0xFFFF)
			    <= 0x7FFF) {
		/* procedure request acked */
		conn->llcp_ack = conn->llcp_req;

		/* copy to active channel map */
		memcpy(&conn->data_chan_map[0],
		       &conn->llcp.chan_map.chm[0],
		       sizeof(conn->data_chan_map));
		conn->data_chan_count =
			util_ones_count_get(&conn->data_chan_map[0],
					    sizeof(conn->data_chan_map));
		conn->chm_update = 1;
	}

}

#if defined(CONFIG_BT_CTLR_LE_ENC)
static inline void event_enc_reject_prep(struct connection *conn,
					 struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;

	if (conn->common.fex_valid &&
	    (conn->llcp_features & BIT(BT_LE_FEAT_BIT_EXT_REJ_IND))) {
		struct pdu_data_llctrl_reject_ext_ind *p;

		pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND;

		p = (void *)&pdu->llctrl.reject_ext_ind;
		p->reject_opcode = PDU_DATA_LLCTRL_TYPE_ENC_REQ;
		p->error_code = conn->llcp.encryption.error_code;

		pdu->len = sizeof(struct pdu_data_llctrl_reject_ext_ind);
	} else {
		struct pdu_data_llctrl_reject_ind *p;

		pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_REJECT_IND;

		p = (void *)&pdu->llctrl.reject_ind;
		p->error_code =	conn->llcp.encryption.error_code;

		pdu->len = sizeof(struct pdu_data_llctrl_reject_ind);
	}

	pdu->len += offsetof(struct pdu_data_llctrl, reject_ind);

	conn->llcp.encryption.error_code = 0;
}

static inline void event_enc_prep(struct connection *conn)
{
	struct radio_pdu_node_tx *node_tx;
	struct pdu_data *pdu_ctrl_tx;

	if (conn->llcp.encryption.initiate) {
		return;
	}

	node_tx = mem_acquire(&_radio.pkt_tx_ctrl_free);
	if (!node_tx) {
		return;
	}

	pdu_ctrl_tx = (void *)node_tx->pdu_data;

	/* master sends encrypted enc start rsp in control priority */
	if (!conn->role) {
		/* calc the Session Key */
		ecb_encrypt(&conn->llcp.encryption.ltk[0],
			    &conn->llcp.encryption.skd[0],
			    NULL, &conn->ccm_rx.key[0]);

		/* copy the Session Key */
		memcpy(&conn->ccm_tx.key[0], &conn->ccm_rx.key[0],
		       sizeof(conn->ccm_tx.key));

		/* copy the IV */
		memcpy(&conn->ccm_tx.iv[0], &conn->ccm_rx.iv[0],
		       sizeof(conn->ccm_tx.iv));

		/* initialise counter */
		conn->ccm_rx.counter = 0;
		conn->ccm_tx.counter = 0;

		/* set direction: slave to master = 0,
		 * master to slave = 1
		 */
		conn->ccm_rx.direction = 0;
		conn->ccm_tx.direction = 1;

		/* enable receive encryption */
		conn->enc_rx = 1;

		/* send enc start resp */
		start_enc_rsp_send(conn, pdu_ctrl_tx);
	}

	/* slave send reject ind or start enc req at control priority */

#if defined(CONFIG_BT_CTLR_FAST_ENC)
	else {
#else /* !CONFIG_BT_CTLR_FAST_ENC */
	else if (!conn->pause_tx || conn->refresh) {
#endif /* !CONFIG_BT_CTLR_FAST_ENC */

		/* place the reject ind packet as next in tx queue */
		if (conn->llcp.encryption.error_code) {
			event_enc_reject_prep(conn, pdu_ctrl_tx);
		}
		/* place the start enc req packet as next in tx queue */
		else {

#if !defined(CONFIG_BT_CTLR_FAST_ENC)
			u8_t err;

			/* TODO BT Spec. text: may finalize the sending
			 * of additional data channel PDUs queued in the
			 * controller.
			 */
			err = enc_rsp_send(conn);
			if (err) {
				mem_release(node_tx, &_radio.pkt_tx_ctrl_free);

				return;
			}
#endif /* !CONFIG_BT_CTLR_FAST_ENC */

			/* calc the Session Key */
			ecb_encrypt(&conn->llcp.encryption.ltk[0],
				    &conn->llcp.encryption.skd[0], NULL,
				    &conn->ccm_rx.key[0]);

			/* copy the Session Key */
			memcpy(&conn->ccm_tx.key[0],
			       &conn->ccm_rx.key[0],
			       sizeof(conn->ccm_tx.key));

			/* copy the IV */
			memcpy(&conn->ccm_tx.iv[0], &conn->ccm_rx.iv[0],
			       sizeof(conn->ccm_tx.iv));

			/* initialise counter */
			conn->ccm_rx.counter = 0;
			conn->ccm_tx.counter = 0;

			/* set direction: slave to master = 0,
			 * master to slave = 1
			 */
			conn->ccm_rx.direction = 1;
			conn->ccm_tx.direction = 0;

			/* enable receive encryption (transmit turned
			 * on when start enc resp from master is
			 * received)
			 */
			conn->enc_rx = 1;

			/* prepare the start enc req */
			pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
			pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl,
						    start_enc_req) +
				sizeof(struct pdu_data_llctrl_start_enc_req);
			pdu_ctrl_tx->llctrl.opcode =
				PDU_DATA_LLCTRL_TYPE_START_ENC_REQ;
		}

#if !defined(CONFIG_BT_CTLR_FAST_ENC)
	} else {
		start_enc_rsp_send(_radio.conn_curr, pdu_ctrl_tx);

		/* resume data packet rx and tx */
		_radio.conn_curr->pause_rx = 0;
		_radio.conn_curr->pause_tx = 0;
#endif /* !CONFIG_BT_CTLR_FAST_ENC */

	}

	ctrl_tx_enqueue(conn, node_tx);

	/* procedure request acked */
	conn->llcp_ack = conn->llcp_req;
}
#endif /* CONFIG_BT_CTLR_LE_ENC */

static inline void event_fex_prep(struct connection *conn)
{
	struct radio_pdu_node_tx *node_tx;

	node_tx = mem_acquire(&_radio.pkt_tx_ctrl_free);
	if (node_tx) {
		struct pdu_data *pdu_ctrl_tx = (void *)node_tx->pdu_data;

		/* procedure request acked */
		conn->llcp_ack = conn->llcp_req;

		/* use initial feature bitmap */
		conn->llcp_features = RADIO_BLE_FEAT;

		/* place the feature exchange req packet as next in tx queue */
		pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
		pdu_ctrl_tx->len =
			offsetof(struct pdu_data_llctrl, feature_req) +
			sizeof(struct pdu_data_llctrl_feature_req);
		pdu_ctrl_tx->llctrl.opcode =
			!conn->role ?
			PDU_DATA_LLCTRL_TYPE_FEATURE_REQ :
			PDU_DATA_LLCTRL_TYPE_SLAVE_FEATURE_REQ;
		(void)memset(&pdu_ctrl_tx->llctrl.feature_req.features[0],
			     0x00,
			     sizeof(pdu_ctrl_tx->llctrl.feature_req.features));
		pdu_ctrl_tx->llctrl.feature_req.features[0] =
			conn->llcp_features & 0xFF;
		pdu_ctrl_tx->llctrl.feature_req.features[1] =
			(conn->llcp_features >> 8) & 0xFF;
		pdu_ctrl_tx->llctrl.feature_req.features[2] =
			(conn->llcp_features >> 16) & 0xFF;

		ctrl_tx_enqueue(conn, node_tx);

		/* Start Procedure Timeout (TODO: this shall not replace
		 * terminate procedure)
		 */
		conn->procedure_expire = conn->procedure_reload;
	}

}

static inline void event_vex_prep(struct connection *conn)
{

	if (conn->llcp_version.tx == 0) {
		struct radio_pdu_node_tx *node_tx;

		node_tx = mem_acquire(&_radio.pkt_tx_ctrl_free);
		if (node_tx) {
			struct pdu_data *pdu_ctrl_tx = (void *)
						       node_tx->pdu_data;

			/* procedure request acked */
			conn->llcp_ack = conn->llcp_req;

			/* set version ind tx-ed flag */
			conn->llcp_version.tx = 1;

			/* place the version ind packet as next in tx queue */
			pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
			pdu_ctrl_tx->len =
				offsetof(struct pdu_data_llctrl, version_ind) +
				sizeof(struct pdu_data_llctrl_version_ind);
			pdu_ctrl_tx->llctrl.opcode =
				PDU_DATA_LLCTRL_TYPE_VERSION_IND;
			pdu_ctrl_tx->llctrl.version_ind.version_number =
				RADIO_BLE_VERSION_NUMBER;
			pdu_ctrl_tx->llctrl.version_ind.company_id =
				RADIO_BLE_COMPANY_ID;
			pdu_ctrl_tx->llctrl.version_ind.sub_version_number =
				RADIO_BLE_SUB_VERSION_NUMBER;

			ctrl_tx_enqueue(conn, node_tx);

			/* Start Procedure Timeout (TODO: this shall not
			 * replace terminate procedure)
			 */
			conn->procedure_expire = conn->procedure_reload;
		}
	} else if (conn->llcp_version.rx != 0) {
		struct radio_pdu_node_rx *node_rx;
		struct pdu_data *pdu_ctrl_rx;

		/* procedure request acked */
		conn->llcp_ack = conn->llcp_req;

		/* Prepare the rx packet structure */
		node_rx = packet_rx_reserve_get(2);
		LL_ASSERT(node_rx);

		node_rx->hdr.handle = conn->handle;
		node_rx->hdr.type = NODE_RX_TYPE_DC_PDU;

		/* prepare version ind structure */
		pdu_ctrl_rx = (void *)node_rx->pdu_data;
		pdu_ctrl_rx->ll_id = PDU_DATA_LLID_CTRL;
		pdu_ctrl_rx->len =
			offsetof(struct pdu_data_llctrl, version_ind) +
			sizeof(struct pdu_data_llctrl_version_ind);
		pdu_ctrl_rx->llctrl.opcode =
			PDU_DATA_LLCTRL_TYPE_VERSION_IND;
		pdu_ctrl_rx->llctrl.version_ind.version_number =
			conn->llcp_version.version_number;
		pdu_ctrl_rx->llctrl.version_ind.company_id =
			conn->llcp_version.company_id;
		pdu_ctrl_rx->llctrl.version_ind.sub_version_number =
			conn->llcp_version.sub_version_number;

		/* enqueue version ind structure into rx queue */
		packet_rx_enqueue();
	} else {
		/* tx-ed but no rx, and new request placed */
		LL_ASSERT(0);
	}

}

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
static inline void event_conn_param_req(struct connection *conn,
					u16_t event_counter,
					u32_t ticks_at_expire)
{
	struct pdu_data_llctrl_conn_param_req *p;
	struct radio_pdu_node_tx *node_tx;
	struct pdu_data *pdu_ctrl_tx;

	node_tx = mem_acquire(&_radio.pkt_tx_ctrl_free);
	if (!node_tx) {
		return;
	}

	/* move to wait for conn_update/rsp/rej */
	conn->llcp_conn_param.state = LLCP_CPR_STATE_RSP_WAIT;

	/* place the conn param req packet as next in tx queue */
	pdu_ctrl_tx = (void *)node_tx->pdu_data;
	pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
	pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl, conn_param_req) +
		sizeof(struct pdu_data_llctrl_conn_param_req);
	pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ;
	p = (void *)&pdu_ctrl_tx->llctrl.conn_param_req;
	p->interval_min = conn->llcp_conn_param.interval;
	p->interval_max = conn->llcp_conn_param.interval;
	p->latency = conn->llcp_conn_param.latency;
	p->timeout = conn->llcp_conn_param.timeout;
	p->preferred_periodicity = 0;
	p->reference_conn_event_count = event_counter;
	p->offset0 = 0x0000;
	p->offset1 = 0xffff;
	p->offset2 = 0xffff;
	p->offset3 = 0xffff;
	p->offset4 = 0xffff;
	p->offset5 = 0xffff;

	ctrl_tx_enqueue(conn, node_tx);

	/* set CUI/CPR mutex */
	_radio.conn_upd = conn;

	/* Start Procedure Timeout (TODO: this shall not replace
	 * terminate procedure).
	 */
	conn->procedure_expire = conn->procedure_reload;

#if defined(CONFIG_BT_CTLR_SCHED_ADVANCED)
	{
		static memq_link_t s_link;
		static struct mayfly s_mfy_sched_offset = {0, 0, &s_link, NULL,
			mayfly_sched_free_win_offset_calc};
		u32_t retval;

		conn->llcp_conn_param.ticks_ref = ticks_at_expire;

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
		if (conn->hdr.ticks_xtal_to_start & XON_BITMASK) {
			u32_t ticks_prepare_to_start =
				max(conn->hdr.ticks_active_to_start,
				    conn->hdr.ticks_preempt_to_start);

			conn->llcp_conn_param.ticks_ref -=
				(conn->hdr.ticks_xtal_to_start &
				 ~XON_BITMASK) - ticks_prepare_to_start;
		}
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */

		conn->llcp_conn_param.pdu_win_offset0 = (u16_t *)&p->offset0;

		s_mfy_sched_offset.param = (void *)conn;

		retval = mayfly_enqueue(RADIO_TICKER_USER_ID_WORKER,
					RADIO_TICKER_USER_ID_JOB, 1,
					&s_mfy_sched_offset);
		LL_ASSERT(!retval);
	}
#else /* !CONFIG_BT_CTLR_SCHED_ADVANCED */
	ARG_UNUSED(ticks_at_expire);
#endif /* !CONFIG_BT_CTLR_SCHED_ADVANCED */
}

static inline void event_conn_param_rsp(struct connection *conn)
{
	struct pdu_data_llctrl_conn_param_rsp *rsp;
	struct radio_pdu_node_tx *node_tx;
	struct pdu_data *pdu;

	/* handle rejects */
	if (conn->llcp_conn_param.status) {
		struct pdu_data_llctrl_reject_ext_ind *rej;

		node_tx = mem_acquire(&_radio.pkt_tx_ctrl_free);
		if (!node_tx) {
			return;
		}

		/* master/slave response with reject ext ind */
		pdu = (void *)node_tx->pdu_data;
		pdu->ll_id = PDU_DATA_LLID_CTRL;
		pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND;
		pdu->len = offsetof(struct pdu_data_llctrl, reject_ext_ind) +
			   sizeof(struct pdu_data_llctrl_reject_ext_ind);

		rej = (void *)&pdu->llctrl.reject_ext_ind;
		rej->reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ;
		rej->error_code = conn->llcp_conn_param.status;

		ctrl_tx_enqueue(conn, node_tx);

		/* procedure request acked */
		conn->llcp_conn_param.ack = conn->llcp_conn_param.req;

		/* reset mutex */
		_radio.conn_upd = NULL;

		return;
	}

	/* master respond with connection update */
	if (!conn->role) {
		if (conn->llcp_req != conn->llcp_ack) {
			return;
		}

		/* Move to waiting for connection update completion */
		conn->llcp_conn_param.state = LLCP_CPR_STATE_UPD;

		/* Initiate connection update procedure */
		conn->llcp.conn_upd.win_size = 1;
		conn->llcp.conn_upd.win_offset_us = 0;
		if (conn->llcp_conn_param.preferred_periodicity) {
			conn->llcp.conn_upd.interval =
				((conn->llcp_conn_param.interval /
				  conn->llcp_conn_param.preferred_periodicity) +
				 1) *
				conn->llcp_conn_param.preferred_periodicity;
		} else {
			conn->llcp.conn_upd.interval =
				conn->llcp_conn_param.interval;
		}
		conn->llcp.conn_upd.latency = conn->llcp_conn_param.latency;
		conn->llcp.conn_upd.timeout = conn->llcp_conn_param.timeout;
		/* conn->llcp.conn_upd.instant     = 0; */
		conn->llcp.conn_upd.state = LLCP_CUI_STATE_SELECT;
		conn->llcp.conn_upd.is_internal = !conn->llcp_conn_param.cmd;
		conn->llcp_type = LLCP_CONN_UPD;
		conn->llcp_ack--;

		return;
	}

	/* slave response with connection parameter response */
	node_tx = mem_acquire(&_radio.pkt_tx_ctrl_free);
	if (!node_tx) {
		return;
	}

	/* place the conn param rsp packet as next in tx queue */
	pdu = (void *)node_tx->pdu_data;
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, conn_param_rsp) +
		sizeof(struct pdu_data_llctrl_conn_param_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP;
	rsp = (void *)&pdu->llctrl.conn_param_rsp;
	rsp->interval_min = conn->llcp_conn_param.interval;
	rsp->interval_max = conn->llcp_conn_param.interval;
	rsp->latency = conn->llcp_conn_param.latency;
	rsp->timeout = conn->llcp_conn_param.timeout;
	rsp->preferred_periodicity =
		conn->llcp_conn_param.preferred_periodicity;
	rsp->reference_conn_event_count =
		conn->llcp_conn_param.reference_conn_event_count;
	rsp->offset0 = conn->llcp_conn_param.offset0;
	rsp->offset1 = conn->llcp_conn_param.offset1;
	rsp->offset2 = conn->llcp_conn_param.offset2;
	rsp->offset3 = conn->llcp_conn_param.offset3;
	rsp->offset4 = conn->llcp_conn_param.offset4;
	rsp->offset5 = conn->llcp_conn_param.offset5;

	ctrl_tx_enqueue(conn, node_tx);

	/* procedure request acked */
	conn->llcp_conn_param.ack = conn->llcp_conn_param.req;

	/* reset mutex */
	_radio.conn_upd = NULL;
}

static inline void event_conn_param_app_req(struct connection *conn)
{
	struct pdu_data_llctrl_conn_param_req *p;
	struct radio_pdu_node_rx *node_rx;
	struct pdu_data *pdu;

	/* defer until encryption setup is complete */
	if (conn->pause_tx) {
		return;
	}

	/* wait for free rx buffer */
	node_rx = packet_rx_reserve_get(2);
	if (!node_rx) {
		return;
	}

	/* move to wait for conn_update/rsp/rej */
	conn->llcp_conn_param.state = LLCP_CPR_STATE_APP_WAIT;

	/* Emulate as Rx-ed CPR data channel PDU */
	node_rx->hdr.handle = conn->handle;
	node_rx->hdr.type = NODE_RX_TYPE_DC_PDU;

	/* place the conn param req packet as next in rx queue */
	pdu = (void *)node_rx->pdu_data;
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, conn_param_req) +
		sizeof(struct pdu_data_llctrl_conn_param_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ;
	p = (void *) &pdu->llctrl.conn_param_req;
	p->interval_min = conn->llcp_conn_param.interval;
	p->interval_max = conn->llcp_conn_param.interval;
	p->latency = conn->llcp_conn_param.latency;
	p->timeout = conn->llcp_conn_param.timeout;

	/* enqueue connection parameter request into rx queue */
	packet_rx_enqueue();
}

static inline void event_conn_param_prep(struct connection *conn,
					 u16_t event_counter,
					 u32_t ticks_at_expire)
{
	struct connection *conn_upd;

	conn_upd = _radio.conn_upd;
	if (conn_upd && (conn_upd != conn)) {
		return;
	}

	switch (conn->llcp_conn_param.state) {
	case LLCP_CPR_STATE_REQ:
		event_conn_param_req(conn, event_counter, ticks_at_expire);
		break;

	case LLCP_CPR_STATE_RSP:
		event_conn_param_rsp(conn);
		break;

	case LLCP_CPR_STATE_APP_REQ:
		event_conn_param_app_req(conn);
		break;

	case LLCP_CPR_STATE_APP_WAIT:
	case LLCP_CPR_STATE_RSP_WAIT:
	case LLCP_CPR_STATE_UPD:
		/* Do nothing */
		break;

	default:
		LL_ASSERT(0);
		break;
	}
}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_LE_PING)
static inline void event_ping_prep(struct connection *conn)
{
	struct radio_pdu_node_tx *node_tx;

	node_tx = mem_acquire(&_radio.pkt_tx_ctrl_free);
	if (node_tx) {
		struct pdu_data *pdu_ctrl_tx = (void *)node_tx->pdu_data;

		/* procedure request acked */
		conn->llcp_ack = conn->llcp_req;

		/* place the ping req packet as next in tx queue */
		pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
		pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl, ping_req) +
				   sizeof(struct pdu_data_llctrl_ping_req);
		pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PING_REQ;

		ctrl_tx_enqueue(conn, node_tx);

		/* Start Procedure Timeout (TODO: this shall not replace
		 * terminate procedure)
		 */
		conn->procedure_expire = conn->procedure_reload;
	}

}
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
static inline void event_len_prep(struct connection *conn)
{
	switch (conn->llcp_length.state) {
	case LLCP_LENGTH_STATE_REQ:
	{
		struct pdu_data_llctrl_length_req *lr;
		struct radio_pdu_node_tx *node_tx;
		struct pdu_data *pdu_ctrl_tx;
		u16_t free_count_rx;

		free_count_rx = packet_rx_acquired_count_get() +
			mem_free_count_get(_radio.pkt_rx_data_free);
		LL_ASSERT(free_count_rx <= 0xFF);

		if (_radio.packet_rx_data_count != free_count_rx) {
			break;
		}

		node_tx = mem_acquire(&_radio.pkt_tx_ctrl_free);
		if (!node_tx) {
			break;
		}

		/* wait for resp before completing the procedure */
		conn->llcp_length.state = LLCP_LENGTH_STATE_ACK_WAIT;

		/* set the default tx octets/time to requested value */
		conn->default_tx_octets = conn->llcp_length.tx_octets;

#if defined(CONFIG_BT_CTLR_PHY)
		conn->default_tx_time = conn->llcp_length.tx_time;
#endif /* CONFIG_BT_CTLR_PHY */

		/* place the length req packet as next in tx queue */
		pdu_ctrl_tx = (void *) node_tx->pdu_data;
		pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
		pdu_ctrl_tx->len =
			offsetof(struct pdu_data_llctrl, length_req) +
			sizeof(struct pdu_data_llctrl_length_req);
		pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_LENGTH_REQ;

		lr = &pdu_ctrl_tx->llctrl.length_req;
		lr->max_rx_octets = RADIO_LL_LENGTH_OCTETS_RX_MAX;
		lr->max_tx_octets = conn->default_tx_octets;
		lr->max_rx_time = RADIO_PKT_TIME(RADIO_LL_LENGTH_OCTETS_RX_MAX,
						 BIT(2));
#if !defined(CONFIG_BT_CTLR_PHY)
		lr->max_tx_time = RADIO_PKT_TIME(conn->default_tx_octets, 0);
#else /* CONFIG_BT_CTLR_PHY */
		lr->max_tx_time = conn->default_tx_time;
#endif /* CONFIG_BT_CTLR_PHY */

		ctrl_tx_enqueue(conn, node_tx);

		/* Start Procedure Timeout (TODO: this shall not replace
		 * terminate procedure).
		 */
		conn->procedure_expire = conn->procedure_reload;
	}
	break;

	case LLCP_LENGTH_STATE_RESIZE:
	{
		struct pdu_data_llctrl_length_rsp *lr;
		struct radio_pdu_node_rx *node_rx;
		struct pdu_data *pdu_ctrl_rx;
		u16_t packet_rx_data_size;
		u16_t free_count_conn;
		u16_t free_count_rx;

		/* Ensure the rx pool is not in use.
		 * This is important to be able to re-size the pool
		 * ensuring there is no chance that an operation on
		 * the pool is pre-empted causing memory corruption.
		 */
		free_count_rx = packet_rx_acquired_count_get() +
			mem_free_count_get(_radio.pkt_rx_data_free);
		LL_ASSERT(free_count_rx <= 0xFF);

		if (_radio.packet_rx_data_count != free_count_rx) {
			/** TODO another role instance has obtained
			 * memory from rx pool.
			 */
			LL_ASSERT(0);
		}

		/* Procedure complete */
		conn->llcp_length.ack = conn->llcp_length.req;
		conn->procedure_expire = 0;

		/* resume data packet tx */
		_radio.conn_curr->pause_tx = 0;

		/* Use the new rx octets/time in the connection */
		conn->max_rx_octets = conn->llcp_length.rx_octets;

#if defined(CONFIG_BT_CTLR_PHY)
		conn->max_rx_time = conn->llcp_length.rx_time;
#endif /* CONFIG_BT_CTLR_PHY */

		/** TODO This design is exception as memory initialization
		 * and allocation is done in radio context here, breaking the
		 * rule that the rx buffers are allocated in application
		 * context.
		 * Design mem_* such that mem_init could interrupt mem_acquire,
		 * when the pool is full?
		 */
		free_count_conn = mem_free_count_get(_radio.conn_free);
		if (_radio.advertiser.conn) {
			free_count_conn++;
		}
		if (_radio.scanner.conn) {
			free_count_conn++;
		}
		packet_rx_data_size = MROUND(offsetof(struct radio_pdu_node_rx,
						      pdu_data) +
					     offsetof(struct pdu_data,
						      lldata) +
					     conn->max_rx_octets);
		/* Resize to lower or higher size if this is the only active
		 * connection, or resize to only higher sizes as there may be
		 * other connections using the current size.
		 */
		if (((free_count_conn + 1) == _radio.connection_count) ||
		    (packet_rx_data_size > _radio.packet_rx_data_size)) {
			/* as rx mem is to be re-sized, release acquired
			 * memq link.
			 */
			while (_radio.packet_rx_acquire !=
				_radio.packet_rx_last) {
				if (_radio.packet_rx_acquire == 0) {
					_radio.packet_rx_acquire =
						_radio.packet_rx_count - 1;
				} else {
					_radio.packet_rx_acquire -= 1;
				}

				node_rx = _radio.packet_rx[
						_radio.packet_rx_acquire];
				mem_release(node_rx->hdr.onion.link,
					&_radio.link_rx_free);

				LL_ASSERT(_radio.link_rx_data_quota <
					  (_radio.packet_rx_count - 1));
				_radio.link_rx_data_quota++;

				/* no need to release node_rx as we mem_init
				 * later down in code.
				 */
			}

			/* calculate the new rx node size and new count */
			if (conn->max_rx_octets < (PDU_AC_SIZE_MAX +
						   PDU_AC_SIZE_EXTRA)) {
				_radio.packet_rx_data_size =
				    MROUND(offsetof(struct radio_pdu_node_rx,
						    pdu_data) +
					   (PDU_AC_SIZE_MAX +
					    PDU_AC_SIZE_EXTRA));
			} else {
				_radio.packet_rx_data_size =
					packet_rx_data_size;
			}
			_radio.packet_rx_data_count =
				_radio.packet_rx_data_pool_size /
				_radio.packet_rx_data_size;
			LL_ASSERT(_radio.packet_rx_data_count);

			/* re-size (re-init) the free rx pool */
			mem_init(_radio.pkt_rx_data_pool,
				 _radio.packet_rx_data_size,
				 _radio.packet_rx_data_count,
				 &_radio.pkt_rx_data_free);

			/* allocate the rx queue include one extra for
			 * generating event in following lines.
			 */
			packet_rx_allocate(4);
		}

		/* Prepare the rx packet structure */
		node_rx = packet_rx_reserve_get(2);
		LL_ASSERT(node_rx);
		node_rx->hdr.handle = conn->handle;
		node_rx->hdr.type = NODE_RX_TYPE_DC_PDU;

		/* prepare length rsp structure */
		pdu_ctrl_rx = (void *) node_rx->pdu_data;
		pdu_ctrl_rx->ll_id = PDU_DATA_LLID_CTRL;
		pdu_ctrl_rx->len =
			offsetof(struct pdu_data_llctrl, length_rsp) +
			sizeof(struct pdu_data_llctrl_length_rsp);
		pdu_ctrl_rx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_LENGTH_RSP;

		lr = &pdu_ctrl_rx->llctrl.length_rsp;
		lr->max_rx_octets = conn->max_rx_octets;
		lr->max_tx_octets = conn->max_tx_octets;
#if !defined(CONFIG_BT_CTLR_PHY)
		lr->max_rx_time = RADIO_PKT_TIME(conn->max_rx_octets, 0);
		lr->max_tx_time = RADIO_PKT_TIME(conn->max_tx_octets, 0);
#else /* CONFIG_BT_CTLR_PHY */
		lr->max_rx_time = conn->max_rx_time;
		lr->max_tx_time = conn->max_tx_time;
#endif /* CONFIG_BT_CTLR_PHY */

		/* enqueue version ind structure into rx queue */
		packet_rx_enqueue();
	}
	break;

	case LLCP_LENGTH_STATE_ACK_WAIT:
	case LLCP_LENGTH_STATE_RSP_WAIT:
		/* no nothing */
		break;

	default:
		LL_ASSERT(0);
		break;
	}
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
static inline void event_phy_req_prep(struct connection *conn)
{
	switch (conn->llcp_phy.state) {
	case LLCP_PHY_STATE_REQ:
	{
		struct pdu_data_llctrl_phy_req *pr;
		struct radio_pdu_node_tx *node_tx;
		struct pdu_data *pdu_ctrl_tx;

		node_tx = mem_acquire(&_radio.pkt_tx_ctrl_free);
		if (!node_tx) {
			break;
		}

		conn->llcp_phy.state = LLCP_PHY_STATE_ACK_WAIT;

		/* update preferred phy */
		conn->phy_pref_tx = conn->llcp_phy.tx;
		conn->phy_pref_rx = conn->llcp_phy.rx;
		conn->phy_pref_flags = conn->llcp_phy.flags;

		/* place the phy req packet as next in tx queue */
		pdu_ctrl_tx = (void *) node_tx->pdu_data;
		pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
		pdu_ctrl_tx->len =
			offsetof(struct pdu_data_llctrl, phy_req) +
			sizeof(struct pdu_data_llctrl_phy_req);
		pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PHY_REQ;

		pr = &pdu_ctrl_tx->llctrl.phy_req;
		pr->tx_phys = conn->llcp_phy.tx;
		pr->rx_phys = conn->llcp_phy.rx;

		ctrl_tx_enqueue(conn, node_tx);

		/* Start Procedure Timeout (TODO: this shall not replace
		 * terminate procedure).
		 */
		conn->procedure_expire = conn->procedure_reload;
	}
	break;

	case LLCP_PHY_STATE_UPD:
	{
		/* Procedure complete */
		conn->llcp_phy.ack = conn->llcp_phy.req;

		/* select only one tx phy, prefer 2M */
		if (conn->llcp_phy.tx & BIT(1)) {
			conn->llcp_phy.tx = BIT(1);
		} else if (conn->llcp_phy.tx & BIT(0)) {
			conn->llcp_phy.tx = BIT(0);
		} else if (conn->llcp_phy.tx & BIT(2)) {
			conn->llcp_phy.tx = BIT(2);
		} else {
			conn->llcp_phy.tx = 0;
		}

		/* select only one rx phy, prefer 2M */
		if (conn->llcp_phy.rx & BIT(1)) {
			conn->llcp_phy.rx = BIT(1);
		} else if (conn->llcp_phy.rx & BIT(0)) {
			conn->llcp_phy.rx = BIT(0);
		} else if (conn->llcp_phy.rx & BIT(2)) {
			conn->llcp_phy.rx = BIT(2);
		} else {
			conn->llcp_phy.rx = 0;
		}

		/* Initiate PHY Update Ind */
		if (conn->llcp_phy.tx != conn->phy_tx) {
			conn->llcp.phy_upd_ind.tx = conn->llcp_phy.tx;
		} else {
			conn->llcp.phy_upd_ind.tx = 0;
		}
		if (conn->llcp_phy.rx != conn->phy_rx) {
			conn->llcp.phy_upd_ind.rx = conn->llcp_phy.rx;
		} else {
			conn->llcp.phy_upd_ind.rx = 0;
		}
		/* conn->llcp.phy_upd_ind.instant = 0; */
		conn->llcp.phy_upd_ind.initiate = 1;
		conn->llcp.phy_upd_ind.cmd = conn->llcp_phy.cmd;

		conn->llcp_type = LLCP_PHY_UPD;
		conn->llcp_ack--;
	}
	break;

	case LLCP_PHY_STATE_ACK_WAIT:
	case LLCP_PHY_STATE_RSP_WAIT:
		/* no nothing */
		break;

	default:
		LL_ASSERT(0);
		break;
	}
}

static inline void event_phy_upd_ind_prep(struct connection *conn,
					  u16_t event_counter)
{
	struct radio_le_phy_upd_cmplt *upd;

	if (conn->llcp.phy_upd_ind.initiate) {
		struct radio_pdu_node_tx *node_tx;

		node_tx = mem_acquire(&_radio.pkt_tx_ctrl_free);
		if (node_tx) {
			struct pdu_data *pdu_ctrl_tx = (void *)
						       node_tx->pdu_data;
			struct pdu_data_llctrl_phy_upd_ind *ind;

			/* reset initiate flag */
			conn->llcp.phy_upd_ind.initiate = 0;

			/* Check if both tx and rx PHY unchanged */
			if (!((conn->llcp.phy_upd_ind.tx |
			       conn->llcp.phy_upd_ind.rx) & 0x07)) {
				/* Procedure complete */
				conn->llcp_ack = conn->llcp_req;

				/* 0 instant */
				conn->llcp.phy_upd_ind.instant = 0;

				/* generate phy update event */
				if (conn->llcp.phy_upd_ind.cmd) {
					struct radio_pdu_node_rx *node_rx;
					struct pdu_data *pdu_data;

					node_rx = packet_rx_reserve_get(2);
					LL_ASSERT(node_rx);

					node_rx->hdr.handle = conn->handle;
					node_rx->hdr.type =
						NODE_RX_TYPE_PHY_UPDATE;

					pdu_data = (void *)&node_rx->pdu_data;
					upd = (void *)pdu_data->lldata;
					upd->status = 0;
					upd->tx = conn->phy_tx;
					upd->rx = conn->phy_rx;

					packet_rx_enqueue();
				}
			} else {
				/* set instant */
				conn->llcp.phy_upd_ind.instant = event_counter +
								 conn->latency +
								 6;
			}

			/* place the phy update ind packet as next in
			 * tx queue
			 */
			pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
			pdu_ctrl_tx->len =
				offsetof(struct pdu_data_llctrl, phy_upd_ind) +
				sizeof(struct pdu_data_llctrl_phy_upd_ind);
			pdu_ctrl_tx->llctrl.opcode =
				PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND;
			ind = &pdu_ctrl_tx->llctrl.phy_upd_ind;
			ind->m_to_s_phy = conn->llcp.phy_upd_ind.tx;
			ind->s_to_m_phy = conn->llcp.phy_upd_ind.rx;
			ind->instant = conn->llcp.phy_upd_ind.instant;

			ctrl_tx_enqueue(conn, node_tx);
		}
	} else if (((event_counter - conn->llcp.phy_upd_ind.instant) & 0xFFFF)
			    <= 0x7FFF) {
		struct radio_pdu_node_rx *node_rx;
		struct pdu_data *pdu_data;
		u8_t old_tx, old_rx;

		/* procedure request acked */
		conn->llcp_ack = conn->llcp_req;

		/* apply new phy */
		old_tx = conn->phy_tx;
		old_rx = conn->phy_rx;
		if (conn->llcp.phy_upd_ind.tx) {
			conn->phy_tx = conn->llcp.phy_upd_ind.tx;
		}
		if (conn->llcp.phy_upd_ind.rx) {
			conn->phy_rx = conn->llcp.phy_upd_ind.rx;
		}
		conn->phy_flags = conn->phy_pref_flags;

		/* generate event if phy changed or initiated by cmd */
		if (!conn->llcp.phy_upd_ind.cmd && (conn->phy_tx == old_tx) &&
		    (conn->phy_rx == old_rx)) {
			return;
		}

		node_rx = packet_rx_reserve_get(2);
		LL_ASSERT(node_rx);

		node_rx->hdr.handle = conn->handle;
		node_rx->hdr.type = NODE_RX_TYPE_PHY_UPDATE;

		pdu_data = (void *)&node_rx->pdu_data;
		upd = (void *)pdu_data->lldata;
		upd->status = 0;
		upd->tx = conn->phy_tx;
		upd->rx = conn->phy_rx;

		packet_rx_enqueue();
	}
}
#endif /* CONFIG_BT_CTLR_PHY */

static void event_connection_prepare(u32_t ticks_at_expire,
				     u32_t remainder, u16_t lazy,
				     struct connection *conn)
{
	u16_t event_counter;

	LL_ASSERT(!_radio.ticker_id_prepare);
	_radio.ticker_id_prepare = RADIO_TICKER_ID_FIRST_CONNECTION +
				   conn->handle;

	/* Calc window widening */
	if (conn->role) {
		conn->slave.window_widening_prepare_us +=
		    conn->slave.window_widening_periodic_us * (lazy + 1);
		if (conn->slave.window_widening_prepare_us >
		    conn->slave.window_widening_max_us) {
			conn->slave.window_widening_prepare_us =
				conn->slave.window_widening_max_us;
		}
	}

	/* save the latency for use in event */
	conn->latency_prepare += lazy;

	/* calc current event counter value */
	event_counter = conn->event_counter + conn->latency_prepare;

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ) || defined(CONFIG_BT_CTLR_PHY)
	/* Check if no other procedure with instant is requested and not in
	 * Encryption setup.
	 */
	if ((conn->llcp_ack == conn->llcp_req) && !conn->pause_rx) {

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		/* check if CPR procedure is requested */
		if (conn->llcp_conn_param.ack != conn->llcp_conn_param.req) {
			/* Stop previous event, to avoid Radio DMA corrupting
			 * the rx queue.
			 */
			event_stop(0, 0, 0, (void *)STATE_ABORT);

			/* handle CPR state machine */
			event_conn_param_prep(conn, event_counter,
					      ticks_at_expire);
		}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_PHY)
		/* check if PHY Req procedure is requested */
		if (conn->llcp_phy.ack != conn->llcp_phy.req) {
			/* Stop previous event, to avoid Radio DMA corrupting
			 * the rx queue.
			 */
			event_stop(0, 0, 0, (void *)STATE_ABORT);

			/* handle PHY Upd state machine */
			event_phy_req_prep(conn);
		}
#endif /* CONFIG_BT_CTLR_PHY */

	}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ || CONFIG_BT_CTLR_PHY */

	/* check if procedure is requested */
	if (conn->llcp_ack != conn->llcp_req) {
		/* Stop previous event, to avoid Radio DMA corrupting the
		 * rx queue
		 */
		event_stop(0, 0, 0, (void *)STATE_ABORT);

		switch (conn->llcp_type) {
		case LLCP_CONN_UPD:
			if (event_conn_upd_prep(conn, event_counter,
						ticks_at_expire) == 0) {
				return;
			}
			break;
		case LLCP_CHAN_MAP:
			event_ch_map_prep(conn, event_counter);
			break;

#if defined(CONFIG_BT_CTLR_LE_ENC)
		case LLCP_ENCRYPTION:
			event_enc_prep(conn);
			break;
#endif /* CONFIG_BT_CTLR_LE_ENC */

		case LLCP_FEATURE_EXCHANGE:
			event_fex_prep(conn);
			break;

		case LLCP_VERSION_EXCHANGE:
			event_vex_prep(conn);
			break;

#if defined(CONFIG_BT_CTLR_LE_PING)
		case LLCP_PING:
			event_ping_prep(conn);
			break;
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CTLR_PHY)
		case LLCP_PHY_UPD:
			event_phy_upd_ind_prep(conn, event_counter);
			break;
#endif /* CONFIG_BT_CTLR_PHY */

		default:
			LL_ASSERT(0);
			break;
		}
	}

	/* Terminate Procedure Request */
	if (conn->llcp_terminate.ack != conn->llcp_terminate.req) {
		struct radio_pdu_node_tx *node_tx;

		/* Stop previous event, to avoid Radio DMA corrupting the rx
		 * queue
		 */
		event_stop(0, 0, 0, (void *)STATE_ABORT);

		node_tx = mem_acquire(&_radio.pkt_tx_ctrl_free);
		if (node_tx) {
			struct pdu_data *pdu_ctrl_tx = (void *)
						       node_tx->pdu_data;

			/* Terminate Procedure acked */
			conn->llcp_terminate.ack = conn->llcp_terminate.req;

			/* place the terminate ind packet in tx queue */
			pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
			pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl,
						    terminate_ind) +
				sizeof(struct pdu_data_llctrl_terminate_ind);
			pdu_ctrl_tx->llctrl.opcode =
				PDU_DATA_LLCTRL_TYPE_TERMINATE_IND;
			pdu_ctrl_tx->llctrl.terminate_ind.error_code =
				conn->llcp_terminate.reason_own;

			ctrl_tx_enqueue(conn, node_tx);

			/* Terminate Procedure timeout is started, will
			 * replace any other timeout running
			 */
			conn->procedure_expire = conn->supervision_reload;

			/* NOTE: if supervision timeout equals connection
			 * interval, dont timeout in current event.
			 */
			if (conn->procedure_expire <= 1) {
				conn->procedure_expire++;
			}
		}
	}

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	/* check if procedure is requested */
	if (conn->llcp_length.ack != conn->llcp_length.req) {
		/* Stop previous event, to avoid Radio DMA corrupting the
		 * rx queue
		 */
		event_stop(0, 0, 0, (void *)STATE_ABORT);

		/* handle DLU state machine */
		event_len_prep(conn);
	}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

	/* Setup XTAL startup and radio active events */
	event_common_prepare(ticks_at_expire, remainder,
			     &conn->hdr.ticks_xtal_to_start,
			     &conn->hdr.ticks_active_to_start,
			     conn->hdr.ticks_preempt_to_start,
			     (RADIO_TICKER_ID_FIRST_CONNECTION + conn->handle),
			     conn->role ? event_slave : event_master,
			     conn);

	/* store the next event counter value */
	conn->event_counter = event_counter + 1;
}

static void connection_configure(struct connection *conn)
{
	adv_scan_conn_configure();
	radio_aa_set(conn->access_addr);
	radio_crc_configure(((0x5bUL) | ((0x06UL) << 8) | ((0x00UL) << 16)),
			    (((u32_t)conn->crc_init[2] << 16) |
			     ((u32_t)conn->crc_init[1] << 8) |
			     ((u32_t)conn->crc_init[0])));
}

static void event_slave_prepare(u32_t ticks_at_expire, u32_t remainder,
				u16_t lazy, void *context)
{
	DEBUG_RADIO_PREPARE_S(1);

	event_connection_prepare(ticks_at_expire, remainder, lazy, context);

	DEBUG_RADIO_PREPARE_S(0);
}

static void event_slave(u32_t ticks_at_expire, u32_t remainder, u16_t lazy,
			void *context)
{
	struct connection *conn = context;
	u8_t data_chan_use = 0;
	u32_t remainder_us;
	u32_t hcto;

	ARG_UNUSED(remainder);
	ARG_UNUSED(lazy);

	DEBUG_RADIO_START_S(1);

	LL_ASSERT(_radio.role == ROLE_NONE);

	LL_ASSERT(_radio.ticker_id_prepare ==
		  (RADIO_TICKER_ID_FIRST_CONNECTION + conn->handle));

	_radio.role = ROLE_SLAVE;
	_radio.state = STATE_RX;
	_radio.ticker_id_prepare = 0;
	_radio.ticker_id_event =
		(RADIO_TICKER_ID_FIRST_CONNECTION + conn->handle);
	_radio.ticks_anchor = ticks_at_expire;
	_radio.packet_counter = 0;
	_radio.crc_expire = 0;

	_radio.conn_curr = conn;

	conn->latency_event = conn->latency_prepare;
	conn->latency_prepare = 0;

	connection_configure(conn);

	radio_tmr_tifs_set(RADIO_TIFS);

#if defined(CONFIG_BT_CTLR_PHY)
	radio_switch_complete_and_tx(conn->phy_rx, 0, conn->phy_tx,
				     conn->phy_flags);
#else /* !CONFIG_BT_CTLR_PHY */
	radio_switch_complete_and_tx(0, 0, 0, 0);
#endif /* !CONFIG_BT_CTLR_PHY */

	rx_packet_set(conn, (void *)
		      _radio.packet_rx[_radio.packet_rx_last]->pdu_data);

	/* Setup Radio Channel */
	if (conn->data_chan_sel) {
#if defined(CONFIG_BT_CTLR_CHAN_SEL_2)
		data_chan_use = chan_sel_2(conn->event_counter - 1,
					   conn->data_chan_id,
					   &conn->data_chan_map[0],
					   conn->data_chan_count);
#else /* !CONFIG_BT_CTLR_CHAN_SEL_2 */
		LL_ASSERT(0);
#endif /* !CONFIG_BT_CTLR_CHAN_SEL_2 */
	} else {
		data_chan_use = chan_sel_1(&conn->data_chan_use,
					   conn->data_chan_hop,
					   conn->latency_event,
					   &conn->data_chan_map[0],
					   conn->data_chan_count);
	}
	chan_set(data_chan_use);

	/* current window widening */
	conn->slave.window_widening_event_us +=
		conn->slave.window_widening_prepare_us;
	conn->slave.window_widening_prepare_us = 0;
	if (conn->slave.window_widening_event_us >
	    conn->slave.window_widening_max_us) {
		conn->slave.window_widening_event_us =
			conn->slave.window_widening_max_us;
	}

	/* current window size */
	conn->slave.window_size_event_us +=
		conn->slave.window_size_prepare_us;
	conn->slave.window_size_prepare_us = 0;

	remainder_us = radio_tmr_start(0, ticks_at_expire +
					  HAL_TICKER_US_TO_TICKS(
						RADIO_TICKER_START_PART_US),
				       _radio.remainder_anchor);

	radio_tmr_aa_capture();
	radio_tmr_aa_save(0);

	hcto = remainder_us + RADIO_TICKER_JITTER_US +
	       (RADIO_TICKER_JITTER_US << 2) +
	       (conn->slave.window_widening_event_us << 1) +
	       conn->slave.window_size_event_us;

#if defined(CONFIG_BT_CTLR_PHY)
	hcto += radio_rx_ready_delay_get(conn->phy_rx, 1);
	hcto += addr_us_get(conn->phy_rx);
	hcto += radio_rx_chain_delay_get(conn->phy_rx, 1);
#else /* !CONFIG_BT_CTLR_PHY */
	hcto += radio_rx_ready_delay_get(0, 0);
	hcto += addr_us_get(0);
	hcto += radio_rx_chain_delay_get(0, 0);
#endif /* !CONFIG_BT_CTLR_PHY */

	radio_tmr_hcto_configure(hcto);

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	radio_gpio_lna_setup();

#if defined(CONFIG_BT_CTLR_PHY)
	radio_gpio_pa_lna_enable(remainder_us +
				 radio_rx_ready_delay_get(conn->phy_rx, 1) -
				 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#else /* !CONFIG_BT_CTLR_PHY */
	radio_gpio_pa_lna_enable(remainder_us +
				 radio_rx_ready_delay_get(0, 0) -
				 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#endif /* !CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CTLR_GPIO_LNA_PIN */

#if defined(CONFIG_BT_CTLR_PROFILE_ISR) || \
    defined(CONFIG_BT_CTLR_GPIO_PA_PIN)
	radio_tmr_end_capture();
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
	radio_rssi_measure();
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

#if (defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
     (RADIO_TICKER_PREEMPT_PART_US <= RADIO_TICKER_PREEMPT_PART_MIN_US))
	/* check if preempt to start has changed */
	if (preempt_calc(&conn->hdr, (RADIO_TICKER_ID_FIRST_CONNECTION +
				      conn->handle), ticks_at_expire) != 0) {
		_radio.state = STATE_STOP;
		radio_disable();
	} else
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */

	{
	/* Ticker Job Silence */
#if (RADIO_TICKER_USER_ID_WORKER_PRIO == RADIO_TICKER_USER_ID_JOB_PRIO)
		u32_t ticker_status;

		ticker_status =
			ticker_job_idle_get(RADIO_TICKER_INSTANCE_ID_RADIO,
					    RADIO_TICKER_USER_ID_WORKER,
					    ticker_job_disable, NULL);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY));
#endif
	}

	/* Route the tx packet to respective connections */
	packet_tx_enqueue(2);

	DEBUG_RADIO_START_S(0);
}

static void event_master_prepare(u32_t ticks_at_expire, u32_t remainder,
				 u16_t lazy, void *context)
{
	DEBUG_RADIO_PREPARE_M(1);

	event_connection_prepare(ticks_at_expire, remainder, lazy, context);

	DEBUG_RADIO_PREPARE_M(0);
}

static void event_master(u32_t ticks_at_expire, u32_t remainder, u16_t lazy,
			 void *context)
{
	struct connection *conn = context;
	struct pdu_data *pdu_data_tx;
	u8_t data_chan_use = 0;
	u32_t remainder_us;

	ARG_UNUSED(remainder);
	ARG_UNUSED(lazy);

	DEBUG_RADIO_START_M(1);

	LL_ASSERT(_radio.role == ROLE_NONE);

	LL_ASSERT(_radio.ticker_id_prepare ==
		  (RADIO_TICKER_ID_FIRST_CONNECTION + conn->handle));

	_radio.role = ROLE_MASTER;
	_radio.state = STATE_TX;
	_radio.ticker_id_prepare = 0;
	_radio.ticker_id_event =
	    (RADIO_TICKER_ID_FIRST_CONNECTION + conn->handle);
	_radio.ticks_anchor = ticks_at_expire;
	_radio.packet_counter = 0;
	_radio.crc_expire = 0;

	_radio.conn_curr = conn;

	conn->latency_event = conn->latency_prepare;
	conn->latency_prepare = 0;

	/* Route the tx packet to respective connections */
	packet_tx_enqueue(2);

	/* prepare transmit packet */
	prepare_pdu_data_tx(conn, &pdu_data_tx);

	pdu_data_tx->sn = conn->sn;
	pdu_data_tx->nesn = conn->nesn;

	connection_configure(conn);

	radio_tmr_tifs_set(RADIO_TIFS);

#if defined(CONFIG_BT_CTLR_PHY)
	radio_switch_complete_and_rx(conn->phy_rx);
#else /* !CONFIG_BT_CTLR_PHY */
	radio_switch_complete_and_rx(0);
#endif /* !CONFIG_BT_CTLR_PHY */

	tx_packet_set(conn, pdu_data_tx);

	/* Setup Radio Channel */
	if (conn->data_chan_sel) {
#if defined(CONFIG_BT_CTLR_CHAN_SEL_2)
		data_chan_use = chan_sel_2(conn->event_counter - 1,
					   conn->data_chan_id,
					   &conn->data_chan_map[0],
					   conn->data_chan_count);
#else /* !CONFIG_BT_CTLR_CHAN_SEL_2 */
		LL_ASSERT(0);
#endif /* !CONFIG_BT_CTLR_CHAN_SEL_2 */
	} else {
		data_chan_use = chan_sel_1(&conn->data_chan_use,
					   conn->data_chan_hop,
					   conn->latency_event,
					   &conn->data_chan_map[0],
					   conn->data_chan_count);
	}
	chan_set(data_chan_use);

	/* normal connection! */
#if SILENT_CONNECTION
	if (!conn->empty || pdu_data_tx->md ||
	    (conn->supervision_expire && (conn->supervision_expire <= 6)) ||
	    (conn->connect_expire && (conn->connect_expire <= 6))) {
#endif
		remainder_us = radio_tmr_start(1,
					       ticks_at_expire +
					       HAL_TICKER_US_TO_TICKS(
						RADIO_TICKER_START_PART_US),
					       _radio.remainder_anchor);

		/* capture end of Tx-ed PDU, used to calculate HCTO. */
		radio_tmr_end_capture();

#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN)
		radio_gpio_pa_setup();

#if defined(CONFIG_BT_CTLR_PHY)
		radio_gpio_pa_lna_enable(remainder_us +
			radio_tx_ready_delay_get(conn->phy_tx,
						 conn->phy_flags) -
			CONFIG_BT_CTLR_GPIO_PA_OFFSET);
#else /* !CONFIG_BT_CTLR_PHY */
		radio_gpio_pa_lna_enable(remainder_us +
					 radio_tx_ready_delay_get(0, 0) -
					 CONFIG_BT_CTLR_GPIO_PA_OFFSET);
#endif /* !CONFIG_BT_CTLR_PHY */
#else /* !CONFIG_BT_CTLR_GPIO_PA_PIN */
		ARG_UNUSED(remainder_us);
#endif /* !CONFIG_BT_CTLR_GPIO_PA_PIN */

#if SILENT_CONNECTION
	/* silent connection! */
	} else {
		u32_t remainder_us;
		u32_t hcto;

		/* start in RX state */
		_radio.state = STATE_RX;
		_radio.packet_counter = 0xFF;

		radio_tmr_tifs_set(RADIO_TIFS);

#if defined(CONFIG_BT_CTLR_PHY)
		radio_switch_complete_and_tx(conn->phy_rx, 0, conn->phy_tx,
					     conn->phy_flags);
#else /* !CONFIG_BT_CTLR_PHY */
		radio_switch_complete_and_tx(0, 0, 0, 0);
#endif /* !CONFIG_BT_CTLR_PHY */

		rx_packet_set(conn, (void *)
			_radio.packet_rx[_radio.packet_rx_last]->pdu_data);

		/* setup pkticker and hcto */
		remainder_us = radio_tmr_start(0,
					       ticks_at_expire +
					       HAL_TICKER_US_TO_TICKS(
						RADIO_TICKER_START_PART_US),
					       _radio.remainder_anchor);

		radio_tmr_aa_capture();
		radio_tmr_aa_save(0);

		hcto = remainder_us + RADIO_TIFS;
#if defined(CONFIG_BT_CTLR_PHY)
		hcto += radio_tx_ready_delay_get(conn->phy_tx, conn->phy_flags);
		hcto += empty_pkt_us_get(conn->phy_rx);
		hcto += addr_us_get(conn->phy_rx);
#else /* !CONFIG_BT_CTLR_PHY */
		hcto += radio_tx_ready_delay_get(0, 0);
		hcto += empty_pkt_us_get(0);
		hcto += addr_us_get(0);
#endif /* !CONFIG_BT_CTLR_PHY */

		/* TODO: account for slave window widening */
		hcto += 256;

		radio_tmr_hcto_configure(hcto);

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
		radio_gpio_lna_setup();

#if defined(CONFIG_BT_CTLR_PHY)
		radio_gpio_pa_lna_enable(remainder_us +
			radio_rx_ready_delay_get(conn->phy_rx, 1) -
			CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#else /* !CONFIG_BT_CTLR_PHY */
		radio_gpio_pa_lna_enable(remainder_us +
					 radio_rx_ready_delay_get(0, 0) -
					 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#endif /* !CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CTLR_GPIO_LNA_PIN */
	}
#endif

#if (defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
     (RADIO_TICKER_PREEMPT_PART_US <= RADIO_TICKER_PREEMPT_PART_MIN_US))
	/* check if preempt to start has changed */
	if (0 !=
	    preempt_calc(&conn->hdr, (RADIO_TICKER_ID_FIRST_CONNECTION +
				      conn->handle), ticks_at_expire)) {
		_radio.state = STATE_STOP;
		radio_disable();
	} else
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */

	{
	/* Ticker Job Silence */
#if (RADIO_TICKER_USER_ID_WORKER_PRIO == RADIO_TICKER_USER_ID_JOB_PRIO)
		u32_t ticker_status;

		ticker_status =
			ticker_job_idle_get(RADIO_TICKER_INSTANCE_ID_RADIO,
					    RADIO_TICKER_USER_ID_WORKER,
					    ticker_job_disable, NULL);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY));
#endif
	}

	DEBUG_RADIO_START_M(0);
}

static void rx_packet_set(struct connection *conn, struct pdu_data *pdu_data_rx)
{
	u16_t max_rx_octets;
	u8_t phy;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	max_rx_octets = conn->max_rx_octets;
#else /* !CONFIG_BT_CTLR_DATA_LENGTH */
	max_rx_octets = RADIO_LL_LENGTH_OCTETS_RX_MIN;
#endif /* !CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
	phy = conn->phy_rx;
#else /* !CONFIG_BT_CTLR_PHY */
	phy = 0;
#endif /* !CONFIG_BT_CTLR_PHY */

	radio_phy_set(phy, 0);

	if (conn->enc_rx) {
		radio_pkt_configure(8, (max_rx_octets + 4), (phy << 1) | 0x01);

		radio_pkt_rx_set(radio_ccm_rx_pkt_set(&conn->ccm_rx, phy,
						      pdu_data_rx));
	} else {
		radio_pkt_configure(8, max_rx_octets, (phy << 1) | 0x01);

		radio_pkt_rx_set(pdu_data_rx);
	}
}

static void tx_packet_set(struct connection *conn, struct pdu_data *pdu_data_tx)
{
	u16_t max_tx_octets;
	u8_t phy, flags;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	max_tx_octets = conn->max_tx_octets;
#else /* !CONFIG_BT_CTLR_DATA_LENGTH */
	max_tx_octets = RADIO_LL_LENGTH_OCTETS_RX_MIN;
#endif /* !CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
	phy = conn->phy_tx;
	flags = conn->phy_flags;
#else /* !CONFIG_BT_CTLR_PHY */
	phy = 0;
	flags = 0;
#endif /* !CONFIG_BT_CTLR_PHY */

	radio_phy_set(phy, flags);

	if (conn->enc_tx) {
		radio_pkt_configure(8, (max_tx_octets + 4), (phy << 1) | 0x01);

		radio_pkt_tx_set(radio_ccm_tx_pkt_set(&conn->ccm_tx,
						      pdu_data_tx));
	} else {
		radio_pkt_configure(8, max_tx_octets, (phy << 1) | 0x01);

		radio_pkt_tx_set(pdu_data_tx);
	}
}

static void prepare_pdu_data_tx(struct connection *conn,
				struct pdu_data **pdu_data_tx)
{
	struct pdu_data *_pdu_data_tx;

	if (/* empty packet */
	    conn->empty ||
	    /* no ctrl or data packet */
	    !conn->pkt_tx_head ||
	    /* data tx paused, only control packets allowed */
	    (conn->pause_tx && (conn->pkt_tx_head != conn->pkt_tx_ctrl))) {
			_pdu_data_tx = empty_tx_enqueue(conn);
	} else {
		u16_t max_tx_octets;

		_pdu_data_tx = (void *)(conn->pkt_tx_head->pdu_data +
					conn->packet_tx_head_offset);

		if (!conn->packet_tx_head_len) {
			conn->packet_tx_head_len = _pdu_data_tx->len;
		}

		if (conn->packet_tx_head_offset) {
			_pdu_data_tx->ll_id = PDU_DATA_LLID_DATA_CONTINUE;
		}

		_pdu_data_tx->len = conn->packet_tx_head_len -
				    conn->packet_tx_head_offset;
		_pdu_data_tx->md = 0;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
#if defined(CONFIG_BT_CTLR_PHY)
		switch (conn->phy_tx_time) {
		default:
		case BIT(0):
			/* 1M PHY, 1us = 1 bit, hence divide by 8.
			 * Deduct 10 bytes for preamble (1), access address (4),
			 * header (2), and CRC (3).
			 */
			max_tx_octets = (conn->max_tx_time >> 3) - 10;
			break;

		case BIT(1):
			/* 2M PHY, 1us = 2 bits, hence divide by 4.
			 * Deduct 11 bytes for preamble (2), access address (4),
			 * header (2), and CRC (3).
			 */
			max_tx_octets = (conn->max_tx_time >> 2) - 11;
			break;

#if defined(CONFIG_BT_CTLR_PHY_CODED)
		case BIT(2):
			if (conn->phy_flags & 0x01) {
				/* S8 Coded PHY, 8us = 1 bit, hence divide by
				 * 64.
				 * Subtract time for preamble (80), AA (256),
				 * CI (16), TERM1 (24), CRC (192) and
				 * TERM2 (24), total 592 us.
				 * Subtract 2 bytes for header.
				 */
				max_tx_octets = ((conn->max_tx_time - 592) >>
						 6) - 2;
			} else {
				/* S2 Coded PHY, 2us = 1 bit, hence divide by
				 * 16.
				 * Subtract time for preamble (80), AA (256),
				 * CI (16), TERM1 (24), CRC (48) and
				 * TERM2 (6), total 430 us.
				 * Subtract 2 bytes for header.
				 */
				max_tx_octets = ((conn->max_tx_time - 430) >>
						 4) - 2;
			}
			break;
#endif /* CONFIG_BT_CTLR_PHY_CODED */
		}

		if (conn->enc_tx) {
			/* deduct the MIC */
			max_tx_octets -= 4;
		}

		if (max_tx_octets > conn->max_tx_octets) {
			max_tx_octets = conn->max_tx_octets;
		}
#else /* !CONFIG_BT_CTLR_PHY */
		max_tx_octets = conn->max_tx_octets;
#endif /* !CONFIG_BT_CTLR_PHY */
#else /* !CONFIG_BT_CTLR_DATA_LENGTH */
		max_tx_octets = RADIO_LL_LENGTH_OCTETS_RX_MIN;
#endif /* !CONFIG_BT_CTLR_DATA_LENGTH */

		if (_pdu_data_tx->len > max_tx_octets) {
			_pdu_data_tx->len = max_tx_octets;
			_pdu_data_tx->md = 1;
		}

		if (conn->pkt_tx_head->next) {
			_pdu_data_tx->md = 1;
		}

		if (!conn->pkt_tx_ctrl &&
		    (conn->pkt_tx_head != conn->pkt_tx_data)) {
			conn->pkt_tx_ctrl = conn->pkt_tx_ctrl_last =
				conn->pkt_tx_head;
		}
	}

	_pdu_data_tx->rfu = 0;

#if !defined(CONFIG_BT_CTLR_DATA_LENGTH_CLEAR)
	_pdu_data_tx->resv = 0;
#endif /* !CONFIG_BT_CTLR_DATA_LENGTH_CLEAR */

	*pdu_data_tx = _pdu_data_tx;
}

static void packet_rx_allocate(u8_t max)
{
	u8_t acquire;

	if (max > _radio.link_rx_data_quota) {
		max = _radio.link_rx_data_quota;
	}

	acquire = _radio.packet_rx_acquire + 1;
	if (acquire == _radio.packet_rx_count) {
		acquire = 0;
	}

	while ((max--) && (acquire != _radio.packet_rx_last)) {
		void *link;
		struct radio_pdu_node_rx *node_rx;

		link = mem_acquire(&_radio.link_rx_free);
		if (!link) {
			break;
		}

		node_rx = mem_acquire(&_radio.pkt_rx_data_free);
		if (!node_rx) {
			mem_release(link, &_radio.link_rx_free);
			break;
		}

		node_rx->hdr.onion.link = link;

		_radio.packet_rx[_radio.packet_rx_acquire] = node_rx;
		_radio.packet_rx_acquire = acquire;

		acquire = _radio.packet_rx_acquire + 1;
		if (acquire == _radio.packet_rx_count) {
			acquire = 0;
		}

		_radio.link_rx_data_quota--;
	}
}

static inline u8_t packet_rx_acquired_count_get(void)
{
	if (_radio.packet_rx_acquire >=
	    _radio.packet_rx_last) {
		return (_radio.packet_rx_acquire -
			_radio.packet_rx_last);
	} else {
		return (_radio.packet_rx_count -
			_radio.packet_rx_last +
			_radio.packet_rx_acquire);
	}
}

static inline struct radio_pdu_node_rx *packet_rx_reserve_get(u8_t count)
{
	if (count > packet_rx_acquired_count_get()) {
		return 0;
	}

	return _radio.packet_rx[_radio.packet_rx_last];
}

static void packet_rx_callback(void)
{
	/* Inline call of callback. If JOB configured as lower priority then
	 * callback will tailchain at end of every radio ISR. If JOB configured
	 * as same then call inline so as to have callback for every radio ISR.
	 */
#if (RADIO_TICKER_USER_ID_WORKER_PRIO == RADIO_TICKER_USER_ID_JOB_PRIO)
	radio_event_callback();
#else
	static memq_link_t s_link;
	static struct mayfly s_mfy_callback = {0, 0, &s_link, NULL,
		(void *)radio_event_callback};

	mayfly_enqueue(RADIO_TICKER_USER_ID_WORKER, RADIO_TICKER_USER_ID_JOB, 1,
		       &s_mfy_callback);
#endif
}

static void packet_rx_enqueue(void)
{
	struct radio_pdu_node_rx *node_rx;
	memq_link_t *link;
	u8_t last;

	LL_ASSERT(_radio.packet_rx_last != _radio.packet_rx_acquire);

	/* Remember the rx node and acquired link mem */
	node_rx = _radio.packet_rx[_radio.packet_rx_last];
	link = node_rx->hdr.onion.link;

	/* serialize release queue with rx queue by storing reference to last
	 * element in release queue
	 */
	node_rx->hdr.onion.packet_release_last = _radio.packet_release_last;

	/* dequeue from acquired rx queue */
	last = _radio.packet_rx_last + 1;
	if (last == _radio.packet_rx_count) {
		last = 0;
	}
	_radio.packet_rx_last = last;

	/* Enqueue into event-cum-data queue */
	link = memq_enqueue(link, node_rx, (void *)&_radio.link_rx_tail);
	LL_ASSERT(link);

	/* callback to trigger application action */
	packet_rx_callback();
}

static void packet_tx_enqueue(u8_t max)
{
	while ((max--) && (_radio.packet_tx_first != _radio.packet_tx_last)) {
		struct pdu_data_q_tx *pdu_data_q_tx;
		struct radio_pdu_node_tx *node_tx_new;
		struct connection *conn;
		u8_t first;

		pdu_data_q_tx = &_radio.pkt_tx[_radio.packet_tx_first];
		node_tx_new = pdu_data_q_tx->node_tx;
		node_tx_new->next = NULL;
		conn = mem_get(_radio.conn_pool, CONNECTION_T_SIZE,
				pdu_data_q_tx->handle);

		if (conn->handle == pdu_data_q_tx->handle) {
			if (conn->pkt_tx_data == 0) {
				conn->pkt_tx_data = node_tx_new;

				if (conn->pkt_tx_head == 0) {
					conn->pkt_tx_head = node_tx_new;
					conn->pkt_tx_last = NULL;
				}
			}

			if (conn->pkt_tx_last) {
				conn->pkt_tx_last->next = node_tx_new;
			}

			conn->pkt_tx_last = node_tx_new;
		} else {
			struct pdu_data *pdu_data_tx;

			pdu_data_tx = (void *)node_tx_new->pdu_data;

			/* By setting it resv, when app gets num cmplt, no
			 * num cmplt is counted, but the buffer is released
			 */
			pdu_data_tx->ll_id = PDU_DATA_LLID_RESV;

			pdu_node_tx_release(pdu_data_q_tx->handle, node_tx_new);
		}

		first = _radio.packet_tx_first + 1;
		if (first == _radio.packet_tx_count) {
			first = 0;
		}
		_radio.packet_tx_first = first;
	}
}

static struct pdu_data *empty_tx_enqueue(struct connection *conn)
{
	struct pdu_data *pdu_data_tx;

	conn->empty = 1;

	pdu_data_tx = (void *)radio_pkt_empty_get();
	pdu_data_tx->ll_id = PDU_DATA_LLID_DATA_CONTINUE;
	pdu_data_tx->len = 0;
	if (conn->pkt_tx_head) {
		pdu_data_tx->md = 1;
	} else {
		pdu_data_tx->md = 0;
	}

	return pdu_data_tx;
}

static void ctrl_tx_last_enqueue(struct connection *conn,
				 struct radio_pdu_node_tx *node_tx)
{
	node_tx->next = conn->pkt_tx_ctrl_last->next;
	conn->pkt_tx_ctrl_last->next = node_tx;
	conn->pkt_tx_ctrl_last = node_tx;
}

static void ctrl_tx_enqueue(struct connection *conn,
			    struct radio_pdu_node_tx *node_tx)
{
	/* check if a packet was tx-ed and not acked by peer */
	if (
	    /* An explicit empty PDU is not enqueued */
	    !conn->empty &&
	    /* and data/ctrl packet is in the head */
	    conn->pkt_tx_head &&
	    /* data PDU tx is not paused */
	    !conn->pause_tx) {
		/* data or ctrl may have been transmitted once, but not acked
		 * by peer, hence place this new ctrl after head
		 */

		/* if data transmited once, keep it at head of the tx list,
		 * as we will insert a ctrl after it, hence advance the
		 * data pointer
		 */
		if (conn->pkt_tx_head == conn->pkt_tx_data) {
			conn->pkt_tx_data = conn->pkt_tx_data->next;
		}

		/* if no ctrl packet already queued, new ctrl added will be
		 * the ctrl pointer and is inserted after head.
		 */
		if (!conn->pkt_tx_ctrl) {
			node_tx->next = conn->pkt_tx_head->next;
			conn->pkt_tx_head->next = node_tx;
			conn->pkt_tx_ctrl = node_tx;
			conn->pkt_tx_ctrl_last = node_tx;
		} else {
			ctrl_tx_last_enqueue(conn, node_tx);
		}
	} else {
		/* No packet needing ACK. */

		/* If first ctrl packet then add it as head else add it to the
		 * tail of the ctrl packets.
		 */
		if (!conn->pkt_tx_ctrl) {
			node_tx->next = conn->pkt_tx_head;
			conn->pkt_tx_head = node_tx;
			conn->pkt_tx_ctrl = node_tx;
			conn->pkt_tx_ctrl_last = node_tx;
		} else {
			ctrl_tx_last_enqueue(conn, node_tx);
		}
	}

	/* Update last pointer if ctrl added at end of tx list */
	if (node_tx->next == 0) {
		conn->pkt_tx_last = node_tx;
	}
}

static void ctrl_tx_sec_enqueue(struct connection *conn,
				  struct radio_pdu_node_tx *node_tx)
{
	if (conn->pause_tx) {
		if (!conn->pkt_tx_ctrl) {
			node_tx->next = conn->pkt_tx_head;
			conn->pkt_tx_head = node_tx;
		} else {
			node_tx->next = conn->pkt_tx_ctrl_last->next;
			conn->pkt_tx_ctrl_last->next = node_tx;
		}
	} else {
		ctrl_tx_enqueue(conn, node_tx);
	}
}

static void pdu_node_tx_release(u16_t handle,
				struct radio_pdu_node_tx *node_tx)
{
	u8_t last;

	last = _radio.packet_release_last + 1;
	if (last == _radio.packet_tx_count) {
		last = 0;
	}

	LL_ASSERT(last != _radio.packet_release_first);

	/* Enqueue app mem for release */
	_radio.pkt_release[_radio.packet_release_last].handle = handle;
	_radio.pkt_release[_radio.packet_release_last].node_tx = node_tx;
	_radio.packet_release_last = last;

	/* callback to trigger application action */
	packet_rx_callback();
}

static void connection_release(struct connection *conn)
{
	u32_t ticker_status;

	/* Enable Ticker Job, we are in a radio event which disabled it if
	 * worker0 and job0 priority where same.
	 */
	mayfly_enable(RADIO_TICKER_USER_ID_WORKER, RADIO_TICKER_USER_ID_JOB, 1);

	/** @todo correctly stop tickers ensuring crystal and radio active are
	 * placed in right states
	 */

	/* Stop Master/Slave role ticker */
	ticker_status =
		ticker_stop(RADIO_TICKER_INSTANCE_ID_RADIO,
			    RADIO_TICKER_USER_ID_WORKER,
			    (RADIO_TICKER_ID_FIRST_CONNECTION + conn->handle),
			    ticker_success_assert, (void *)__LINE__);
	LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
		  (ticker_status == TICKER_STATUS_BUSY));

	/* Stop Marker 0 and event single-shot tickers */
	if ((_radio.state == STATE_ABORT) &&
	    (_radio.ticker_id_prepare == (RADIO_TICKER_ID_FIRST_CONNECTION +
					  conn->handle))) {
		/* Reset the stored ticker id in prepare phase that
		 * initiated the connection event abort (pre-emption).
		 */
		_radio.ticker_id_prepare = 0;

		ticker_status =
			ticker_stop(RADIO_TICKER_INSTANCE_ID_RADIO,
				    RADIO_TICKER_USER_ID_WORKER,
				    RADIO_TICKER_ID_MARKER_0,
				    ticker_success_assert, (void *)__LINE__);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY));
		ticker_status =
		    ticker_stop(RADIO_TICKER_INSTANCE_ID_RADIO,
				RADIO_TICKER_USER_ID_WORKER,
				RADIO_TICKER_ID_EVENT,
				ticker_success_assert, (void *)__LINE__);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY));
	}

	/* flush and release, data packet before ctrl */
	while ((conn->pkt_tx_head != conn->pkt_tx_ctrl) &&
	       (conn->pkt_tx_head != conn->pkt_tx_data)) {
		struct radio_pdu_node_tx *node_tx;
		struct pdu_data *pdu_data_tx;

		/* By setting it resv, when app gets num cmplt, no num cmplt
		 * is counted, but the buffer is released
		 */
		node_tx = conn->pkt_tx_head;
		pdu_data_tx = (void *)node_tx->pdu_data;
		pdu_data_tx->ll_id = PDU_DATA_LLID_RESV;

		conn->pkt_tx_head = conn->pkt_tx_head->next;

		pdu_node_tx_release(conn->handle, node_tx);
	}

	/* flush and release, ctrl packet before data */
	while ((conn->pkt_tx_head) &&
	       (conn->pkt_tx_head != conn->pkt_tx_data)) {
		void *release;

		release = conn->pkt_tx_head;
		conn->pkt_tx_head = conn->pkt_tx_head->next;
		conn->pkt_tx_ctrl = conn->pkt_tx_head;

		mem_release(release, &_radio.pkt_tx_ctrl_free);
	}
	conn->pkt_tx_ctrl = NULL;
	conn->pkt_tx_ctrl_last = NULL;

	/* flush and release, rest of data */
	while (conn->pkt_tx_head) {
		struct radio_pdu_node_tx *node_tx;
		struct pdu_data *pdu_data_tx;

		/* By setting it resv, when app gets num cmplt, no num cmplt
		 * is counted, but the buffer is released
		 */
		node_tx = conn->pkt_tx_head;
		pdu_data_tx = (void *)node_tx->pdu_data;
		pdu_data_tx->ll_id = PDU_DATA_LLID_RESV;

		conn->pkt_tx_head = conn->pkt_tx_head->next;
		conn->pkt_tx_data = conn->pkt_tx_head;

		pdu_node_tx_release(conn->handle, node_tx);
	}

	conn->handle = 0xffff;

	/* reset mutex */
	if (_radio.conn_upd == conn) {
		_radio.conn_upd = NULL;
	}
}

static void terminate_ind_rx_enqueue(struct connection *conn, u8_t reason)
{
	struct radio_pdu_node_rx *node_rx;
	memq_link_t *link;

	/* Prepare the rx packet structure */
	node_rx = (void *)&conn->llcp_terminate.radio_pdu_node_rx;
	LL_ASSERT(node_rx->hdr.onion.link);

	node_rx->hdr.handle = conn->handle;
	node_rx->hdr.type = NODE_RX_TYPE_TERMINATE;
	*((u8_t *)node_rx->pdu_data) = reason;

	/* Get the link mem reserved in the connection context */
	link = node_rx->hdr.onion.link;

	/* Serialize release queue with rx queue by storing reference to
	 * last element in release queue
	 */
	node_rx->hdr.onion.packet_release_last = _radio.packet_release_last;

	/* Enqueue into event-cum-data queue */
	link = memq_enqueue(link, node_rx, (void *)&_radio.link_rx_tail);
	LL_ASSERT(link);

	/* callback to trigger application action */
	packet_rx_callback();
}

static u8_t conn_update(struct connection *conn, struct pdu_data *pdu_data_rx)
{
	if (((pdu_data_rx->llctrl.conn_update_ind.instant -
	      conn->event_counter) & 0xFFFF) > 0x7FFF) {
		return BT_HCI_ERR_INSTANT_PASSED;
	}

	/* different transaction collision */
	if (conn->llcp_req != conn->llcp_ack) {
		return BT_HCI_ERR_DIFF_TRANS_COLLISION;
	}

	/* set mutex, if only not already set. As a master the mutex shall
	 * be set, but a slave we accept it as new 'set' of mutex.
	 */
	if (_radio.conn_upd == 0) {
		LL_ASSERT(conn->role);

		_radio.conn_upd = conn;
	}

	conn->llcp.conn_upd.win_size =
		pdu_data_rx->llctrl.conn_update_ind.win_size;
	conn->llcp.conn_upd.win_offset_us =
		pdu_data_rx->llctrl.conn_update_ind.win_offset * 1250;
	conn->llcp.conn_upd.interval =
		pdu_data_rx->llctrl.conn_update_ind.interval;
	conn->llcp.conn_upd.latency =
		pdu_data_rx->llctrl.conn_update_ind.latency;
	conn->llcp.conn_upd.timeout =
		pdu_data_rx->llctrl.conn_update_ind.timeout;
	conn->llcp.conn_upd.instant =
		pdu_data_rx->llctrl.conn_update_ind.instant;
	conn->llcp.conn_upd.state = LLCP_CUI_STATE_INPROG;
	conn->llcp.conn_upd.is_internal = 0;

	conn->llcp_type = LLCP_CONN_UPD;
	conn->llcp_ack--;

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	if ((conn->llcp_conn_param.req != conn->llcp_conn_param.ack) &&
	    (conn->llcp_conn_param.state == LLCP_CPR_STATE_RSP_WAIT)) {
		conn->llcp_conn_param.ack = conn->llcp_conn_param.req;
	}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

	return 0;
}

#if defined (CONFIG_BT_CTLR_XTAL_ADVANCED) && \
    defined(CONFIG_BT_CTLR_SCHED_ADVANCED)
static u32_t conn_update_req(struct connection *conn)
{
	if (conn->llcp_req != conn->llcp_ack) {
		return 1;
	}

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	if (conn->llcp_conn_param.req != conn->llcp_conn_param.ack) {
		return 1;
	}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

	if (!conn->role) {
		conn->llcp.conn_upd.win_size = 1;
		conn->llcp.conn_upd.win_offset_us = 0;
		conn->llcp.conn_upd.interval = conn->conn_interval;
		conn->llcp.conn_upd.latency = conn->latency;
		conn->llcp.conn_upd.timeout = conn->conn_interval *
					      conn->supervision_reload *
					      125 / 1000;
		/* conn->llcp.conn_upd.instant     = 0; */
		conn->llcp.conn_upd.state = LLCP_CUI_STATE_USE;
		conn->llcp.conn_upd.is_internal = 1;

		conn->llcp_type = LLCP_CONN_UPD;
		conn->llcp_ack--;

		return 0;

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	} else if (!conn->llcp_conn_param.disabled &&
		   (!conn->common.fex_valid ||
		    (conn->llcp_features &
		     BIT(BT_LE_FEAT_BIT_CONN_PARAM_REQ)))) {
		/** Perform slave intiated conn param req */
		conn->llcp_conn_param.status = 0;
		conn->llcp_conn_param.interval = conn->conn_interval;
		conn->llcp_conn_param.latency = conn->latency;
		conn->llcp_conn_param.timeout = conn->conn_interval *
						conn->supervision_reload *
						125 / 1000;
		conn->llcp_conn_param.state = LLCP_CPR_STATE_REQ;
		conn->llcp_conn_param.cmd = 0;
		conn->llcp_conn_param.ack--;

		return 0;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

	}

	return 2;
}
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED && CONFIG_BT_CTLR_SCHED_ADVANCED */

static u32_t chan_map_update(struct connection *conn,
			     struct pdu_data *pdu_data_rx)
{
	if (((pdu_data_rx->llctrl.chan_map_ind.instant -
	      conn->event_counter) & 0xffff) > 0x7fff) {
		return 1;
	}

	LL_ASSERT(conn->llcp_req == conn->llcp_ack);

	memcpy(&conn->llcp.chan_map.chm[0],
	       &pdu_data_rx->llctrl.chan_map_ind.chm[0],
	       sizeof(conn->llcp.chan_map.chm));
	conn->llcp.chan_map.instant =
		pdu_data_rx->llctrl.chan_map_ind.instant;
	conn->llcp.chan_map.initiate = 0;

	conn->llcp_type = LLCP_CHAN_MAP;
	conn->llcp_ack--;

	return 0;
}

#if defined(CONFIG_BT_CTLR_PHY)
static inline u8_t phy_upd_ind_recv(struct radio_pdu_node_rx *node_rx,
				    u8_t *rx_enqueue)
{
	struct connection *conn = _radio.conn_curr;
	struct pdu_data_llctrl_phy_upd_ind *ind;
	struct pdu_data *pdu_data_rx;

	pdu_data_rx = (void *)node_rx->pdu_data;
	ind = &pdu_data_rx->llctrl.phy_upd_ind;

	/* Both tx and rx PHY unchanged */
	if (!((ind->m_to_s_phy | ind->s_to_m_phy) & 0x07)) {
		struct radio_le_phy_upd_cmplt *upd;

		/* Not in PHY Update Procedure or PDU in wrong state */
		if ((conn->llcp_phy.ack == conn->llcp_phy.req) ||
		    (conn->llcp_phy.state != LLCP_PHY_STATE_RSP_WAIT)) {
			return 0;
		}

		/* Procedure complete */
		conn->llcp_phy.ack = conn->llcp_phy.req;
		conn->procedure_expire = 0;

		/* Ignore event generation if not local cmd initiated */
		if (!conn->llcp_phy.cmd) {
			return 0;
		}

		/* generate phy update complete event */
		node_rx->hdr.type = NODE_RX_TYPE_PHY_UPDATE;

		upd = (void *)pdu_data_rx->lldata;
		upd->status = 0;
		upd->tx = conn->phy_tx;
		upd->rx = conn->phy_rx;

		/* enqueue the phy update complete */
		*rx_enqueue = 1;

		return 0;
	}

	/* instant passed */
	if (((ind->instant - conn->event_counter) & 0xffff) > 0x7fff) {
		return BT_HCI_ERR_INSTANT_PASSED;
	}

	/* different transaction collision */
	if (conn->llcp_req != conn->llcp_ack) {
		return BT_HCI_ERR_DIFF_TRANS_COLLISION;
	}

	if ((conn->llcp_phy.ack != conn->llcp_phy.req) &&
	    (conn->llcp_phy.state == LLCP_PHY_STATE_RSP_WAIT)) {
		conn->llcp_phy.ack = conn->llcp_phy.req;
		conn->llcp.phy_upd_ind.cmd = conn->llcp_phy.cmd;

		/* Procedure complete, just wait for instant */
		conn->procedure_expire = 0;
	}

	conn->llcp.phy_upd_ind.tx = ind->s_to_m_phy;
	conn->llcp.phy_upd_ind.rx = ind->m_to_s_phy;
	conn->llcp.phy_upd_ind.instant = ind->instant;
	conn->llcp.phy_upd_ind.initiate = 0;

	conn->llcp_type = LLCP_PHY_UPD;
	conn->llcp_ack--;

	if (conn->llcp.phy_upd_ind.tx) {
		conn->phy_tx_time = conn->llcp.phy_upd_ind.tx;
	}

	return 0;
}
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_LE_ENC)
static void enc_req_reused_send(struct connection *conn,
				struct radio_pdu_node_tx *node_tx)
{
	struct pdu_data *pdu_ctrl_tx;

	pdu_ctrl_tx = (void *)node_tx->pdu_data;
	pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
	pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl, enc_req) +
			   sizeof(struct pdu_data_llctrl_enc_req);
	pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_ENC_REQ;
	memcpy(&pdu_ctrl_tx->llctrl.enc_req.rand[0],
	       &conn->llcp.encryption.rand[0],
	       sizeof(pdu_ctrl_tx->llctrl.enc_req.rand));
	pdu_ctrl_tx->llctrl.enc_req.ediv[0] =
		conn->llcp.encryption.ediv[0];
	pdu_ctrl_tx->llctrl.enc_req.ediv[1] =
		conn->llcp.encryption.ediv[1];
	/* NOTE: if not sufficient random numbers, ignore waiting */
	entropy_nrf_get_entropy_isr(_radio.entropy, pdu_ctrl_tx->llctrl.enc_req.skdm,
				    sizeof(pdu_ctrl_tx->llctrl.enc_req.skdm));
	entropy_nrf_get_entropy_isr(_radio.entropy, pdu_ctrl_tx->llctrl.enc_req.ivm,
				    sizeof(pdu_ctrl_tx->llctrl.enc_req.ivm));
}

static u8_t enc_rsp_send(struct connection *conn)
{
	struct radio_pdu_node_tx *node_tx;
	struct pdu_data *pdu_ctrl_tx;

	/* acquire tx mem */
	node_tx = mem_acquire(&_radio.pkt_tx_ctrl_free);
	if (!node_tx) {
		return 1;
	}

	pdu_ctrl_tx = (void *)node_tx->pdu_data;
	pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
	pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl, enc_rsp) +
			   sizeof(struct pdu_data_llctrl_enc_rsp);
	pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_ENC_RSP;
	/* NOTE: if not sufficient random numbers, ignore waiting */
	entropy_nrf_get_entropy_isr(_radio.entropy, pdu_ctrl_tx->llctrl.enc_rsp.skds,
				    sizeof(pdu_ctrl_tx->llctrl.enc_rsp.skds));
	entropy_nrf_get_entropy_isr(_radio.entropy, pdu_ctrl_tx->llctrl.enc_rsp.ivs,
				    sizeof(pdu_ctrl_tx->llctrl.enc_rsp.ivs));

	/* things from slave stored for session key calculation */
	memcpy(&conn->llcp.encryption.skd[8],
	       &pdu_ctrl_tx->llctrl.enc_rsp.skds[0], 8);
	memcpy(&conn->ccm_rx.iv[4],
	       &pdu_ctrl_tx->llctrl.enc_rsp.ivs[0], 4);

	ctrl_tx_enqueue(conn, node_tx);

	return 0;
}

static u8_t start_enc_rsp_send(struct connection *conn,
			       struct pdu_data *pdu_ctrl_tx)
{
	struct radio_pdu_node_tx *node_tx = NULL;

	if (!pdu_ctrl_tx) {
		/* acquire tx mem */
		node_tx = mem_acquire(&_radio.pkt_tx_ctrl_free);
		if (!node_tx) {
			return 1;
		}

		pdu_ctrl_tx = (void *)node_tx->pdu_data;
	}

	/* enable transmit encryption */
	conn->enc_tx = 1;

	pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
	pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl, enc_rsp);
	pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_START_ENC_RSP;

	if (node_tx) {
		ctrl_tx_enqueue(conn, node_tx);
	}

	return 0;
}

static u8_t pause_enc_rsp_send(struct connection *conn, u8_t req)
{
	struct radio_pdu_node_tx *node_tx;
	struct pdu_data *pdu_ctrl_tx;

	if (req) {
		/* acquire tx mem */
		node_tx = mem_acquire(&_radio.pkt_tx_ctrl_free);
		if (!node_tx) {
			return 1;
		}

		/* key refresh */
		conn->refresh = 1;
	} else if (!conn->role) {
		/* acquire tx mem */
		node_tx = mem_acquire(&_radio.pkt_tx_ctrl_free);
		if (!node_tx) {
			return 1;
		}

		/* disable transmit encryption */
		conn->enc_tx = 0;
	} else {
		/* disable transmit encryption */
		conn->enc_tx = 0;

		return 0;
	}

	/* pause data packet rx */
	conn->pause_rx = 1;

	/* disable receive encryption */
	conn->enc_rx = 0;

	/* Enqueue pause enc rsp */
	pdu_ctrl_tx = (void *)node_tx->pdu_data;
	pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
	pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl, enc_rsp);
	pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP;

	ctrl_tx_enqueue(conn, node_tx);

	return 0;
}
#endif /* CONFIG_BT_CTLR_LE_ENC */

static u8_t unknown_rsp_send(struct connection *conn, u8_t type)
{
	struct radio_pdu_node_tx *node_tx;
	struct pdu_data *pdu_ctrl_tx;

	/* acquire tx mem */
	node_tx = mem_acquire(&_radio.pkt_tx_ctrl_free);
	if (!node_tx) {
		return 1;
	}

	pdu_ctrl_tx = (void *)node_tx->pdu_data;
	pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
	pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl, unknown_rsp) +
			   sizeof(struct pdu_data_llctrl_unknown_rsp);
	pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP;
	pdu_ctrl_tx->llctrl.unknown_rsp.type = type;

	ctrl_tx_enqueue(conn, node_tx);

	return 0;
}

static u8_t feature_rsp_send(struct connection *conn,
			     struct pdu_data *pdu_data_rx)
{
	struct pdu_data_llctrl_feature_req *req;
	struct radio_pdu_node_tx *node_tx;
	struct pdu_data *pdu_ctrl_tx;

	/* acquire tx mem */
	node_tx = mem_acquire(&_radio.pkt_tx_ctrl_free);
	if (!node_tx) {
		return 1;
	}

	/* AND the feature set to get Feature USED */
	req = &pdu_data_rx->llctrl.feature_req;
	_radio.conn_curr->llcp_features &= feat_get(&req->features[0]);

	/* features exchanged */
	_radio.conn_curr->common.fex_valid = 1;

	/* Enqueue feature response */
	pdu_ctrl_tx = (void *)node_tx->pdu_data;
	pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
	pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl, feature_rsp) +
		sizeof(struct pdu_data_llctrl_feature_rsp);
	pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_FEATURE_RSP;
	(void)memset(&pdu_ctrl_tx->llctrl.feature_rsp.features[0], 0x00,
		     sizeof(pdu_ctrl_tx->llctrl.feature_rsp.features));
	pdu_ctrl_tx->llctrl.feature_req.features[0] =
		conn->llcp_features & 0xFF;
	pdu_ctrl_tx->llctrl.feature_req.features[1] =
		(conn->llcp_features >> 8) & 0xFF;
	pdu_ctrl_tx->llctrl.feature_req.features[2] =
		(conn->llcp_features >> 16) & 0xFF;

	ctrl_tx_sec_enqueue(conn, node_tx);

	return 0;
}

static u8_t version_ind_send(struct connection *conn,
			     struct pdu_data *pdu_data_rx, u8_t *rx_enqueue)
{
	struct pdu_data_llctrl_version_ind *v;
	struct radio_pdu_node_tx *node_tx;
	struct pdu_data *pdu_ctrl_tx;

	if (!conn->llcp_version.tx) {
		/* acquire tx mem */
		node_tx = mem_acquire(&_radio.pkt_tx_ctrl_free);
		if (!node_tx) {
			return 1;
		}
		conn->llcp_version.tx = 1;

		pdu_ctrl_tx = (void *)node_tx->pdu_data;
		pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
		pdu_ctrl_tx->len =
			offsetof(struct pdu_data_llctrl, version_ind) +
			sizeof(struct pdu_data_llctrl_version_ind);
		pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_VERSION_IND;
		v = &pdu_ctrl_tx->llctrl.version_ind;
		v->version_number = RADIO_BLE_VERSION_NUMBER;
		v->company_id =	RADIO_BLE_COMPANY_ID;
		v->sub_version_number =	RADIO_BLE_SUB_VERSION_NUMBER;

		ctrl_tx_sec_enqueue(conn, node_tx);

		/* Apple work-around, add empty packet before version_ind */
		empty_tx_enqueue(conn);

	} else if (!conn->llcp_version.rx) {
		/* Procedure complete */
		conn->procedure_expire = 0;

		/* enqueue the version ind */
		*rx_enqueue = 1;
	} else {
		/* Tx-ed and Rx-ed before, ignore this invalid Rx. */
		return 0;
	}

	v = &pdu_data_rx->llctrl.version_ind;
	conn->llcp_version.version_number = v->version_number;
	conn->llcp_version.company_id = v->company_id;
	conn->llcp_version.sub_version_number = v->sub_version_number;
	conn->llcp_version.rx = 1;

	return 0;
}

#if defined(CONFIG_BT_CTLR_LE_PING)
static u8_t ping_resp_send(struct connection *conn)
{
	struct radio_pdu_node_tx *node_tx;
	struct pdu_data *pdu_ctrl_tx;

	/* acquire tx mem */
	node_tx = mem_acquire(&_radio.pkt_tx_ctrl_free);
	if (!node_tx) {
		return 1;
	}

	pdu_ctrl_tx = (void *)node_tx->pdu_data;
	pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
	pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl, ping_rsp) +
			   sizeof(struct pdu_data_llctrl_ping_rsp);
	pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PING_RSP;

	ctrl_tx_enqueue(conn, node_tx);

	return 0;
}
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ) || defined(CONFIG_BT_CTLR_PHY)
static u8_t reject_ext_ind_send(struct connection *conn,
				u8_t reject_opcode, u8_t error_code)
{
	struct radio_pdu_node_tx *node_tx;
	struct pdu_data *pdu_ctrl_tx;

	/* acquire tx mem */
	node_tx = mem_acquire(&_radio.pkt_tx_ctrl_free);
	if (!node_tx) {
		return 1;
	}

	pdu_ctrl_tx = (void *)node_tx->pdu_data;
	pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
	pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl, reject_ext_ind) +
		sizeof(struct pdu_data_llctrl_reject_ext_ind);
	pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND;
	pdu_ctrl_tx->llctrl.reject_ext_ind.reject_opcode = reject_opcode;
	pdu_ctrl_tx->llctrl.reject_ext_ind.error_code = error_code;

	ctrl_tx_enqueue(conn, node_tx);

	return 0;
}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ  || PHY */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
#if !defined(CONFIG_BT_CTLR_PHY)
static void length_resp_send(struct connection *conn,
			     struct radio_pdu_node_tx *node_tx,
			     u16_t eff_rx_octets, u16_t eff_tx_octets)
#else /* CONFIG_BT_CTLR_PHY */
static void length_resp_send(struct connection *conn,
			     struct radio_pdu_node_tx *node_tx,
			     u16_t eff_rx_octets, u16_t eff_rx_time,
			     u16_t eff_tx_octets, u16_t eff_tx_time)
#endif /* CONFIG_BT_CTLR_PHY */
{
	struct pdu_data *pdu_ctrl_tx;

	pdu_ctrl_tx = (void *) node_tx->pdu_data;
	pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
	pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl, length_rsp) +
		sizeof(struct pdu_data_llctrl_length_rsp);
	pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_LENGTH_RSP;
	pdu_ctrl_tx->llctrl.length_rsp.max_rx_octets = eff_rx_octets;
	pdu_ctrl_tx->llctrl.length_rsp.max_tx_octets = eff_tx_octets;

#if !defined(CONFIG_BT_CTLR_PHY)
	pdu_ctrl_tx->llctrl.length_rsp.max_rx_time =
		RADIO_PKT_TIME(eff_rx_octets, 0);
	pdu_ctrl_tx->llctrl.length_rsp.max_tx_time =
		RADIO_PKT_TIME(eff_tx_octets, 0);
#else /* CONFIG_BT_CTLR_PHY */
	pdu_ctrl_tx->llctrl.length_rsp.max_rx_time = eff_rx_time;
	pdu_ctrl_tx->llctrl.length_rsp.max_tx_time = eff_tx_time;
#endif /* CONFIG_BT_CTLR_PHY */

	ctrl_tx_enqueue(conn, node_tx);
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
static u8_t phy_rsp_send(struct connection *conn, struct pdu_data *pdu_data_rx)
{
	struct pdu_data_llctrl_phy_req *p;
	struct radio_pdu_node_tx *node_tx;
	struct pdu_data *pdu_ctrl_tx;
	struct pdu_data_llctrl *c;

	/* acquire tx mem */
	node_tx = mem_acquire(&_radio.pkt_tx_ctrl_free);
	if (!node_tx) {
		return 1;
	}

	/* Wait for peer master to complete the procedure */
	conn->llcp_phy.state = LLCP_PHY_STATE_RSP_WAIT;
	if (conn->llcp_phy.ack ==
	    conn->llcp_phy.req) {
		conn->llcp_phy.ack--;

		conn->llcp_phy.cmd = 0;

		conn->llcp_phy.tx =
			conn->phy_pref_tx;
		conn->llcp_phy.rx =
			conn->phy_pref_rx;

		/* Start Procedure Timeout (TODO: this shall not
		 * replace terminate procedure).
		 */
		conn->procedure_expire =
			conn->procedure_reload;
	}

	c = &pdu_data_rx->llctrl;
	p = &c->phy_req;

	conn->llcp_phy.tx &= p->rx_phys;
	conn->llcp_phy.rx &= p->tx_phys;

	pdu_ctrl_tx = (void *)node_tx->pdu_data;
	pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
	pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl, phy_rsp) +
			   sizeof(struct pdu_data_llctrl_phy_rsp);
	pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PHY_RSP;
	pdu_ctrl_tx->llctrl.phy_rsp.tx_phys = conn->phy_pref_tx;
	pdu_ctrl_tx->llctrl.phy_rsp.rx_phys = conn->phy_pref_rx;

	ctrl_tx_enqueue(conn, node_tx);

	return 0;
}
#endif /* CONFIG_BT_CTLR_PHY */

void ll_radio_state_abort(void)
{
	event_stop(0, 0, 0, (void *)STATE_ABORT);
}

u32_t ll_radio_state_is_idle(void)
{
	return radio_is_idle();
}

void radio_ticks_active_to_start_set(u32_t ticks_active_to_start)
{
	_radio.ticks_active_to_start = ticks_active_to_start;
}

struct radio_adv_data *radio_adv_data_get(void)
{
	return &_radio.advertiser.adv_data;
}

struct radio_adv_data *radio_scan_data_get(void)
{
	return &_radio.advertiser.scan_data;
}

static struct connection *connection_get(u16_t handle)
{
	struct connection *conn;

	if (handle < _radio.connection_count) {
		conn = mem_get(_radio.conn_pool, CONNECTION_T_SIZE, handle);
		if ((conn) && (conn->handle == handle)) {
			return conn;
		}
	}

	return 0;
}

static inline void role_active_disable(u8_t ticker_id_stop,
				       u32_t ticks_xtal_to_start,
				       u32_t ticks_active_to_start)
{
	static memq_link_t link_inact;
	static struct mayfly s_mfy_radio_inactive = {0, 0, &link_inact, NULL,
						     mayfly_radio_inactive};
	u32_t volatile ret_cb_evt = TICKER_STATUS_BUSY;
	u32_t ret;

	/* Step 2: Is caller before Event? Stop Event */
	ret = ticker_stop(RADIO_TICKER_INSTANCE_ID_RADIO,
			  RADIO_TICKER_USER_ID_APP, RADIO_TICKER_ID_EVENT,
			  ticker_if_done, (void *)&ret_cb_evt);

	if (ret == TICKER_STATUS_BUSY) {
		mayfly_enable(RADIO_TICKER_USER_ID_APP,
			      RADIO_TICKER_USER_ID_JOB, 1);

		LL_ASSERT(ret_cb_evt != TICKER_STATUS_BUSY);
	}

	if (ret_cb_evt == TICKER_STATUS_SUCCESS) {
		static memq_link_t link_xtal;
		static struct mayfly s_mfy_xtal_stop = {0, 0, &link_xtal, NULL,
							mayfly_xtal_stop};
		u32_t volatile ret_cb_m0 = TICKER_STATUS_BUSY;

		/* Reset the stored ticker id in prepare phase. */
		LL_ASSERT(_radio.ticker_id_prepare);
		_radio.ticker_id_prepare = 0;

		/* Step 2.1: Is caller between Primary and Marker0?
		 * Stop the Marker0 event
		 */
		ret = ticker_stop(RADIO_TICKER_INSTANCE_ID_RADIO,
				  RADIO_TICKER_USER_ID_APP,
				  RADIO_TICKER_ID_MARKER_0,
				  ticker_if_done, (void *)&ret_cb_m0);

		if (ret == TICKER_STATUS_BUSY) {
			mayfly_enable(RADIO_TICKER_USER_ID_APP,
				      RADIO_TICKER_USER_ID_JOB, 1);

			LL_ASSERT(ret_cb_m0 != TICKER_STATUS_BUSY);
		}

		if (ret_cb_m0 == TICKER_STATUS_SUCCESS) {
#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
			/* If in reduced prepare, use the absolute value */
			ticks_xtal_to_start &= ~XON_BITMASK;
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */

			/* Step 2.1.1: Check and deassert Radio Active or XTAL
			 * start
			 */
			if (ticks_active_to_start > ticks_xtal_to_start) {
				/* radio active asserted, handle deasserting
				 * here
				 */
				ret = mayfly_enqueue(
						RADIO_TICKER_USER_ID_APP,
						RADIO_TICKER_USER_ID_WORKER, 0,
						&s_mfy_radio_inactive);
				LL_ASSERT(!ret);
			} else {
				/* XTAL started, handle XTAL stop here */
				ret = mayfly_enqueue(
						RADIO_TICKER_USER_ID_APP,
						RADIO_TICKER_USER_ID_WORKER, 0,
						&s_mfy_xtal_stop);
				LL_ASSERT(!ret);
			}
		} else if (ret_cb_m0 == TICKER_STATUS_FAILURE) {
			/* Step 2.1.2: Deassert Radio Active and XTAL start */

			/* radio active asserted, handle deasserting here */
			ret = mayfly_enqueue(RADIO_TICKER_USER_ID_APP,
					     RADIO_TICKER_USER_ID_WORKER, 0,
					     &s_mfy_radio_inactive);
			LL_ASSERT(!ret);

			/* XTAL started, handle XTAL stop here */
			ret = mayfly_enqueue(RADIO_TICKER_USER_ID_APP,
					     RADIO_TICKER_USER_ID_WORKER, 0,
					     &s_mfy_xtal_stop);
			LL_ASSERT(!ret);
		} else {
			LL_ASSERT(0);
		}
	} else if (ret_cb_evt == TICKER_STATUS_FAILURE) {
		/* Step 3: Caller inside Event, handle graceful stop of Event
		 * (role dependent)
		 */

		/* Stop ticker "may" be in use for direct adv or scanner,
		 * hence stop may fail if ticker not used.
		 *
		 * Connection instances do not use a stop ticker, hence do not
		 * try to stop an invalid ticker id.
		 */
		if (ticker_id_stop != TICKER_NULL) {
			u32_t volatile ret_cb_stop = TICKER_STATUS_BUSY;

			ret = ticker_stop(RADIO_TICKER_INSTANCE_ID_RADIO,
					  RADIO_TICKER_USER_ID_APP,
					  ticker_id_stop, ticker_if_done,
					  (void *)&ret_cb_stop);

			if (ret == TICKER_STATUS_BUSY) {
				mayfly_enable(RADIO_TICKER_USER_ID_APP,
					      RADIO_TICKER_USER_ID_JOB, 1);

				LL_ASSERT(ret_cb_stop != TICKER_STATUS_BUSY);
			}

			LL_ASSERT((ret_cb_stop == TICKER_STATUS_SUCCESS) ||
				  (ret_cb_stop == TICKER_STATUS_FAILURE));
		}

		/* Force Radio ISR execution and wait for role to stop */
		if (_radio.role != ROLE_NONE) {
			static memq_link_t link_radio;
			static struct mayfly s_mfy_radio_stop = {
				0, 0, &link_radio, NULL, mayfly_radio_stop};

			/* Radio state STOP is supplied in params */
			s_mfy_radio_stop.param = (void *)STATE_STOP;

			/* Stop Radio Tx/Rx */
			ret = mayfly_enqueue(RADIO_TICKER_USER_ID_APP,
					     RADIO_TICKER_USER_ID_WORKER, 0,
					     &s_mfy_radio_stop);
			LL_ASSERT(!ret);

			/* wait for radio ISR to exit */
			while (_radio.role != ROLE_NONE) {
				cpu_sleep();
			}
		}
	} else {
		LL_ASSERT(0);
	}
}

static u32_t role_disable(u8_t ticker_id_primary, u8_t ticker_id_stop)
{
	u32_t volatile ret_cb = TICKER_STATUS_BUSY;
	u32_t ticks_active_to_start = 0;
	u32_t ticks_xtal_to_start = 0;
	u32_t ret;

	/* Determine xtal, active and start ticks. Stop directed adv stop
	 * ticker.
	 */
	switch (ticker_id_primary) {
	case RADIO_TICKER_ID_ADV:
		ticks_xtal_to_start =
			_radio.advertiser.hdr.ticks_xtal_to_start;
		ticks_active_to_start =
			_radio.advertiser.hdr.ticks_active_to_start;

		/* Stop ticker "may" be in use for direct adv,
		 * hence stop may fail if ticker not used.
		 */
		ret = ticker_stop(RADIO_TICKER_INSTANCE_ID_RADIO,
				  RADIO_TICKER_USER_ID_APP, ticker_id_stop,
				  ticker_if_done, (void *)&ret_cb);
		if (ret == TICKER_STATUS_BUSY) {
			/* wait for ticker to be stopped */
			while (ret_cb == TICKER_STATUS_BUSY) {
				cpu_sleep();
			}
		}

		LL_ASSERT((ret_cb == TICKER_STATUS_SUCCESS) ||
			  (ret_cb == TICKER_STATUS_FAILURE));
		break;

	case RADIO_TICKER_ID_SCAN:
		ticks_xtal_to_start =
			_radio.scanner.hdr.ticks_xtal_to_start;
		ticks_active_to_start =
			_radio.scanner.hdr.ticks_active_to_start;
		break;
	default:
		if (ticker_id_primary >= RADIO_TICKER_ID_FIRST_CONNECTION) {
			struct connection *conn;
			u16_t conn_handle;

			conn_handle = ticker_id_primary -
				      RADIO_TICKER_ID_FIRST_CONNECTION;
			conn = connection_get(conn_handle);
			if (!conn) {
				return 1;
			}

			ticks_xtal_to_start =
				conn->hdr.ticks_xtal_to_start;
			ticks_active_to_start =
				conn->hdr.ticks_active_to_start;
		} else {
			LL_ASSERT(0);
		}
		break;
	}

	LL_ASSERT(!_radio.ticker_id_stop);
	_radio.ticker_id_stop = ticker_id_primary;

	/* Step 1: Is Primary started? Stop the Primary ticker */
	ret_cb = TICKER_STATUS_BUSY;
	ret = ticker_stop(RADIO_TICKER_INSTANCE_ID_RADIO,
			  RADIO_TICKER_USER_ID_APP, ticker_id_primary,
			  ticker_if_done, (void *)&ret_cb);

	if (ret == TICKER_STATUS_BUSY) {
		/* if inside our event, enable Job. */
		if (_radio.ticker_id_event == ticker_id_primary) {
			mayfly_enable(RADIO_TICKER_USER_ID_APP,
				      RADIO_TICKER_USER_ID_JOB, 1);
		}

		/* wait for ticker to be stopped */
		while (ret_cb == TICKER_STATUS_BUSY) {
			cpu_sleep();
		}
	}

	if (ret_cb != TICKER_STATUS_SUCCESS) {
		goto role_disable_cleanup;
	}

	/* Inside our event, gracefully handle XTAL and Radio actives */
	if ((_radio.ticker_id_prepare == ticker_id_primary)
	    || (_radio.ticker_id_event == ticker_id_primary)) {

		role_active_disable(ticker_id_stop,
				    ticks_xtal_to_start, ticks_active_to_start);
	}

	if (!_radio.ticker_id_stop) {
		ret_cb = TICKER_STATUS_FAILURE;
	}

role_disable_cleanup:
	_radio.ticker_id_stop = 0;

	return ret_cb;
}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
u32_t radio_adv_enable(u8_t phy_p, u16_t interval, u8_t chan_map,
		       u8_t filter_policy, u8_t rl_idx)
#else /* !CONFIG_BT_CTLR_ADV_EXT */
u32_t radio_adv_enable(u16_t interval, u8_t chan_map, u8_t filter_policy,
		       u8_t rl_idx)
#endif /* !CONFIG_BT_CTLR_ADV_EXT */
{
	u32_t volatile ret_cb = TICKER_STATUS_BUSY;
	u32_t ticks_slot_offset;
	struct connection *conn;
	struct pdu_adv *pdu_adv;
	u32_t slot_us;
	u8_t chan_cnt;
	u32_t ret;

	if (_radio.advertiser.is_enabled) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	pdu_adv = (void *)&_radio.advertiser.adv_data.data
					[_radio.advertiser.adv_data.last][0];

	if ((pdu_adv->type == PDU_ADV_TYPE_ADV_IND) ||
	    (pdu_adv->type == PDU_ADV_TYPE_DIRECT_IND)) {
		void *link;

		if (_radio.advertiser.conn) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		link = mem_acquire(&_radio.link_rx_free);
		if (!link) {
			return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
		}

		conn = mem_acquire(&_radio.conn_free);
		if (!conn) {
			mem_release(link, &_radio.link_rx_free);

			return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
		}

		conn->handle = 0xFFFF;
		conn->llcp_features = RADIO_BLE_FEAT;
		conn->data_chan_sel = 0;
		conn->data_chan_use = 0;
		conn->event_counter = 0;
		conn->latency_prepare = 0;
		conn->latency_event = 0;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
		conn->default_tx_octets = _radio.default_tx_octets;
		conn->max_tx_octets = RADIO_LL_LENGTH_OCTETS_RX_MIN;
		conn->max_rx_octets = RADIO_LL_LENGTH_OCTETS_RX_MIN;

#if defined(CONFIG_BT_CTLR_PHY)
		conn->default_tx_time = _radio.default_tx_time;
		conn->max_tx_time =
			RADIO_PKT_TIME(RADIO_LL_LENGTH_OCTETS_RX_MIN, 0);
		conn->max_rx_time =
			RADIO_PKT_TIME(RADIO_LL_LENGTH_OCTETS_RX_MIN, 0);
#endif /* CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
		conn->phy_pref_tx = _radio.default_phy_tx;
		conn->phy_tx = BIT(0);
		conn->phy_pref_flags = 0;
		conn->phy_flags = 0;
		conn->phy_tx_time = BIT(0);
		conn->phy_pref_rx = _radio.default_phy_rx;
		conn->phy_rx = BIT(0);
#endif /* CONFIG_BT_CTLR_PHY */

		conn->role = 1;
		conn->connect_expire = 6;
		conn->common.fex_valid = 0;
		conn->slave.latency_enabled = 0;
		conn->slave.latency_cancel = 0;
		conn->slave.window_widening_prepare_us = 0;
		conn->slave.window_widening_event_us = 0;
		conn->slave.ticks_to_offset = 0;
		conn->supervision_expire = 0;
		conn->procedure_expire = 0;

#if defined(CONFIG_BT_CTLR_LE_PING)
		conn->apto_expire = 0;
		conn->appto_expire = 0;
#endif /* CONFIG_BT_CTLR_LE_PING */

		conn->llcp_req = 0;
		conn->llcp_ack = 0;
		conn->llcp_version.tx = 0;
		conn->llcp_version.rx = 0;
		conn->llcp_terminate.req = 0;
		conn->llcp_terminate.ack = 0;
		conn->llcp_terminate.reason_peer = 0;
		conn->llcp_terminate.radio_pdu_node_rx.hdr.onion.link = link;

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		conn->llcp_conn_param.req = 0;
		conn->llcp_conn_param.ack = 0;
		conn->llcp_conn_param.disabled = 0;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
		conn->llcp_length.req = 0;
		conn->llcp_length.ack = 0;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
		conn->llcp_phy.req = 0;
		conn->llcp_phy.ack = 0;
#endif /* CONFIG_BT_CTLR_PHY */

		conn->sn = 0;
		conn->nesn = 0;
		conn->pause_rx = 0;
		conn->pause_tx = 0;
		conn->enc_rx = 0;
		conn->enc_tx = 0;
		conn->refresh = 0;
		conn->empty = 0;
		conn->pkt_tx_head = NULL;
		conn->pkt_tx_ctrl = NULL;
		conn->pkt_tx_ctrl_last = NULL;
		conn->pkt_tx_data = NULL;
		conn->pkt_tx_last = NULL;
		conn->packet_tx_head_len = 0;
		conn->packet_tx_head_offset = 0;

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
		conn->rssi_latest = 0x7F;
		conn->rssi_reported = 0x7F;
		conn->rssi_sample_count = 0;
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

		/* wait for stable 32KHz clock */
		k32src_wait();

		_radio.advertiser.conn = conn;
	} else {
		conn = NULL;
	}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	_radio.advertiser.phy_p = phy_p;
#endif /* CONFIG_BT_CTLR_ADV_EXT */

	_radio.advertiser.chan_map = chan_map;
	_radio.advertiser.filter_policy = filter_policy;
#if defined(CONFIG_BT_CTLR_PRIVACY)
	_radio.advertiser.rl_idx = rl_idx;
#else
	ARG_UNUSED(rl_idx);
#endif /* CONFIG_BT_CTLR_PRIVACY */

	_radio.advertiser.hdr.ticks_active_to_start =
		_radio.ticks_active_to_start;
	_radio.advertiser.hdr.ticks_xtal_to_start =
		HAL_TICKER_US_TO_TICKS(RADIO_TICKER_XTAL_OFFSET_US);
	_radio.advertiser.hdr.ticks_preempt_to_start =
		HAL_TICKER_US_TO_TICKS(RADIO_TICKER_PREEMPT_PART_MIN_US);

	chan_cnt = util_ones_count_get(&chan_map, sizeof(chan_map));

	if (pdu_adv->type == PDU_ADV_TYPE_DIRECT_IND) {
		/* Max. chain is DIRECT_IND * channels + CONNECT_IND */
		slot_us = ((RADIO_TICKER_START_PART_US + 176 + 152 + 40) *
			   chan_cnt) - 40 + 352;
	} else if (pdu_adv->type == PDU_ADV_TYPE_NONCONN_IND) {
		slot_us = (RADIO_TICKER_START_PART_US + 376) * chan_cnt;
	} else {
		/* Max. chain is ADV/SCAN_IND + SCAN_REQ + SCAN_RESP */
		slot_us = (RADIO_TICKER_START_PART_US + 376 + 152 + 176 +
			   152 + 376) * chan_cnt;
	}
	_radio.advertiser.hdr.ticks_slot = HAL_TICKER_US_TO_TICKS(slot_us);

	ticks_slot_offset =
		max(_radio.advertiser.hdr.ticks_active_to_start,
		    _radio.advertiser.hdr.ticks_xtal_to_start);

	/* High Duty Cycle Directed Advertising if interval is 0. */
	_radio.advertiser.is_hdcd = !interval &&
				    (pdu_adv->type == PDU_ADV_TYPE_DIRECT_IND);
	if (_radio.advertiser.is_hdcd) {
		u32_t ticks_now = ticker_ticks_now_get();

		ret = ticker_start(RADIO_TICKER_INSTANCE_ID_RADIO,
				   RADIO_TICKER_USER_ID_APP,
				   RADIO_TICKER_ID_ADV, ticks_now, 0,
				   (ticks_slot_offset +
				    _radio.advertiser.hdr.ticks_slot),
				   TICKER_NULL_REMAINDER, TICKER_NULL_LAZY,
				   (ticks_slot_offset +
				    _radio.advertiser.hdr.ticks_slot),
				   radio_event_adv_prepare, NULL,
				   ticker_if_done, (void *)&ret_cb);

		if (ret == TICKER_STATUS_BUSY) {
			while (ret_cb == TICKER_STATUS_BUSY) {
				cpu_sleep();
			}
		}

		if (ret_cb != TICKER_STATUS_SUCCESS) {
			goto failure_cleanup;
		}

		ret_cb = TICKER_STATUS_BUSY;
		ret =
			ticker_start(RADIO_TICKER_INSTANCE_ID_RADIO,
				     RADIO_TICKER_USER_ID_APP,
				     RADIO_TICKER_ID_ADV_STOP, ticks_now,
				     HAL_TICKER_US_TO_TICKS(
					(1280 * 1000) +
					RADIO_TICKER_XTAL_OFFSET_US),
				     TICKER_NULL_PERIOD, TICKER_NULL_REMAINDER,
				     TICKER_NULL_LAZY, TICKER_NULL_SLOT,
				     event_adv_stop, NULL, ticker_if_done,
				     (void *)&ret_cb);
	} else {
		ret =
			ticker_start(RADIO_TICKER_INSTANCE_ID_RADIO,
				     RADIO_TICKER_USER_ID_APP,
				     RADIO_TICKER_ID_ADV,
				     ticker_ticks_now_get(), 0,
				     HAL_TICKER_US_TO_TICKS((u64_t)interval *
							    625),
				     TICKER_NULL_REMAINDER, TICKER_NULL_LAZY,
				     (ticks_slot_offset +
				      _radio.advertiser.hdr.ticks_slot),
				     radio_event_adv_prepare, NULL,
				     ticker_if_done, (void *)&ret_cb);
	}

	if (ret == TICKER_STATUS_BUSY) {
		while (ret_cb == TICKER_STATUS_BUSY) {
			cpu_sleep();
		}
	}

	if (ret_cb == TICKER_STATUS_SUCCESS) {
		_radio.advertiser.is_enabled = 1;

		if (!_radio.scanner.is_enabled) {
			ll_adv_scan_state_cb(BIT(0));
		}

		return 0;
	}

failure_cleanup:

	if (conn) {
		mem_release(conn->llcp_terminate.radio_pdu_node_rx.hdr.
			    onion.link, &_radio.link_rx_free);
		mem_release(conn, &_radio.conn_free);
	}

	return BT_HCI_ERR_CMD_DISALLOWED;
}

u32_t radio_adv_disable(void)
{
	u32_t status;

	status = role_disable(RADIO_TICKER_ID_ADV,
			      RADIO_TICKER_ID_ADV_STOP);
	if (!status) {
		struct connection *conn;

		_radio.advertiser.is_enabled = 0;

		if (!_radio.scanner.is_enabled) {
			ll_adv_scan_state_cb(0);
		}

		conn = _radio.advertiser.conn;
		if (conn) {
			_radio.advertiser.conn = NULL;

			mem_release(conn->llcp_terminate.radio_pdu_node_rx.hdr.onion.link,
				    &_radio.link_rx_free);
			mem_release(conn, &_radio.conn_free);
		}
	}

	return status ? BT_HCI_ERR_CMD_DISALLOWED : 0;
}

u32_t ll_adv_is_enabled(void)
{
	return _radio.advertiser.is_enabled;
}

u32_t radio_adv_filter_pol_get(void)
{
	/* NOTE: filter_policy is only written in thread mode; if is_enabled is
	 * unset by ISR, returning the stale filter_policy is acceptable because
	 * the unset code path in ISR will generate a connection complete
	 * event.
	 */
	if (_radio.advertiser.is_enabled) {
		return _radio.advertiser.filter_policy;
	}

	return 0;
}

u32_t radio_scan_enable(u8_t type, u8_t init_addr_type, u8_t *init_addr,
			u16_t interval, u16_t window, u8_t filter_policy,
			u8_t rpa_gen, u8_t rl_idx)
{
	u32_t volatile ret_cb = TICKER_STATUS_BUSY;
	u32_t ticks_slot_offset;
	u32_t ticks_interval;
	u32_t ticks_anchor;
	u32_t us_offset;
	u32_t ret;

	if (_radio.scanner.is_enabled) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	_radio.scanner.type = type;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	_radio.scanner.phy = type >> 1;
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_PRIVACY)
	_radio.scanner.rpa_gen = rpa_gen;
	_radio.scanner.rl_idx = rl_idx;
#endif /* CONFIG_BT_CTLR_PRIVACY */

	_radio.scanner.init_addr_type = init_addr_type;
	memcpy(&_radio.scanner.init_addr[0], init_addr, BDADDR_SIZE);
	_radio.scanner.ticks_window = HAL_TICKER_US_TO_TICKS((u64_t)window *
							     625);
	_radio.scanner.filter_policy = filter_policy;

	_radio.scanner.hdr.ticks_active_to_start =
		_radio.ticks_active_to_start;
	_radio.scanner.hdr.ticks_xtal_to_start =
		HAL_TICKER_US_TO_TICKS(RADIO_TICKER_XTAL_OFFSET_US);
	_radio.scanner.hdr.ticks_preempt_to_start =
		HAL_TICKER_US_TO_TICKS(RADIO_TICKER_PREEMPT_PART_MIN_US);
	_radio.scanner.hdr.ticks_slot =
		_radio.scanner.ticks_window +
		HAL_TICKER_US_TO_TICKS(RADIO_TICKER_START_PART_US);

	ticks_interval = HAL_TICKER_US_TO_TICKS((u64_t) interval * 625);
	if (_radio.scanner.hdr.ticks_slot >
	    (ticks_interval -
	     HAL_TICKER_US_TO_TICKS(RADIO_TICKER_XTAL_OFFSET_US))) {
		_radio.scanner.hdr.ticks_slot =
			(ticks_interval -
			 HAL_TICKER_US_TO_TICKS(RADIO_TICKER_XTAL_OFFSET_US));
	}

	ticks_slot_offset = max(_radio.scanner.hdr.ticks_active_to_start,
				_radio.scanner.hdr.ticks_xtal_to_start);

	ticks_anchor = ticker_ticks_now_get();

	if ((_radio.scanner.conn) ||
	    !IS_ENABLED(CONFIG_BT_CTLR_SCHED_ADVANCED)) {
		us_offset = 0;
	}
#if defined(CONFIG_BT_CTLR_SCHED_ADVANCED)
	else {
		u32_t ticks_ref = 0;

		sched_after_mstr_free_slot_get(RADIO_TICKER_USER_ID_APP,
					       (ticks_slot_offset +
						_radio.scanner.hdr.ticks_slot),
					       &ticks_ref, &us_offset);
		/* Use the ticks_ref as scanner's anchor if a free time space
		 * after any master role is available (indicated by a non-zero
		 * us_offset value).
		 */
		if (us_offset) {
			ticks_anchor = ticks_ref;
		}
	}
#endif /* CONFIG_BT_CTLR_SCHED_ADVANCED */

	ret = ticker_start(RADIO_TICKER_INSTANCE_ID_RADIO,
			   RADIO_TICKER_USER_ID_APP, RADIO_TICKER_ID_SCAN,
			   (ticks_anchor + HAL_TICKER_US_TO_TICKS(us_offset)),
			   0, ticks_interval,
			   HAL_TICKER_REMAINDER((u64_t) interval * 625),
			   TICKER_NULL_LAZY,
			   (ticks_slot_offset +
			    _radio.scanner.hdr.ticks_slot),
			   event_scan_prepare, NULL, ticker_if_done,
			   (void *)&ret_cb);

	if (ret == TICKER_STATUS_BUSY) {
		while (ret_cb == TICKER_STATUS_BUSY) {
			cpu_sleep();
		}
	}

	if (ret_cb != TICKER_STATUS_SUCCESS) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	_radio.scanner.is_enabled = 1;

	if (!_radio.advertiser.is_enabled) {
		ll_adv_scan_state_cb(BIT(1));
	}

	return 0;
}

u32_t radio_scan_disable(void)
{
	u32_t status;

	status = role_disable(RADIO_TICKER_ID_SCAN,
			      RADIO_TICKER_ID_SCAN_STOP);
	if (!status) {
		struct connection *conn;

		_radio.scanner.is_enabled = 0;

		if (!_radio.advertiser.is_enabled) {
			ll_adv_scan_state_cb(0);
		}

		conn = _radio.scanner.conn;
		if (conn) {
			_radio.scanner.conn = NULL;

			mem_release(conn->llcp_terminate.
				    radio_pdu_node_rx.hdr.onion.link,
				    &_radio.link_rx_free);
			mem_release(conn, &_radio.conn_free);
		}
	}

	return status ? BT_HCI_ERR_CMD_DISALLOWED : 0;
}

u32_t ll_scan_is_enabled(void)
{
	/* NOTE: BIT(0) - passive scanning enabled
	 *       BIT(1) - active scanning enabled
	 *       BIT(2) - initiator enabled
	 */
	return ((u32_t)_radio.scanner.is_enabled << _radio.scanner.type) |
	       (_radio.scanner.conn ? BIT(2) : 0);
}

u32_t radio_scan_filter_pol_get(void)
{
	/* NOTE: filter_policy is only written in thread mode; if is_enabled is
	 * unset by ISR, returning the stale filter_policy is acceptable because
	 * the unset code path in ISR will generate a connection complete
	 * event.
	 */
	if (_radio.scanner.is_enabled) {
		return _radio.scanner.filter_policy;
	}

	return 0;
}

u32_t radio_connect_enable(u8_t adv_addr_type, u8_t *adv_addr, u16_t interval,
			   u16_t latency, u16_t timeout)
{
	struct connection *conn;
	u32_t conn_interval_us;
	u32_t access_addr;
	void *link;

	if (_radio.scanner.conn) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	link = mem_acquire(&_radio.link_rx_free);
	if (!link) {
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	conn = mem_acquire(&_radio.conn_free);
	if (!conn) {
		mem_release(link, &_radio.link_rx_free);
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	_radio.scanner.adv_addr_type = adv_addr_type;
	memcpy(&_radio.scanner.adv_addr[0], adv_addr, BDADDR_SIZE);
	_radio.scanner.conn_interval = interval;
	_radio.scanner.conn_latency = latency;
	_radio.scanner.conn_timeout = timeout;
	_radio.scanner.ticks_conn_slot = HAL_TICKER_US_TO_TICKS(
		RADIO_TICKER_START_PART_US + radio_tx_ready_delay_get(0, 0) +
		328 + RADIO_TIFS + 328);

	conn->handle = 0xFFFF;
	conn->llcp_features = RADIO_BLE_FEAT;
	access_addr = access_addr_get();
	memcpy(&conn->access_addr[0], &access_addr, sizeof(conn->access_addr));
	bt_rand(&conn->crc_init[0], 3);
	memcpy(&conn->data_chan_map[0], &_radio.data_chan_map[0],
	       sizeof(conn->data_chan_map));
	conn->data_chan_count = _radio.data_chan_count;
	conn->data_chan_sel = 0;
	conn->data_chan_hop = 6;
	conn->data_chan_use = 0;
	conn->event_counter = 0;
	conn->conn_interval = _radio.scanner.conn_interval;
	conn->latency_prepare = 0;
	conn->latency_event = 0;
	conn->latency = _radio.scanner.conn_latency;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	conn->default_tx_octets = _radio.default_tx_octets;
	conn->max_tx_octets = RADIO_LL_LENGTH_OCTETS_RX_MIN;
	conn->max_rx_octets = RADIO_LL_LENGTH_OCTETS_RX_MIN;

#if defined(CONFIG_BT_CTLR_PHY)
	conn->default_tx_time = _radio.default_tx_time;
	conn->max_tx_time = RADIO_PKT_TIME(RADIO_LL_LENGTH_OCTETS_RX_MIN, 0);
	conn->max_rx_time = RADIO_PKT_TIME(RADIO_LL_LENGTH_OCTETS_RX_MIN, 0);
#endif /* CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
	conn->phy_pref_tx = _radio.default_phy_tx;
	conn->phy_tx = BIT(0);
	conn->phy_pref_flags = 0;
	conn->phy_flags = 0;
	conn->phy_tx_time = BIT(0);
	conn->phy_pref_rx = _radio.default_phy_rx;
	conn->phy_rx = BIT(0);
#endif /* CONFIG_BT_CTLR_PHY */

	conn->role = 0;
	conn->connect_expire = 6;
	conn->common.fex_valid = 0;
	conn->master.terminate_ack = 0;
	conn_interval_us =
		(u32_t)_radio.scanner.conn_interval * 1250;
	conn->supervision_reload =
		RADIO_CONN_EVENTS((_radio.scanner.conn_timeout * 10 * 1000),
				  conn_interval_us);
	conn->supervision_expire = 0;
	conn->procedure_reload =
		RADIO_CONN_EVENTS((40 * 1000 * 1000), conn_interval_us);
	conn->procedure_expire = 0;

#if defined(CONFIG_BT_CTLR_LE_PING)
	/* APTO in no. of connection events */
	conn->apto_reload = RADIO_CONN_EVENTS((30 * 1000 * 1000),
					      conn_interval_us);
	/* Dispatch LE Ping PDU 6 connection events (that peer would listen to)
	 * before 30s timeout
	 * TODO: "peer listens to" is greater than 30s due to latency
	 */
	conn->appto_reload = (conn->apto_reload > (conn->latency + 6)) ?
			     (conn->apto_reload - (conn->latency + 6)) :
			     conn->apto_reload;
	conn->apto_expire = 0;
	conn->appto_expire = 0;
#endif /* CONFIG_BT_CTLR_LE_PING */

	conn->llcp_req = 0;
	conn->llcp_ack = 0;
	conn->llcp_version.tx = 0;
	conn->llcp_version.rx = 0;
	conn->llcp_terminate.req = 0;
	conn->llcp_terminate.ack = 0;
	conn->llcp_terminate.reason_peer = 0;
	conn->llcp_terminate.radio_pdu_node_rx.hdr.onion.link = link;

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	conn->llcp_conn_param.req = 0;
	conn->llcp_conn_param.ack = 0;
	conn->llcp_conn_param.disabled = 0;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	conn->llcp_length.req = 0;
	conn->llcp_length.ack = 0;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
	conn->llcp_phy.req = 0;
	conn->llcp_phy.ack = 0;
#endif /* CONFIG_BT_CTLR_PHY */

	conn->sn = 0;
	conn->nesn = 0;
	conn->pause_rx = 0;
	conn->pause_tx = 0;
	conn->enc_rx = 0;
	conn->enc_tx = 0;
	conn->refresh = 0;
	conn->empty = 0;
	conn->pkt_tx_head = NULL;
	conn->pkt_tx_ctrl = NULL;
	conn->pkt_tx_ctrl_last = NULL;
	conn->pkt_tx_data = NULL;
	conn->pkt_tx_last = NULL;
	conn->packet_tx_head_len = 0;
	conn->packet_tx_head_offset = 0;

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
	conn->rssi_latest = 0x7F;
	conn->rssi_reported = 0x7F;
	conn->rssi_sample_count = 0;
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

	/* wait for stable 32KHz clock */
	k32src_wait();

	_radio.scanner.conn = conn;

	return 0;
}

u32_t ll_connect_disable(void)
{
	u32_t status;

	if (_radio.scanner.conn == 0) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	status = radio_scan_disable();

	return status;
}

u32_t ll_conn_update(u16_t handle, u8_t cmd, u8_t status, u16_t interval,
		     u16_t latency, u16_t timeout)
{
	struct connection *conn;

	conn = connection_get(handle);
	if (!conn) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	if (!cmd) {
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		if (!conn->llcp_conn_param.disabled &&
		    (!conn->common.fex_valid ||
		     (conn->llcp_features &
		      BIT(BT_LE_FEAT_BIT_CONN_PARAM_REQ)))) {
			cmd++;
		} else if (conn->role) {
			return BT_HCI_ERR_UNSUPP_REMOTE_FEATURE;
		}
#else /* !CONFIG_BT_CTLR_CONN_PARAM_REQ */
		if (conn->role) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}
#endif /* !CONFIG_BT_CTLR_CONN_PARAM_REQ */
	}

	if (!cmd) {
		if (conn->llcp_req != conn->llcp_ack) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		conn->llcp.conn_upd.win_size = 1;
		conn->llcp.conn_upd.win_offset_us = 0;
		conn->llcp.conn_upd.interval = interval;
		conn->llcp.conn_upd.latency = latency;
		conn->llcp.conn_upd.timeout = timeout;
		/* conn->llcp.conn_upd.instant     = 0; */
		conn->llcp.conn_upd.state = LLCP_CUI_STATE_USE;
		conn->llcp.conn_upd.is_internal = 0;

		conn->llcp_type = LLCP_CONN_UPD;
		conn->llcp_req++;
	} else {
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		cmd--;

		if (cmd) {
			if ((conn->llcp_conn_param.req ==
			     conn->llcp_conn_param.ack) ||
			    (conn->llcp_conn_param.state !=
			     LLCP_CPR_STATE_APP_WAIT)) {
				return BT_HCI_ERR_CMD_DISALLOWED;
			}

			conn->llcp_conn_param.status = status;
			conn->llcp_conn_param.state = cmd;
			conn->llcp_conn_param.cmd = 1;
		} else {
			if (conn->llcp_conn_param.req !=
			    conn->llcp_conn_param.ack) {
				return BT_HCI_ERR_CMD_DISALLOWED;
			}

			conn->llcp_conn_param.status = 0;
			conn->llcp_conn_param.interval = interval;
			conn->llcp_conn_param.latency = latency;
			conn->llcp_conn_param.timeout = timeout;
			conn->llcp_conn_param.state = cmd;
			conn->llcp_conn_param.cmd = 1;
			conn->llcp_conn_param.req++;
		}

#else /* !CONFIG_BT_CTLR_CONN_PARAM_REQ */
		/* CPR feature not supported */
		return BT_HCI_ERR_CMD_DISALLOWED;
#endif /* !CONFIG_BT_CTLR_CONN_PARAM_REQ */
	}

	return 0;
}

u32_t ll_chm_update(u8_t *chm)
{
	u8_t instance;

	memcpy(&_radio.data_chan_map[0], chm,
	       sizeof(_radio.data_chan_map));
	_radio.data_chan_count =
		util_ones_count_get(&_radio.data_chan_map[0],
				    sizeof(_radio.data_chan_map));

	instance = _radio.connection_count;
	while (instance--) {
		struct connection *conn;

		conn = connection_get(instance);
		if (!conn || conn->role) {
			continue;
		}

		if (conn->llcp_req != conn->llcp_ack) {
			return 1;
		}

		memcpy(&conn->llcp.chan_map.chm[0], chm,
		       sizeof(conn->llcp.chan_map.chm));
		/* conn->llcp.chan_map.instant     = 0; */
		conn->llcp.chan_map.initiate = 1;

		conn->llcp_type = LLCP_CHAN_MAP;
		conn->llcp_req++;
	}

	return 0;
}

u32_t ll_chm_get(u16_t handle, u8_t *chm)
{
	struct connection *conn;

	conn = connection_get(handle);
	if (!conn) {
		return 1;
	}

	/* Iterate until we are sure the ISR did not modify the value while
	 * we were reading it from memory.
	 */
	do {
		conn->chm_update = 0;
		memcpy(chm, conn->data_chan_map, sizeof(conn->data_chan_map));
	} while (conn->chm_update);

	return 0;
}

#if defined(CONFIG_BT_CTLR_LE_ENC)
u32_t ll_enc_req_send(u16_t handle, u8_t *rand, u8_t *ediv, u8_t *ltk)
{
	struct connection *conn;
	struct radio_pdu_node_tx *node_tx;

	conn = connection_get(handle);
	if (!conn || (conn->llcp_req != conn->llcp_ack)) {
		return 1;
	}

	node_tx = ll_tx_mem_acquire();
	if (node_tx) {
		struct pdu_data *pdu_data_tx;

		pdu_data_tx = (void *)node_tx->pdu_data;

		memcpy(&conn->llcp.encryption.ltk[0], ltk,
		       sizeof(conn->llcp.encryption.ltk));

		if ((conn->enc_rx == 0) && (conn->enc_tx == 0)) {
			struct pdu_data_llctrl_enc_req *enc_req;

			pdu_data_tx->ll_id = PDU_DATA_LLID_CTRL;
			pdu_data_tx->len =
				offsetof(struct pdu_data_llctrl, enc_rsp) +
				sizeof(struct pdu_data_llctrl_enc_req);
			pdu_data_tx->llctrl.opcode =
				PDU_DATA_LLCTRL_TYPE_ENC_REQ;
			enc_req = (void *)
				&pdu_data_tx->llctrl.enc_req;
			memcpy(enc_req->rand, rand, sizeof(enc_req->rand));
			enc_req->ediv[0] = ediv[0];
			enc_req->ediv[1] = ediv[1];
			bt_rand(enc_req->skdm, sizeof(enc_req->skdm));
			bt_rand(enc_req->ivm, sizeof(enc_req->ivm));
		} else if ((conn->enc_rx != 0) && (conn->enc_tx != 0)) {
			memcpy(&conn->llcp.encryption.rand[0], rand,
			       sizeof(conn->llcp.encryption.rand));

			conn->llcp.encryption.ediv[0] = ediv[0];
			conn->llcp.encryption.ediv[1] = ediv[1];

			pdu_data_tx->ll_id = PDU_DATA_LLID_CTRL;
			pdu_data_tx->len = offsetof(struct pdu_data_llctrl,
						    enc_req);
			pdu_data_tx->llctrl.opcode =
				PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_REQ;
		} else {
			ll_tx_mem_release(node_tx);

			return 1;
		}

		if (ll_tx_mem_enqueue(handle, node_tx)) {
			ll_tx_mem_release(node_tx);

			return 1;
		}

		conn->llcp.encryption.initiate = 1;

		conn->llcp_type = LLCP_ENCRYPTION;
		conn->llcp_req++;

		return 0;
	}

	return 1;
}

u32_t ll_start_enc_req_send(u16_t handle, u8_t error_code,
			    u8_t const *const ltk)
{
	struct connection *conn;

	conn = connection_get(handle);
	if (!conn) {
		return 1;
	}

	if (error_code) {
		if (conn->refresh == 0) {
			if (conn->llcp_req != conn->llcp_ack) {
				return 1;
			}

			conn->llcp.encryption.error_code = error_code;
			conn->llcp.encryption.initiate = 0;

			conn->llcp_type = LLCP_ENCRYPTION;
			conn->llcp_req++;
		} else {
			if (conn->llcp_terminate.ack !=
			    conn->llcp_terminate.req) {
				return 1;
			}

			conn->llcp_terminate.reason_own = error_code;

			conn->llcp_terminate.req++;
		}
	} else {
		memcpy(&conn->llcp.encryption.ltk[0], ltk,
		       sizeof(conn->llcp.encryption.ltk));

		if (conn->llcp_req != conn->llcp_ack) {
			return 1;
		}

		conn->llcp.encryption.error_code = 0;
		conn->llcp.encryption.initiate = 0;

		conn->llcp_type = LLCP_ENCRYPTION;
		conn->llcp_req++;
	}

	return 0;
}
#endif /* CONFIG_BT_CTLR_LE_ENC */

u32_t ll_feature_req_send(u16_t handle)
{
	struct connection *conn;

	conn = connection_get(handle);
	if (!conn || (conn->llcp_req != conn->llcp_ack)) {
		return 1;
	}

	conn->llcp_type = LLCP_FEATURE_EXCHANGE;
	conn->llcp_req++;

	return 0;
}

u32_t ll_version_ind_send(u16_t handle)
{
	struct connection *conn;

	conn = connection_get(handle);
	if (!conn || (conn->llcp_req != conn->llcp_ack)) {
		return 1;
	}

	conn->llcp_type = LLCP_VERSION_EXCHANGE;
	conn->llcp_req++;

	return 0;
}

u32_t ll_terminate_ind_send(u16_t handle, u8_t reason)
{
	struct connection *conn;

	conn = connection_get(handle);
	if (!conn || (conn->llcp_terminate.ack != conn->llcp_terminate.req)) {
		return 1;
	}

	conn->llcp_terminate.reason_own = reason;

	conn->llcp_terminate.req++;

	return 0;
}

u32_t ll_tx_pwr_lvl_get(u16_t handle, u8_t type, s8_t *tx_pwr_lvl)
{
	struct connection *conn;

	conn = connection_get(handle);
	if (!conn) {
		return 1;
	}

	/*TODO: check type here for current or maximum */

	/* TODO: Support TX Power Level other than 0dBm */
	*tx_pwr_lvl = 0;

	return 0;
}

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
u32_t ll_rssi_get(u16_t handle, u8_t *rssi)
{
	struct connection *conn;

	conn = connection_get(handle);
	if (!conn) {
		return 1;
	}

	*rssi = conn->rssi_latest;

	return 0;
}
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

#if defined(CONFIG_BT_CTLR_LE_PING)
u32_t ll_apto_get(u16_t handle, u16_t *apto)
{
	struct connection *conn;

	conn = connection_get(handle);
	if (!conn) {
		return 1;
	}

	*apto = conn->apto_reload * conn->conn_interval * 125 / 1000;

	return 0;
}

u32_t ll_apto_set(u16_t handle, u16_t apto)
{
	struct connection *conn;

	conn = connection_get(handle);
	if (!conn) {
		return 1;
	}

	conn->apto_reload = RADIO_CONN_EVENTS(apto * 10 * 1000,
					      conn->conn_interval * 1250);

	return 0;
}
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
u32_t ll_length_req_send(u16_t handle, u16_t tx_octets, u16_t tx_time)
{
	struct connection *conn;

	conn = connection_get(handle);
	if (!conn || (conn->llcp_req != conn->llcp_ack) ||
	    (conn->llcp_length.req != conn->llcp_length.ack)) {
		return 1;
	}

	/* TODO: parameter check tx_octets and tx_time */

	conn->llcp_length.state = LLCP_LENGTH_STATE_REQ;
	conn->llcp_length.tx_octets = tx_octets;

#if defined(CONFIG_BT_CTLR_PHY)
	conn->llcp_length.tx_time = tx_time;
#endif /* CONFIG_BT_CTLR_PHY */

	conn->llcp_length.req++;

	return 0;
}

void ll_length_default_get(u16_t *max_tx_octets, u16_t *max_tx_time)
{
	*max_tx_octets = _radio.default_tx_octets;
	*max_tx_time = _radio.default_tx_time;
}

u32_t ll_length_default_set(u16_t max_tx_octets, u16_t max_tx_time)
{
	/* TODO: parameter check (for BT 5.0 compliance) */

	_radio.default_tx_octets = max_tx_octets;
	_radio.default_tx_time = max_tx_time;

	return 0;
}

void ll_length_max_get(u16_t *max_tx_octets, u16_t *max_tx_time,
		       u16_t *max_rx_octets, u16_t *max_rx_time)
{
	*max_tx_octets = RADIO_LL_LENGTH_OCTETS_RX_MAX;
	*max_tx_time = RADIO_PKT_TIME(RADIO_LL_LENGTH_OCTETS_RX_MAX, BIT(2));
	*max_rx_octets = RADIO_LL_LENGTH_OCTETS_RX_MAX;
	*max_rx_time = RADIO_PKT_TIME(RADIO_LL_LENGTH_OCTETS_RX_MAX, BIT(2));
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
u32_t ll_phy_get(u16_t handle, u8_t *tx, u8_t *rx)
{
	struct connection *conn;

	conn = connection_get(handle);
	if (!conn) {
		return 1;
	}

	/* TODO: context safe read */
	*tx = conn->phy_tx;
	*rx = conn->phy_rx;

	return 0;
}

u32_t ll_phy_default_set(u8_t tx, u8_t rx)
{
	/* TODO: validate against supported phy */

	_radio.default_phy_tx = tx;
	_radio.default_phy_rx = rx;

	return 0;
}

u32_t ll_phy_req_send(u16_t handle, u8_t tx, u8_t flags, u8_t rx)
{
	struct connection *conn;

	conn = connection_get(handle);
	if (!conn || (conn->llcp_req != conn->llcp_ack) ||
	    (conn->llcp_phy.req != conn->llcp_phy.ack)) {
		return 1;
	}

	conn->llcp_phy.state = LLCP_PHY_STATE_REQ;
	conn->llcp_phy.cmd = 1;
	conn->llcp_phy.tx = tx;
	conn->llcp_phy.flags = flags;
	conn->llcp_phy.rx = rx;
	conn->llcp_phy.req++;

	return 0;
}
#endif /* CONFIG_BT_CTLR_PHY */

static u8_t tx_cmplt_get(u16_t *handle, u8_t *first, u8_t last)
{
	u8_t _first;
	u8_t cmplt;

	_first = *first;
	if (_first == last) {
		return 0;
	}

	cmplt = 0;
	*handle = _radio.pkt_release[_first].handle;
	do {
		struct radio_pdu_node_tx *node_tx;
		struct pdu_data *pdu_data_tx;

		if (*handle != _radio.pkt_release[_first].handle) {
			break;
		}

		node_tx = _radio.pkt_release[_first].node_tx;
		/*@FIXME: assign before first 3 if conditions */
		pdu_data_tx = (void *)node_tx->pdu_data;
		if ((!node_tx) || (node_tx == (void *)1) ||
		    ((((u32_t)node_tx & ~(0x00000003)) != 0) &&
		     (pdu_data_tx) && (pdu_data_tx->len != 0) &&
		     ((pdu_data_tx->ll_id == PDU_DATA_LLID_DATA_START) ||
		      (pdu_data_tx->ll_id == PDU_DATA_LLID_DATA_CONTINUE)))) {

			/* data packet, hence count num cmplt */
			_radio.pkt_release[_first].node_tx = (void *)1;

			cmplt++;
		} else {
			/* ctrl packet, hence not num cmplt */
			_radio.pkt_release[_first].node_tx = (void *)2;
		}

		if (((u32_t)node_tx & ~(0x00000003)) != 0) {
			mem_release(node_tx, &_radio.pkt_tx_data_free);
		}

		_first = _first + 1;
		if (_first == _radio.packet_tx_count) {
			_first = 0;
		}

	} while (_first != last);

	*first = _first;

	return cmplt;
}

u8_t ll_rx_get(void **node_rx, u16_t *handle)
{
	u8_t cmplt;

	cmplt = 0;
	if (_radio.link_rx_head != _radio.link_rx_tail) {
		struct radio_pdu_node_rx *_node_rx;

		_node_rx = _radio.link_rx_head->mem;

		cmplt = tx_cmplt_get(handle, &_radio.packet_release_first,
				     _node_rx->hdr.onion.packet_release_last);
		if (!cmplt) {
			u8_t first, cmplt_prev, cmplt_curr;
			u16_t h;

			first = _radio.packet_release_first;
			cmplt_curr = 0;
			do {
				cmplt_prev = cmplt_curr;
				cmplt_curr = tx_cmplt_get(&h, &first,
						_radio.packet_release_last);
			} while ((cmplt_prev != 0) ||
				 (cmplt_prev != cmplt_curr));

			*node_rx = _node_rx;
		} else {
			*node_rx = NULL;
		}
	} else {
		cmplt = tx_cmplt_get(handle, &_radio.packet_release_first,
				     _radio.packet_release_last);

		*node_rx = NULL;
	}

	return cmplt;
}

void ll_rx_dequeue(void)
{
	struct radio_pdu_node_rx *node_rx = NULL;
	memq_link_t *link;

	link = memq_dequeue(_radio.link_rx_tail, &_radio.link_rx_head,
			    (void **)&node_rx);
	LL_ASSERT(link);

	mem_release(link, &_radio.link_rx_free);

	switch (node_rx->hdr.type) {
	case NODE_RX_TYPE_DC_PDU:
	case NODE_RX_TYPE_REPORT:

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	case NODE_RX_TYPE_EXT_1M_REPORT:
	case NODE_RX_TYPE_EXT_CODED_REPORT:
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
	case NODE_RX_TYPE_SCAN_REQ:
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

	case NODE_RX_TYPE_CONNECTION:
	case NODE_RX_TYPE_CONN_UPDATE:
	case NODE_RX_TYPE_ENC_REFRESH:

#if defined(CONFIG_BT_CTLR_LE_PING)
	case NODE_RX_TYPE_APTO:
#endif /* CONFIG_BT_CTLR_LE_PING */

	case NODE_RX_TYPE_CHAN_SEL_ALGO:

#if defined(CONFIG_BT_CTLR_PHY)
	case NODE_RX_TYPE_PHY_UPDATE:
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
	case NODE_RX_TYPE_RSSI:
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

#if defined(CONFIG_BT_CTLR_PROFILE_ISR)
	case NODE_RX_TYPE_PROFILE:
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

#if defined(CONFIG_BT_CTLR_ADV_INDICATION)
	case NODE_RX_TYPE_ADV_INDICATION:
#endif /* CONFIG_BT_CTLR_ADV_INDICATION */

		/* release data link credit quota */
		LL_ASSERT(_radio.link_rx_data_quota <
			  (_radio.packet_rx_count - 1));

		_radio.link_rx_data_quota++;
		break;

	case NODE_RX_TYPE_TERMINATE:
		/* did not use data link quota */
		break;

	default:
		LL_ASSERT(0);
		break;
	}

	if (node_rx->hdr.type == NODE_RX_TYPE_CONNECTION) {
		struct radio_le_conn_cmplt *radio_le_conn_cmplt;
		struct connection *conn = NULL;
		struct pdu_data *pdu_data_rx;
		u8_t bm;

		pdu_data_rx = (void *)node_rx->pdu_data;
		radio_le_conn_cmplt = (void *)pdu_data_rx->lldata;
		if ((radio_le_conn_cmplt->status == BT_HCI_ERR_ADV_TIMEOUT) ||
		    radio_le_conn_cmplt->role) {
			if (radio_le_conn_cmplt->status ==
			    BT_HCI_ERR_ADV_TIMEOUT) {
				conn = _radio.advertiser.conn;
				_radio.advertiser.conn = NULL;
			}

			LL_ASSERT(_radio.advertiser.is_enabled);
			_radio.advertiser.is_enabled = 0;
		} else {
			LL_ASSERT(_radio.scanner.is_enabled);
			_radio.scanner.is_enabled = 0;
		}

		if (conn) {
			struct radio_pdu_node_rx *node_rx = (void *)
				&conn->llcp_terminate.radio_pdu_node_rx;

			mem_release(node_rx->hdr.onion.link,
				    &_radio.link_rx_free);
			mem_release(conn, &_radio.conn_free);
		}

		bm = ((u8_t)_radio.scanner.is_enabled << 1) |
		     _radio.advertiser.is_enabled;

		if (!bm) {
			ll_adv_scan_state_cb(0);
		}
	}
}

void ll_rx_mem_release(void **node_rx)
{
	struct radio_pdu_node_rx *_node_rx;
	struct connection *conn;

	_node_rx = *node_rx;
	while (_node_rx) {
		struct radio_pdu_node_rx *_node_rx_free;

		_node_rx_free = _node_rx;
		_node_rx = _node_rx->hdr.onion.next;

		switch (_node_rx_free->hdr.type) {
		case NODE_RX_TYPE_DC_PDU:
		case NODE_RX_TYPE_REPORT:

#if defined(CONFIG_BT_CTLR_ADV_EXT)
		case NODE_RX_TYPE_EXT_1M_REPORT:
		case NODE_RX_TYPE_EXT_CODED_REPORT:
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
		case NODE_RX_TYPE_SCAN_REQ:
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

		case NODE_RX_TYPE_CONNECTION:
		case NODE_RX_TYPE_CONN_UPDATE:
		case NODE_RX_TYPE_ENC_REFRESH:

#if defined(CONFIG_BT_CTLR_LE_PING)
		case NODE_RX_TYPE_APTO:
#endif /* CONFIG_BT_CTLR_LE_PING */

		case NODE_RX_TYPE_CHAN_SEL_ALGO:

#if defined(CONFIG_BT_CTLR_PHY)
		case NODE_RX_TYPE_PHY_UPDATE:
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
		case NODE_RX_TYPE_RSSI:
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

#if defined(CONFIG_BT_CTLR_PROFILE_ISR)
		case NODE_RX_TYPE_PROFILE:
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

#if defined(CONFIG_BT_CTLR_ADV_INDICATION)
		case NODE_RX_TYPE_ADV_INDICATION:
#endif /* CONFIG_BT_CTLR_ADV_INDICATION */

			mem_release(_node_rx_free,
				    &_radio.pkt_rx_data_free);
			break;

		case NODE_RX_TYPE_TERMINATE:
			conn = mem_get(_radio.conn_pool, CONNECTION_T_SIZE,
				       _node_rx_free->hdr.handle);

			mem_release(conn, &_radio.conn_free);
			break;

		default:
			LL_ASSERT(0);
			break;
		}
	}

	*node_rx = _node_rx;

	packet_rx_allocate(0xff);
}

static void rx_fc_lock(u16_t handle)
{
	if (_radio.fc_req == _radio.fc_ack) {
		u8_t req;

		_radio.fc_handle[_radio.fc_req] = handle;
		req = _radio.fc_req + 1;
		if (req == TRIPLE_BUFFER_SIZE) {
			req = 0;
		}
		_radio.fc_req = req;
	}
}

u8_t do_radio_rx_fc_set(u16_t handle, u8_t req, u8_t ack)
{
	if (req == ack) {
		if (_radio.link_rx_head == _radio.link_rx_tail) {
			u8_t ack1 = ack;

			if (ack1 == 0) {
				ack1 = TRIPLE_BUFFER_SIZE;
			}
			_radio.fc_handle[--ack1] = handle;
			_radio.fc_ack = ack1;

			/* check if ISR updated FC by changing fc_req */
			if (req != _radio.fc_req) {
				_radio.fc_ack = ack;

				return 1;
			}
		} else {
			return 1;
		}
	} else if (((req == 0) &&
		    (_radio.fc_handle[TRIPLE_BUFFER_SIZE - 1] != handle)) ||
		   ((req != 0) && (_radio.fc_handle[req - 1] != handle))) {
		return 1;
	}

	return 0;
}

u8_t radio_rx_fc_set(u16_t handle, u8_t fc)
{
	if (_radio.fc_ena) {
		u8_t req = _radio.fc_req;
		u8_t ack = _radio.fc_ack;

		if (fc) {
			if (handle != 0xffff) {
				return do_radio_rx_fc_set(handle, req, ack);
			}
		} else if ((_radio.link_rx_head == _radio.link_rx_tail) &&
			   (req != ack)
		    ) {
			_radio.fc_ack = req;

			if ((_radio.link_rx_head != _radio.link_rx_tail) &&
			    (req == _radio.fc_req)) {
				_radio.fc_ack = ack;
			}
		}
	}

	return 0;
}

u8_t radio_rx_fc_get(u16_t *handle)
{
	u8_t req = _radio.fc_req;
	u8_t ack = _radio.fc_ack;

	if (req != ack) {
		if (handle) {
			*handle = _radio.fc_handle[ack];
		}
		return 1;
	}

	return 0;
}

void *ll_tx_mem_acquire(void)
{
	return mem_acquire(&_radio.pkt_tx_data_free);
}

void ll_tx_mem_release(void *node_tx)
{
	mem_release(node_tx, &_radio.pkt_tx_data_free);
}

static void ticker_op_latency_cancelled(u32_t ticker_status, void *params)
{
	struct connection *conn = params;

	LL_ASSERT(ticker_status == TICKER_STATUS_SUCCESS);

	conn->slave.latency_cancel = 0;
}

u32_t ll_tx_mem_enqueue(u16_t handle, void *node_tx)
{
	u8_t last;
	struct connection *conn;
	struct pdu_data *pdu_data;

	last = _radio.packet_tx_last + 1;
	if (last == _radio.packet_tx_count) {
		last = 0;
	}

	pdu_data = (void *)((struct radio_pdu_node_tx *)node_tx)->pdu_data;
	conn = connection_get(handle);
	if (!conn || (last == _radio.packet_tx_first)) {
		return 1;
	}

	LL_ASSERT(pdu_data->len <= (_radio.packet_tx_data_size -
				    offsetof(struct radio_pdu_node_tx,
					     pdu_data) -
				    offsetof(struct pdu_data, lldata)));

	_radio.pkt_tx[_radio.packet_tx_last].handle = handle;
	_radio.pkt_tx[_radio.packet_tx_last].node_tx = node_tx;
	_radio.packet_tx_last = last;

	/* break slave latency */
	if (conn->role && conn->latency_event && !conn->slave.latency_cancel) {
		u32_t ticker_status;

		conn->slave.latency_cancel = 1;

		ticker_status = ticker_update(RADIO_TICKER_INSTANCE_ID_RADIO,
				 RADIO_TICKER_USER_ID_APP,
				 RADIO_TICKER_ID_FIRST_CONNECTION +
				 conn->handle, 0, 0, 0, 0, 1, 0,
				 ticker_op_latency_cancelled,
				 (void *)conn);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY));
	}

	return 0;
}

void __weak ll_adv_scan_state_cb(u8_t bm)
{
}
