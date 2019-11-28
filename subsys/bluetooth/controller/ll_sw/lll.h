/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#if defined(CONFIG_BT_CTLR_RX_PDU_META)
#include "lll_meta.h"
#endif /* CONFIG_BT_CTLR_RX_PDU_META */

#define TICKER_INSTANCE_ID_CTLR 0
#define TICKER_USER_ID_LLL      MAYFLY_CALL_ID_0
#define TICKER_USER_ID_ULL_HIGH MAYFLY_CALL_ID_1
#define TICKER_USER_ID_ULL_LOW  MAYFLY_CALL_ID_2
#define TICKER_USER_ID_THREAD   MAYFLY_CALL_ID_PROGRAM

#define EVENT_PIPELINE_MAX 7
#define EVENT_DONE_MAX 3

#define HDR_ULL(p)     ((void *)((u8_t *)(p) + sizeof(struct evt_hdr)))
#define HDR_ULL2LLL(p) ((struct lll_hdr *)((u8_t *)(p) + \
					   sizeof(struct ull_hdr)))
#define HDR_LLL2EVT(p) ((struct evt_hdr *)((struct lll_hdr *)(p))->parent)

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
#define XON_BITMASK BIT(31) /* XTAL has been retained from previous prepare */
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */

#if defined(CONFIG_BT_BROADCASTER) && defined(CONFIG_BT_ADV_SET)
#define BT_CTLR_ADV_MAX (CONFIG_BT_ADV_SET + 1)
#else
#define BT_CTLR_ADV_MAX 1
#endif

enum {
	TICKER_ID_LLL_PREEMPT = 0,

#if defined(CONFIG_BT_BROADCASTER)
	TICKER_ID_ADV_STOP,
	TICKER_ID_ADV_BASE,
#if defined(CONFIG_BT_CTLR_ADV_EXT) || defined(CONFIG_BT_HCI_MESH_EXT)
	TICKER_ID_ADV_LAST = ((TICKER_ID_ADV_BASE) + (BT_CTLR_ADV_MAX) - 1),
#endif /* !CONFIG_BT_CTLR_ADV_EXT || !CONFIG_BT_HCI_MESH_EXT */
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
	TICKER_ID_SCAN_STOP,
	TICKER_ID_SCAN_BASE,
	TICKER_ID_SCAN_LAST = TICKER_ID_SCAN_BASE,
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CONN)
	TICKER_ID_CONN_BASE,
	TICKER_ID_CONN_LAST = ((TICKER_ID_CONN_BASE) + (CONFIG_BT_MAX_CONN) -
			       1),
#endif /* CONFIG_BT_CONN */

	TICKER_ID_MAX,
};

#if defined(CONFIG_BT_BROADCASTER) && !defined(CONFIG_BT_CTLR_ADV_EXT) && \
	!defined(CONFIG_BT_HCI_MESH_EXT)
#define TICKER_ID_ADV_LAST TICKER_ID_ADV_BASE
#endif

#define TICKER_ID_ULL_BASE ((TICKER_ID_LLL_PREEMPT) + 1)

enum ull_status {
	ULL_STATUS_SUCCESS,
	ULL_STATUS_FAILURE,
	ULL_STATUS_BUSY,
};

struct evt_hdr {
	u32_t ticks_xtal_to_start;
	u32_t ticks_active_to_start;
	u32_t ticks_preempt_to_start;
	u32_t ticks_slot;
};

struct ull_hdr {
	u8_t ref; /* Number of ongoing (between Prepare and Done) events */
	void (*disabled_cb)(void *param);
	void *disabled_param;
};

struct lll_hdr {
	void *parent;
	u8_t is_stop:1;
};

struct lll_prepare_param {
	u32_t ticks_at_expire;
	u32_t remainder;
	u16_t lazy;
	void  *param;
};

typedef int (*lll_prepare_cb_t)(struct lll_prepare_param *prepare_param);
typedef int (*lll_is_abort_cb_t)(void *next, int prio, void *curr,
				 lll_prepare_cb_t *resume_cb, int *resume_prio);
typedef void (*lll_abort_cb_t)(struct lll_prepare_param *prepare_param,
			       void *param);

struct lll_event {
	struct lll_prepare_param prepare_param;
	lll_prepare_cb_t         prepare_cb;
	lll_is_abort_cb_t        is_abort_cb;
	lll_abort_cb_t           abort_cb;
	int                      prio;
	u8_t                     is_resume:1;
	u8_t                     is_aborted:1;
};

enum node_rx_type {
	/* Unused */
	NODE_RX_TYPE_NONE = 0x00,
	/* Signals completion of RX event */
	NODE_RX_TYPE_EVENT_DONE = 0x01,
	/* Signals arrival of RX Data Channel payload */
	NODE_RX_TYPE_DC_PDU = 0x02,
	/* Signals release of RX Data Channel payload */
	NODE_RX_TYPE_DC_PDU_RELEASE = 0x03,

#if defined(CONFIG_BT_OBSERVER)
	/* Advertisement report from scanning */
	NODE_RX_TYPE_REPORT = 0x04,
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	NODE_RX_TYPE_EXT_1M_REPORT = 0x05,
	NODE_RX_TYPE_EXT_CODED_REPORT = 0x06,
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
	NODE_RX_TYPE_SCAN_REQ = 0x07,
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

#if defined(CONFIG_BT_CONN)
	NODE_RX_TYPE_CONNECTION = 0x08,
	NODE_RX_TYPE_TERMINATE = 0x09,
	NODE_RX_TYPE_CONN_UPDATE = 0x0A,
	NODE_RX_TYPE_ENC_REFRESH = 0x0B,

#if defined(CONFIG_BT_CTLR_LE_PING)
	NODE_RX_TYPE_APTO = 0x0C,
#endif /* CONFIG_BT_CTLR_LE_PING */

	NODE_RX_TYPE_CHAN_SEL_ALGO = 0x0D,

#if defined(CONFIG_BT_CTLR_PHY)
	NODE_RX_TYPE_PHY_UPDATE = 0x0E,
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
	NODE_RX_TYPE_RSSI = 0x0F,
#endif /* CONFIG_BT_CTLR_CONN_RSSI */
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CTLR_PROFILE_ISR)
	NODE_RX_TYPE_PROFILE = 0x10,
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

#if defined(CONFIG_BT_CTLR_ADV_INDICATION)
	NODE_RX_TYPE_ADV_INDICATION = 0x11,
#endif /* CONFIG_BT_CTLR_ADV_INDICATION */

#if defined(CONFIG_BT_CTLR_SCAN_INDICATION)
	NODE_RX_TYPE_SCAN_INDICATION = 0x12,
#endif /* CONFIG_BT_CTLR_SCAN_INDICATION */

#if defined(CONFIG_BT_HCI_MESH_EXT)
	NODE_RX_TYPE_MESH_ADV_CPLT = 0x13,
	NODE_RX_TYPE_MESH_REPORT = 0x14,
#endif /* CONFIG_BT_HCI_MESH_EXT */

/* Following proprietary defines must be at end of enum range */
#if defined(CONFIG_BT_CTLR_USER_EXT)
	NODE_RX_TYPE_USER_START = 0x15,
	NODE_RX_TYPE_USER_END = NODE_RX_TYPE_USER_START +
				CONFIG_BT_CTLR_USER_EVT_RANGE,
#endif /* CONFIG_BT_CTLR_USER_EXT */

};

/* Footer of node_rx_hdr */
struct node_rx_ftr {
	void  *param;
	void  *extra;
	u32_t ticks_anchor;
	u32_t us_radio_end;
	u32_t us_radio_rdy;
	u8_t  rssi;
#if defined(CONFIG_BT_CTLR_PRIVACY)
	u8_t  rl_idx;
#endif /* CONFIG_BT_CTLR_PRIVACY */
#if defined(CONFIG_BT_CTLR_EXT_SCAN_FP)
	u8_t  direct;
#endif /* CONFIG_BT_CTLR_EXT_SCAN_FP */
#if defined(CONFIG_BT_HCI_MESH_EXT)
	u8_t  chan_idx;
#endif /* CONFIG_BT_HCI_MESH_EXT */
};


/* Header of node_rx_pdu */
struct node_rx_hdr {
	union {
		void        *next;
		memq_link_t *link;
		u8_t        ack_last;
	};

	enum node_rx_type   type;
	u8_t                user_meta; /* User metadata */
	u16_t               handle;

	union {
#if defined(CONFIG_BT_CTLR_RX_PDU_META)
		lll_rx_pdu_meta_t  rx_pdu_meta;
#endif /* CONFIG_BT_CTLR_RX_PDU_META */
		struct node_rx_ftr rx_ftr;
	};
};

struct node_rx_pdu {
	struct node_rx_hdr hdr;
	u8_t               pdu[0];
};

enum {
	EVENT_DONE_EXTRA_TYPE_NONE,
	EVENT_DONE_EXTRA_TYPE_CONN,
/* Following proprietary defines must be at end of enum range */
#if defined(CONFIG_BT_CTLR_USER_EXT)
	EVENT_DONE_EXTRA_TYPE_USER_START,
	EVENT_DONE_EXTRA_TYPE_USER_END = EVENT_DONE_EXTRA_TYPE_USER_START +
		CONFIG_BT_CTLR_USER_EVT_RANGE,
#endif /* CONFIG_BT_CTLR_USER_EXT */

};

struct event_done_extra_slave {
	u32_t start_to_address_actual_us;
	u32_t window_widening_event_us;
	u32_t preamble_to_addr_us;
};

struct event_done_extra {
	u8_t type;
	union {
		struct {
			u16_t trx_cnt;
			u8_t  crc_valid;
#if defined(CONFIG_BT_CTLR_LE_ENC)
			u8_t  mic_state;
#endif /* CONFIG_BT_CTLR_LE_ENC */
			union {
				struct event_done_extra_slave slave;
			};
		};
	};
};

struct node_rx_event_done {
	struct node_rx_hdr      hdr;
	void                    *param;
	struct event_done_extra extra;
};

static inline void lll_hdr_init(void *lll, void *parent)
{
	struct lll_hdr *hdr = lll;

	hdr->parent = parent;
	hdr->is_stop = 0U;
}

static inline int lll_stop(void *lll)
{
	struct lll_hdr *hdr = lll;
	int ret = !!hdr->is_stop;

	hdr->is_stop = 1U;

	return ret;
}

static inline int lll_is_stop(void *lll)
{
	struct lll_hdr *hdr = lll;

	return !!hdr->is_stop;
}

int lll_init(void);
int lll_reset(void);
int lll_prepare(lll_is_abort_cb_t is_abort_cb, lll_abort_cb_t abort_cb,
		lll_prepare_cb_t prepare_cb, int prio,
		struct lll_prepare_param *prepare_param);
void lll_resume(void *param);
void lll_disable(void *param);
u32_t lll_radio_is_idle(void);

int ull_prepare_enqueue(lll_is_abort_cb_t is_abort_cb,
			       lll_abort_cb_t abort_cb,
			       struct lll_prepare_param *prepare_param,
			       lll_prepare_cb_t prepare_cb, int prio,
			       u8_t is_resume);
void *ull_prepare_dequeue_get(void);
void *ull_prepare_dequeue_iter(u8_t *idx);
void *ull_pdu_rx_alloc_peek(u8_t count);
void *ull_pdu_rx_alloc_peek_iter(u8_t *idx);
void *ull_pdu_rx_alloc(void);
void ull_rx_put(memq_link_t *link, void *rx);
void ull_rx_sched(void);
void *ull_event_done_extra_get(void);
void *ull_event_done(void *param);
