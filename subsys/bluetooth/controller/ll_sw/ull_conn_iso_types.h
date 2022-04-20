/*
 * Copyright (c) 2021 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct ll_conn;

typedef void (*ll_iso_stream_released_cb_t)(struct ll_conn *conn);


struct ll_conn_iso_stream {
	struct ll_iso_stream_hdr hdr;
	struct ll_conn_iso_group *group;
	struct lll_conn_iso_stream lll;
	uint32_t sync_delay;
	uint8_t  cis_id;
	uint8_t  terminate_reason;
	uint32_t offset;          /* Offset of CIS from ACL event in us */
	ll_iso_stream_released_cb_t released_cb; /* CIS release callback */
	uint8_t  framed:1;
	uint8_t  established:1;   /* 0 if CIS has not yet been established.
				   * 1 if CIS has been established and host
				   * notified.
				   */
	uint8_t teardown:1;       /* 1 if CIS teardown has been initiated */
};

struct ll_conn_iso_group {
	struct ull_hdr            ull;
	struct lll_conn_iso_group lll;

	uint32_t sync_delay;
	uint32_t c_latency;	/* Transport_Latency_M_To_S = CIG_Sync_Delay +
				 * (FT_M_To_S) × ISO_Interval +
				 * SDU_Interval_M_To_S
				 */
	uint32_t p_latency;	/* Transport_Latency_S_To_M = CIG_Sync_Delay +
				 * (FT_S_To_M) × ISO_Interval +
				 * SDU_Interval_S_To_M
				 */
	uint32_t c_sdu_interval;
	uint32_t p_sdu_interval;
	uint32_t cig_ref_point;	/* CIG reference point timestamp (us) based on
				 * controller's clock.
				 */
	uint16_t iso_interval;
	uint8_t  cig_id;
	uint8_t  started:1;     /* 1 if CIG started and ticker is running */
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
