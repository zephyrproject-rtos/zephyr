/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct ll_conn {
	struct evt_hdr  evt;
	struct ull_hdr  ull;
	struct lll_conn lll;

	u16_t connect_expire;
	u16_t supervision_reload;
	u16_t supervision_expire;
	u16_t procedure_reload;
	u16_t procedure_expire;

#if defined(CONFIG_BT_CTLR_LE_PING)
	u16_t appto_reload;
	u16_t appto_expire;
	u16_t apto_reload;
	u16_t apto_expire;
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	u16_t default_tx_octets;

#if defined(CONFIG_BT_CTLR_PHY)
	u16_t default_tx_time;
#endif /* CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

	union {
		struct {
			u8_t fex_valid:1;
		} common;

#if defined(CONFIG_BT_PERIPHERAL)
		struct {
			u8_t  fex_valid:1;
			u8_t  latency_cancel:1;
			u8_t  sca:3;
			u32_t force;
			u32_t ticks_to_offset;
		} slave;
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CENTRAL)
		struct {
			u8_t fex_valid:1;
			u8_t terminate_ack:1;
		} master;
#endif /* CONFIG_BT_CENTRAL */
	};
#if defined(CONFIG_BT_CTLR_PHY)
	u8_t phy_pref_tx:3;
	u8_t phy_pref_flags:1;
	u8_t phy_pref_rx:3;
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_LLID_DATA_START_EMPTY)
	/* Detect empty L2CAP start frame */
	u8_t  start_empty:1;
#endif /* CONFIG_BT_CTLR_LLID_DATA_START_EMPTY */

	struct node_tx *tx_head;
	struct node_tx *tx_ctrl;
	struct node_tx *tx_ctrl_last;
	struct node_tx *tx_data;
	struct node_tx *tx_data_last;

	u8_t chm_updated;
};

struct node_rx_cc {
	u8_t  status;
	u8_t  role;
	u8_t  peer_addr_type;
	u8_t  peer_addr[BDADDR_SIZE];
#if defined(CONFIG_BT_CTLR_PRIVACY)
	u8_t  peer_rpa[BDADDR_SIZE];
	u8_t  local_rpa[BDADDR_SIZE];
#endif /* CONFIG_BT_CTLR_PRIVACY */
	u16_t interval;
	u16_t latency;
	u16_t timeout;
	u8_t  sca;
};

struct node_rx_cu {
	u8_t  status;
	u16_t interval;
	u16_t latency;
	u16_t timeout;
};

struct node_rx_cs {
	u8_t csa;
};

struct node_rx_pu {
	u8_t status;
	u8_t tx;
	u8_t rx;
};
