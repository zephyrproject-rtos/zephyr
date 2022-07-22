/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * expected features on purpose defined here
 * to keep implementation separate from test
 */
#if defined(CONFIG_BT_CTLR_LE_ENC)
#define FEAT_ENCODED 0x01
#else
#define FEAT_ENCODED 0x00
#endif

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
#define FEAT_PARAM_REQ 0x02
#else
#define FEAT_PARAM_REQ 0x00
#endif

#if defined(CONFIG_BT_CTLR_EXT_REJ_IND)
#define FEAT_EXT_REJ 0x04
#else
#define FEAT_EXT_REJ 0x00
#endif

#if defined(CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG)
#define FEAT_PERIPHERAL_FREQ 0x08
#else
#define FEAT_PERIPHERAL_FREQ 0x00
#endif

#if defined(CONFIG_BT_CTLR_LE_PING)
#define FEAT_PING 0x10
#else
#define FEAT_PING 0x00
#endif

#if defined(CONFIG_BT_CTLR_DATA_LENGTH_MAX)
#define FEAT_DLE 0x20
#else
#define FEAT_DLE 0x00
#endif

#if defined(CONFIG_BT_CTLR_PRIVACY)
#define FEAT_PRIVACY 0x40
#else
#define FEAT_PRIVACY 0x00
#endif

#if defined(CONFIG_BT_CTLR_EXT_SCAN_FP)
#define FEAT_EXT_SCAN 0x80
#else
#define FEAT_EXT_SCAN 0x00
#endif

#if defined(CONFIG_BT_CTLR_PHY_2M)
#define FEAT_PHY_2M 0x100
#else
#define FEAT_PHY_2M 0x00
#endif

#if defined(CONFIG_BT_CTLR_SMI_TX)
#define FEAT_SMI_TX 0x200
#else
#define FEAT_SMI_TX 0x00
#endif

#if defined(CONFIG_BT_CTLR_SMI_RX)
#define FEAT_SMI_RX 0x400
#else
#define FEAT_SMI_RX 0x00
#endif

#if defined(CONFIG_BT_CTLR_PHY_CODED)
#define FEAT_PHY_CODED 0x800
#else
#define FEAT_PHY_CODED 0x00
#endif

#if defined(CONFIG_MISSING)
#define FEAT_EXT_ADV 0x1000
#else
#define FEAT_EXT_ADV 0x00
#endif

#if defined(CONFIG_MISSING)
#define FEAT_PER_ADV 0x2000
#else
#define FEAT_PER_ADV 0x00
#endif

#if defined(CONFIG_BT_CTLR_CHAN_SEL_2)
#define FEAT_CHAN_SEL_ALGO2 0x4000
#else
#define FEAT_CHAN_SEL_ALGO2 0x00
#endif

#if defined(CONFIG_MISSING)
#define FEAT_PWR_CLASS1 0x8000
#else
#define FEAT_PWR_CLASS1 0x00
#endif

#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN)
#define FEAT_MIN_CHANN 0x10000
#else
#define FEAT_MIN_CHANN 0x00
#endif

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
#define FEAT_CONNECTION_CTE_REQ 0x20000
#else
#define FEAT_CONNECTION_CTE_REQ 0x00
#endif

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RSP)
#define FEAT_CONNECTION_CTE_RSP 0x40000
#else
#define FEAT_CONNECTION_CTE_RSP 0x00
#endif

#if defined(CONFIG_MISSING)
#define FEAT_CONNECTIONLESS_CTE_TX 0x80000
#else
#define FEAT_CONNECTIONLESS_CTE_TX 0x00
#endif

#if defined(CONFIG_MISSING)
#define FEAT_CONNECTIONLESS_CTE_RX 0x100000
#else
#define FEAT_CONNECTIONLESS_CTE_RX 0x00
#endif

#if defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_TX)
#define FEAT_ANT_SWITCH_CTE_TX 0x200000
#else
#define FEAT_ANT_SWITCH_CTE_TX 0x00
#endif

#if defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_RX)
#define FEAT_ANT_SWITCH_CTE_RX 0x400000
#else
#define FEAT_ANT_SWITCH_CTE_RX 0x00
#endif

#if defined(CONFIG_BT_CTLR_DF_CTE_RX)
#define FEAT_RX_CTE 0x800000
#else
#define FEAT_RX_CTE 0x00
#endif

#if defined(CONFIG_MISSING)
#define FEAT_PER_ADV_SYNC_TX 0x1000000
#else
#define FEAT_PER_ADV_SYNC_TX 0x00
#endif

#if defined(CONFIG_MISSING)
#define FEAT_PER_ADV_SYNC_RX 0x2000000
#else
#define FEAT_PER_ADV_SYNC_RX 0x00
#endif

#if defined(CONFIG_MISSING)
#define FEAT_SLEEP_UPD 0x4000000
#else
#define FEAT_SLEEP_UPD 0x00
#endif

#if defined(CONFIG_MISSING)
#define FEAT_RPK_VALID 0x8000000
#else
#define FEAT_RPK_VALID 0x00
#endif

#if defined(CONFIG_MISSING)
#define FEAT_ISO_CENTRAL 0x10000000
#else
#define FEAT_ISO_CENTRAL 0x00
#endif

#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
#define FEAT_ISO_PERIPHERAL 0x20000000
#else
#define FEAT_ISO_PERIPHERAL 0x00
#endif

#if defined(CONFIG_MISSING)
#define FEAT_ISO_BROADCAST 0x40000000
#else
#define FEAT_ISO_BROADCAST 0x00
#endif

#if defined(CONFIG_MISSING)
#define FEAT_ISO_RECEIVER 0x80000000
#else
#define FEAT_ISO_RECEIVER 0x00
#endif

#if defined(CONFIG_MISSING)
#define FEAT_ISO_CHANNELS 0x100000000
#else
#define FEAT_ISO_CHANNELS 0x00
#endif

#if defined(CONFIG_MISSING)
#define FEAT_LE_PWR_REQ 0x200000000
#else
#define FEAT_LE_PWR_REQ 0x00
#endif

#if defined(CONFIG_MISSING)
#define FEAT_LE_PWR_IND 0x400000000
#else
#define FEAT_LE_PWR_IND 0x00
#endif

#if defined(CONFIG_MISSING)
#define FEAT_LE_PATH_LOSS 0x800000000
#else
#define FEAT_LE_PATH_LOSS 0x00
#endif

#define DEFAULT_FEATURE                                                                            \
	(FEAT_ENCODED | FEAT_PARAM_REQ | FEAT_EXT_REJ | FEAT_PERIPHERAL_FREQ | FEAT_PING |         \
	 FEAT_DLE | FEAT_PRIVACY | FEAT_EXT_SCAN | FEAT_PHY_2M | FEAT_SMI_TX | FEAT_SMI_RX |       \
	 FEAT_PHY_CODED | FEAT_EXT_ADV | FEAT_PER_ADV | FEAT_CHAN_SEL_ALGO2 | FEAT_PWR_CLASS1 |    \
	 FEAT_MIN_CHANN | FEAT_CONNECTION_CTE_REQ | FEAT_CONNECTION_CTE_RSP |                      \
	 FEAT_ANT_SWITCH_CTE_TX | FEAT_ANT_SWITCH_CTE_RX | FEAT_RX_CTE | FEAT_PER_ADV_SYNC_TX |    \
	 FEAT_PER_ADV_SYNC_RX | FEAT_SLEEP_UPD | FEAT_RPK_VALID | FEAT_ISO_CENTRAL |               \
	 FEAT_ISO_PERIPHERAL | FEAT_ISO_BROADCAST | FEAT_ISO_RECEIVER | FEAT_ISO_CHANNELS |        \
	 FEAT_LE_PWR_REQ | FEAT_LE_PWR_IND | FEAT_LE_PATH_LOSS)

/*
 * The following two are defined as per
 * Core Spec V5.3 Volume 6, Part B, chapter 4.6
 */
#define EXPECTED_FEAT_EXCH_VALID 0x000000EFF787CF2F
#define FEAT_FILTER_OCTET0 0xFFFFFFFFFFFFFF00
#define COMMON_FEAT_OCTET0(x) (FEAT_FILTER_OCTET0 | ((x) & ~FEAT_FILTER_OCTET0))
