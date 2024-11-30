/*
 * Copyright (c) 2024 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _W5500_SOCKET_H_
#define _W5500_SOCKET_H_

#include <zephyr/device.h>

/* Sn_IR values */
#define W5500_Sn_IR_SENDOK  0x10
#define W5500_Sn_IR_TIMEOUT 0x08
#define W5500_Sn_IR_RECV    0x04
#define W5500_Sn_IR_DISCON  0x02
#define W5500_Sn_IR_CON     0x01

/* Sn_SR values */
#define W5500_SOCK_CLOSED      0x00
#define W5500_SOCK_INIT        0x13
#define W5500_SOCK_LISTEN      0x14
#define W5500_SOCK_SYNSENT     0x15
#define W5500_SOCK_SYNRECV     0x16
#define W5500_SOCK_ESTABLISHED 0x17
#define W5500_SOCK_FIN_WAIT    0x18
#define W5500_SOCK_CLOSING     0x1A
#define W5500_SOCK_TIME_WAIT   0x1B
#define W5500_SOCK_CLOSE_WAIT  0x1C
#define W5500_SOCK_LAST_ACK    0x1D
#define W5500_SOCK_UDP         0x22
#define W5500_SOCK_IPRAW       0x32

#define W5500_SOCKET_LUT_MAX_ENTRIES (W5500_MAX_SOCK_NUM)
#define W5500_SOCKET_LUT_UNASSIGNED  W5500_MAX_SOCK_NUM

#define W5500_SOCKET_LISTEN_CTX_UNASSIGNED W5500_MAX_SOCK_NUM

struct w5500_socket_lookup_entry {
	uint8_t socknum;
};

#endif // _W5500_SOCKET_H_
