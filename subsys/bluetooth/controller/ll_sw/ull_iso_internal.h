/*
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Calculate ISO PDU buffers required considering SDU fragmentation */
#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
/* Internal ISO Tx SDU maximum length.
 * A length that is minimum of the resultant combination of the HCI ISO data fragments provided and
 * the user configured maximum transmit SDU length (CONFIG_BT_CTLR_ISO_TX_SDU_LEN_MAX).
 */
#define BT_CTLR_ISO_TX_SDU_LEN_MAX MIN(((CONFIG_BT_CTLR_ISO_TX_BUFFER_SIZE * \
					 CONFIG_BT_CTLR_ISO_TX_BUFFERS) - \
					BT_HCI_ISO_SDU_HDR_SIZE), \
				       CONFIG_BT_CTLR_ISO_TX_SDU_LEN_MAX)

BUILD_ASSERT(BT_CTLR_ISO_TX_SDU_LEN_MAX == CONFIG_BT_CTLR_ISO_TX_SDU_LEN_MAX,
	     "Insufficient ISO Buffer Size and Count for the required ISO SDU Length");

/* Internal ISO Tx buffers to be allocated.
 * Based on the internal ISO Tx SDU maximum length, calculate the required ISO Tx buffers (PDUs)
 * required to fragment the SDU into PDU sizes scheduled for transmission.
 */
/* Defines the minimum required PDU length to help calculate the number of PDU buffers required
 * At least one of CONFIG_BT_CTLR_ADV_ISO or CONFIG_BT_CTLR_CONN_ISO will always be enabled,
 * so if either of them is not defined, it can default to the other
 */
#define BT_CTLR_ISO_TX_MIN_PDU_LEN \
	MIN(COND_CODE_1(IS_ENABLED(CONFIG_BT_CTLR_ADV_ISO), \
			(CONFIG_BT_CTLR_ADV_ISO_PDU_LEN_MAX), \
			(CONFIG_BT_CTLR_CONN_ISO_PDU_LEN_MAX)), \
	    COND_CODE_1(IS_ENABLED(CONFIG_BT_CTLR_CONN_ISO), \
			(CONFIG_BT_CTLR_CONN_ISO_PDU_LEN_MAX), \
			(CONFIG_BT_CTLR_ADV_ISO_PDU_LEN_MAX)))
#define BT_CTLR_ISO_TX_PDU_BUFFERS \
	(DIV_ROUND_UP(BT_CTLR_ISO_TX_SDU_LEN_MAX, \
		      MIN((CONFIG_BT_CTLR_ISO_TX_BUFFER_SIZE - BT_HCI_ISO_SDU_TS_HDR_SIZE), \
			  BT_CTLR_ISO_TX_MIN_PDU_LEN)) * \
	 CONFIG_BT_CTLR_ISO_TX_BUFFERS)
#else /* !CONFIG_BT_CTLR_ADV_ISO && !CONFIG_BT_CTLR_CONN_ISO */
#define BT_CTLR_ISO_TX_PDU_BUFFERS 0
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

void ull_iso_resume_ticker_start(struct lll_event *resume_event,
				 uint16_t group_handle,
				 uint16_t stream_handle,
				 uint8_t  role,
				 uint32_t ticks_anchor,
				 uint32_t resume_timeout);
