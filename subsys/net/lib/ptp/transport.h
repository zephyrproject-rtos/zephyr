/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file transport.h
 * @brief Function implementing abstraction over networking protocols.
 */

#ifndef ZEPHYR_INCLUDE_PTP_TRANSPORT_H_
#define ZEPHYR_INCLUDE_PTP_TRANSPORT_H_

#include <zephyr/net/net_ip.h>

#include "port.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PTP_SOCKET_PORT_EVENT (319)
#define PTP_SOCKET_PORT_GENERAL (320)

/**
 * @brief Values used to identify PTP Port socket based on used port.
 */
enum ptp_socket {
	PTP_SOCKET_EVENT,
	PTP_SOCKET_GENERAL,
	PTP_SOCKET_CNT
};

/**
 * @brief Types of PTP networking protocols.
 */
enum ptp_net_protocol {
	PTP_NET_PROTOCOL_UDP_IPv4 = 1,
	PTP_NET_PROTOCOL_UDP_IPv6,
	PTP_NET_PROTOCOL_IEEE_802_3,
};

/**
 * @brief Function handling opening specified transport network connection.
 *
 * @param[in] port Pointer to the PTP Port structure.
 *
 * @return 0 on success, negative otherwise.
 */
int ptp_transport_open(struct ptp_port *port);

/**
 * @brief Function for closing specified transport network connection.
 *
 * @param[in] port Pointer to the PTP Port structure.
 *
 * @return 0 on success, negative otherwise.
 */
int ptp_transport_close(struct ptp_port *port);

/**
 * @brief Function for sending PTP message using a specified transport. The message is sent
 * to the default multicast address.
 *
 * @note Address specified in the message is ignored.
 *
 * @param[in] port Pointer to the PTP Port structure.
 * @param[in] msg  Pointer to the message to be send.
 * @param[in] idx  Index of the socket to be used to send message.
 *
 * @return Number of sent bytes.
 */
int ptp_transport_send(struct ptp_port *port, struct ptp_msg *msg, enum ptp_socket idx);

/**
 * @brief Function for sending PTP message using a specified transport. The message is sent
 * to the address provided with @ref ptp_msg message structure.
 *
 * @param[in] port Pointer to the PTP Port structure.
 * @param[in] msg  Pointer to the message to be send.
 * @param[in] idx  Index of the socket to be used to send message.
 *
 * @return Number of sent bytes.
 */
int ptp_transport_sendto(struct ptp_port *port, struct ptp_msg *msg, enum ptp_socket idx);

/**
 * @brief Function for receiving a PTP message using a specified transport.
 *
 * @param[in] port Pointer to the PTP Port structure.
 * @param[in] idx  Index of the socket to be used to send message.
 *
 * @return
 */
int ptp_transport_recv(struct ptp_port *port, struct ptp_msg *msg, enum ptp_socket idx);

/**
 * @brief Function for getting transport's protocol address.
 *
 * @param[in] port Pointer to the PTP Port structure.
 * @param[in] addr Pointer to the buffer to store PTP Port's IP address.
 *
 * @return 0 if can't get IP address, otherwise length of the address.
 */
int ptp_transport_protocol_addr(struct ptp_port *port, uint8_t *addr);

/**
 * @brief Function for getting transport's physical address.
 *
 * @param[in] port Pointer to the PTP Port structure.
 *
 * @return Pointer to the structure holding hardware link layer address.
 */
struct net_linkaddr *ptp_transport_physical_addr(struct ptp_port *port);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PTP_TRANSPORT_H_ */
