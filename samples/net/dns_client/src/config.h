/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifdef CONFIG_NET_IPV6
#define LOCAL_ADDR		{ { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0,\
				      0, 0, 0, 0, 0, 0x1 } } }
#define REMOTE_ADDR		{ { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0,\
				      0, 0, 0, 0, 0, 0x2 } } }
#else
#define LOCAL_ADDR		{ { { 192, 0, 2, 1 } } }
#define REMOTE_ADDR		{ { { 192, 0, 2, 2 } } }
#endif

#define MY_PORT			0
#define PEER_PORT		5353

#define APP_SLEEP_MSECS		400

/* The DNS server may return more than 1 IP address.
 * This value controls the max number of IP addresses
 * that the client will store per query.
 */
#define MAX_ADDRESSES		4

#endif
