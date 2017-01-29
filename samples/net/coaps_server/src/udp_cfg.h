/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UDP_CONFIG_H_
#define UDP_CONFIG_H_

#define MCAST_IP_ADDR { { { 0xff, 0x84, 0, 0, 0, 0, 0, 0, \
			    0, 0, 0, 0, 0, 0, 0, 0x2 } } }

#define SERVER_PREFIX_LEN 64

static struct in6_addr server_addr;
static struct in6_addr mcast_addr = MCAST_IP_ADDR;

#define SERVER_PORT	5684

#endif
