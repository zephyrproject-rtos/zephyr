/**
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVERS_BLUETOOTH_HCI_USERCHAN_BOTTOM_H
#define DRIVERS_BLUETOOTH_HCI_USERCHAN_BOTTOM_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool user_chan_rx_ready(int fd);
int user_chan_is_ipaddr_ok(char ip_addr[]);
int user_chan_socket_open(unsigned short bt_dev_index);
int user_chan_net_connect(char ip_addr[], unsigned int port);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_BLUETOOTH_HCI_USERCHAN_BOTTOM_H */
