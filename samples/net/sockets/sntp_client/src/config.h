/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef CONFIG_NET_CONFIG_SETTINGS
#ifdef CONFIG_NET_IPV6
#define SERVER_ADDR6		CONFIG_NET_CONFIG_PEER_IPV6_ADDR
#endif
#define SERVER_ADDR		CONFIG_NET_CONFIG_PEER_IPV4_ADDR
#else
#ifdef CONFIG_NET_IPV6
#define SERVER_ADDR6		"2001:db8::2"
#endif
#define SERVER_ADDR		"192.0.2.2"
#endif

#endif
