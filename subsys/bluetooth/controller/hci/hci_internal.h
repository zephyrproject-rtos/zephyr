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

#define HCI_CLASS_EVT_REQUIRED    0
#define HCI_CLASS_EVT_DISCARDABLE 1
#define HCI_CLASS_EVT_CONNECTION  2
#define HCI_CLASS_ACL_DATA        3

#if defined(CONFIG_SOC_FAMILY_NRF)
#define BT_HCI_VS_HW_PLAT BT_HCI_VS_HW_PLAT_NORDIC
#if defined(CONFIG_SOC_SERIES_NRF51X)
#define BT_HCI_VS_HW_VAR  BT_HCI_VS_HW_VAR_NORDIC_NRF51X;
#elif defined(CONFIG_SOC_SERIES_NRF52X)
#define BT_HCI_VS_HW_VAR  BT_HCI_VS_HW_VAR_NORDIC_NRF52X;
#endif
#else
#define BT_HCI_VS_HW_PLAT 0
#define BT_HCI_VS_HW_VAR  0
#endif /* CONFIG_SOC_FAMILY_NRF */

void hci_init(struct k_poll_signal *signal_host_buf);
struct net_buf *hci_cmd_handle(struct net_buf *cmd);
void hci_evt_encode(struct radio_pdu_node_rx *node_rx, struct net_buf *buf);
s8_t hci_get_class(struct radio_pdu_node_rx *node_rx);
#if defined(CONFIG_BT_CONN)
int hci_acl_handle(struct net_buf *acl, struct net_buf **evt);
void hci_acl_encode(struct radio_pdu_node_rx *node_rx, struct net_buf *buf);
void hci_num_cmplt_encode(struct net_buf *buf, u16_t handle, u8_t num);
#endif
