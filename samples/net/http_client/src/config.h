/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <net/net_ip.h>

#define APP_NAP_TIME 3000

#define HTTP_POOL_BUF_CTR	4
#define HTTP_POOL_BUF_SIZE	1024
#define HTTP_STATUS_STR_SIZE	32

/* server port */
#define SERVER_PORT		80
/* rx tx timeout */
#define HTTP_NETWORK_TIMEOUT	300

#ifdef CONFIG_NET_SAMPLES_IP_ADDRESSES
#ifdef CONFIG_NET_IPV6
#define LOCAL_ADDR	CONFIG_NET_SAMPLES_MY_IPV6_ADDR
#define SERVER_ADDR	CONFIG_NET_SAMPLES_PEER_IPV6_ADDR
#else
#define LOCAL_ADDR	CONFIG_NET_SAMPLES_MY_IPV4_ADDR
#define SERVER_ADDR	CONFIG_NET_SAMPLES_PEER_IPV4_ADDR
#endif
#else
#ifdef CONFIG_NET_IPV6
#define LOCAL_ADDR	"2001:db8::1"
#define SERVER_ADDR	"2001:db8::2"
#else
#define LOCAL_ADDR	"192.168.1.101"
#define SERVER_ADDR	"192.168.1.10"
#endif
#endif /* CONFIG */

/* It seems enough to hold 'Content-Length' and its value */
#define CON_LEN_SIZE	48

/* Default HTTP Header Field values for HTTP Requests */
#define ACCEPT		"text/plain"
#define ACCEPT_ENC	"identity"
#define ACCEPT_LANG	"en-US"
#define CONNECTION	"Close"
#define USER_AGENT	"ZephyrHTTPClient/1.7"
#define HOST_NAME	SERVER_ADDR /* or example.com, www.example.com */

#define HEADER_FIELDS	"Accept: "ACCEPT"\r\n" \
			"Accept-Encoding: "ACCEPT_ENC"\r\n" \
			"Accept-Language: "ACCEPT_LANG"\r\n" \
			"User-Agent: "USER_AGENT"\r\n" \
			"Host: "HOST_NAME"\r\n" \
			"Connection: "CONNECTION"\r\n"

/* Parsing and token tracking becomes a bit complicated if the
 * RX buffer is fragmented. for example: an HTTP response with
 * header fields that lie in two fragments. So, here we have
 * two options:
 *
 * - Use the fragmented buffer, but increasing the fragment size
 * - Linearize the buffer, it works better but consumes more memory
 *
 * Comment the following define to test the first case, set the
 * CONFIG_NET_NBUF_DATA_SIZE variable to 384 or 512. See the
 * prj_frdm_k64f.conf file.
 */
#define LINEARIZE_BUFFER

#ifndef LINEARIZE_BUFFER
#if CONFIG_NET_NBUF_DATA_SIZE <= 256
#error set CONFIG_NET_NBUF_DATA_SIZE to 384 or 512
#endif
#endif
