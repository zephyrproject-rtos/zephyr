/** @file
 * @brief NET_MGMT socket definitions.
 *
 * Definitions for NET_MGMT socket support.
 */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SOCKET_NET_MGMT_H_
#define ZEPHYR_INCLUDE_NET_SOCKET_NET_MGMT_H_

#include <zephyr/types.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Socket NET_MGMT library
 * @defgroup socket_net_mgmt Socket NET_MGMT library
 * @since 2.0
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/* Protocols of the protocol family PF_NET_MGMT */
#define NET_MGMT_EVENT_PROTO 0x01

/* Socket NET_MGMT options */
#define SOL_NET_MGMT_BASE 100
#define SOL_NET_MGMT_RAW (SOL_NET_MGMT_BASE + 1)

/** @endcond */

/**
 * @name Socket options for NET_MGMT sockets
 * @{
 */

/** Set Ethernet Qav parameters */
#define SO_NET_MGMT_ETHERNET_SET_QAV_PARAM 1

/** Get Ethernet Qav parameters */
#define SO_NET_MGMT_ETHERNET_GET_QAV_PARAM 2

/** @} */ /* for @name */


/**
 * Each network management message is prefixed with this header.
 */
struct net_mgmt_msghdr {
	/** Network management version */
	uint32_t nm_msg_version;

	/** Length of the data */
	uint32_t nm_msg_len;

	/** The actual message data follows */
	uint8_t nm_msg[];
};

/**
 * Version of the message is placed to the header. Currently we have
 * following versions.
 *
 * Network management message versions:
 *
 *  0x0001 : The net_mgmt event info message follows directly
 *           after the header.
 */
#define NET_MGMT_SOCKET_VERSION_1 0x0001

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_SOCKET_NET_MGMT_H_ */
