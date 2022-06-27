/*
 * Copyright (c) 2021 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* CIS */
#define LL_CIS_HANDLE_BASE CONFIG_BT_MAX_CONN

#if defined(CONFIG_BT_CTLR_CONN_ISO_STREAMS)
#define LAST_VALID_CIS_HANDLE \
	(CONFIG_BT_CTLR_CONN_ISO_STREAMS + LL_CIS_HANDLE_BASE - 1)
#else
#define LAST_VALID_CIS_HANDLE (LL_CIS_HANDLE_BASE - 1)
#endif /* CONFIG_BT_CTLR_CONN_ISO_STREAMS */

#define LL_CIS_IDX_FROM_HANDLE(_handle) \
	((_handle) - LL_CIS_HANDLE_BASE)

#if defined(CONFIG_BT_CTLR_CONN_ISO)
#define IS_CIS_HANDLE(_handle) \
	(((_handle) >= LL_CIS_HANDLE_BASE) && \
	((_handle) <= LAST_VALID_CIS_HANDLE))
#else
#define IS_CIS_HANDLE(_handle) 0
#endif /* CONFIG_BT_CTLR_CONN_ISO */


#if defined(CONFIG_BT_CTLR_ADV_ISO)
#define IS_ADV_ISO_HANDLE(_handle) \
	(((_handle) >= BT_CTLR_ADV_ISO_STREAM_HANDLE_BASE) && \
	((_handle) <= (BT_CTLR_ADV_ISO_STREAM_HANDLE_BASE + BT_CTLR_ADV_ISO_STREAM_MAX - 1)))
#else
#define IS_ADV_ISO_HANDLE(_handle) 0
#endif /* CONFIG_BT_CTLR_ADV_ISO */

/* Common members for ll_conn_iso_stream and ll_broadcast_iso_stream */
struct ll_iso_stream_hdr {
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
