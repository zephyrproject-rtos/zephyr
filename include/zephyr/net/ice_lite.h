/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ICE-lite agent (@rfc{8445})
 * @ingroup ice_lite
 */

#ifndef ZEPHYR_INCLUDE_NET_ICE_LITE_H_
#define ZEPHYR_INCLUDE_NET_ICE_LITE_H_

/**
 * @brief Lite implementation of ICE (@rfc{8445,section-2.5})
 * @defgroup ice_lite ICE-lite agent
 * @since 4.6
 * @version 0.1.0
 * @ingroup networking
 *
 * The agent answers the connectivity checks of a full ICE agent and reports the
 * address the peer nominates. It offers host candidates only, sends no checks
 * and is always the controlled agent. One agent serves one component of one
 * data stream.
 *
 * The agent does no I/O: the application owns the UDP socket, which it shares
 * with the data protocol, and passes every received datagram to
 * ice_lite_input().
 * @{
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/net/net_ip.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Length of the username fragment of the agent, @kconfig{CONFIG_ICE_LITE_UFRAG_LEN}. */
#define ICE_LITE_UFRAG_LEN ((size_t)CONFIG_ICE_LITE_UFRAG_LEN)
/** Length of the password of the agent, @kconfig{CONFIG_ICE_LITE_PWD_LEN}. */
#define ICE_LITE_PWD_LEN   ((size_t)CONFIG_ICE_LITE_PWD_LEN)

/** Size of a buffer that holds any response of ice_lite_input(). */
#define ICE_LITE_RESPONSE_SIZE 128U

/**
 * @brief Priority of a host candidate (@rfc{8445,section-5.1.2.1})
 *
 * @param local_pref Local preference, 0 to 65535: 65535 for a device with one
 *        address, else unique per address.
 * @param component Component id, 1 to 256.
 */
#define ICE_LITE_HOST_PRIORITY(local_pref, component)                                              \
	((126UL << 24) | ((uint32_t)(local_pref) << 8) | (256UL - (uint32_t)(component)))

/**
 * @brief An ICE-lite agent
 *
 * Set up with ice_lite_init(). The members are for reading.
 */
struct ice_lite_agent {
	/** Username fragment, for the peer. */
	char ufrag[ICE_LITE_UFRAG_LEN + 1U];
	/** Password, for the peer. */
	char pwd[ICE_LITE_PWD_LEN + 1U];
	/** Tie-breaker, sent in the response to a role conflict (@rfc{8445,section-7.3.1.1}). */
	uint64_t tiebreaker;
	/** The peer has nominated an address. */
	bool nominated;
	/** The address last nominated: where the data is sent. */
	struct net_sockaddr_storage remote;
};

/**
 * @brief Set up an agent with new credentials
 *
 * Also restarts ICE on an agent that is in use.
 *
 * @param[out] agent Agent.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p agent is NULL.
 * @retval -EIO No random credentials or tie-breaker could be generated.
 */
int ice_lite_init(struct ice_lite_agent *agent);

/**
 * @brief Process a received datagram
 *
 * Answers a connectivity check and records the nomination it may carry. A
 * check that fails authentication gets an error response and changes nothing.
 *
 * @param[in,out] agent Agent.
 * @param buf Datagram.
 * @param len Its length.
 * @param src Address the datagram came from, IPv4 or IPv6.
 * @param[out] resp Buffer for the response, of @ref ICE_LITE_RESPONSE_SIZE bytes.
 * @param resp_size Its size.
 *
 * @return The length of the response in @p resp, to be sent to @p src.
 * @retval 0 STUN message that needs no response.
 * @retval -ENOMSG Not a STUN message, or a malformed or damaged one: the
 *         datagram is for the data protocol.
 * @retval -EINVAL NULL argument.
 * @retval -EAFNOSUPPORT @p src is neither IPv4 nor IPv6, or it is IPv6 and
 *         @kconfig{CONFIG_NET_IPV6} is not enabled.
 * @retval -EMSGSIZE The response does not fit into @p resp.
 * @retval -EIO The cryptographic backend failed.
 */
int ice_lite_input(struct ice_lite_agent *agent, const uint8_t *buf, size_t len,
		   const struct net_sockaddr *src, uint8_t *resp, size_t resp_size);

/**
 * @brief Build a keepalive
 *
 * A Binding indication, to be sent to the nominated address when no data has
 * been sent to it for 15 seconds (@rfc{8445,section-11}).
 *
 * @param[out] buf Buffer for the message.
 * @param size Its size, at least 28 bytes.
 *
 * @return The length of the message in @p buf.
 * @retval -EINVAL @p buf is NULL.
 * @retval -EMSGSIZE The message does not fit into @p buf.
 * @retval -EIO No random transaction id could be generated.
 */
int ice_lite_keepalive(uint8_t *buf, size_t size);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_NET_ICE_LITE_H_ */
