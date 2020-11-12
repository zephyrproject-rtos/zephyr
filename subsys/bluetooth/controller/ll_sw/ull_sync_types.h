/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LL_SYNC_STATE_IDLE       0x00
#define LL_SYNC_STATE_ADDR_MATCH 0x01
#define LL_SYNC_STATE_CREATED    0x02

#if defined(CONFIG_BT_CTLR_SYNC_ISO)
struct ll_sync_iso {
	struct evt_hdr evt;
	struct ull_hdr ull;
	struct lll_sync_iso lll;

	struct node_rx_hdr node_rx_lost;
	struct node_rx_hdr node_rx_estab;
};

struct node_rx_sync_iso {
	uint8_t status;
	uint16_t interval;
};
#endif /* CONFIG_BT_CTLR_SYNC_ISO */

struct ll_sync_set {
	struct evt_hdr evt;
	struct ull_hdr ull;
	struct lll_sync lll;

	uint16_t skip;
	uint16_t timeout;
	uint16_t volatile timeout_reload; /* Non-zero when sync established */
	uint16_t timeout_expire;

	/* node rx type with memory aligned storage for sync lost reason.
	 * HCI will reference the value using the pdu member of
	 * struct node_rx_pdu.
	 */
	struct {
		struct node_rx_hdr hdr;
		union {
			uint8_t    pdu[0] __aligned(4);
			uint8_t    reason;
		};
	} node_rx_lost;

#if defined(CONFIG_BT_CTLR_SYNC_ISO)
	struct ll_sync_iso *sync_iso;
#endif /* CONFIG_BT_CTLR_SYNC_ISO */
};

struct node_rx_sync {
	uint8_t status;
	uint8_t  phy;
	uint16_t interval;
	uint8_t  sca;
};
