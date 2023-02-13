/*
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
/* Calculate ISO PDU buffers required considering SDU fragmentation */
/* FIXME: Calculation considering both Connected and Broadcast ISO PDU
 *        fragmentation.
 */
#if defined(CONFIG_BT_CTLR_CONN_ISO)
#define BT_CTLR_ISO_TX_BUFFERS (((CONFIG_BT_CTLR_CONN_ISO_SDU_LEN_MAX + \
				  CONFIG_BT_CTLR_CONN_ISO_PDU_LEN_MAX - 1U) / \
				 CONFIG_BT_CTLR_CONN_ISO_PDU_LEN_MAX) * \
				CONFIG_BT_CTLR_ISO_TX_BUFFERS)
#else /* !CONFIG_BT_CTLR_CONN_ISO */
#define BT_CTLR_ISO_TX_BUFFERS CONFIG_BT_CTLR_ISO_TX_BUFFERS
#endif /* !CONFIG_BT_CTLR_CONN_ISO */
#else /* !CONFIG_BT_CTLR_ADV_ISO && !CONFIG_BT_CTLR_CONN_ISO */
#define BT_CTLR_ISO_TX_BUFFERS 0
#endif /* !CONFIG_BT_CTLR_ADV_ISO && !CONFIG_BT_CTLR_CONN_ISO */

int ull_iso_init(void);
int ull_iso_reset(void);
struct ll_iso_datapath *ull_iso_datapath_alloc(void);
void ull_iso_datapath_release(struct ll_iso_datapath *dp);
void ll_iso_rx_put(memq_link_t *link, void *rx);
void *ll_iso_rx_get(void);
void ll_iso_rx_dequeue(void);
void ll_iso_transmit_test_send_sdu(uint16_t handle, uint32_t ticks_at_expire);
