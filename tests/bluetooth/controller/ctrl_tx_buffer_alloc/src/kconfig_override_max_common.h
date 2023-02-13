/*
 * Copyright (c) 2020 Demant
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Common Kconfig settings
 */

#ifdef CONFIG_BT_CTLR_LLCP_COMMON_TX_CTRL_BUF_NUM
#undef CONFIG_BT_CTLR_LLCP_COMMON_TX_CTRL_BUF_NUM
#define CONFIG_BT_CTLR_LLCP_COMMON_TX_CTRL_BUF_NUM \
	(CONFIG_BT_CTLR_LLCP_CONN * CONFIG_BT_CTLR_LLCP_TX_PER_CONN_TX_CTRL_BUF_NUM_MAX)
#endif /* CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM */
