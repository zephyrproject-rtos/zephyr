/* NOTE: Definitions used between Thread and ULL/LLL implementations */

enum node_rx_type {
	NODE_RX_TYPE_NONE,
	NODE_RX_TYPE_EVENT_DONE,
	NODE_RX_TYPE_DC_PDU,
	NODE_RX_TYPE_REPORT,

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	NODE_RX_TYPE_EXT_1M_REPORT,
	NODE_RX_TYPE_EXT_CODED_REPORT,
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
	NODE_RX_TYPE_SCAN_REQ,
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

	NODE_RX_TYPE_CONNECTION,
	NODE_RX_TYPE_TERMINATE,
	NODE_RX_TYPE_CONN_UPDATE,
	NODE_RX_TYPE_ENC_REFRESH,

#if defined(CONFIG_BT_CTLR_LE_PING)
	NODE_RX_TYPE_APTO,
#endif /* CONFIG_BT_CTLR_LE_PING */

	NODE_RX_TYPE_CHAN_SEL_ALGO,

#if defined(CONFIG_BT_CTLR_PHY)
	NODE_RX_TYPE_PHY_UPDATE,
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
	NODE_RX_TYPE_RSSI,
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

#if defined(CONFIG_BT_CTLR_PROFILE_ISR)
	NODE_RX_TYPE_PROFILE,
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

#if defined(CONFIG_BT_CTLR_ADV_INDICATION)
	NODE_RX_TYPE_ADV_INDICATION,
#endif /* CONFIG_BT_CTLR_ADV_INDICATION */

#if defined(CONFIG_BT_CTLR_SCAN_INDICATION)
	NODE_RX_TYPE_SCAN_INDICATION,
#endif /* CONFIG_BT_CTLR_SCAN_INDICATION */

#if defined(CONFIG_BT_HCI_MESH_EXT)
	NODE_RX_TYPE_MESH_ADV_CPLT,
	NODE_RX_TYPE_MESH_REPORT,
#endif /* CONFIG_BT_HCI_MESH_EXT */
};

struct node_rx_hdr {
	union {
		void        *next;
		memq_link_t *link;
		u8_t        ack_last;
	};

	enum node_rx_type   type;
	u16_t               handle;
};

struct node_rx_pdu {
	struct node_rx_hdr hdr;
	u8_t               pdu[0];
};

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
	u8_t  mca;
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
