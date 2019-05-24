/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
extern s32_t    hci_hbuf_total;
extern u32_t    hci_hbuf_sent;
extern u32_t    hci_hbuf_acked;
extern atomic_t hci_state_mask;

#define HCI_STATE_BIT_RESET 0
#endif

#define HCI_CLASS_EVT_REQUIRED    0 /* Mesh and connection-{established,
				     * disconnected}
				     */
#define HCI_CLASS_EVT_DISCARDABLE 1 /* Best-effort reporting. Discardable
				     * over HCI in case of overflow
				     */
#define HCI_CLASS_EVT_CONNECTION  2 /* Connection management; e.g.
				     * terminate, update, encryption
				     */
#define HCI_CLASS_ACL_DATA        3 /* Asynchronous Connection Less (general
				     * data)
				     */

void hci_init(struct k_poll_signal *signal_host_buf);
struct net_buf *hci_cmd_handle(struct net_buf *cmd, void **node_rx);
void hci_evt_encode(struct node_rx_pdu *node_rx, struct net_buf *buf);
s8_t hci_get_class(struct node_rx_pdu *node_rx);
#if defined(CONFIG_BT_CONN)
int hci_acl_handle(struct net_buf *acl, struct net_buf **evt);
void hci_acl_encode(struct node_rx_pdu *node_rx, struct net_buf *buf);
void hci_num_cmplt_encode(struct net_buf *buf, u16_t handle, u8_t num);
#endif
int hci_vendor_cmd_handle(u16_t ocf, struct net_buf *cmd,
			  struct net_buf **evt);
int hci_vendor_cmd_handle_common(u16_t ocf, struct net_buf *cmd,
			     struct net_buf **evt);
void *hci_cmd_complete(struct net_buf **buf, u8_t plen);
void hci_evt_create(struct net_buf *buf, u8_t evt, u8_t len);
