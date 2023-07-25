/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 * @brief Internal Transport SAR Configuration API
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_MESH_SAR_CFG_INTERNAL_H__
#define ZEPHYR_SUBSYS_BLUETOOTH_MESH_SAR_CFG_INTERNAL_H__

/** SAR Transmitter Configuraion state encoded length */
#define BT_MESH_SAR_TX_LEN 4
/** SAR Receiver Configuraion state encoded length */
#define BT_MESH_SAR_RX_LEN 3

/** SAR Transmitter Configuration state initializer */
#define BT_MESH_SAR_TX_INIT                                                       \
	{                                                                         \
		CONFIG_BT_MESH_SAR_TX_SEG_INT_STEP,                               \
			CONFIG_BT_MESH_SAR_TX_UNICAST_RETRANS_COUNT,              \
			CONFIG_BT_MESH_SAR_TX_UNICAST_RETRANS_WITHOUT_PROG_COUNT, \
			CONFIG_BT_MESH_SAR_TX_UNICAST_RETRANS_INT_STEP,           \
			CONFIG_BT_MESH_SAR_TX_UNICAST_RETRANS_INT_INC,            \
			CONFIG_BT_MESH_SAR_TX_MULTICAST_RETRANS_COUNT,            \
			CONFIG_BT_MESH_SAR_TX_MULTICAST_RETRANS_INT,              \
	}

/** SAR Receiver Configuration state initializer */
#define BT_MESH_SAR_RX_INIT                                                    \
	{                                                                      \
		CONFIG_BT_MESH_SAR_RX_SEG_THRESHOLD,                           \
			CONFIG_BT_MESH_SAR_RX_ACK_DELAY_INC,                   \
			CONFIG_BT_MESH_SAR_RX_DISCARD_TIMEOUT,                 \
			CONFIG_BT_MESH_SAR_RX_SEG_INT_STEP,                    \
			CONFIG_BT_MESH_SAR_RX_ACK_RETRANS_COUNT,               \
	}

#define BT_MESH_SAR_TX_SEG_INT_MS ((bt_mesh.sar_tx.seg_int_step + 1) * 10)
#define BT_MESH_SAR_TX_RETRANS_COUNT(addr)                                     \
	(BT_MESH_ADDR_IS_UNICAST(addr) ?                                       \
		 (bt_mesh.sar_tx.unicast_retrans_count + 1) :                  \
		 (bt_mesh.sar_tx.multicast_retrans_count + 1))
#define BT_MESH_SAR_TX_RETRANS_NO_PROGRESS                                     \
	(bt_mesh.sar_tx.unicast_retrans_without_prog_count + 1)
#define BT_MESH_SAR_TX_UNICAST_RETRANS_INT_STEP_MS                             \
	((bt_mesh.sar_tx.unicast_retrans_int_step + 1) * 25)
#define BT_MESH_SAR_TX_UNICAST_RETRANS_INT_INC_MS                              \
	((bt_mesh.sar_tx.unicast_retrans_int_inc + 1) * 25)
#define BT_MESH_SAR_TX_UNICAST_RETRANS_TIMEOUT_MS(ttl)                         \
	((ttl > 0) ? (BT_MESH_SAR_TX_UNICAST_RETRANS_INT_STEP_MS +             \
		      BT_MESH_SAR_TX_UNICAST_RETRANS_INT_INC_MS * (ttl - 1)) : \
		     BT_MESH_SAR_TX_UNICAST_RETRANS_INT_STEP_MS)
#define BT_MESH_SAR_TX_MULTICAST_RETRANS_TIMEOUT_MS                            \
	((bt_mesh.sar_tx.multicast_retrans_int + 1) * 25)
#define BT_MESH_SAR_TX_RETRANS_TIMEOUT_MS(addr, ttl)                           \
	(BT_MESH_ADDR_IS_UNICAST(addr) ?                                       \
		 BT_MESH_SAR_TX_UNICAST_RETRANS_TIMEOUT_MS(ttl) :              \
		 BT_MESH_SAR_TX_MULTICAST_RETRANS_TIMEOUT_MS)

#define BT_MESH_SAR_RX_SEG_THRESHOLD (bt_mesh.sar_rx.seg_thresh)
#define BT_MESH_SAR_RX_ACK_DELAY_INC_X2 (bt_mesh.sar_rx.ack_delay_inc * 2 + 3)
#define BT_MESH_SAR_RX_ACK_RETRANS_COUNT (bt_mesh.sar_rx.ack_retrans_count + 1)
#define BT_MESH_SAR_RX_SEG_INT_MS ((bt_mesh.sar_rx.rx_seg_int_step + 1) * 10)
#define BT_MESH_SAR_RX_DISCARD_TIMEOUT_MS                                      \
	((bt_mesh.sar_rx.discard_timeout + 1) * 5 * MSEC_PER_SEC)

void bt_mesh_sar_tx_encode(struct net_buf_simple *buf,
			   const struct bt_mesh_sar_tx *tx);
void bt_mesh_sar_rx_encode(struct net_buf_simple *buf,
			   const struct bt_mesh_sar_rx *rx);
void bt_mesh_sar_tx_decode(struct net_buf_simple *buf,
			   struct bt_mesh_sar_tx *tx);
void bt_mesh_sar_rx_decode(struct net_buf_simple *buf,
			   struct bt_mesh_sar_rx *rx);

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_MESH_SAR_CFG_INTERNAL_H__ */
