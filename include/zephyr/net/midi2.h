/*
 * Copyright (c) 2025 Titouan Christophe
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_MIDI2_H_
#define ZEPHYR_INCLUDE_NET_MIDI2_H_

/**
 * @defgroup net_midi2 Network MIDI 2.0
 * @since 4.3
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

/**
 * @defgroup netmidi10 User Datagram Protocol for Universal MIDI Packets
 * @ingroup net_midi2
 * @{
 * @details Definitions based on the following document
 * <a href="https://midi.org/network-midi-2-0">
 * User Datagram Protocol for Universal MIDI Packets
 * Network MIDI 2.0 (UDP) Transport Specification - Document version 1.0
 * (MIDI Association Document: M2-124-UM)
 * </a>
 * @}
 */


#include <stdint.h>
#include <zephyr/audio/midi.h>
#include <zephyr/net/socket.h>

/**
 * Size, in bytes, of the nonce sent to the client for authentication
 * @see netmidi10: 6.7 Invitation Reply: Authentication Required
 */
#define NETMIDI2_NONCE_SIZE  16

/**
 * @brief      Statically declare a Network (UDP) MIDI 2.0 endpoint host
 * @param      _var_name  The name of the variable holding the server data
 * @param      _ep_name   The UMP endpoint name
 * @param      _piid      The UMP Product Instance ID. If NULL,
 *                        HWINFO device ID will be used at runtime.
 * @param      _port      The UDP port to listen to, or 0 for automatic assignment
 */
#define NETMIDI2_EP_DEFINE(_var_name, _ep_name, _piid, _port) \
	static struct netmidi2_ep _var_name = { \
		.name = (_ep_name), \
		.piid = (_piid), \
		.addr4.sin_port = (_port), \
		.auth_type = NETMIDI2_AUTH_NONE, \
	}

/**
 * @brief      Statically declare a Network (UDP) MIDI 2.0 endpoint host,
 *             with a predefined shared secret key for authentication
 * @param      _var_name  The name of the variable holding the server data
 * @param      _ep_name   The UMP endpoint name
 * @param      _piid      The UMP Product Instance ID. If NULL,
 *                        HWINFO device ID will be used at runtime.
 * @param      _port      The UDP port to listen to, or 0 for automatic assignment
 * @param      _secret    The shared secret key clients must provide to connect
 */
#define NETMIDI2_EP_DEFINE_WITH_AUTH(_var_name, _ep_name, _piid, _port, _secret) \
	static struct netmidi2_ep _var_name = { \
		.name = (_ep_name), \
		.piid = (_piid), \
		.addr4.sin_port = (_port), \
		.auth_type = NETMIDI2_AUTH_SHARED_SECRET, \
		.shared_auth_secret = (_secret), \
	}

/**
 * @brief      Statically declare a Network (UDP) MIDI 2.0 endpoint host,
 *             with a predefined list of users/passwords for authentication
 * @param      _var_name  The name of the variable holding the server data
 * @param      _ep_name   The UMP endpoint name
 * @param      _piid      The UMP Product Instance ID. If NULL,
 *                        HWINFO device ID will be used at runtime.
 * @param      _port      The UDP port to listen to, or 0 for automatic assignment
 * @param      ...	  The username/password pairs (struct netmidi2_user)
 *
 * Example usage:
 * @code
 * NETMIDI2_EP_DEFINE_WITH_USERS(my_server, "endpoint", NULL, 0,
 *                                {.name="user1", .password="passwd1"},
 *                                {.name="user2", .password="passwd2"})
 * @endcode
 */
#define NETMIDI2_EP_DEFINE_WITH_USERS(_var_name, _ep_name, _piid, _port, ...) \
	static const struct netmidi2_userlist users_of_##_var_name = { \
		.n_users = ARRAY_SIZE(((struct netmidi2_user []) { __VA_ARGS__ })), \
		.users = { __VA_ARGS__ }, \
	}; \
	static struct netmidi2_ep _var_name = { \
		.name = (_ep_name), \
		.piid = (_piid), \
		.addr4.sin_port = (_port), \
		.auth_type = NETMIDI2_AUTH_USER_PASSWORD, \
		.userlist = &users_of_##_var_name, \
	}

struct netmidi2_ep;

/**
 * @brief      A username/password pair for user-based authentication
 */
struct netmidi2_user {
	/** The user name for authentication */
	const char *name;
	/** The password for authentication */
	const char *password;
};

/**
 * @brief      A list of users for user-based authentication
 */
struct netmidi2_userlist {
	/** Number of users in the list */
	size_t n_users;
	/** The user/password pairs */
	const struct netmidi2_user users[];
};

/**
 * @brief      A Network MIDI2 session, representing a connection to a peer
 */
struct netmidi2_session {
	/**
	 * State of this session
	 * @see netmidi10: 6.1 Session States
	 */
	enum {
		/** The session is not in use */
		NETMIDI2_SESSION_NOT_INITIALIZED = 0,
		/** Device may be aware of each other (e.g. through mDNS),
		 *  however neither device has sent an Invitation.
		 *  The two Devices are not in a Session.
		 */
		NETMIDI2_SESSION_IDLE,
		/** Client has sent Invitation, however the Host has not
		 *  accepted the Invitation. A Bye Command has not been
		 *  sent or received.
		 */
		NETMIDI2_SESSION_PENDING_INVITATION,
		/** Host has replied with Invitation Reply: Authentication
		 *  Required or Invitation Reply: User Authentication Required.
		 *  No timeout has yet occurred. This state is different from
		 * Idle, because the Client and Host need to store the Nonce.
		 */
		NETMIDI2_SESSION_AUTH_REQUIRED,
		/** Invitation accepted, UMP Commands can be exchanged. */
		NETMIDI2_SESSION_ESTABLISHED,
		/** One Device has sent the Session Reset Command and is
		 *  waiting for Session Reset Reply, and a timeout has not
		 *  yet occurred.
		 */
		NETMIDI2_SESSION_PENDING_RESET,
		/** one Endpoint has sent Bye and is waiting for Bye Reply,
		 *  and a timeout has not yet occurred.
		 */
		NETMIDI2_SESSION_PENDING_BYE,
	} state;
	/** Sequence number of the next universal MIDI packet to send */
	uint16_t tx_ump_seq;
	/** Sequence number of the next universal MIDI packet to receive */
	uint16_t rx_ump_seq;
	/** Remote address of the peer */
	struct sockaddr_storage addr;
	/** Length of the peer's remote address */
	socklen_t addr_len;
	/** The Network MIDI2 endpoint to which this session belongs */
	struct netmidi2_ep *ep;
#if defined(CONFIG_NETMIDI2_HOST_AUTH) || defined(__DOXYGEN__)
	/** The username to which this session belongs */
	const struct netmidi2_user *user;
	/** The crypto nonce used to authorize this session */
	char nonce[NETMIDI2_NONCE_SIZE];
#endif
	/** The transmission buffer for that peer */
	struct net_buf *tx_buf;
	/** The transmission work for that peer */
	struct k_work tx_work;
};

/**
 * @brief      Type of authentication in Network MIDI2
 */
enum netmidi2_auth_type {
	/** No authentication required */
	NETMIDI2_AUTH_NONE,
	/** Authentication with a shared secret key */
	NETMIDI2_AUTH_SHARED_SECRET,
	/** Authentication with username and password */
	NETMIDI2_AUTH_USER_PASSWORD,
};

/**
 * @brief      A Network MIDI2.0 Endpoint
 */
struct netmidi2_ep {
	/** The endpoint name */
	const char *name;
	/** The endpoint product instance id */
	const char *piid;
	/** The local endpoint address */
	union {
		struct sockaddr addr;
		struct sockaddr_in addr4;
		struct sockaddr_in6 addr6;
	};
	/** The listening socket wrapped in a poll descriptor */
	struct zsock_pollfd pollsock;
	/** The function to call when data is received from a client */
	void (*rx_packet_cb)(struct netmidi2_session *session,
			     const struct midi_ump ump);
	/** List of peers to this endpoint */
	struct netmidi2_session peers[CONFIG_NETMIDI2_HOST_MAX_CLIENTS];
	/** The type of authentication required to establish a session
	 *  with this host endpoint
	 */
	enum netmidi2_auth_type auth_type;
#if defined(CONFIG_NETMIDI2_HOST_AUTH) || defined(__DOXYGEN__)
	union {
		/** A shared authentication key */
		const char *shared_auth_secret;
		/** A list of users/passwords */
		const struct netmidi2_userlist *userlist;
	};
#endif
};

/**
 * @brief      Start hosting a network (UDP) Universal MIDI Packet endpoint
 * @param      ep    The network endpoint to start
 * @return     0 on success, -errno on error
 */
int netmidi2_host_ep_start(struct netmidi2_ep *ep);

/**
 * @brief      Send a Universal MIDI Packet to all clients connected to the endpoint
 * @param      ep    The endpoint
 * @param[in]  ump   The packet to send
 */
void netmidi2_broadcast(struct netmidi2_ep *ep, const struct midi_ump ump);

/**
 * @brief      Send a Universal MIDI Packet to a single client
 * @param      sess  The session identifying the single client
 * @param[in]  ump   The packet to send
 */
void netmidi2_send(struct netmidi2_session *sess, const struct midi_ump ump);

/** @} */

#endif
