/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
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
#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
#define EVENT_DONE_LINK_CNT 0
#else
#define EVENT_DONE_LINK_CNT 1
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */

#define ADV_INT_UNIT_US      625U
#define SCAN_INT_UNIT_US     625U
#define CONN_INT_UNIT_US     1250U
#define ISO_INT_UNIT_US      CONN_INT_UNIT_US
#define PERIODIC_INT_UNIT_US 1250U

/* Timeout for Host to accept/reject cis create request */
/* See BTCore5.3, 4.E.6.7 - Default value 0x1f40 * 625us */
#define DEFAULT_CONNECTION_ACCEPT_TIMEOUT_US (5 * USEC_PER_SEC)

/* Intervals after which connection or sync establishment is considered lost */
#define CONN_ESTAB_COUNTDOWN 6U

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
#define XON_BITMASK BIT(31) /* XTAL has been retained from previous prepare */
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */

#if defined(CONFIG_BT_BROADCASTER)
#if defined(CONFIG_BT_CTLR_ADV_SET)
#define BT_CTLR_ADV_SET CONFIG_BT_CTLR_ADV_SET
#else /* CONFIG_BT_CTLR_ADV_SET */
#define BT_CTLR_ADV_SET 1
#endif /* CONFIG_BT_CTLR_ADV_SET */
#else /* !CONFIG_BT_BROADCASTER */
#define BT_CTLR_ADV_SET 0
#endif /* !CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
#if defined(CONFIG_BT_CTLR_ADV_EXT)
#if defined(CONFIG_BT_CTLR_PHY_CODED)
#define BT_CTLR_SCAN_SET 2
#else /* !CONFIG_BT_CTLR_PHY_CODED */
#define BT_CTLR_SCAN_SET 1
#endif /* !CONFIG_BT_CTLR_PHY_CODED */
#else /* !CONFIG_BT_CTLR_ADV_EXT */
#define BT_CTLR_SCAN_SET 1
#endif /* !CONFIG_BT_CTLR_ADV_EXT */
#else /* !CONFIG_BT_OBSERVER */
#define BT_CTLR_SCAN_SET 0
#endif /* !CONFIG_BT_OBSERVER */

enum {
	TICKER_ID_LLL_PREEMPT = 0,

#if defined(CONFIG_BT_BROADCASTER)
	TICKER_ID_ADV_STOP,
	TICKER_ID_ADV_BASE,
#if defined(CONFIG_BT_CTLR_ADV_EXT) || defined(CONFIG_BT_HCI_MESH_EXT)
	TICKER_ID_ADV_LAST = ((TICKER_ID_ADV_BASE) + (BT_CTLR_ADV_SET) - 1),
#if defined(CONFIG_BT_CTLR_ADV_EXT)
#if (CONFIG_BT_CTLR_ADV_AUX_SET > 0)
	TICKER_ID_ADV_AUX_BASE,
	TICKER_ID_ADV_AUX_LAST = ((TICKER_ID_ADV_AUX_BASE) +
				  (CONFIG_BT_CTLR_ADV_AUX_SET) - 1),
#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
	TICKER_ID_ADV_SYNC_BASE,
	TICKER_ID_ADV_SYNC_LAST = ((TICKER_ID_ADV_SYNC_BASE) +
				   (CONFIG_BT_CTLR_ADV_SYNC_SET) - 1),
#if defined(CONFIG_BT_CTLR_ADV_ISO)
	TICKER_ID_ADV_ISO_BASE,
	TICKER_ID_ADV_ISO_LAST = ((TICKER_ID_ADV_ISO_BASE) +
				  (CONFIG_BT_CTLR_ADV_ISO_SET) - 1),
#endif /* CONFIG_BT_CTLR_ADV_ISO */
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_AUX_SET > 0 */
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#endif /* !CONFIG_BT_CTLR_ADV_EXT || !CONFIG_BT_HCI_MESH_EXT */
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
	TICKER_ID_SCAN_STOP,
	TICKER_ID_SCAN_BASE,
	TICKER_ID_SCAN_LAST = ((TICKER_ID_SCAN_BASE) + (BT_CTLR_SCAN_SET) - 1),
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	TICKER_ID_SCAN_AUX_BASE,
	TICKER_ID_SCAN_AUX_LAST = ((TICKER_ID_SCAN_AUX_BASE) +
				   (CONFIG_BT_CTLR_SCAN_AUX_SET) - 1),
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	TICKER_ID_SCAN_SYNC_BASE,
	TICKER_ID_SCAN_SYNC_LAST = ((TICKER_ID_SCAN_SYNC_BASE) +
				    (CONFIG_BT_PER_ADV_SYNC_MAX) - 1),
#if defined(CONFIG_BT_CTLR_SYNC_ISO)
	TICKER_ID_SCAN_SYNC_ISO_BASE,
	TICKER_ID_SCAN_SYNC_ISO_LAST = ((TICKER_ID_SCAN_SYNC_ISO_BASE) +
					(CONFIG_BT_CTLR_SCAN_SYNC_ISO_SET) - 1),
#endif /* CONFIG_BT_CTLR_SYNC_ISO */
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CONN)
	TICKER_ID_CONN_BASE,
	TICKER_ID_CONN_LAST = ((TICKER_ID_CONN_BASE) + (CONFIG_BT_MAX_CONN) -
			       1),
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CTLR_CONN_ISO)
	TICKER_ID_CONN_ISO_BASE,
	TICKER_ID_CONN_ISO_LAST = ((TICKER_ID_CONN_ISO_BASE) +
				   (CONFIG_BT_CTLR_CONN_ISO_GROUPS) - 1),
	TICKER_ID_CONN_ISO_RESUME_BASE,
	TICKER_ID_CONN_ISO_RESUME_LAST = ((TICKER_ID_CONN_ISO_RESUME_BASE) +
					  (CONFIG_BT_CTLR_CONN_ISO_GROUPS) - 1),
#endif /* CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_USER_EXT) && \
	(CONFIG_BT_CTLR_USER_TICKER_ID_RANGE > 0)
	TICKER_ID_USER_BASE,
	TICKER_ID_USER_LAST = (TICKER_ID_USER_BASE +
			       CONFIG_BT_CTLR_USER_TICKER_ID_RANGE - 1),
#endif /* CONFIG_BT_CTLR_USER_EXT */

	TICKER_ID_MAX,
};

#if defined(CONFIG_BT_BROADCASTER) && !defined(CONFIG_BT_CTLR_ADV_EXT) && \
	!defined(CONFIG_BT_HCI_MESH_EXT)
#define TICKER_ID_ADV_LAST TICKER_ID_ADV_BASE
#endif

/* Define the Broadcast ISO Stream Handle base value */
#if defined(CONFIG_BT_CTLR_ADV_ISO)
#define BT_CTLR_ADV_ISO_STREAM_MAX CONFIG_BT_CTLR_ADV_ISO_STREAM_MAX
#if defined(CONFIG_BT_MAX_CONN)
#define BT_CTLR_ADV_ISO_STREAM_HANDLE_BASE (CONFIG_BT_MAX_CONN)
#else /* !CONFIG_BT_MAX_CONN */
#define BT_CTLR_ADV_ISO_STREAM_HANDLE_BASE 0
#endif /* !CONFIG_BT_MAX_CONN */
#define LL_BIS_ADV_HANDLE_FROM_IDX(stream_handle) \
	((stream_handle) + (BT_CTLR_ADV_ISO_STREAM_HANDLE_BASE))
#else /* !CONFIG_BT_CTLR_ADV_ISO */
#define BT_CTLR_ADV_ISO_STREAM_MAX 0
#endif /* CONFIG_BT_CTLR_ADV_ISO */

/* Define the ISO Synchronized Receiver Stream Handle base value */
#if defined(CONFIG_BT_CTLR_SYNC_ISO)
#define BT_CTLR_SYNC_ISO_STREAM_MAX CONFIG_BT_CTLR_SYNC_ISO_STREAM_MAX
#if defined(CONFIG_BT_CTLR_ADV_ISO)
#define BT_CTLR_SYNC_ISO_STREAM_HANDLE_BASE \
	(BT_CTLR_ADV_ISO_STREAM_HANDLE_BASE + \
	 CONFIG_BT_CTLR_ADV_ISO_STREAM_COUNT)
#elif defined(CONFIG_BT_MAX_CONN)
#define BT_CTLR_SYNC_ISO_STREAM_HANDLE_BASE (CONFIG_BT_MAX_CONN)
#else /* !CONFIG_BT_MAX_CONN */
#define BT_CTLR_SYNC_ISO_STREAM_HANDLE_BASE 0
#endif /* !CONFIG_BT_MAX_CONN */
#define LL_BIS_SYNC_HANDLE_FROM_IDX(stream_handle) \
	((stream_handle) + (BT_CTLR_SYNC_ISO_STREAM_HANDLE_BASE))
#else /* !CONFIG_BT_CTLR_SYNC_ISO */
#define BT_CTLR_SYNC_ISO_STREAM_MAX 0
#endif /* !CONFIG_BT_CTLR_SYNC_ISO */

/* Define the ISO Connections Stream Handle base value */
#if defined(CONFIG_BT_CTLR_CONN_ISO)
#if defined(CONFIG_BT_CTLR_SYNC_ISO)
#define BT_CTLR_CONN_ISO_STREAM_HANDLE_BASE \
	(BT_CTLR_SYNC_ISO_STREAM_HANDLE_BASE + \
	 CONFIG_BT_CTLR_SYNC_ISO_STREAM_COUNT)
#elif defined(CONFIG_BT_CTLR_ADV_ISO)
#define BT_CTLR_CONN_ISO_STREAM_HANDLE_BASE \
	(BT_CTLR_ADV_ISO_STREAM_HANDLE_BASE + \
	 CONFIG_BT_CTLR_ADV_ISO_STREAM_COUNT)
#else /* !CONFIG_BT_CTLR_ADV_ISO && !CONFIG_BT_CTLR_SYNC_ISO */
#define BT_CTLR_CONN_ISO_STREAM_HANDLE_BASE (CONFIG_BT_MAX_CONN)
#endif /* !CONFIG_BT_CTLR_ADV_ISO && !CONFIG_BT_CTLR_SYNC_ISO */
#define LL_CIS_HANDLE_BASE (BT_CTLR_CONN_ISO_STREAM_HANDLE_BASE)
#define LL_CIS_IDX_FROM_HANDLE(handle) \
	((handle) - LL_CIS_HANDLE_BASE)
#endif /* CONFIG_BT_CTLR_CONN_ISO */

#define TICKER_ID_ULL_BASE ((TICKER_ID_LLL_PREEMPT) + 1)

enum done_result {
	DONE_COMPLETED,
	DONE_ABORTED,
	DONE_LATE
};

/* Forward declaration data type to store CTE IQ samples report related data */
struct cte_conn_iq_report;

struct ull_hdr {
	uint8_t volatile ref;  /* Number of ongoing (between Prepare and Done)
				* events
				*/

	/* Event parameters */
	/* TODO: The intention is to use the greater of the
	 *       ticks_prepare_to_start or ticks_active_to_start as the prepare
	 *       offset. At the prepare tick generate a software interrupt
	 *       serviceable by application as the per role configurable advance
	 *       radio event notification, usable for data acquisitions.
	 *       ticks_preempt_to_start is the per role dynamic preempt offset,
	 *       which shall be based on role's preparation CPU usage
	 *       requirements.
	 */
	struct {
		uint32_t ticks_active_to_start;
		uint32_t ticks_prepare_to_start;
		uint32_t ticks_preempt_to_start;
		uint32_t ticks_slot;
	};

	/* ULL context disabled callback and its parameter */
	void (*disabled_cb)(void *param);
	void *disabled_param;
};

struct lll_hdr {
	void *parent;
#if defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
	uint8_t score;
	uint8_t latency;
	int8_t  prio;
#endif /* CONFIG_BT_CTLR_JIT_SCHEDULING */
};

#define HDR_LLL2ULL(p) (((struct lll_hdr *)(p))->parent)

struct lll_prepare_param {
	uint32_t ticks_at_expire;
	uint32_t remainder;
	uint16_t lazy;
#if defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
	int8_t  prio;
#endif /* CONFIG_BT_CTLR_JIT_SCHEDULING */
	uint8_t force;
	void *param;
};

typedef int (*lll_prepare_cb_t)(struct lll_prepare_param *prepare_param);
typedef int (*lll_is_abort_cb_t)(void *next, void *curr,
				 lll_prepare_cb_t *resume_cb);
typedef void (*lll_abort_cb_t)(struct lll_prepare_param *prepare_param,
			       void *param);

struct lll_event {
	struct lll_prepare_param prepare_param;
	lll_prepare_cb_t         prepare_cb;
	lll_is_abort_cb_t        is_abort_cb;
	lll_abort_cb_t           abort_cb;
	uint8_t                  is_resume:1;
	uint8_t                  is_aborted:1;
};

#define DEFINE_NODE_RX_USER_TYPE(i, _) NODE_RX_TYPE_##i

enum node_rx_type {
	/* Unused */
	NODE_RX_TYPE_NONE = 0x00,
	/* Signals release of node */
	NODE_RX_TYPE_RELEASE,
	/* Signals completion of RX event */
	NODE_RX_TYPE_EVENT_DONE,
	/* Signals arrival of RX Data Channel payload */
	NODE_RX_TYPE_DC_PDU,
	/* Signals arrival of isochronous payload */
	NODE_RX_TYPE_ISO_PDU,
	/* Advertisement report from scanning */
	NODE_RX_TYPE_REPORT,
	NODE_RX_TYPE_EXT_1M_REPORT,
	NODE_RX_TYPE_EXT_2M_REPORT,
	NODE_RX_TYPE_EXT_CODED_REPORT,
	NODE_RX_TYPE_EXT_AUX_REPORT,
	NODE_RX_TYPE_EXT_AUX_RELEASE,
	NODE_RX_TYPE_EXT_SCAN_TERMINATE,
	NODE_RX_TYPE_SYNC,
	NODE_RX_TYPE_SYNC_REPORT,
	NODE_RX_TYPE_SYNC_LOST,
	NODE_RX_TYPE_SYNC_CHM_COMPLETE,
	NODE_RX_TYPE_SYNC_ISO,
	NODE_RX_TYPE_SYNC_ISO_LOST,
	NODE_RX_TYPE_EXT_ADV_TERMINATE,
	NODE_RX_TYPE_BIG_COMPLETE,
	NODE_RX_TYPE_BIG_CHM_COMPLETE,
	NODE_RX_TYPE_BIG_TERMINATE,
	NODE_RX_TYPE_SCAN_REQ,
	NODE_RX_TYPE_CONNECTION,
	NODE_RX_TYPE_TERMINATE,
	NODE_RX_TYPE_CONN_UPDATE,
	NODE_RX_TYPE_ENC_REFRESH,
	NODE_RX_TYPE_APTO,
	NODE_RX_TYPE_CHAN_SEL_ALGO,
	NODE_RX_TYPE_PHY_UPDATE,
	NODE_RX_TYPE_RSSI,
	NODE_RX_TYPE_PROFILE,
	NODE_RX_TYPE_ADV_INDICATION,
	NODE_RX_TYPE_SCAN_INDICATION,
	NODE_RX_TYPE_CIS_REQUEST,
	NODE_RX_TYPE_CIS_ESTABLISHED,
	NODE_RX_TYPE_REQ_PEER_SCA_COMPLETE,
	NODE_RX_TYPE_MESH_ADV_CPLT,
	NODE_RX_TYPE_MESH_REPORT,
	NODE_RX_TYPE_SYNC_IQ_SAMPLE_REPORT,
	NODE_RX_TYPE_CONN_IQ_SAMPLE_REPORT,
	NODE_RX_TYPE_DTM_IQ_SAMPLE_REPORT,
	NODE_RX_TYPE_IQ_SAMPLE_REPORT_ULL_RELEASE,
	NODE_RX_TYPE_IQ_SAMPLE_REPORT_LLL_RELEASE,

#if defined(CONFIG_BT_CTLR_USER_EXT)
	/* No entries shall be added after the NODE_RX_TYPE_USER_START/END */
	NODE_RX_TYPE_USER_START,
	LISTIFY(CONFIG_BT_CTLR_USER_EVT_RANGE, DEFINE_NODE_RX_USER_TYPE, (,), _),
	NODE_RX_TYPE_USER_END,
#endif /* CONFIG_BT_CTLR_USER_EXT */
};

/* Footer of node_rx_hdr */
struct node_rx_ftr {
	union {
		void *param;
		struct {
			uint8_t  status;
			uint8_t  num_events;
			uint16_t conn_handle;
		} param_adv_term;
	};
	union {
		void *extra;   /* Used as next pointer for extended PDU
				* chaining, to reserve node_rx for CSA#2 event
				* generation etc.
				*/
		void *aux_ptr;
		uint8_t aux_phy;
		struct cte_conn_iq_report *iq_report;
	};
	uint32_t ticks_anchor;
	uint32_t radio_end_us;
	uint8_t  rssi;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	uint8_t  rl_idx;
	uint8_t  lrpa_used:1;
#endif /* CONFIG_BT_CTLR_PRIVACY */

#if defined(CONFIG_BT_CTLR_EXT_SCAN_FP)
	uint8_t  direct:1;
#endif /* CONFIG_BT_CTLR_EXT_SCAN_FP */

#if defined(CONFIG_BT_CTLR_ADV_EXT) && defined(CONFIG_BT_OBSERVER)
	uint8_t  phy_flags:1;
	uint8_t  scan_req:1;
	uint8_t  scan_rsp:1;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	uint8_t  direct_resolved:1;
#endif /* CONFIG_BT_CTLR_PRIVACY */

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	uint8_t  sync_status:2;
	uint8_t  sync_rx_enabled:1;
#if defined(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST)
	uint8_t  devmatch:1;
#endif /* CONFIG_BT_CTLR_FILTER_ACCEPT_LIST */
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */

	uint8_t  aux_sched:1;
	uint8_t  aux_lll_sched:1;
	uint8_t  aux_failed:1;

	uint16_t aux_data_len;
#endif /* CONFIG_BT_CTLR_ADV_EXT && CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_HCI_MESH_EXT)
	uint8_t  chan_idx;
#endif /* CONFIG_BT_HCI_MESH_EXT */
};

/* Meta-information for isochronous PDUs in node_rx_hdr */
struct node_rx_iso_meta {
	uint64_t payload_number:39; /* cisPayloadNumber */
	uint64_t status:8;          /* Status of reception (OK/not OK) */
	uint32_t timestamp;         /* Time of reception */
};

/* Define invalid/unassigned Controller state/role instance handle */
#define NODE_RX_HANDLE_INVALID 0xFFFF

/* Define invalid/unassigned Controller LLL context handle */
#define LLL_HANDLE_INVALID     0xFFFF

/* Define invalid/unassigned Controller Advertising LLL context handle */
#define LLL_ADV_HANDLE_INVALID 0xFF

/* Header of node_rx_pdu */
struct node_rx_hdr {
	union {
		void        *next;    /* For slist, by hci module */
		memq_link_t *link;    /* Supply memq_link from ULL to LLL */
		uint8_t     ack_last; /* Tx ack queue index at this node rx */
	};

	enum node_rx_type type;
	uint8_t           user_meta; /* User metadata */
	uint16_t          handle;    /* State/Role instance handle */

	union {
		struct node_rx_ftr rx_ftr;
#if defined(CONFIG_BT_CTLR_SYNC_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
		struct node_rx_iso_meta rx_iso_meta;
#endif
#if defined(CONFIG_BT_CTLR_RX_PDU_META)
		lll_rx_pdu_meta_t  rx_pdu_meta;
#endif /* CONFIG_BT_CTLR_RX_PDU_META */
	};
};

/* Template node rx type with memory aligned offset to PDU buffer.
 * NOTE: offset to memory aligned pdu buffer location is used to reference
 *       node rx type specific information, like, terminate or sync lost reason
 *       from a dedicated node rx structure storage location.
 */
struct node_rx_pdu {
	struct node_rx_hdr hdr;
	union {
		uint8_t    pdu[0] __aligned(4);
	};
};

enum {
	EVENT_DONE_EXTRA_TYPE_NONE,

#if defined(CONFIG_BT_CONN)
	EVENT_DONE_EXTRA_TYPE_CONN,
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CTLR_ADV_EXT) || defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
#if defined(CONFIG_BT_BROADCASTER)
	EVENT_DONE_EXTRA_TYPE_ADV,
	EVENT_DONE_EXTRA_TYPE_ADV_AUX,
#if defined(CONFIG_BT_CTLR_ADV_ISO)
	EVENT_DONE_EXTRA_TYPE_ADV_ISO_COMPLETE,
	EVENT_DONE_EXTRA_TYPE_ADV_ISO_TERMINATE,
#endif /* CONFIG_BT_CTLR_ADV_ISO */
#endif /* CONFIG_BT_BROADCASTER */
#endif /* CONFIG_BT_CTLR_ADV_EXT || CONFIG_BT_CTLR_JIT_SCHEDULING */

#if defined(CONFIG_BT_OBSERVER)
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	EVENT_DONE_EXTRA_TYPE_SCAN,
	EVENT_DONE_EXTRA_TYPE_SCAN_AUX,
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	EVENT_DONE_EXTRA_TYPE_SYNC,
#if defined(CONFIG_BT_CTLR_SYNC_ISO)
	EVENT_DONE_EXTRA_TYPE_SYNC_ISO_ESTAB,
	EVENT_DONE_EXTRA_TYPE_SYNC_ISO,
	EVENT_DONE_EXTRA_TYPE_SYNC_ISO_TERMINATE,
#endif /* CONFIG_BT_CTLR_SYNC_ISO */
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CTLR_CONN_ISO)
	EVENT_DONE_EXTRA_TYPE_CIS,
#endif /* CONFIG_BT_CTLR_CONN_ISO */

/* Following proprietary defines must be at end of enum range */
#if defined(CONFIG_BT_CTLR_USER_EXT)
	EVENT_DONE_EXTRA_TYPE_USER_START,
	EVENT_DONE_EXTRA_TYPE_USER_END = EVENT_DONE_EXTRA_TYPE_USER_START +
		CONFIG_BT_CTLR_USER_EVT_RANGE,
#endif /* CONFIG_BT_CTLR_USER_EXT */

};

struct event_done_extra_drift {
	uint32_t start_to_address_actual_us;
	uint32_t window_widening_event_us;
	uint32_t preamble_to_addr_us;
};

struct event_done_extra {
	uint8_t type;
#if defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
	uint8_t result;
#endif /* CONFIG_BT_CTLR_JIT_SCHEDULING */
	union {
		struct {
			union {
#if defined(CONFIG_BT_CTLR_CONN_ISO)
				uint32_t trx_performed_bitmask;
#endif /* CONFIG_BT_CTLR_CONN_ISO */

				struct {
					uint16_t trx_cnt;
					uint8_t  crc_valid:1;
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING) && \
	defined(CONFIG_BT_CTLR_CTEINLINE_SUPPORT)
					/* Used to inform ULL that periodic
					 * advertising sync scan should be
					 * terminated.
					 */
					uint8_t  sync_term:1;
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING && \
	* CONFIG_BT_CTLR_CTEINLINE_SUPPORT
	*/
				};
			};

#if defined(CONFIG_BT_CTLR_LE_ENC)
			uint8_t  mic_state;
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_PERIPHERAL) || defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
			union {
				struct event_done_extra_drift drift;
			};
#endif /* CONFIG_BT_PERIPHERAL || CONFIG_BT_CTLR_SYNC_PERIODIC */
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

#if defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
	hdr->score = 0U;
	hdr->latency = 0U;
#endif /* CONFIG_BT_CTLR_JIT_SCHEDULING */
}

/* If ISO vendor data path is not used, queue directly to ll_iso_rx */
#if defined(CONFIG_BT_CTLR_ISO_VENDOR_DATA_PATH)
#define iso_rx_put(link, rx) ull_iso_rx_put(link, rx)
#define iso_rx_sched() ull_iso_rx_sched()
#else
#define iso_rx_put(link, rx) ll_iso_rx_put(link, rx)
#define iso_rx_sched() ll_rx_sched()
#endif /* CONFIG_BT_CTLR_ISO_VENDOR_DATA_PATH */

struct node_tx_iso;

void lll_done_score(void *param, uint8_t result);

int lll_init(void);
int lll_deinit(void);
int lll_reset(void);
void lll_resume(void *param);
void lll_disable(void *param);
void lll_done_ull_inc(void);
uint32_t lll_radio_is_idle(void);
uint32_t lll_radio_tx_ready_delay_get(uint8_t phy, uint8_t flags);
uint32_t lll_radio_rx_ready_delay_get(uint8_t phy, uint8_t flags);
int8_t lll_radio_tx_pwr_min_get(void);
int8_t lll_radio_tx_pwr_max_get(void);
int8_t lll_radio_tx_pwr_floor(int8_t tx_pwr_lvl);

int lll_csrand_get(void *buf, size_t len);
int lll_csrand_isr_get(void *buf, size_t len);
int lll_rand_get(void *buf, size_t len);
int lll_rand_isr_get(void *buf, size_t len);

struct lll_event *ull_prepare_enqueue(lll_is_abort_cb_t is_abort_cb,
				      lll_abort_cb_t abort_cb,
				      struct lll_prepare_param *prepare_param,
				      lll_prepare_cb_t prepare_cb,
				      uint8_t is_resume);
void *ull_prepare_dequeue_get(void);
void *ull_prepare_dequeue_iter(uint8_t *idx);
void ull_prepare_dequeue(uint8_t caller_id);
void *ull_pdu_rx_alloc_peek(uint8_t count);
void *ull_pdu_rx_alloc_peek_iter(uint8_t *idx);
void *ull_pdu_rx_alloc(void);
void *ull_iso_pdu_rx_alloc_peek(uint8_t count);
void *ull_iso_pdu_rx_alloc(void);
void ull_rx_put(memq_link_t *link, void *rx);
void ull_rx_put_done(memq_link_t *link, void *done);
void ull_rx_sched(void);
void ull_rx_sched_done(void);
void ull_rx_put_sched(memq_link_t *link, void *rx);
void ull_iso_rx_put(memq_link_t *link, void *rx);
void ull_iso_rx_sched(void);
void *ull_iso_tx_ack_dequeue(void);
void ull_iso_lll_ack_enqueue(uint16_t handle, struct node_tx_iso *tx);
void ull_iso_lll_event_prepare(uint16_t handle, uint64_t event_count);
struct event_done_extra *ull_event_done_extra_get(void);
struct event_done_extra *ull_done_extra_type_set(uint8_t type);
void *ull_event_done(void *param);

int lll_prepare(lll_is_abort_cb_t is_abort_cb,
		lll_abort_cb_t abort_cb,
		lll_prepare_cb_t prepare_cb, int8_t event_prio,
		struct lll_prepare_param *prepare_param);
int lll_resume_enqueue(lll_prepare_cb_t resume_cb, int resume_prio);
int lll_prepare_resolve(lll_is_abort_cb_t is_abort_cb, lll_abort_cb_t abort_cb,
			lll_prepare_cb_t prepare_cb,
			struct lll_prepare_param *prepare_param,
			uint8_t is_resume, uint8_t is_dequeue);
