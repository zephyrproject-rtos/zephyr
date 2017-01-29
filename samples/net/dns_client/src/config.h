/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifdef CONFIG_NET_SAMPLES_IP_ADDRESSES
#ifdef CONFIG_NET_IPV6
#define LOCAL_ADDR		CONFIG_NET_SAMPLES_MY_IPV6_ADDR
#define REMOTE_ADDR		CONFIG_NET_SAMPLES_PEER_IPV6_ADDR
#else
#define LOCAL_ADDR		CONFIG_NET_SAMPLES_MY_IPV4_ADDR
#define REMOTE_ADDR		CONFIG_NET_SAMPLES_PEER_IPV4_ADDR
#endif
#else
#ifdef CONFIG_NET_IPV6
#define LOCAL_ADDR		"2001:db8::1"
#define REMOTE_ADDR		"2001:db8::2"
#else
#define LOCAL_ADDR		"192.168.1.101"
#define REMOTE_ADDR		"192.168.1.10"
#endif
#endif

#define REMOTE_PORT		5353

#define APP_SLEEP_MSECS		400

/* The DNS server may return more than 1 IP address.
 * This value controls the max number of IP addresses
 * that the client will store per query.
 */
#define MAX_ADDRESSES		4

#endif
