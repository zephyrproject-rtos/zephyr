/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
extern int32_t    hci_hbuf_total;
extern uint32_t    hci_hbuf_sent;
extern uint32_t    hci_hbuf_acked;
extern atomic_t hci_state_mask;

#define HCI_STATE_BIT_RESET 0
#endif

#define HCI_CLASS_NONE            0 /* Invalid class */
#define HCI_CLASS_EVT_REQUIRED    1 /* Mesh and connection-{established,
				     * disconnected}
				     */
#define HCI_CLASS_EVT_DISCARDABLE 2 /* Best-effort reporting. Discardable
				     * over HCI in case of overflow
				     */
#define HCI_CLASS_EVT_CONNECTION  3 /* Connection management; e.g.
				     * terminate, update, encryption
				     */
#define HCI_CLASS_EVT_LLCP        4 /* LL Control Procedures */
#define HCI_CLASS_ACL_DATA        5 /* Asynchronous Connection Less (general
				     * data)
				     */

void hci_init(struct k_poll_signal *signal_host_buf);
struct net_buf *hci_cmd_handle(struct net_buf *cmd, void **node_rx);
void hci_evt_encode(struct node_rx_pdu *node_rx, struct net_buf *buf);
uint8_t hci_get_class(struct node_rx_pdu *node_rx);
void hci_disconn_complete_encode(struct pdu_data *pdu_data, uint16_t handle,
				 struct net_buf *buf);
void hci_disconn_complete_process(uint16_t handle);
#if defined(CONFIG_BT_CONN)
int hci_acl_handle(struct net_buf *acl, struct net_buf **evt);
void hci_acl_encode(struct node_rx_pdu *node_rx, struct net_buf *buf);
void hci_num_cmplt_encode(struct net_buf *buf, uint16_t handle, uint8_t num);
#endif
int hci_vendor_cmd_handle(uint16_t ocf, struct net_buf *cmd,
			  struct net_buf **evt);
uint8_t hci_vendor_read_static_addr(struct bt_hci_vs_static_addr addrs[],
				 uint8_t size);
void hci_vendor_read_key_hierarchy_roots(uint8_t ir[16], uint8_t er[16]);
int hci_vendor_cmd_handle_common(uint16_t ocf, struct net_buf *cmd,
			     struct net_buf **evt);
uint8_t hci_vendor_read_std_codecs(
	const struct bt_hci_std_codec_info_v2 **codecs);
uint8_t hci_vendor_read_vs_codecs(
	const struct bt_hci_vs_codec_info_v2 **codecs);
uint8_t hci_vendor_read_codec_capabilities(uint8_t coding_format,
					   uint16_t company_id,
					   uint16_t vs_codec_id,
					   uint8_t transport,
					   uint8_t direction,
					   uint8_t *num_capabilities,
					   size_t *capabilities_bytes,
					   const uint8_t **capabilities);
uint8_t hci_vendor_read_ctlr_delay(uint8_t coding_format,
				   uint16_t company_id,
				   uint16_t vs_codec_id,
				   uint8_t transport,
				   uint8_t direction,
				   uint8_t codec_config_len,
				   const uint8_t *codec_config,
				   uint32_t *min_delay,
				   uint32_t *max_delay);
