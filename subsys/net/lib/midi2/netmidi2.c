/*
 * Copyright (c) 2025 Titouan Christophe
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/midi2.h>

#include <stdio.h>
#include <zephyr/net/socket_service.h>

#if defined(CONFIG_MIDI2_UMP_STREAM_RESPONDER)
#include <ump_stream_responder.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_midi2, CONFIG_NET_MIDI2_LOG_LEVEL);

#define NETMIDI2_BUFSIZE 256

NET_BUF_POOL_DEFINE(netmidi2_pool, 2 + CONFIG_NETMIDI2_HOST_MAX_CLIENTS,
		    NETMIDI2_BUFSIZE, 0, NULL);

/**
 * Size, in bytes, of the digest sent by the client to authenticate (sha256)
 */
#define NETMIDI2_DIGEST_SIZE 32

/* See netmidi10: 5.5: Command Codes and Packet Types */
#define COMMAND_INVITATION				0x01
#define COMMAND_INVITATION_WITH_AUTH			0x02
#define COMMAND_INVITATION_WITH_USER_AUTH		0x03
#define COMMAND_INVITATION_REPLY_ACCEPTED		0x10
#define COMMAND_INVITATION_REPLY_PENDING		0x11
#define COMMAND_INVITATION_REPLY_AUTH_REQUIRED		0x12
#define COMMAND_INVITATION_REPLY_USER_AUTH_REQUIRED	0x13
#define COMMAND_PING					0x20
#define COMMAND_PING_REPLY				0x21
#define COMMAND_RETRANSMIT_REQUEST			0x80
#define COMMAND_RETRANSMIT_ERROR			0x81
#define COMMAND_SESSION_RESET				0x82
#define COMMAND_SESSION_RESET_REPLY			0x83
#define COMMAND_NAK					0x8F
#define COMMAND_BYE					0xF0
#define COMMAND_BYE_REPLY				0xF1
#define COMMAND_UMP_DATA				0xFF

/* See netmidi10: 6.4 / Table 11: Capabilities for Invitation */
#define CLIENT_CAP_INV_WITH_AUTH	BIT(0)
#define CLIENT_CAP_INV_WITH_USER_AUTH	BIT(1)

/* See netmidi10: 6.7 / Table 15: Values for Authentication State */
#define AUTH_STATE_FIRST_REQUEST	0x00
#define AUTH_STATE_INCORRECT_DIGEST	0x01

/* See netmidi10: 6.15 / Table 25: List of NAK Reasons */
#define NAK_OTHER			0x00
#define NAK_COMMAND_NOT_SUPPORTED	0x01
#define NAK_COMMAND_NOT_EXPECTED	0x02
#define NAK_COMMAND_MALFORMED		0x03
#define NAK_BAD_PING_REPLY		0x20

/* Logging macros that include the peer's address:port */
#define SESS_LOG_DBG(_s, _fmt, ...) SESS_LOG(DBG, _s, _fmt, ##__VA_ARGS__)
#define SESS_LOG_INF(_s, _fmt, ...) SESS_LOG(INFO, _s, _fmt, ##__VA_ARGS__)
#define SESS_LOG_WRN(_s, _fmt, ...) SESS_LOG(WARN, _s, _fmt, ##__VA_ARGS__)
#define SESS_LOG_ERR(_s, _fmt, ...) SESS_LOG(ERR, _s, _fmt, ##__VA_ARGS__)

#define SESS_LOG(_lvl, _s, _fmt, ...) \
	do { \
		const struct sockaddr *addr = net_sad(&(_s)->addr); \
		const struct sockaddr_in6 *addr6 = net_sin6(addr); \
		char __pn[INET6_ADDRSTRLEN]; \
		net_addr_ntop(addr->sa_family, &addr6->sin6_addr, __pn, sizeof(__pn)); \
		NET_##_lvl("%s:%d " _fmt, __pn, addr6->sin6_port, ##__VA_ARGS__); \
	} while (0)

#define SESSION_HAS_STATE(session, expected_state) \
	((session) && (session)->state == expected_state)

#if defined(CONFIG_NETMIDI2_HOST_AUTH)
#include <zephyr/crypto/crypto.h>
#include <zephyr/random/random.h>

static inline const struct netmidi2_user *netmidi2_find_user(const struct netmidi2_ep *ep,
							     const char *name, size_t namelen)
{
	if (ep->auth_type != NETMIDI2_AUTH_USER_PASSWORD) {
		return NULL;
	}

	for (size_t i = 0; i < ep->userlist->n_users; i++) {
		if (strncmp(ep->userlist->users[i].name, name, namelen) == 0) {
			return &ep->userlist->users[i];
		}
	}

	return NULL;
}

static bool netmidi2_auth_session(const struct netmidi2_session *sess,
				  struct net_buf *buf, size_t payload_len)
{
	const struct device *hasher = device_get_binding(CONFIG_CRYPTO_MBEDTLS_SHIM_DRV_NAME);
	const uint8_t *auth_digest = buf->data;
	const struct netmidi2_user *user;
	uint8_t output[NETMIDI2_DIGEST_SIZE];
	struct hash_ctx ctx = {.flags = crypto_query_hwcaps(hasher)};
	struct hash_pkt hash = {.out_buf = output, .ctx = &ctx};
	int ret;

	if (hasher == NULL) {
		SESS_LOG_ERR(sess, "mbedtls crypto pseudo-device unavailable");
		return false;
	}

	if (buf->len < NETMIDI2_DIGEST_SIZE || payload_len < NETMIDI2_DIGEST_SIZE) {
		SESS_LOG_ERR(sess, "Incomplete authentication digest");
		return false;
	}

	/* Pull authentication digest from the command packet */
	net_buf_pull(buf, NETMIDI2_DIGEST_SIZE);

	ret = hash_begin_session(hasher, &ctx, CRYPTO_HASH_ALGO_SHA256);
	if (ret != 0) {
		SESS_LOG_ERR(sess, "Unable to begin hash session");
		return false;
	}

	/* 1. Start hashing with the session nonce */
	hash.in_buf = (uint8_t *) sess->nonce;
	hash.in_len = NETMIDI2_NONCE_SIZE;
	ret = hash_update(&ctx, &hash);
	if (ret != 0) {
		SESS_LOG_ERR(sess, "Unable to hash nonce");
		goto end;
	}

	if (sess->ep->auth_type == NETMIDI2_AUTH_SHARED_SECRET) {
		/* 2. Finalize hashing with the shared secret */
		hash.in_buf = (uint8_t *) sess->ep->shared_auth_secret;
		hash.in_len = strlen(sess->ep->shared_auth_secret);
		ret = hash_compute(&ctx, &hash);
		if (ret != 0) {
			SESS_LOG_ERR(sess, "Unable to hash shared secret");
			goto end;
		}
	} else if (sess->ep->auth_type == NETMIDI2_AUTH_USER_PASSWORD) {
		user = netmidi2_find_user(sess->ep, buf->data,
					  payload_len - NETMIDI2_DIGEST_SIZE);
		if (user == NULL) {
			LOG_ERR("No matching user found");
			goto end;
		}

		/* Remove username from buffer */
		net_buf_pull(buf, payload_len - NETMIDI2_DIGEST_SIZE);

		/* 2. Continue hashing with the username */
		hash.in_buf = (uint8_t *) user->name;
		hash.in_len = strlen(user->name);
		ret = hash_update(&ctx, &hash);
		if (ret != 0) {
			SESS_LOG_ERR(sess, "Unable to hash username");
			goto end;
		}

		/* 3. Finalize hashing with the password */
		hash.in_buf = (uint8_t *) user->password;
		hash.in_len = strlen(user->password);
		ret = hash_compute(&ctx, &hash);
		if (ret != 0) {
			SESS_LOG_ERR(sess, "Unable to hash password");
			goto end;
		}
	}

end:
	hash_free_session(hasher, &ctx);
	return ret == 0 && memcmp(hash.out_buf, auth_digest, NETMIDI2_DIGEST_SIZE) == 0;
}
#endif /* CONFIG_NETMIDI2_HOST_AUTH */

static inline void netmidi2_free_session(struct netmidi2_session *session)
{
	SESS_LOG_INF(session, "Free client session");

	k_work_cancel(&session->tx_work);
	if (session->tx_buf != NULL) {
		net_buf_unref(session->tx_buf);
	}
	memset(session, 0, sizeof(*session) - sizeof(struct k_work));
}

static inline struct netmidi2_session *netmidi2_match_session(struct netmidi2_ep *ep,
							      struct sockaddr *peer_addr,
							      socklen_t peer_addr_len)
{
	for (size_t i = 0; i < CONFIG_NETMIDI2_HOST_MAX_CLIENTS; i++) {
		if (ep->peers[i].addr_len == peer_addr_len &&
		    memcmp(&ep->peers[i].addr, peer_addr, peer_addr_len) == 0) {
			LOG_DBG("Found matching client session %d", i);
			return &ep->peers[i];
		}
	}

	return NULL;
}

static inline void netmidi2_free_inactive_sessions(struct netmidi2_ep *ep)
{
	struct netmidi2_session *sess;
	const uint8_t bye_timeout[] = {COMMAND_BYE, 0, 0x04, 0};

	for (size_t i = 0; i < CONFIG_NETMIDI2_HOST_MAX_CLIENTS; i++) {
		sess = &ep->peers[i];
		if (sess->state != NETMIDI2_SESSION_IDLE &&
		    sess->state != NETMIDI2_SESSION_ESTABLISHED) {
			SESS_LOG_WRN(sess, "Cleanup inactive session");
			zsock_sendto(ep->pollsock.fd, bye_timeout, sizeof(bye_timeout),
				     0, net_sad(&sess->addr), sess->addr_len);
			netmidi2_free_session(sess);
		}
	}
}

static inline struct netmidi2_session *netmidi2_try_alloc_session(struct netmidi2_ep *ep,
								  struct sockaddr *peer_addr,
								  socklen_t peer_addr_len)
{
	struct netmidi2_session *sess;

	for (size_t i = 0; i < CONFIG_NETMIDI2_HOST_MAX_CLIENTS; i++) {
		sess = &ep->peers[i];
		if (sess->state == NETMIDI2_SESSION_NOT_INITIALIZED) {
			sess->state = NETMIDI2_SESSION_IDLE;
			sess->addr_len = peer_addr_len;
			sess->ep = ep;
			memcpy(&sess->addr, peer_addr, peer_addr_len);
			SESS_LOG_INF(sess, "new client session (%d)", i);
			return sess;
		}
	}

	return NULL;
}

static inline struct netmidi2_session *netmidi2_alloc_session(struct netmidi2_ep *ep,
							      struct sockaddr *peer_addr,
							      socklen_t peer_addr_len)
{
	struct netmidi2_session *session;

	session = netmidi2_try_alloc_session(ep, peer_addr, peer_addr_len);
	if (session == NULL) {
		netmidi2_free_inactive_sessions(ep);
		session = netmidi2_try_alloc_session(ep, peer_addr, peer_addr_len);
	}

	if (session == NULL) {
		LOG_ERR("No available client session");
	}

	return session;
}

/**
 * @brief      Perform transmission work for an endpoint peer's session
 * @param      work  The work item (must be contained in a netmidi2_session)
 */
static void netmidi2_session_tx_work(struct k_work *work)
{
	struct netmidi2_session *session = CONTAINER_OF(work, struct netmidi2_session, tx_work);
	struct net_buf *buf = session->tx_buf;

	session->tx_buf = NULL;

	zsock_sendto(session->ep->pollsock.fd, buf->data, buf->len, 0,
		     net_sad(&session->addr), session->addr_len);
	net_buf_unref(buf);
}

static inline const char *netmidi2_ep_get_name(const struct netmidi2_ep *ep)
{
	return (ep->name == NULL) ? "" : ep->name;
}

#if defined(CONFIG_MIDI2_UMP_STREAM_RESPONDER)
#define DEFAULT_PIID ump_product_instance_id()
#else
#define DEFAULT_PIID ""
#endif

static inline const char *netmidi2_ep_get_piid(const struct netmidi2_ep *ep)
{
	return (ep->piid == NULL) ? DEFAULT_PIID : ep->piid;
}

/**
 * @brief      Write the header for a CommandPacket into a session tx buffer
 * @param      sess                   The peer's session
 * @param[in]  command_code           The command code
 * @param[in]  command_specific_data  The command specific data
 * @param[in]  payload_len_words      Payload length, in 32b words
 * @return     0 on success, -errno on error
 */
static inline int sess_buf_add_header(struct netmidi2_session *sess,
				      const uint8_t command_code,
				      const uint16_t command_specific_data,
				      const uint8_t payload_len_words)
{
	if (sess->tx_buf == NULL) {
		/* If no current tx buffer, allocate a new one */
		sess->tx_buf = net_buf_alloc(&netmidi2_pool, K_FOREVER);
		if (sess->tx_buf == NULL) {
			SESS_LOG_ERR(sess, "Unable to allocate Tx buffer");
			return -ENOBUFS;
		}
		/* Prefix with Network MIDI2.0 UDP header */
		net_buf_add_mem(sess->tx_buf, "MIDI", 4);
	}

	if (net_buf_tailroom(sess->tx_buf) < 4*(1 + payload_len_words)) {
		SESS_LOG_WRN(sess, "Not enough room in Tx buffer");
		return -ENOMEM;
	}

	net_buf_add_u8(sess->tx_buf, command_code);
	net_buf_add_u8(sess->tx_buf, payload_len_words);
	net_buf_add_be16(sess->tx_buf, command_specific_data);

	return 0;
}

/**
 * @brief      Write some bytes into a session tx buffer, and add padding
 *             zeros at the tail to stay aligned on 4 bytes
 * @param      session  The session to write to
 * @param[in]  mem      The memory to be copied
 * @param[in]  size     The size in bytes of the memory to be copied
 */
static inline void sess_buf_add_mem_padded(struct netmidi2_session *session,
					   const uint8_t *mem, size_t size)
{
	if (session->tx_buf == NULL) {
		return;
	}

	net_buf_add_mem(session->tx_buf, mem, size);

	switch (size % 4) {
	case 1:
		net_buf_add_be24(session->tx_buf, 0);
		break;
	case 2:
		net_buf_add_be16(session->tx_buf, 0);
		break;
	case 3:
		net_buf_add_u8(session->tx_buf, 0);
		break;
	}
}

/**
 * @brief      Send a Command Packet (from words) to a client session
 * @remark     The Command Packet is appended to the session's tx buffer,
 *             and transmission is scheduled, so the command packet is not
 *             transmitted immediately, and may be sent along with others in
 *             a single Network MIDI2.0 UDP packet.
 * @param      sess                   The recipient session
 * @param[in]  command_code           The command code
 * @param[in]  command_specific_data  The command specific data
 * @param[in]  payload                The command payload as words (4B)
 * @param[in]  payload_len_words      Payload length, in words (4B)
 * @return     0 on success, -errno otherwise
 *
 * @see netmidi10: 5.4 Command Packet Header and Payload
 */
static int netmidi2_session_sendcmd(struct netmidi2_session *sess,
				    const uint8_t command_code,
				    const uint16_t command_specific_data,
				    const uint32_t *payload,
				    const uint8_t payload_len_words)
{
	int ret = sess_buf_add_header(sess, command_code,
				      command_specific_data,
				      payload_len_words);
	if (ret != 0) {
		return ret;
	}

	for (size_t i = 0; i < payload_len_words; i++) {
		net_buf_add_be32(sess->tx_buf, payload[i]);
	}
	k_work_submit(&sess->tx_work);
	return 0;
}

/**
 * @brief      Immediately send a Command Packet to a remote without client session
 * @param[in]  ep                     The emitting UMP endpoint
 * @param[in]  peer_addr              The recipient's address
 * @param[in]  peer_addr_len          The recipient's address length
 * @param[in]  command_specific_data  The command specific data
 * @param[in]  payload                The command payload
 * @param[in]  payload_len_words      Payload length, in words (4B)
 */
static int netmidi2_quick_reply(struct netmidi2_ep *ep,
				const struct sockaddr *peer_addr,
				const socklen_t peer_addr_len,
				const uint8_t command_code,
				const uint16_t command_specific_data,
				const uint32_t *payload,
				const uint8_t payload_len_words)
{
	NET_BUF_SIMPLE_DEFINE(txbuf, 28);

	if (4 * (1 + payload_len_words) > txbuf.size) {
		return -ENOBUFS;
	}

	/* Network MIDI2.0 UDP header */
	net_buf_simple_add_mem(&txbuf, "MIDI", 4);
	/* Command packet header */
	net_buf_simple_add_u8(&txbuf, command_code);
	net_buf_simple_add_u8(&txbuf, payload_len_words);
	net_buf_simple_add_be16(&txbuf, command_specific_data);

	/* Payload */
	for (size_t i = 0; i < payload_len_words; i++) {
		net_buf_simple_add_be32(&txbuf, payload[i]);
	}

	zsock_sendto(ep->pollsock.fd, txbuf.data, txbuf.len, 0,
		     peer_addr, peer_addr_len);
	return 0;
}

/**
 * @brief      Quickly send a NAK message to a remote without client session
 * @param[in]  ep               The endpoint sending the NAK
 * @param[in]  peer_addr        The peer address
 * @param[in]  peer_addr_len    The peer address length
 * @param[in]  nak_reason       The NAK reason
 * @param[in]  nakd_cmd_header  The command packet header this NAK is replying to
 */
static inline int netmidi2_quick_nak(struct netmidi2_ep *ep,
				     const struct sockaddr *peer_addr,
				     const socklen_t peer_addr_len,
				     const uint8_t nak_reason,
				     const uint32_t nakd_cmd_header)
{
	return netmidi2_quick_reply(ep, peer_addr, peer_addr_len,
				    COMMAND_NAK, nak_reason << 8,
				    &nakd_cmd_header, 1);
}

/**
 * @brief      Send a message "Invitation reply: ..." to a client.
 *             The type of Invitation Reply command code depends on the
 *             session state
 * @param      session  The session to which the message shall be send
 * @return     0 on success, -errno on error
 *
 * @see netmidi10: 6.5 Invitation Reply: Accepted
 */
static int netmidi2_send_invitation_reply(struct netmidi2_session *session,
					  uint8_t authentication_state)
{
	int ret;
	uint8_t command_code;

	const char *name = netmidi2_ep_get_name(session->ep);
	const char *piid = netmidi2_ep_get_piid(session->ep);
	const size_t namelen = strlen(name);
	const size_t piidlen = strlen(piid);
	const size_t namelen_words = DIV_ROUND_UP(namelen, 4);
	const size_t piidlen_words = DIV_ROUND_UP(piidlen, 4);
	const uint16_t specific_data = (namelen_words << 8) | authentication_state;

	size_t total_words = namelen_words + piidlen_words;

	if (session->state == NETMIDI2_SESSION_ESTABLISHED) {
		command_code = COMMAND_INVITATION_REPLY_ACCEPTED;
	}

#if defined(CONFIG_NETMIDI2_HOST_AUTH)
	else if (session->state == NETMIDI2_SESSION_AUTH_REQUIRED) {
		total_words += DIV_ROUND_UP(NETMIDI2_NONCE_SIZE, 4);

		if (session->ep->auth_type == NETMIDI2_AUTH_SHARED_SECRET) {
			command_code = COMMAND_INVITATION_REPLY_AUTH_REQUIRED;
		} else if (session->ep->auth_type == NETMIDI2_AUTH_USER_PASSWORD) {
			command_code = COMMAND_INVITATION_REPLY_USER_AUTH_REQUIRED;
		} else {
			return -EINVAL;
		}
	}
#endif /* CONFIG_NETMIDI2_HOST_AUTH */

	else {
		return -EINVAL;
	}

	ret = sess_buf_add_header(session, command_code, specific_data, total_words);
	if (ret != 0) {
		return ret;
	}

#if defined(CONFIG_NETMIDI2_HOST_AUTH)
	if (session->state == NETMIDI2_SESSION_AUTH_REQUIRED) {
		sys_rand_get(session->nonce, NETMIDI2_NONCE_SIZE);
		sess_buf_add_mem_padded(session, session->nonce, NETMIDI2_NONCE_SIZE);
	}
#endif /* CONFIG_NETMIDI2_HOST_AUTH */

	sess_buf_add_mem_padded(session, name, namelen);
	sess_buf_add_mem_padded(session, piid, piidlen);
	k_work_submit(&session->tx_work);
	return 0;
}

/**
 * @brief      Consume the leading Command Packet from a buffer that contains
 *             a Network MIDI 2.0 UDP packet
 * @param      ep             The endpoint that receives the packet
 * @param      peer_addr      The network address of the sender
 * @param[in]  peer_addr_len  The length of the sender's network address
 * @param      rx             The received buffer
 * @return     non-zero if an error occurred, and the rx buffer is left in an
 *             unspecified state. 0 on success, in which case the head of the
 *             rx buffer is at the next Command Packet (or empty)
 */
static int netmidi2_dispatch_cmdpkt(struct netmidi2_ep *ep,
				    struct sockaddr *peer_addr,
				    socklen_t peer_addr_len,
				    struct net_buf *rx)
{
	struct midi_ump ump;
	size_t payload_len;
	uint32_t cmd_header;
	uint8_t cmd_code, payload_len_words;
	uint16_t cmd_data;
	struct netmidi2_session *session;

	if (rx->len < 4) {
		LOG_ERR("Incomplete UDP MIDI command packet header");
		return -1;
	}

	cmd_header = net_buf_pull_be32(rx);
	cmd_code = cmd_header >> 24;
	payload_len_words = (cmd_header >> 16) & 0xff;
	payload_len = 4 * payload_len_words;
	cmd_data = cmd_header & 0xffff;

	if (payload_len > rx->len) {
		netmidi2_quick_nak(ep, peer_addr, peer_addr_len,
				   NAK_COMMAND_MALFORMED, cmd_header);
		LOG_ERR("Incomplete UDP MIDI command packet payload");
		return -1;
	}

	switch (cmd_code) {
	case COMMAND_PING:
		/* See netmidi10: 6.13 Ping */
		if (payload_len_words != 1) {
			netmidi2_quick_nak(ep, peer_addr, peer_addr_len,
					   NAK_COMMAND_MALFORMED, cmd_header);
			LOG_ERR("Invalid payload length for PING packet");
			return -1;
		}
		/* Simple reply with 1 word from the PING request */
		netmidi2_quick_reply(ep, peer_addr, peer_addr_len,
				     COMMAND_PING_REPLY, 0,
				     (uint32_t [1]) {net_buf_pull_be32(rx)}, 1);
		return 0;

	case COMMAND_INVITATION:
		/* See netmidi10: 6.13 Ping
		 * We currently don't care about the peer's name or product
		 * instance id. Simply pull the entire payload at once.
		 */
		net_buf_pull(rx, payload_len);

		session = netmidi2_alloc_session(ep, peer_addr, peer_addr_len);
		if (session == NULL) {
			return -1;
		}

		if (ep->auth_type == NETMIDI2_AUTH_NONE) {
			session->state = NETMIDI2_SESSION_ESTABLISHED;
			netmidi2_send_invitation_reply(session, AUTH_STATE_FIRST_REQUEST);
		}

#if !CONFIG_NETMIDI2_HOST_AUTH
		return 0;
#else
		/* See netmidi10: 6.7 Invitation Reply: Authentication Required */
		else {
			session->state = NETMIDI2_SESSION_AUTH_REQUIRED;
			netmidi2_send_invitation_reply(session, AUTH_STATE_FIRST_REQUEST);
		}

		return 0;

	case COMMAND_INVITATION_WITH_AUTH:
	case COMMAND_INVITATION_WITH_USER_AUTH:
		/* See netmidi10: 6.9 - 6.10 Invitation with (User) Authentication */
		session = netmidi2_match_session(ep, peer_addr, peer_addr_len);
		if (!SESSION_HAS_STATE(session, NETMIDI2_SESSION_AUTH_REQUIRED)) {
			netmidi2_quick_nak(ep, peer_addr, peer_addr_len,
					   NAK_COMMAND_NOT_EXPECTED, cmd_header);
			LOG_WRN("No session to authenticate found");
			return -1;
		}

		if (!netmidi2_auth_session(session, rx, payload_len)) {
			SESS_LOG_WRN(session, "Invalid auth digest");
			netmidi2_send_invitation_reply(session, AUTH_STATE_INCORRECT_DIGEST);
			return -1;
		}

		session->state = NETMIDI2_SESSION_ESTABLISHED;
		netmidi2_send_invitation_reply(session, AUTH_STATE_FIRST_REQUEST);
		return 0;
#endif /* CONFIG_NETMIDI2_HOST_AUTH */

	case COMMAND_BYE:
		session = netmidi2_match_session(ep, peer_addr, peer_addr_len);
		if (session == NULL) {
			netmidi2_quick_nak(ep, peer_addr, peer_addr_len,
					   NAK_COMMAND_NOT_EXPECTED, cmd_header);
			LOG_WRN("Receiving BYE without session");
			return -1;
		}
		net_buf_pull(rx, payload_len);
		netmidi2_quick_reply(ep, peer_addr, peer_addr_len,
				     COMMAND_BYE_REPLY, 0, NULL, 0);
		netmidi2_free_session(session);
		return 0;

	case COMMAND_UMP_DATA:
		session = netmidi2_match_session(ep, peer_addr, peer_addr_len);
		if (!SESSION_HAS_STATE(session, NETMIDI2_SESSION_ESTABLISHED)) {
			netmidi2_quick_nak(ep, peer_addr, peer_addr_len,
					   NAK_COMMAND_NOT_EXPECTED, cmd_header);
			LOG_WRN("Receiving UMP data without established session");
			return -1;
		}

		if (session->rx_ump_seq == cmd_data) {
			session->rx_ump_seq++;
		} else {
			SESS_LOG_WRN(session, "UMP Rx sequence mismatch (got %d, expected %d)",
				     cmd_data, session->rx_ump_seq);
			session->rx_ump_seq = 1 + cmd_data;
		}

		if (payload_len_words < 1 || payload_len_words > 4) {
			netmidi2_quick_nak(ep, peer_addr, peer_addr_len,
					   NAK_COMMAND_MALFORMED, cmd_header);
			SESS_LOG_ERR(session, "Invalid UMP length");
			return -1;
		}

		for (size_t i = 0; i < payload_len_words; i++) {
			ump.data[i] = net_buf_pull_be32(rx);
		}

		if (UMP_NUM_WORDS(ump) != payload_len_words) {
			netmidi2_quick_nak(ep, peer_addr, peer_addr_len,
					   NAK_COMMAND_MALFORMED, cmd_header);
			SESS_LOG_ERR(session, "Invalid UMP payload size for its message type");
			return -1;
		}

		if (ep->rx_packet_cb != NULL) {
			ep->rx_packet_cb(session, ump);
		}
		return 0;

	case COMMAND_SESSION_RESET:
		session = netmidi2_match_session(ep, peer_addr, peer_addr_len);
		if (!SESSION_HAS_STATE(session, NETMIDI2_SESSION_ESTABLISHED)) {
			LOG_WRN("Receiving session reset without established session");
			netmidi2_quick_nak(ep, peer_addr, peer_addr_len,
					   NAK_COMMAND_NOT_EXPECTED, cmd_header);
			return -1;
		}

		session->tx_ump_seq = 0;
		session->rx_ump_seq = 0;
		SESS_LOG_INF(session, "Reset session");
		netmidi2_session_sendcmd(session, COMMAND_SESSION_RESET_REPLY, 0, NULL, 0);
		return 0;

	default:
		LOG_WRN("Unknown command code %02X", cmd_code);
		net_buf_pull(rx, payload_len);
		netmidi2_quick_nak(ep, peer_addr, peer_addr_len,
				   NAK_COMMAND_NOT_SUPPORTED, cmd_header);
		return 0;
	}
	return 0;
}

/**
 * @brief      Service handler: receive a Network MIDI 2.0 UDP packet,
 *             and dispatch all the command packets that it contains
 * @param      pev   the service event that triggers this handler
 */
static void netmidi2_service_handler(struct net_socket_service_event *pev)
{
	int ret;
	struct netmidi2_ep *ep = pev->user_data;
	struct pollfd *pfd = &pev->event;
	struct sockaddr peer_addr;
	socklen_t peer_addr_len = sizeof(peer_addr);
	struct net_buf *rxbuf;

	rxbuf = net_buf_alloc(&netmidi2_pool, K_FOREVER);
	if (rxbuf == NULL) {
		NET_ERR("Cannot allocate Rx buf");
		return;
	}

	ret = zsock_recvfrom(pfd->fd, rxbuf->data, rxbuf->size, 0,
			     &peer_addr, &peer_addr_len);
	if (ret < 0) {
		LOG_ERR("Rx error: %d (%d)", ret, errno);
		goto end;
	}
	rxbuf->len = ret;

	NET_HEXDUMP_DBG(rxbuf->data, rxbuf->len, "Received UDP packet");

	/* Check for magic header */
	if (rxbuf->len < 4 || memcmp(rxbuf->data, "MIDI", 4) != 0) {
		LOG_WRN("Not a MIDI packet");
		goto end;
	}

	net_buf_pull(rxbuf, 4);

	/* Parse contained command packets */
	ret = 0;
	while (ret == 0 && rxbuf->len >= 4) {
		ret = netmidi2_dispatch_cmdpkt(ep, &peer_addr, peer_addr_len, rxbuf);
	}

end:
	net_buf_unref(rxbuf);
}

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(netmidi2_service, netmidi2_service_handler, 1);

int netmidi2_host_ep_start(struct netmidi2_ep *ep)
{
	socklen_t addr_len = 0;
	int ret, sock, af;

#if defined(CONFIG_NET_IPV6)
	af = AF_INET6;
	addr_len = sizeof(struct sockaddr_in6);
#else
	af = AF_INET;
	addr_len = sizeof(struct sockaddr_in);
#endif

	ep->addr.sa_family = af;
	sock = zsock_socket(af, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Unable to create socket: %d", errno);
		return -ENOMEM;
	}

#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_IPV4)
	socklen_t optlen = sizeof(int);
	int opt = 0;

	/* Enable sharing of IPv4 and IPv6 on same socket */
	ret = zsock_setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, optlen);
	if (ret < 0) {
		LOG_WRN("Cannot turn off IPV6_V6ONLY option");
	}
#endif

	ret = zsock_bind(sock, &ep->addr, addr_len);
	if (ret < 0) {
		zsock_close(sock);
		LOG_ERR("Failed to bind UDP socket: %d", errno);
		return -EIO;
	}

	for (size_t i = 0; i < CONFIG_NETMIDI2_HOST_MAX_CLIENTS; i++) {
		k_work_init(&ep->peers[i].tx_work, netmidi2_session_tx_work);
	}

	ep->pollsock.fd = sock;
	ep->pollsock.events = POLLIN;
	ret = net_socket_service_register(&netmidi2_service, &ep->pollsock, 1, ep);
	if (ret < 0) {
		zsock_close(sock);
		LOG_ERR("Failed to register service: %d", ret);
		return -EIO;
	}

	LOG_INF("Started UDP-MIDI2 server (%d)", ntohs(ep->addr4.sin_port));
	return 0;
}

void netmidi2_broadcast(struct netmidi2_ep *ep, const struct midi_ump ump)
{
	for (size_t i = 0; i < CONFIG_NETMIDI2_HOST_MAX_CLIENTS; i++) {
		if (ep->peers[i].state == NETMIDI2_SESSION_ESTABLISHED) {
			netmidi2_send(&ep->peers[i], ump);
		}
	}
}

void netmidi2_send(struct netmidi2_session *sess, const struct midi_ump ump)
{
	netmidi2_session_sendcmd(sess, COMMAND_UMP_DATA, sess->tx_ump_seq++,
				 ump.data, UMP_NUM_WORDS(ump));
}
