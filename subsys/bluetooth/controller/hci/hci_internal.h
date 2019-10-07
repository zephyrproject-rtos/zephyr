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


#if defined(CONFIG_BT_LL_SW_SPLIT)
#define PDU_DATA(node_rx) ((void *)node_rx->pdu)
#else
#define PDU_DATA(node_rx) ((void *) \
				((struct radio_pdu_node_rx *)node_rx)->pdu_data)
#endif /* CONFIG_BT_LL_SW_SPLIT */


void hci_init(struct k_poll_signal *signal_host_buf);
struct net_buf *hci_cmd_handle(struct net_buf *cmd, void **node_rx);
void hci_evt_encode(struct node_rx_pdu *node_rx, struct net_buf *buf);
u8_t hci_get_class(struct node_rx_pdu *node_rx);
#if defined(CONFIG_BT_CONN)
int hci_acl_handle(struct net_buf *acl, struct net_buf **evt);
void hci_acl_encode(struct node_rx_pdu *node_rx, struct net_buf *buf);
void hci_num_cmplt_encode(struct net_buf *buf, u16_t handle, u8_t num);
void hci_remote_version_info_encode(struct net_buf *buf,
				    struct pdu_data *pdu_data, u16_t handle);
#endif
int hci_vendor_cmd_handle(u16_t ocf, struct net_buf *cmd,
			  struct net_buf **evt);
int hci_vendor_cmd_handle_common(u16_t ocf, struct net_buf *cmd,
			     struct net_buf **evt);
void *hci_cmd_complete(struct net_buf **buf, u8_t plen);
void hci_evt_create(struct net_buf *buf, u8_t evt, u8_t len);
