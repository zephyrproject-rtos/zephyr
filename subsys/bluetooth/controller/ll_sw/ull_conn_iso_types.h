/*
 * Copyright (c) 2021 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct ll_conn_iso_stream {
	struct ll_conn_iso_group *group;
	struct lll_conn_iso_stream lll;
	uint32_t sync_delay;
};

struct ll_conn_iso_group {
	struct evt_hdr            evt;
	struct ull_hdr            ull;
	struct lll_conn_iso_group lll;

	uint32_t sync_delay;
	uint32_t c_latency;
	uint32_t p_latency;
	uint16_t iso_interval;

#if defined(CONFIG_BT_CTLR_CONN_ISO_STREAMS_PER_GROUP)
	struct ll_conn_iso_stream *streams
				[CONFIG_BT_CTLR_CONN_ISO_STREAMS_PER_GROUP];
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
