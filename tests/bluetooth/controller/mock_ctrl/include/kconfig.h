/*
 * Copyright (c) 2020 Demant
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Common Kconfig settings
 */

#ifndef CONFIG_BT_LL_SW_SPLIT
#define CONFIG_BT_LL_SW_SPLIT y
#endif

#define CONFIG_BT_CONN
#define CONFIG_BT_MAX_CONN 4

/* ensure that proper configuration is set */

#ifndef CONFIG_BT_PERIPHERAL
#define CONFIG_BT_PERIPHERAL y
#endif
#ifndef CONFIG_BT_CENTRAL
#define CONFIG_BT_CENTRAL y
#endif

#ifndef CONFIG_BT_CTLR_PHY
#define CONFIG_BT_CTLR_PHY 1
#endif
#ifndef CONFIG_BT_CTLR_PHY_2M
#define CONFIG_BT_CTLR_PHY_2M y
#endif

#ifndef CONFIG_BT_CTLR_DATA_LENGTH
#define CONFIG_BT_CTLR_DATA_LENGTH y
#endif
#ifndef CONFIG_BT_CTLR_DATA_LENGTH_MAX
#define CONFIG_BT_CTLR_DATA_LENGTH_MAX 251
#endif

#ifndef CONFIG_BT_CTLR_LE_ENC
#define CONFIG_BT_CTLR_LE_ENC y
#endif

#ifndef CONFIG_BT_CTLR_LE_PING
#define CONFIG_BT_CTLR_LE_PING y
#endif

#ifndef CONFIG_BT_CTLR_CONN_RSSI
#define CONFIG_BT_CTLR_CONN_RSSI y
#endif

#ifndef CONFIG_BT_CTLR_MIN_USED_CHAN
#define CONFIG_BT_CTLR_MIN_USED_CHAN y
#endif

#define CONFIG_BT_CTLR_LLCP_CONN 4

/* Kconfig Cheats */
#define CONFIG_BT_LOG_LEVEL 1
#define CONFIG_BT_CTLR_COMPANY_ID 0x1234
#define CONFIG_BT_CTLR_SUBVERSION_NUMBER 0x5678
#define CONFIG_NET_BUF_USER_DATA_SIZE 4096
#define CONFIG_BT_CTLR_ASSERT_HANDLER y
#define CONFIG_BT_BUF_ACL_TX_COUNT 7
#define CONFIG_BT_BUF_ACL_TX_SIZE 27
#define CONFIG_BT_CTLR_RX_BUFFERS 7
#define PROC_CTX_BUF_NUM 3
#define TX_CTRL_BUF_NUM 2
