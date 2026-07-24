/*
 * Copyright (c) 2022 René Beckmann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file udp.h
 *
 * @brief MQTT-SN UDP Transport Implementation
 *
 * @defgroup mqtt_sn_udp MQTT-SN UDP transport
 * @since 3.3
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

#ifndef ZEPHYR_INCLUDE_NET_MQTT_SN_UDP_H_
#define ZEPHYR_INCLUDE_NET_MQTT_SN_UDP_H_

#include <zephyr/mqtt_sn.h>
#include <zephyr/net/net_ip.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Transport struct for UDP based transport.
 */
struct mqtt_sn_transport_udp {
	/** Parent struct */
	struct mqtt_sn_transport tp;

	/** Socket FD */
	int sock;

	/** Address of broadcasts */
	struct net_sockaddr bcaddr;
	/** Size of `bcaddr` */
	net_socklen_t bcaddrlen;
};

/**
 * @brief Retrieve UDP transport from common part.
 *
 * @param transport Pointer to common part of the transport
 *
 * @return Pointer to UDP transport
 */
#define UDP_TRANSPORT(transport) CONTAINER_OF(transport, struct mqtt_sn_transport_udp, tp)

/**
 * @brief Initialize the UDP transport.
 *
 * @param[in] udp The transport to be initialized
 * @param[in] gwaddr Pre-initialized gateway address
 * @param[in] addrlen Size of the gwaddr structure.
 */
int mqtt_sn_transport_udp_init(struct mqtt_sn_transport_udp *udp, struct net_sockaddr *gwaddr,
			       net_socklen_t addrlen);
#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_MQTT_SN_UDP_H_ */

/**@}  */
