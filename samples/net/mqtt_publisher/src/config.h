/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef CONFIG_NET_SAMPLES_IP_ADDRESSES
#ifdef CONFIG_NET_IPV6
#define ZEPHYR_ADDR		CONFIG_NET_SAMPLES_MY_IPV6_ADDR
#define SERVER_ADDR		CONFIG_NET_SAMPLES_PEER_IPV6_ADDR
#else
#define ZEPHYR_ADDR		CONFIG_NET_SAMPLES_MY_IPV4_ADDR
#define SERVER_ADDR		CONFIG_NET_SAMPLES_PEER_IPV4_ADDR
#endif
#else
#ifdef CONFIG_NET_IPV6
#define ZEPHYR_ADDR		"2001:db8::1"
#define SERVER_ADDR		"2001:db8::2"
#else
#define ZEPHYR_ADDR		"192.168.1.101"
#define SERVER_ADDR		"192.168.1.10"
#endif
#endif

#define SERVER_PORT		1883

#define APP_SLEEP_MSECS		500
#define APP_TX_RX_TIMEOUT       300

#define APP_CONNECT_TRIES	10

#define APP_MAX_ITERATIONS	100

#define MQTT_CLIENTID		"zephyr_publisher"

/* Set the following to 1 to enable the Bluemix topic format */
#define APP_BLUEMIX_TOPIC	0

/* These are the parameters for the Bluemix topic format */
#if APP_BLUEMIX_TOPIC
#define BLUEMIX_DEVTYPE		"sensor"
#define BLUEMIX_DEVID		"carbon"
#define BLUEMIX_EVENT		"status"
#define BLUEMIX_FORMAT		"json"
#endif

#endif
