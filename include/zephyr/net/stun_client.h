/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief STUN client over UDP sockets
 */

#ifndef ZEPHYR_INCLUDE_NET_STUN_CLIENT_H_
#define ZEPHYR_INCLUDE_NET_STUN_CLIENT_H_

/**
 * @brief STUN client
 * @defgroup stun_client STUN client
 * @since 4.6
 * @version 0.1.0
 * @ingroup stun
 *
 * Asks a STUN server which transport address it sees this device at — the
 * address on the outside of the NAT, if there is one. Built on the Binding
 * transaction of @ref stun, with the sockets, the clock and the random number
 * generator that the transaction leaves to its caller.
 *
 * A protocol that shares its socket with STUN — ICE does — uses the transaction
 * directly instead: the calls here block and consume everything that arrives on
 * the socket in the meantime.
 * @{
 */

#include <stdint.h>

#include <zephyr/net/net_ip.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Port STUN servers listen on by default (RFC 8489 §18.6). */
#define STUN_CLIENT_DEFAULT_PORT 3478

/**
 * @brief Ask a STUN server for the address it sees a socket at
 *
 * Sends a Binding request from @p sock and waits for the answer,
 * retransmitting as RFC 8489 §6.2.1 has it. The address returned is that of
 * @p sock as the server sees it, so it is the one to tell a peer that is to
 * reach this very socket.
 *
 * The call blocks. For its duration the socket is the transaction's alone:
 * datagrams that are not the server's answer are received and dropped.
 *
 * @param sock UDP socket, of the family of @p server.
 * @param server Address and port of the STUN server.
 * @param server_len Size of @p server.
 * @param timeout_ms Longest time to wait, in milliseconds; 0 to wait for as
 *        long as the retransmission schedule lasts: 39.5 seconds, more if
 *        the calling thread is held up between the transmissions.
 * @param mapped Receives the address and port the server saw.
 *
 * @retval 0 on success.
 * @retval -ETIMEDOUT No answer in time.
 * @retval -EPROTO The server answered with an error, or with a response that
 *         cannot be used.
 * @retval -EINVAL NULL argument, @p server_len too short for the address, or
 *         a server without a port.
 * @retval -EAFNOSUPPORT The server address is neither IPv4 nor IPv6, or it is
 *         IPv6 and the IPv6 stack is not enabled.
 * @retval -EIO No random transaction id could be generated.
 * @return Another negative errno if sending or receiving failed.
 */
int stun_client_query(int sock, const struct net_sockaddr *server, net_socklen_t server_len,
		      uint32_t timeout_ms, struct net_sockaddr *mapped);

/**
 * @brief Ask a STUN server, given by name, for the address it sees this device at
 *
 * Resolves the name, opens a UDP socket of the family of the first address it
 * resolves to, calls stun_client_query() and closes the socket. The address
 * returned tells the public IP address of the device; the port belonged to the
 * socket that is gone by the time the call returns.
 *
 * @param host Host name or literal address of the server: "stun.example.org",
 *        "192.0.2.1", "2001:db8::1".
 * @param port Port of the server; 0 for @ref STUN_CLIENT_DEFAULT_PORT.
 * @param timeout_ms Longest time to wait for the answer, see
 *        stun_client_query(). Resolving the name is not counted.
 * @param mapped Receives the address and port the server saw.
 *
 * @retval 0 on success.
 * @retval -EHOSTUNREACH The name could not be resolved. A host name, as
 *         opposed to a literal address, takes @kconfig{CONFIG_DNS_RESOLVER}.
 * @return Otherwise as stun_client_query(), or a negative errno if no socket
 *         could be opened.
 */
int stun_client_simple(const char *host, uint16_t port, uint32_t timeout_ms,
		       struct net_sockaddr *mapped);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_NET_STUN_CLIENT_H_ */
