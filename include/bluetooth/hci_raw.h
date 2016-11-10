/** @file
 *  @brief Bluetooth HCI RAW channel handling
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __BT_HCI_RAW_H
#define __BT_HCI_RAW_H

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

#endif /* __BT_HCI_RAW_H */
