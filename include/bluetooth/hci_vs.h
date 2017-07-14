/* hci_vs.h - Bluetooth Host Control Interface Vendor Specific definitions */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_HCI_VS_H
#define __BT_HCI_VS_H

#include <bluetooth/hci.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BT_HCI_OP_VS_READ_VERSION_INFO		BT_OP(BT_OGF_VS, 0x0001)
struct bt_hci_rp_vs_read_version_info {
	u8_t  status;
	u16_t hw_platform;
	u16_t hw_variant;
	u8_t  fw_variant;
	u8_t  fw_version;
	u16_t fw_revision;
	u32_t fw_build;
} __packed;

#define BT_HCI_OP_VS_READ_SUPPORTED_COMMANDS	BT_OP(BT_OGF_VS, 0x0002)
struct bt_hci_rp_vs_read_supported_commands {
	u8_t  status;
	u8_t  commands[64];
} __packed;

#define BT_HCI_OP_VS_READ_SUPPORTED_FEATURES	BT_OP(BT_OGF_VS, 0x0003)
struct bt_hci_rp_vs_read_supported_features {
	u8_t  status;
	u8_t  features[8];
} __packed;

#ifdef __cplusplus
}
#endif

#endif /* __BT_HCI_VS_H */
