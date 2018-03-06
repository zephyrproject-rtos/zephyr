/* NOTE: Definitions used between Thread and ULL/LLL implementations */

/* TODO: change node_rx to ull_rx */
enum node_rx_type {
	_NODE_RX_TYPE_NONE,
	_NODE_RX_TYPE_EVENT_DONE,
	_NODE_RX_TYPE_DC_PDU_LLL,

	ULL_RX_TYPE_SCAN_REQ,
	ULL_RX_TYPE_ADV_IND,
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
