/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_NET_L2_CANBUS_INTERNAL_H_
#define ZEPHYR_SUBSYS_NET_L2_CANBUS_INTERNAL_H_


#ifdef NET_CAN_USE_CAN_FD
#define NET_CAN_DL 64
#else
#define NET_CAN_DL 8
#endif/*NET_CAN_USE_CAN_FD*/

/* Protocol control information*/
#define NET_CAN_PCI_SF 0x00 /* Single frame*/
#define NET_CAN_PCI_FF 0x01 /* First frame */
#define NET_CAN_PCI_CF 0x02 /* Consecutive frame */
#define NET_CAN_PCI_FC 0x03 /* Flow control frame */

#define NET_CAN_PCI_TYPE_BYTE        0
#define NET_CAN_PCI_TYPE_POS         4
#define NET_CAN_PCI_TYPE_MASK        0xF0
#define NET_CAN_PCI_TYPE_SF   (NET_CAN_PCI_SF << NET_CAN_PCI_TYPE_POS)
#define NET_CAN_PCI_TYPE_FF   (NET_CAN_PCI_FF << NET_CAN_PCI_TYPE_POS)
#define NET_CAN_PCI_TYPE_CF   (NET_CAN_PCI_CF << NET_CAN_PCI_TYPE_POS)
#define NET_CAN_PCI_TYPE_FC   (NET_CAN_PCI_FC << NET_CAN_PCI_TYPE_POS)

#define NET_CAN_PCI_SF_DL_MASK       0x0F

#define NET_CAN_PCI_FF_DL_UPPER_BYTE 0
#define NET_CAN_PCI_FF_DL_UPPER_MASK 0x0F
#define NET_CAN_PCI_FF_DL_LOWER_BYTE 1

#define NET_CAN_PCI_FS_BYTE          0
#define NET_CAN_PCI_FS_MASK          0x0F
#define NET_CAN_PCI_BS_BYTE          1
#define NET_CAN_PCI_ST_MIN_BYTE      2

#define NET_CAN_PCI_FS_CTS           0x0
#define NET_CAN_PCI_FS_WAIT          0x1
#define NET_CAN_PCI_FS_OVFLW         0x2

#define NET_CAN_PCI_SN_MASK          0x0F

#define NET_CAN_FF_DL_MIN            (NET_CAN_CAN_DL)

#define NET_CAN_WFT_FIRST            0xFF

#define NET_CAN_BS_TIME K_MSEC(1000)
#define NET_CAN_A_TIME  K_MSEC(1000)

#define NET_CAN_FF_CF_TIME K_MSEC(1)

#define NET_CAN_STMIN_MAX            0xFA
#define NET_CAN_STMIN_MS_MAX         0x7F
#define NET_CAN_STMIN_US_BEGIN       0xF1
#define NET_CAN_STMIN_US_END         0xF9

#ifdef __cplusplus
extern "C" {
#endif

enum net_can_isotp_tx_state {
	NET_CAN_TX_STATE_UNUSED,
	NET_CAN_TX_STATE_RESET,
	NET_CAN_TX_STATE_WAIT_FC,
	NET_CAN_TX_STATE_SEND_CF,
	NET_CAN_TX_STATE_WAIT_ST,
	NET_CAN_TX_STATE_WAIT_TX_BACKLOG,
	NET_CAN_TX_STATE_FIN,
	NET_CAN_TX_STATE_ERR
};

enum net_can_isotp_rx_state {
	NET_CAN_RX_STATE_UNUSED,
	NET_CAN_RX_STATE_RESET,
	NET_CAN_RX_STATE_FF,
	NET_CAN_RX_STATE_CF,
	NET_CAN_RX_STATE_FIN,
	NET_CAN_RX_STATE_TIMEOUT
};

struct canbus_l2_ctx {
	struct canbus_isotp_tx_ctx tx_ctx[CONFIG_NET_PKT_TX_COUNT];
	struct canbus_isotp_rx_ctx rx_ctx[CONFIG_NET_PKT_RX_COUNT];
	struct k_mutex tx_ctx_mtx;
	struct k_mutex rx_ctx_mtx;
	struct k_sem tx_sem;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_NET_L2_CANBUS_INTERNAL_H_ */
