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
 * @defgroup socket_net_mgmt Network Core Library
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
 * struct sockaddr_nm - The sockaddr structure for NET_MGMT sockets
 *
 * Similar concepts are used as in Linux AF_NETLINK. The NETLINK name is not
 * used in order to avoid confusion between Zephyr and Linux as the
 * implementations are different.
 *
 * The socket domain (address family) is AF_NET_MGMT, and the type of socket
 * is either SOCK_RAW or SOCK_DGRAM, because this is a datagram-oriented
 * service.
 *
 * The protocol (protocol type) selects for which feature the socket is used.
 *
 * When used with bind(), the nm_pid field of the sockaddr_nm can be
 * filled with the calling thread' own id. The nm_pid serves here as the local
 * address of this net_mgmt socket. The application is responsible for picking
 * a unique integer value to fill in nm_pid.
 */
struct sockaddr_nm {
	/** AF_NET_MGMT address family. */
	sa_family_t nm_family;

	/** Network interface related to this address */
	int nm_ifindex;

	/** Thread id or similar that is used to separate the different
	 * sockets. Application can decide how the pid is constructed.
	 */
	uintptr_t nm_pid;

	/** net_mgmt mask */
	uint32_t nm_mask;
};


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
