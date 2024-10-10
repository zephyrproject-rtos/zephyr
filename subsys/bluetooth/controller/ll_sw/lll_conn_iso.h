/*
 * Copyright (c) 2021 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LLL_CIS_FLUSH_NONE      0
#define LLL_CIS_FLUSH_PENDING   1
#define LLL_CIS_FLUSH_COMPLETE  2

struct lll_conn_iso_stream_rxtx {
	uint64_t payload_count:39; /* cisPayloadCounter */
	uint64_t phy_flags:1;      /* S2 or S8 coding scheme */
	uint64_t max_pdu:8;        /* Maximum PDU size */
	uint64_t ft:8;             /* Flush timeout (FT) */
	uint64_t bn:4;             /* Burst number (BN) */
	uint64_t phy:3;            /* PHY */
	uint64_t rfu0:1;

	uint8_t bn_curr:4;        /* Current burst number */
	uint8_t rfu1:4;

#if defined(CONFIG_BT_CTLR_LE_ENC)
	struct ccm ccm;
#endif /* CONFIG_BT_CTLR_LE_ENC */
};

struct lll_conn_iso_stream {
	uint16_t acl_handle;        /* ACL connection handle (for encryption,
				     * channel map, crc init)
				     */
	uint16_t handle;            /* CIS handle */

	/* Connection parameters */
	uint8_t  access_addr[4];    /* Access address */
	uint32_t offset;            /* Offset of CIS from start of CIG in us */
	uint32_t sub_interval;      /* Interval between subevents in us */
	uint8_t  nse:5;             /* Number of subevents */
	struct lll_conn_iso_stream_rxtx rx; /* RX parameters */
	struct lll_conn_iso_stream_rxtx tx; /* TX parameters */

	/* Event and payload counters */
	uint64_t event_count:39;    /* cisEventCount */

	/* Acknowledgment and flow control */
	uint8_t sn:1;               /* Sequence number */
	uint8_t nesn:1;             /* Next expected sequence number */
	uint8_t cie:1;              /* Close isochronous event */
	uint8_t npi:1;              /* 1 if CIS LLL has Tx-ed Null PDU Indicator */
	uint8_t flush:2;            /* See states LLL_CIS_FLUSH_XXX */
	uint8_t active:1;           /* 1 if CIS LLL is active */
	uint8_t datapath_ready_rx:1;/* 1 if datapath for RX is ready */

#if !defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
	/* Lazy at CIS active. Number of previously skipped CIG events that is
	 * determined when CIS is made active and subtracted from total CIG
	 * events that where skipped when this CIS gets to use radio for the
	 * first time.
	 */
	uint16_t lazy_active;
#endif /* !CONFIG_BT_CTLR_JIT_SCHEDULING */

	/* Resumption information */
	uint8_t next_subevent;      /* Next subevent to schedule */

	/* Transmission queue */
	MEMQ_DECLARE(tx);
	memq_link_t link_tx;
	memq_link_t *link_tx_free;
};

#define LLL_CONN_ISO_EVENT_COUNT_MAX BIT64_MASK(39)

struct lll_conn_iso_group {
	struct lll_hdr hdr;

	uint16_t handle;      /* CIG handle (internal) */

	/* Resumption information */
	uint16_t resume_cis;  /* CIS handle to schedule at resume */

	/* ISO group information */
	uint32_t num_cis:5;   /* Number of CISes in this CIG */
	uint32_t role:1;      /* 0: CENTRAL, 1: PERIPHERAL*/
	uint32_t paused:1;    /* 1: CIG is paused */
	uint32_t rfu0:1;

	/* ISO interval to calculate timestamp under FT > 1,
	 * maximum ISO interval of 4 seconds can be represented in 22-bits.
	 */
	uint32_t iso_interval_us:22;
	uint32_t rfu1:2;

	/* Accumulates LLL prepare callback latencies */
	uint16_t latency_prepare;
	uint16_t lazy_prepare;
	uint16_t latency_event;

#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
	/* Window widening. Relies on vendor specific conversion macros, e.g.
	 * EVENT_US_FRAC_TO_TICKS().
	 */
	uint32_t window_widening_periodic_us_frac; /* Widening in us fractions
						    * per ISO interval.
						    */
	uint32_t window_widening_prepare_us_frac;  /* Widening in us fractions
						    * for active prepare.
						    */
	uint32_t window_widening_event_us_frac;    /* Accumulated widening in
						    * us fractions for active
						    * event.
						    */
	uint32_t window_widening_max_us;	   /* Maximum widening in us */
#endif /* CONFIG_BT_CTLR_PERIPHERAL_ISO */
};

int lll_conn_iso_init(void);
int lll_conn_iso_reset(void);
void lll_conn_iso_done(struct lll_conn_iso_group *cig, uint32_t trx_performed,
		       uint16_t prog_to_anchor_us, uint8_t mic_state);
void lll_conn_iso_flush(uint16_t handle, struct lll_conn_iso_stream *lll);

extern struct lll_conn_iso_stream *
ull_conn_iso_lll_stream_get_by_group(struct lll_conn_iso_group *cig_lll,
				     uint16_t *handle_iter);
extern struct lll_conn_iso_stream *
ull_conn_iso_lll_stream_sorted_get_by_group(struct lll_conn_iso_group *cig_lll,
					    uint16_t *handle_iter);
extern struct lll_conn_iso_group *
ull_conn_iso_lll_group_get_by_stream(struct lll_conn_iso_stream *cis_lll);
extern struct lll_conn_iso_stream *ull_conn_iso_lll_stream_get(uint16_t handle);
extern void
ull_conn_iso_lll_cis_established(struct lll_conn_iso_stream *cis_lll);
extern void ll_iso_rx_put(memq_link_t *link, void *rx);
extern void ll_rx_sched(void);
