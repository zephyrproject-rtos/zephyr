/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

/* The startup time needs to be longish if DHCP is enabled as setting
 * DHCP up takes some time.
 */
#define APP_STARTUP_TIME K_SECONDS(20)

#ifdef CONFIG_NET_APP_SETTINGS
#ifdef CONFIG_NET_IPV6
#define ZEPHYR_ADDR            CONFIG_NET_APP_MY_IPV6_ADDR
#else
#define ZEPHYR_ADDR            CONFIG_NET_APP_MY_IPV4_ADDR
#endif
#else
#ifdef CONFIG_NET_IPV6
#define ZEPHYR_ADDR            "2001:db8::1"
#else
#define ZEPHYR_ADDR            "192.0.2.1"
#endif
#endif

#ifndef ZEPHYR_PORT
#define ZEPHYR_PORT		8080
#endif

#define HTTP_AUTH_URL		"/auth"
#define HTTP_AUTH_TYPE		"Basic"

/* HTTP Basic Auth, see https://tools.ietf.org/html/rfc7617 */
#define HTTP_AUTH_REALM		"Zephyr"
#define HTTP_AUTH_USERNAME	"zephyr"
#define HTTP_AUTH_PASSWORD	"0123456789"

#endif
