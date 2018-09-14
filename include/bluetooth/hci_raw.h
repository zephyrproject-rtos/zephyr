/** @file
 *  @brief Bluetooth HCI RAW channel handling
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_HCI_RAW_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_HCI_RAW_H_

/**
 * @brief HCI RAW channel
 * @defgroup hci_raw HCI RAW channel
 * @ingroup bluetooth
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Send packet to the Bluetooth controller
 *
 * Send packet to the Bluetooth controller. Caller needs to
 * implement netbuf pool.
 *
 * @param buf netbuf packet to be send
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_send(struct net_buf *buf);

/** @brief Enable Bluetooth RAW channel
 *
 *  Enable Bluetooth RAW HCI channel.
 *
 *  @param rx_queue netbuf queue where HCI packets received from the Bluetooth
 *  controller are to be queued. The queue is defined in the caller while
 *  the available buffers pools are handled in the stack.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_enable_raw(struct k_fifo *rx_queue);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HCI_RAW_H_ */
