/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifdef CONFIG_NET_SAMPLES_IP_ADDRESSES
#ifdef CONFIG_NET_IPV6
#define ZEPHYR_ADDR		CONFIG_NET_SAMPLES_MY_IPV6_ADDR
#else
#define ZEPHYR_ADDR		CONFIG_NET_SAMPLES_MY_IPV4_ADDR
#endif
#else
#ifdef CONFIG_NET_IPV6
#define ZEPHYR_ADDR		"2001:db8::1"
#else
#define ZEPHYR_ADDR		"192.168.1.101"
#endif
#endif

#define ZEPHYR_PORT		80

#define HTTP_AUTH_URL		"/auth"
#define HTTP_AUTH_TYPE		"Basic"

/* HTTP Basic Auth, see https://tools.ietf.org/html/rfc7617 */
#define HTTP_AUTH_REALM		"Zephyr"
#define HTTP_AUTH_USERNAME	"zephyr"
#define HTTP_AUTH_PASSWORD	"0123456789"
#define HTTP_AUTH_CREDENTIALS	"emVwaHlyOjAxMjM0NTY3ODk="

#define APP_SLEEP_MSECS		500

#ifdef CONFIG_MBEDTLS
#define SERVER_PORT		443
#endif

#endif
