/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define APP_REQ_TIMEOUT K_SECONDS(5)

/* The startup time needs to be longish if DHCP is enabled as setting
 * DHCP up takes some time.
 */
#define APP_STARTUP_TIME K_SECONDS(20)

#define POST_CONTENT_TYPE "application/x-www-form-urlencoded"
#define POST_PAYLOAD "os=ZephyrRTOS&arch=" CONFIG_ARCH

#define SERVER_PORT  4443

#define HOSTNAME "localhost"   /* for cert verification if that is enabled */

#if defined(CONFIG_NET_IPV6)
#define SERVER_ADDR CONFIG_NET_APP_PEER_IPV6_ADDR
#else
#define SERVER_ADDR CONFIG_NET_APP_PEER_IPV4_ADDR
#endif
