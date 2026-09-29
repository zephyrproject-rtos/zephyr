/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_MISC_UART_BT_SRC_COMMON_H_
#define ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_MISC_UART_BT_SRC_COMMON_H_

#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>

#define TX_FIFO_SIZE DT_PROP(DT_NODELABEL(nus_uart), tx_fifo_size)

/* Written by the DUT before any peer is subscribed. Longer than the TX FIFO,
 * so the writer finishes only if the driver drops what does not fit, and the
 * first subscriber receives the first TX_FIFO_SIZE bytes of it.
 */
#define UNSUBSCRIBED_MSG "unsubscribed-unsubscribed-unsubscribed-unsubscribed-unsubscribed-"
BUILD_ASSERT(sizeof(UNSUBSCRIBED_MSG) - 1 > TX_FIFO_SIZE);

#define SUBSCRIBED_MSG "subscribed"

#endif /* ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_MISC_UART_BT_SRC_COMMON_H_ */
