/*
 * Copyright (c) 2022 René Beckmann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *
 * @brief Deprecate MQTT-SN header
 */

#ifndef ZEPHYR_INCLUDE_NET_MQTT_SN_DEPRECATED_H_
#define ZEPHYR_INCLUDE_NET_MQTT_SN_DEPRECATED_H_

#include <zephyr/mqtt_sn.h>

#ifdef CONFIG_MQTT_SN_TRANSPORT_UDP
#include <zephyr/net/mqtt_sn/udp.h>
#endif

#warning This header is deprecated, include <zephyr/mqtt_sn.h> instead

#endif /* ZEPHYR_INCLUDE_NET_MQTT_SN_DEPRECATED_H_ */
