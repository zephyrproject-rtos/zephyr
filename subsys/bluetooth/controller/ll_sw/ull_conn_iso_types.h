/*
 * Copyright (c) 2021 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct ll_conn;

typedef void (*ll_iso_stream_released_cb_t)(struct ll_conn *conn);

#define CIG_STATE_NO_CIG       0
#define CIG_STATE_CONFIGURABLE 1 /* Central only */
#define CIG_STATE_INITIATING   2 /* Central only */
#define CIG_STATE_ACTIVE       3
#define CIG_STATE_INACTIVE     4

struct ll_conn_iso_stream {
	struct ll_iso_stream_hdr hdr;
	struct lll_conn_iso_stream lll;

	struct ll_conn_iso_group *group;

	uint16_t c_max_sdu:12;    /* Maximum SDU size C_To_P */
	uint16_t p_max_sdu:12;    /* Maximum SDU size P_To_C */
	uint8_t  framed:1;        /* 0: unframed, 1: framed */
	uint16_t established:1;   /* 0 if CIS has not yet been established.
				   * 1 if CIS has been established and host
				   * notified.
				   */
	uint16_t trx_performed:1; /* 1 if CIS had a transaction */
	uint16_t teardown:1;      /* 1 if CIS teardown has been initiated */

	union {
		struct {
			uint8_t  c_rtn;
			uint8_t  p_rtn;
			uint16_t instant;
		} central;
	};

	uint32_t offset;          /* Offset of CIS from ACL event in us */
	uint32_t sync_delay;

	ll_iso_stream_released_cb_t released_cb; /* CIS release callback */

	uint16_t event_expire;     /* Supervision & Connect Timeout event
				    * counter
				    */
	uint8_t  terminate_reason;
	uint8_t  cis_id;

#if defined(CONFIG_BT_CTLR_ISOAL_PSN_IGNORE)
	uint64_t pkt_seq_num:39;
#endif /* CONFIG_BT_CTLR_ISOAL_PSN_IGNORE */
};

struct ll_conn_iso_group {
	struct ull_hdr            ull;
	struct lll_conn_iso_group lll;

	uint32_t c_sdu_interval;
	uint32_t p_sdu_interval;
	uint32_t c_latency;	/* Transport_Latency_M_To_S = CIG_Sync_Delay +
				 * (FT_M_To_S) × ISO_Interval +
				 * SDU_Interval_M_To_S
				 */
	uint32_t p_latency;	/* Transport_Latency_S_To_M = CIG_Sync_Delay +
				 * (FT_S_To_M) × ISO_Interval +
				 * SDU_Interval_S_To_M
				 */
	uint32_t cig_ref_point;	/* CIG reference point timestamp (us) based on
				 * controller's clock.
				 */
	uint32_t sync_delay;

	uint16_t iso_interval;
	uint8_t  cig_id;

	uint8_t  state:3;       /* CIG_STATE_NO_CIG, CIG_STATE_CONFIGURABLE (central only),
				 * CIG_STATE_INITIATING (central only), CIG_STATE_ACTIVE or
				 * CIG_STATE_INACTIVE.
				 */
	uint8_t  sca_update:4;  /* (new SCA)+1 to trigger restart of ticker */

	union {
		struct {
			uint8_t sca;
			uint8_t packing;
			uint8_t framing;
			uint8_t test:1; /* HCI_LE_Set_CIG_Parameters_Test */
		} central;
	};
};

struct node_rx_conn_iso_req {
	uint16_t cis_handle;
	uint8_t  cig_id;
	uint8_t  cis_id;
};

struct node_rx_conn_iso_estab {
	uint16_t cis_handle;
	uint8_t  status;
};
