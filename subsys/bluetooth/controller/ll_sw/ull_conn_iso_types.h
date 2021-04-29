/*
 * Copyright (c) 2021 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #define LL_CIS_HANDLE_BASE CONFIG_BT_MAX_CONN

 #define LL_CIS_IDX_FROM_HANDLE(_handle) \
	((_handle) - LL_CIS_HANDLE_BASE)

struct ll_conn_iso_stream {
	struct ll_conn_iso_group *group;
	struct lll_conn_iso_stream lll;
	uint32_t sync_delay;
	uint8_t  cis_id;
	struct ll_iso_datapath *datapath_in;
	struct ll_iso_datapath *datapath_out;
	uint32_t offset;        /* Offset of CIS from ACL event in us */
	uint8_t  established;	/* 0 if CIS has not yet been established.
				 * 1 if CIS has been established and host
				 * notified.
				 */
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
	uint16_t iso_interval;
	uint8_t  cig_id;

#if defined(CONFIG_BT_CTLR_CONN_ISO_STREAMS_PER_GROUP)
	uint16_t cis_handles[CONFIG_BT_CTLR_CONN_ISO_STREAMS_PER_GROUP];
#endif /* CONFIG_BT_CTLR_CONN_ISO_STREAMS */
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
