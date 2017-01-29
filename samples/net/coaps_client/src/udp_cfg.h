/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UDP_CONFIG_H_
#define UDP_CONFIG_H_

/* admin-local, dynamically allocated multicast address */
#define MCAST_IP_ADDR { { { 0xff, 0x84, 0, 0, 0, 0, 0, 0, \
			    0, 0, 0, 0, 0, 0, 0, 0x2 } } }

static struct in6_addr client_addr;

#define SERVER_PORT	5684
#define CLIENT_PORT	8484

#define UDP_TX_TIMEOUT 100	/* Timeout in milliseconds */

#endif
