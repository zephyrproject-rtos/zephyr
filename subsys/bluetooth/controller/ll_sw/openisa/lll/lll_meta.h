/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CTLR_RX_PDU_META)
/*
 * typedef struct { } lll_rx_pdu_meta_t;
 */
#error Please define typedef struct lll_rx_pdu_meta_t, LLL_RX_PDU_META_PDU_OFFSET and \
	BT_CTLR_USER_RX_BUFFER_OVERHEAD when enabling CONFIG_BT_CTLR_RX_PDU_META
#endif /* CONFIG_BT_CTLR_RX_PDU_META */
