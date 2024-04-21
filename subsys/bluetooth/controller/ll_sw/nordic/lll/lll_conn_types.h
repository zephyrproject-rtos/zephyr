/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Vendor specific data in connection context */
struct lll_conn_vendor {
	union {
#if defined(CONFIG_BT_CTLR_LE_ENC)
		struct {
			struct ccm ccm_tx;
			struct ccm ccm_rx;
		};
#endif /* CONFIG_BT_CTLR_LE_ENC */

		uint8_t rfi[0];
	};
};
