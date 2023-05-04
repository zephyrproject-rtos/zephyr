/*
 * Copyright (c) 2021 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* BIS Broadcaster */
#if defined(CONFIG_BT_CTLR_ADV_ISO)
#define LL_BIS_ADV_HANDLE_BASE BT_CTLR_ADV_ISO_STREAM_HANDLE_BASE
#define LL_BIS_ADV_IDX_FROM_HANDLE(conn_handle) \
	((conn_handle) - (LL_BIS_ADV_HANDLE_BASE))
#define IS_ADV_ISO_HANDLE(conn_handle) \
	(((conn_handle) >= (LL_BIS_ADV_HANDLE_BASE)) && \
	 ((conn_handle) <= ((LL_BIS_ADV_HANDLE_BASE) + \
			    (BT_CTLR_ADV_ISO_STREAM_MAX) - 1U)))
#else
#define LL_BIS_ADV_IDX_FROM_HANDLE(conn_handle) 0U
#define IS_ADV_ISO_HANDLE(conn_handle) 0U
#endif /* CONFIG_BT_CTLR_ADV_ISO */

/* BIS Synchronized Receiver */
#if defined(CONFIG_BT_CTLR_SYNC_ISO)
#define LL_BIS_SYNC_HANDLE_BASE BT_CTLR_SYNC_ISO_STREAM_HANDLE_BASE
#define LL_BIS_SYNC_IDX_FROM_HANDLE(conn_handle) \
	((conn_handle) - (LL_BIS_SYNC_HANDLE_BASE))
#define IS_SYNC_ISO_HANDLE(conn_handle) \
	(((conn_handle) >= (LL_BIS_SYNC_HANDLE_BASE)) && \
	 ((conn_handle) <= ((LL_BIS_SYNC_HANDLE_BASE) + \
			    (BT_CTLR_SYNC_ISO_STREAM_MAX) - 1U)))
#else
#define LL_BIS_SYNC_IDX_FROM_HANDLE(conn_handle) 0U
#define IS_SYNC_ISO_HANDLE(conn_handle) 0U
#endif /* CONFIG_BT_CTLR_SYNC_ISO */

/* CIS */
#if defined(CONFIG_BT_CTLR_CONN_ISO)
#define LL_CIS_HANDLE_BASE (BT_CTLR_CONN_ISO_STREAM_HANDLE_BASE)
#define LL_CIS_IDX_FROM_HANDLE(_handle) \
	((_handle) - LL_CIS_HANDLE_BASE)
#define LL_CIS_HANDLE_LAST \
	(CONFIG_BT_CTLR_CONN_ISO_STREAMS + LL_CIS_HANDLE_BASE - 1)
#define IS_CIS_HANDLE(_handle) \
	(((_handle) >= LL_CIS_HANDLE_BASE) && \
	((_handle) <= LL_CIS_HANDLE_LAST))
#else
#define IS_CIS_HANDLE(_handle) 0
#endif /* CONFIG_BT_CTLR_CONN_ISO */

struct ll_iso_test_mode_data {
	uint32_t received_cnt;
	uint32_t missed_cnt;
	uint32_t failed_cnt;
	uint32_t rx_sdu_counter;
	uint64_t tx_sdu_counter:53; /* 39 + 22 - 8 */
	uint64_t tx_enabled:1;
	uint64_t tx_payload_type:4; /* Support up to 16 payload types (BT 5.3: 3, VS: 13) */
	uint64_t rx_enabled:1;
	uint64_t rx_payload_type:4;
};

/* Common members for ll_conn_iso_stream and ll_broadcast_iso_stream */
struct ll_iso_stream_hdr {
	struct ll_iso_test_mode_data test_mode;
	struct ll_iso_datapath *datapath_in;
	struct ll_iso_datapath *datapath_out;
};

struct ll_iso_datapath {
	uint8_t  path_dir;
	uint8_t  path_id;
	uint8_t  coding_format;
	uint16_t company_id;
	isoal_sink_handle_t sink_hdl;
	isoal_source_handle_t source_hdl;
};
