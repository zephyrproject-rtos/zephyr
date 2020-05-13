/*
 * Copyright (c) 2020 Demant
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Common Kconfig settings
 */

#define CONFIG_BT_CONN
#define CONFIG_BT_MAX_CONN 4

#ifndef CONFIG_BT_CTLR_PHY
#define CONFIG_BT_CTLR_PHY 1
#endif

#ifndef CONFIG_BT_CTLR_LE_ENC
#define CONFIG_BT_CTLR_LE_ENC 1
#endif

/* Kconfig Cheats */
#define CONFIG_BT_LOG_LEVEL 1
#define CONFIG_BT_CTLR_COMPANY_ID 0x1234
#define CONFIG_BT_CTLR_SUBVERSION_NUMBER 0x5678
#define CONFIG_NET_BUF_USER_DATA_SIZE 4096
#define CONFIG_BT_CTLR_ASSERT_HANDLER y

#define PROC_CTX_BUF_NUM 15
#define TX_CTRL_BUF_NUM 2
#define NTF_BUF_NUM 2
