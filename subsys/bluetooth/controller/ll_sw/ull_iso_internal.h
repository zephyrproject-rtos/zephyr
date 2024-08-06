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
uint32_t ull_iso_big_sync_delay(uint8_t num_bis, uint32_t bis_spacing, uint8_t nse,
				uint32_t sub_interval, uint8_t phy, uint8_t max_pdu, bool enc);

/* Must be implemented by vendor if vendor-specific data path is supported */
bool ll_data_path_configured(uint8_t data_path_dir, uint8_t data_path_id);
/* Must be implemented by vendor if vendor-specific data path is supported */
bool ll_data_path_sink_create(uint16_t handle,
			      struct ll_iso_datapath *datapath,
			      isoal_sink_sdu_alloc_cb *sdu_alloc,
			      isoal_sink_sdu_emit_cb *sdu_emit,
			      isoal_sink_sdu_write_cb *sdu_write);
/* Must be implemented by vendor if vendor-specific data path is supported */
bool ll_data_path_source_create(uint16_t handle,
				struct ll_iso_datapath *datapath,
				isoal_source_pdu_alloc_cb *pdu_alloc,
				isoal_source_pdu_write_cb *pdu_write,
				isoal_source_pdu_emit_cb *pdu_emit,
				isoal_source_pdu_release_cb *pdu_release);

/* Must be implemented by vendor if vendor-specific data path is supported */
void ll_data_path_tx_pdu_release(uint16_t handle, struct node_tx_iso *node_tx);
