/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_CONTROLLER_LL_SW_NATIVE_HCI_HCI_VENDOR_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_CONTROLLER_LL_SW_NATIVE_HCI_HCI_VENDOR_H_

#define BT_HCI_VS_HW_PLAT 0U
#define BT_HCI_VS_HW_VAR  0U

inline int hci_vendor_cmd_handle(uint16_t ocf, struct net_buf *cmd,
				 struct net_buf **evt)
{
	return hci_vendor_cmd_handle_common(ocf, cmd, evt);
}

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_CONTROLLER_LL_SW_NATIVE_HCI_HCI_VENDOR_H_ */
