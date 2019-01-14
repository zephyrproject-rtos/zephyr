/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct node_rx_cc {
	u8_t  status;
	u8_t  role;
	u8_t  peer_addr_type;
	u8_t  peer_addr[BDADDR_SIZE];
#if defined(CONFIG_BT_CTLR_PRIVACY)
	u8_t  peer_rpa[BDADDR_SIZE];
	u8_t  own_addr_type;
	u8_t  own_addr[BDADDR_SIZE];
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
