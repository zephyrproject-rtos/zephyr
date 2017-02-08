/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HCI_CONTROLLER_H_
#define _HCI_CONTROLLER_H_

struct net_buf *hci_cmd_handle(struct net_buf *cmd);
int hci_acl_handle(struct net_buf *acl);
void hci_evt_encode(struct radio_pdu_node_rx *node_rx, struct net_buf *buf);
void hci_acl_encode(struct radio_pdu_node_rx *node_rx, struct net_buf *buf);
void hci_num_cmplt_encode(struct net_buf *buf, uint16_t handle, uint8_t num);
bool hci_evt_is_discardable(struct radio_pdu_node_rx *node_rx);
#endif /* _HCI_CONTROLLER_H_ */
