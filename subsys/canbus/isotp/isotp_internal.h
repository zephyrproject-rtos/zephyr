/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_NET_CAN_ISOTP_INTERNAL_H_
#define ZEPHYR_SUBSYS_NET_CAN_ISOTP_INTERNAL_H_


#include <zephyr/canbus/isotp.h>
#include <zephyr/sys/slist.h>

/*
 * Abbreviations
 * BS      Block Size
 * CAN_DL  CAN LL data size
 * CF      Consecutive Frame
 * CTS     Continue to send
 * DLC     Data length code
 * FC      Flow Control
 * FF      First Frame
 * SF      Single Frame
 * FS      Flow Status
 * AE      Adders Extension
 * SN      Sequence Number
 * ST      Separation time
 * PCI     Process Control Information
 */

/* This is for future use when we have CAN-FD */
#ifdef ISOTP_USE_CAN_FD
/* #define ISOTP_CAN_DL CONFIG_ISOTP_TX_DL* */
#define ISOTP_CAN_DL 8
#else
#define ISOTP_CAN_DL 8
#endif/*ISOTP_USE_CAN_FD*/

/* Protocol control information*/
#define ISOTP_PCI_SF 0x00 /* Single frame*/
#define ISOTP_PCI_FF 0x01 /* First frame */
#define ISOTP_PCI_CF 0x02 /* Consecutive frame */
#define ISOTP_PCI_FC 0x03 /* Flow control frame */

#define ISOTP_PCI_TYPE_BYTE        0
#define ISOTP_PCI_TYPE_POS         4
#define ISOTP_PCI_TYPE_MASK        0xF0
#define ISOTP_PCI_TYPE_SF   (ISOTP_PCI_SF << ISOTP_PCI_TYPE_POS)
#define ISOTP_PCI_TYPE_FF   (ISOTP_PCI_FF << ISOTP_PCI_TYPE_POS)
#define ISOTP_PCI_TYPE_CF   (ISOTP_PCI_CF << ISOTP_PCI_TYPE_POS)
#define ISOTP_PCI_TYPE_FC   (ISOTP_PCI_FC << ISOTP_PCI_TYPE_POS)

#define ISOTP_PCI_SF_DL_MASK       0x0F

#define ISOTP_PCI_FF_DL_UPPER_BYTE 0
#define ISOTP_PCI_FF_DL_UPPER_MASK 0x0F
#define ISOTP_PCI_FF_DL_LOWER_BYTE 1

#define ISOTP_PCI_FS_BYTE          0
#define ISOTP_PCI_FS_MASK          0x0F
#define ISOTP_PCI_BS_BYTE          1
#define ISOTP_PCI_ST_MIN_BYTE      2

#define ISOTP_PCI_FS_CTS           0x0
#define ISOTP_PCI_FS_WAIT          0x1
#define ISOTP_PCI_FS_OVFLW         0x2

#define ISOTP_PCI_SN_MASK          0x0F

#define ISOTP_FF_DL_MIN            (ISOTP_CAN_DL)

#define ISOTP_STMIN_MAX            0xFA
#define ISOTP_STMIN_MS_MAX         0x7F
#define ISOTP_STMIN_US_BEGIN       0xF1
#define ISOTP_STMIN_US_END         0xF9

#define ISOTP_WFT_FIRST            0xFF

#define ISOTP_BS (CONFIG_ISOTP_BS_TIMEOUT)
#define ISOTP_A  (CONFIG_ISOTP_A_TIMEOUT)
#define ISOTP_CR (CONFIG_ISOTP_CR_TIMEOUT)

/* Just before the sender would time out*/
#define ISOTP_ALLOC_TIMEOUT (CONFIG_ISOTP_A_TIMEOUT - 100)

#ifdef __cplusplus
extern "C" {
#endif

enum isotp_rx_state {
	ISOTP_RX_STATE_WAIT_FF_SF,
	ISOTP_RX_STATE_PROCESS_SF,
	ISOTP_RX_STATE_PROCESS_FF,
	ISOTP_RX_STATE_TRY_ALLOC,
	ISOTP_RX_STATE_SEND_FC,
	ISOTP_RX_STATE_WAIT_CF,
	ISOTP_RX_STATE_SEND_WAIT,
	ISOTP_RX_STATE_ERR,
	ISOTP_RX_STATE_RECYCLE,
	ISOTP_RX_STATE_UNBOUND
};

enum isotp_tx_state {
	ISOTP_TX_STATE_RESET,
	ISOTP_TX_SEND_SF,
	ISOTP_TX_SEND_FF,
	ISOTP_TX_WAIT_FC,
	ISOTP_TX_SEND_CF,
	ISOTP_TX_WAIT_ST,
	ISOTP_TX_WAIT_BACKLOG,
	ISOTP_TX_WAIT_FIN,
	ISOTP_TX_ERR
};

struct isotp_global_ctx {
	sys_slist_t alloc_list;
	sys_slist_t ff_sf_alloc_list;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_NET_CAN_ISOTP_INTERNAL_H_ */
