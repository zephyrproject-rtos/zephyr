/*
 *  SPDX-FileCopyrightText: 2026 Sayed Naser Moravej
 *
 *  SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(srtp, CONFIG_SRTP_LOG_LEVEL);

#include <zephyr/net/srtp.h>
#include <zephyr/net/net_log.h>
#include <zephyr/random/random.h>
#include <zephyr/net/igmp.h>


#include <net_private.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/net/rtp.h>
#include <srtp.h>


int rtp_transport_socket_start_tx(struct rtp_session *session);
int rtp_transport_socket_send(struct rtp_session *session, struct rtp_packet *rtp_pkt,
			      uint8_t padding);

/*global from the util.c file begin*/
srtp_err_status_t create_policy_from_params(srtp_policy_t *policy,
                                            const policy_params_t *params)
{
    if (policy == NULL || params == NULL) {
        return srtp_err_status_bad_param;
    }

    srtp_policy_t p;
    srtp_err_status_t status = srtp_policy_create(&p);
    if (status != srtp_err_status_ok) {
        return status;
    }

    srtp_profile_t profile = srtp_profile_reserved;
    if (params->gcm_on) {
        switch (params->key_size) {
        case 128:
            profile = srtp_profile_aead_aes_128_gcm;
            break;
        case 256:
            profile = srtp_profile_aead_aes_256_gcm;
            break;
        default:
            srtp_policy_destroy(p);
            return srtp_err_status_bad_param;
        }
    } else {
        switch (params->key_size) {
        case 128:
            if (params->tag_size == 4) {
                profile = srtp_profile_aes128_cm_sha1_32;
            } else {
                profile = srtp_profile_aes128_cm_sha1_80;
            }
            break;
        case 192:
            if (params->tag_size == 4) {
                profile = srtp_profile_aes192_cm_sha1_32;
            } else {
                profile = srtp_profile_aes192_cm_sha1_80;
            }
            break;
        case 256:
            if (params->tag_size == 4) {
                profile = srtp_profile_aes256_cm_sha1_32;
            } else {
                profile = srtp_profile_aes256_cm_sha1_80;
            }
            break;
        default:
            srtp_policy_destroy(p);
            return srtp_err_status_bad_param;
        }
    }

    status = srtp_policy_set_profile(p, profile);
    if (status != srtp_err_status_ok) {
        srtp_policy_destroy(p);
        return status;
    }

    status = srtp_policy_set_sec_serv(p, params->sec_servs, params->sec_servs);
    if (status != srtp_err_status_ok) {
        srtp_policy_destroy(p);
        return status;
    }

    *policy = p;
    return srtp_err_status_ok;
}


#define API_LOCK_TIMEOUT K_MSEC(100)

#include <zephyr/net/socket_service.h>

#define CTX_LOCK_TIMEOUT   K_MSEC(1)
#define RTP_SOCKET_MAX_FDS CONFIG_RTP_TRANSPORT_SOCKET_MAX_SESSIONS
struct srtp_socket_context {
	struct zsock_pollfd fds[RTP_SOCKET_MAX_FDS];
	struct srtp_session *sessions[RTP_SOCKET_MAX_FDS];
	struct k_mutex lock;
};

static struct srtp_socket_context socket_ctx;
static void srtp_socket_svc_handler(struct net_socket_service_event *pev);
NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(srtp_socket_svc, srtp_socket_svc_handler, RTP_SOCKET_MAX_FDS);

static int srtp_socket_init(void)
{
	k_mutex_init(&socket_ctx.lock);

	for (size_t i = 0; i < ARRAY_SIZE(socket_ctx.fds); i++) {
		socket_ctx.fds[i].fd = -1;
	}

	return 0;
}
/// @brief creates and initializes an SRTP session with the specified parameters. need a srtp session instead of the rtp. finished.
/// @param session
/// @param iface
/// @param sock_addr
/// @param
/// @param payload_type
/// @param callback
/// @param user_data
/// @return
int srtp_session_init(struct srtp_session *session, struct net_if *iface,
		     struct net_sockaddr *sock_addr, enum rtp_role role, uint8_t payload_type,
		     rtp_rx_cb_t callback, void *user_data)
{
	int ret;
	srtp_err_status_t status;
	size_t key_size = 128;
	size_t tag_size = 8;
	session->srtp_ctx = NULL;

	LOG_INF("Using %s [0x%x]\n", srtp_get_version_string(), srtp_get_version());

	/* initialize srtp library */
	status = srtp_init();
	if (status) {
		LOG_ERR("srtp initialization failed with error code %d.", status);
		return -ECANCELED;
	}

		/* set up the srtp policy and master key
	 * create policy structure, using the default mechanisms.
	 */
	policy_params_t params;

	params.sec_servs = sec_serv_conf_and_auth;
	params.gcm_on = false;
	params.key_size = key_size;
	params.tag_size = tag_size;

	ret = create_policy_from_params(&session->policy, &params);
	if (ret != srtp_err_status_ok) {
		LOG_ERR("failed to create policy from parameters");
		return ret;
	}
	/*****************************should be checked begin */
	session->rtp_se->ssrc = 0xdeadbeefu;
	/*****************************should be checked end */

	ret = srtp_policy_set_ssrc(session->policy, (srtp_ssrc_t){ ssrc_specific, session->rtp_se->ssrc});
	if (ret != srtp_err_status_ok) {
		LOG_ERR("failed to set SSRC in policy");
		return ret;
	}

	srtp_profile_t profile;

	ret = srtp_policy_get_profile(session->policy, &profile);
	if (ret != srtp_err_status_ok) {
		LOG_ERR("failed to get profile from policy");
		return ret;
	}

	session->key_len = srtp_profile_get_master_key_length(profile);
	session->salt_len = srtp_profile_get_master_salt_length(profile);
	size_t input_key_len = session->key_len + session->salt_len;
	size_t net_key_len = hex2bin(CONFIG_NET_SRTP_KEY,
		strlen(CONFIG_NET_SRTP_KEY), session->key, sizeof(session->key));

	/* check that hex string is the right length */
	if (net_key_len != input_key_len) {
		LOG_ERR("wrong number of digits in key (should be %d digits, found %d).",
			2 * input_key_len, 2 * net_key_len);
		return -EBADMSG;
	}

	ret = rtp_session_init(session->rtp_se, iface,
		     sock_addr, role, payload_type,
		     callback, user_data, RTP_TRANSPORT_SOCKET);

//////////////////////beging ssrc//////////////////
	// session->ssrc = sys_rand32_get();
//////////////////////end ssrc//////////////////
	return ret;
}


/// @brief needs srtp session instead of the rtp. finished.
/// @param session
/// @return
static int srtp_transport_socket_start_rx(struct srtp_session *session)
{
	struct net_sockaddr_storage *sock_addr = &session->rtp_se->rtp_context.sock_addr;
	int slot = -1;
	int fd;
	int ret;

	ret = k_mutex_lock(&socket_ctx.lock, K_FOREVER);
	if (ret < 0) {
		NET_DBG("Failed to lock context (%d)", ret);
		return ret;
	}

	if (session->rtp_se->transport.socket_rx_fd != -1) {
		ret = -EAGAIN;
		goto unlock;
	}

	for (size_t i = 0; i < ARRAY_SIZE(socket_ctx.fds); i++) {
		if (socket_ctx.fds[i].fd == -1) {
			slot = i;
			break;
		}
	}

	if (slot == -1) {
		NET_DBG("No free socket service slot, consider increasing "
			"CONFIG_RTP_TRANSPORT_SOCKET_MAX_SESSIONS");
		ret = -ENOMEM;
		goto unlock;
	}

	switch (sock_addr->ss_family) {
#ifdef CONFIG_NET_IPV4
	case NET_AF_INET: {
		struct net_sockaddr_in *addr_in = net_sin(net_sad(sock_addr));
		int on = 1;

		fd = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
		if (fd < 0) {
			NET_DBG("Failed to create RX socket (%d)", errno);
			ret = -errno;
			goto unlock;
		}

		ret = zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_REUSEADDR, &on, sizeof(on));
		if (ret < 0) {
			NET_DBG("Failed to set SO_REUSEADDR (%d)", errno);
			goto close;
		}

		if (net_ipv4_is_addr_mcast(&addr_in->sin_addr)) {
			struct net_ip_mreqn mreqn = {};

			memcpy(&mreqn.imr_multiaddr, &addr_in->sin_addr,
			       sizeof(mreqn.imr_multiaddr));
			mreqn.imr_ifindex = net_if_get_by_iface(session->rtp_se->rtp_context.iface);

			ret = zsock_setsockopt(fd, NET_IPPROTO_IP, ZSOCK_IP_ADD_MEMBERSHIP, &mreqn,
					       sizeof(mreqn));
			if (ret < 0 && errno != EALREADY) {
				NET_DBG("Failed to join multicast group (%d)", errno);
				goto close;
			}
		}

		ret = zsock_bind(fd, net_sad(sock_addr), net_family2size(sock_addr->ss_family));
		if (ret < 0) {
			NET_DBG("Failed to bind RX socket (%d)", errno);
			goto close;
		}

		break;
	}
#endif /* CONFIG_NET_IPV4 */
#ifdef CONFIG_NET_IPV6
	case NET_AF_INET6: {
		struct net_sockaddr_in6 *addr_in6 = net_sin6(net_sad(sock_addr));
		int on = 1;

		fd = zsock_socket(NET_AF_INET6, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
		if (fd < 0) {
			NET_DBG("Failed to create RX socket (%d)", errno);
			ret = -errno;
			goto unlock;
		}

		ret = zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_REUSEADDR, &on, sizeof(on));
		if (ret < 0) {
			NET_DBG("Failed to set SO_REUSEADDR (%d)", errno);
			goto close;
		}

		if (net_ipv6_is_addr_mcast(&addr_in6->sin6_addr)) {
			struct net_ipv6_mreq mreq = {
				.ipv6mr_multiaddr = addr_in6->sin6_addr,
				.ipv6mr_ifindex = net_if_get_by_iface(session->rtp_se->rtp_context.iface),
			};

			ret = zsock_setsockopt(fd, NET_IPPROTO_IPV6, ZSOCK_IPV6_ADD_MEMBERSHIP,
					       &mreq, sizeof(mreq));
			if (ret < 0 && errno != EALREADY) {
				NET_DBG("Failed to join multicast group (%d)", errno);
				goto close;
			}
		}

		ret = zsock_bind(fd, net_sad(sock_addr), net_family2size(sock_addr->ss_family));
		if (ret < 0) {
			NET_DBG("Failed to bind RX socket (%d)", errno);
			goto close;
		}

		break;
	}
#endif /* CONFIG_NET_IPV6 */
	default:
		NET_DBG("Family %s not supported", net_family2str(sock_addr->ss_family));
		ret = -ENOTSUP;
		goto unlock;
	}

#ifdef CONFIG_NET_PKT_TIMESTAMP
	uint8_t ts_flags = ZSOCK_SOF_TIMESTAMPING_RX_HARDWARE;

	ret = zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_TIMESTAMPING, &ts_flags,
			       sizeof(ts_flags));
	if (ret < 0) {
		NET_DBG("Failed to set timestamping option (%d), continuing without", errno);
	}
#endif

	socket_ctx.fds[slot].fd = fd;
	socket_ctx.fds[slot].events = ZSOCK_POLLIN;
	socket_ctx.sessions[slot] = session;
	session->rtp_se->transport.socket_rx_fd = fd;

	ret = net_socket_service_register(&srtp_socket_svc, socket_ctx.fds,
					  ARRAY_SIZE(socket_ctx.fds), &socket_ctx);
	if (ret < 0) {
		NET_DBG("Failed to register socket service (%d)", ret);
		socket_ctx.fds[slot].fd = -1;
		socket_ctx.sessions[slot] = NULL;
		session->rtp_se->transport.socket_rx_fd = -1;
		(void)zsock_close(fd);
		goto unlock;
	}

	goto unlock;
close:
	(void)zsock_close(fd);
	ret = -errno;
unlock:
	(void)k_mutex_unlock(&socket_ctx.lock);
	return ret;
}

/// @brief needs srtp session instead of the rtp. finished.
/// @param session
/// @return
int srtp_session_start_rx(struct srtp_session *session)
{
		int ret;

	__ASSERT_NO_MSG(session != NULL);

	ret = srtp_transport_socket_start_rx(session);
	if (ret < 0) {
		return ret;
	}

#if CONFIG_RTP_LOG_LEVEL >= LOG_LEVEL_DBG
	struct net_sockaddr_storage *sock_addr = &session->rtp_se->rtp_context.sock_addr;

	switch (sock_addr->ss_family) {
#ifdef CONFIG_NET_IPV4
	case NET_AF_INET: {
		struct net_sockaddr_in *sock_addr_in = net_sin(net_sad(sock_addr));

		NET_DBG("%s started receiver on %s:%d", session->rtp_se->name,
			net_sprint_ipv4_addr((void *)&sock_addr_in->sin_addr),
			net_ntohs(sock_addr_in->sin_port));
		break;
	}
#endif /* CONFIG_NET_IPV4 */
#ifdef CONFIG_NET_IPV6
	case NET_AF_INET6: {
		struct net_sockaddr_in6 *sock_addr_in6 = net_sin6(net_sad(sock_addr));

		NET_DBG("%s started receiver on [%s]:%d", session->rtp_se->name,
			net_sprint_ipv6_addr((void *)&sock_addr_in6->sin6_addr),
			net_ntohs(sock_addr_in6->sin6_port));
		break;
	}
#endif /* CONFIG_NET_IPV6 */
	default:
		break; /* Ignore */
	}
#endif /* CONFIG_RTP_LOG_LEVEL >= LOG_LEVEL_DBG */

	return 0;
}

static int rtp_session_start_tx(struct rtp_session *session)
{
	struct net_sockaddr_storage *sock_addr;
	int ret;

	__ASSERT_NO_MSG(session != NULL);

	sock_addr = &session->rtp_context.sock_addr;

	/* Create random timestamp and sequence number */
	session->timestamp = sys_rand32_get();
	session->sequence_number = sys_rand16_get();

	switch (sock_addr->ss_family) {
#ifdef CONFIG_NET_IPV4
	case NET_AF_INET: {
		struct net_sockaddr_in *sock_addr_in = net_sin(net_sad(sock_addr));

		if (net_ipv4_is_addr_unspecified(&sock_addr_in->sin_addr)) {
			NET_DBG("Invalid ipv4 address");
			return -EINVAL;
		}
		break;
	}
#endif /* CONFIG_NET_IPV4 */
#ifdef CONFIG_NET_IPV6
	case NET_AF_INET6: {
		struct net_sockaddr_in6 *sock_addr_in6 = net_sin6(net_sad(sock_addr));

		if (net_ipv6_is_addr_unspecified(&sock_addr_in6->sin6_addr)) {
			NET_DBG("Invalid ipv6 address");
			return -EINVAL;
		}
		break;
	}
#endif /* CONFIG_NET_IPV6 */
	default:
		NET_DBG("Family %s not supported", net_family2str(sock_addr->ss_family));
		return -ENOTSUP;
	}

	ret = rtp_transport_socket_start_tx(session);
	if (ret < 0) {
		return ret;
	}

#if CONFIG_RTP_LOG_LEVEL >= LOG_LEVEL_DBG
	switch (sock_addr->ss_family) {
#ifdef CONFIG_NET_IPV4
	case NET_AF_INET: {
		struct net_sockaddr_in *sock_addr_in = net_sin(net_sad(sock_addr));

		NET_DBG("%s started transmitter to %s:%d", session->name,
			net_sprint_ipv4_addr((void *)&sock_addr_in->sin_addr),
			net_ntohs(sock_addr_in->sin_port));
		break;
	}
#endif /* CONFIG_NET_IPV4 */
#ifdef CONFIG_NET_IPV6
	case NET_AF_INET6: {
		struct net_sockaddr_in6 *sock_addr_in6 = net_sin6(net_sad(sock_addr));

		NET_DBG("%s started transmitter to [%s]:%d", session->name,
			net_sprint_ipv6_addr((void *)&sock_addr_in6->sin6_addr),
			net_ntohs(sock_addr_in6->sin6_port));
		break;
	}
#endif /* CONFIG_NET_IPV6 */
	default:
		break; /* Ignore */
	}
#endif /* CONFIG_RTP_LOG_LEVEL >= LOG_LEVEL_DBG */

	return 0;
}

/// @brief needs srtp session instead of the rtp. finished.
/// @param session
/// @return
static int modified_rtp_session_start(struct srtp_session *session)
{
	enum rtp_role role;
	int ret;

	if (session == NULL) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&session->rtp_se->lock, API_LOCK_TIMEOUT);
	if (ret < 0) {
		NET_DBG("Failed to take session lock (%d)", ret);
		return ret;
	}

	role = session->rtp_se->rtp_context.role;

	if (role == RTP_ROLE_SINK || role == RTP_ROLE_BOTH) {
		ret = srtp_session_start_rx(session);
		if (ret < 0) {
			goto unlock;
		}
	}

	if (role == RTP_ROLE_SOURCE || role == RTP_ROLE_BOTH) {
		ret = rtp_session_start_tx(session->rtp_se);
		if (ret < 0) {
			if (role == RTP_ROLE_BOTH) {
				(void)rtp_session_stop(session->rtp_se);
			}
		}
	}

unlock:
	(void)k_mutex_unlock(&session->rtp_se->lock);
	return ret;
}


/// @brief needs srtp session instead of the rtp. finished.
/// @param session
/// @return
int srtp_session_start(struct srtp_session *session)
{
	int ret;
	srtp_err_status_t status;

	if (session == NULL) {
		return -EINVAL;
	}

	ret = srtp_policy_add_key(session->policy, session->key, session->key_len, session->key + session->key_len, session->salt_len,
					NULL, 0);
	if (ret != srtp_err_status_ok) {
		LOG_ERR("error: failed to set key in policy");
		return ret;
	}

	status = srtp_create(&session->srtp_ctx, session->policy);
	if (status) {
		LOG_ERR("srtp_create() failed with code %d.", status);
		return -EINVAL;
	}

	return modified_rtp_session_start(session);
}



/// @brief stops an SRTP session and deallocates its resources. needs srtp session instead of the rtp. finished.
/// @param session the SRTP session to stop
/// @return 0 on success, negative error code on failure
int srtp_session_stop(struct srtp_session *session)
{
	srtp_err_status_t status;
	int ret;

	if (session == NULL) {
		return -EINVAL;
	}

	rtp_session_stop(session->rtp_se);

	ret = srtp_dealloc(session->srtp_ctx);
	if (ret) {
		LOG_ERR("Failed to dealloc SRTP");
		return ret;
	}

	status = srtp_shutdown();
	if (status) {
		LOG_ERR("srtp shutdown failed with error code %d.", status);
		return -ECANCELED;
	}

	return 0;
}

int srtp_session_add_csrc(struct srtp_session *session, uint32_t csrc)
{
	return rtp_session_add_csrc(session->rtp_se, csrc);
}

int srtp_session_remove_csrc(struct srtp_session *session, uint32_t csrc)
{
	return rtp_session_remove_csrc(session->rtp_se, csrc);
}

int srtp_session_send(struct srtp_session *session, void *data, size_t len, uint32_t delta_ts,
		     uint8_t padding, uint8_t marker, struct rtp_header_extension *hdr_x,
		     uint32_t *timestamp)
{
	struct rtp_session_context *rtp_context;
	struct rtp_packet rtp_pkt = {0};
	struct rtp_header *rtp_header = &rtp_pkt.header;
	int ret;
	srtp_err_status_t status;

	if (session == NULL) {
		return -EINVAL;
	}

	if (data == NULL && len > 0) {
		return -EINVAL;
	}

	if (marker > 1) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&session->rtp_se->lock, API_LOCK_TIMEOUT);
	if (ret < 0) {
		NET_DBG("Failed to take session lock (%d)", ret);
		return ret;
	}
	rtp_context = &session->rtp_se->rtp_context;

	rtp_header_set_v(rtp_header, RTP_VERSION);
	rtp_header_set_p(rtp_header, (padding > 0) ? 1 : 0);
	rtp_header_set_cc(rtp_header, session->rtp_se->csrc_len);
	rtp_header_set_m(rtp_header, marker);
	rtp_header_set_pt(rtp_header, rtp_context->payload_type);

	rtp_header->seq = session->rtp_se->sequence_number++;
	rtp_header->ts = session->rtp_se->timestamp;
	rtp_header->ssrc = session->rtp_se->ssrc; // Keep in CPU endianness

	if (hdr_x != NULL) {
		if (hdr_x->length > 0 && hdr_x->data == NULL) {
			ret = -EINVAL;
			goto unlock;
		}

		rtp_header_set_x(rtp_header, 1);
		memcpy(&rtp_header->header_extension, hdr_x, sizeof(*hdr_x));
	}

#if CONFIG_RTP_MAX_CSRC_COUNT > 0
	uint8_t cc_count = rtp_header->vpxcc & RTP_HDR_CC_MASK;
	for (size_t i = 0; i < cc_count; i++) {
		// Note: Adjust to session->rtp_se.csrc[i] if your struct uses a dot instead of ->
		rtp_header->csrc[i] = session->rtp_se->csrc[i];
	}
#endif /* CONFIG_RTP_MAX_CSRC_COUNT > 0 */

	uint8_t temp_buf[CONFIG_RTP_TRANSPORT_SOCKET_BUF_SIZE];
	uint8_t *cursor = temp_buf;

	// Fixed 12-byte header
	*cursor++ = rtp_header->vpxcc;
	*cursor++ = rtp_header->mpt;
	sys_put_be16(rtp_header->seq, cursor);  cursor += 2;
	sys_put_be32(rtp_header->ts, cursor);   cursor += 4;
	sys_put_be32(rtp_header->ssrc, cursor); cursor += 4;

	// CSRC list
	uint8_t cc = rtp_header->vpxcc & RTP_HDR_CC_MASK;
	for (size_t i = 0; i < cc; i++) {
		sys_put_be32(rtp_header->csrc[i], cursor); cursor += 4;
	}

	// Header Extension
	if (rtp_header->vpxcc & RTP_HDR_X_MASK) {
		sys_put_be16(rtp_header->header_extension.definition, cursor); cursor += 2;
		sys_put_be16(rtp_header->header_extension.length, cursor); cursor += 2;

		// Length is in 32-bit words, multiply by 4 for bytes
		size_t ext_bytes = (size_t)rtp_header->header_extension.length * 4;
		memcpy(cursor, rtp_header->header_extension.data, ext_bytes);
		cursor += ext_bytes;
	}

	size_t header_wire_len = cursor - temp_buf;

	if (len > 0) {
		memcpy(cursor, data, len);
	}

	size_t temp_len = header_wire_len + len;
	size_t protected_len = sizeof(temp_buf);

	status = srtp_protect(session->srtp_ctx, temp_buf, temp_len, temp_buf, &protected_len, 0);
	if (status != srtp_err_status_ok) {
		LOG_ERR("SRTP encryption failed with code: %d", status);
		ret = -EIO;
		goto unlock;
	}

	rtp_pkt.payload = temp_buf + header_wire_len;
	rtp_pkt.payload_len = protected_len - header_wire_len;

	ret = rtp_transport_socket_send(session->rtp_se, &rtp_pkt, padding);

	if (timestamp != NULL) {
		*timestamp = session->rtp_se->timestamp;
	}

unlock:
	/* Timestamp is increased, regardless of whether the block is transmitted or not */
	session->rtp_se->timestamp += delta_ts;
	(void)k_mutex_unlock(&session->rtp_se->lock);
	return ret;
}


static void srtp_socket_svc_handler(struct net_socket_service_event *pev);

static int srtp_parse_raw(srtp_t srtp_ctx, struct rtp_packet *packet, uint8_t *data, size_t len)
{
	srtp_err_status_t status;
	uint8_t decrypted_data[128];
	size_t decrypted_len = sizeof(decrypted_data);
	uint8_t *cursor = decrypted_data;


	if (len < RTP_MIN_HEADER_LEN) {
		NET_DBG("RTP packet too small (%zu)", len);
		return -EINVAL;
	}

	status = srtp_unprotect(srtp_ctx, data, len,
				decrypted_data, &decrypted_len);
	if (status) {
		LOG_ERR("error: srtp unprotection failed with code %d%s\n", status,
			status == srtp_err_status_replay_fail ? " (replay check failed)"
			: status == srtp_err_status_auth_fail ? " (auth check failed)"
								: "");
		return -ENODATA;
	}

	uint8_t *end = cursor + decrypted_len;

	packet->header.vpxcc = *cursor;
	cursor += sizeof(uint8_t);
	packet->header.mpt = *cursor;
	cursor += sizeof(uint8_t);

	packet->header.seq = sys_get_be16(cursor);
	cursor += sizeof(uint16_t);

	packet->header.ts = sys_get_be32(cursor);
	cursor += sizeof(uint32_t);

	packet->header.ssrc = sys_get_be32(cursor);
	cursor += sizeof(uint32_t);
	if (rtp_header_get_v(&packet->header) != RTP_VERSION) {
		NET_DBG("Invalid RTP version (%d)", rtp_header_get_v(&packet->header));
		return -EINVAL;
	}

	if (rtp_header_get_cc(&packet->header) > 0) {
		size_t csrc_count = rtp_header_get_cc(&packet->header);
		size_t csrc_skip = 0;

		if (end - cursor < (ptrdiff_t)(csrc_count * sizeof(uint32_t))) {
			NET_DBG("Data too small for cc");
			return -EINVAL;
		}

#if CONFIG_RTP_MAX_CSRC_COUNT > 0
		if (csrc_count > ARRAY_SIZE(packet->header.csrc)) {
			NET_DBG("Size of csrc too small, ignoring following csrcs. Please increase "
				"CONFIG_RTP_MAX_CSRC_COUNT");
			csrc_skip = csrc_count - ARRAY_SIZE(packet->header.csrc);
			csrc_count = ARRAY_SIZE(packet->header.csrc);
			rtp_header_set_cc(&packet->header, csrc_count);
		}

		for (size_t i = 0; i < csrc_count; i++) {
			packet->header.csrc[i] = sys_get_be32(cursor);
			cursor += sizeof(uint32_t);
		}
#else
		NET_DBG("Received packet with cc > 0, but CONFIG_RTP_MAX_CSRC_COUNT = 0");

		csrc_skip = csrc_count;
		rtp_header_set_cc(&packet->header, 0);
#endif /* CONFIG_RTP_MAX_CSRC_COUNT > 0 */

		cursor += csrc_skip * sizeof(uint32_t);
	}

	if (rtp_header_get_x(&packet->header) == 1) {
		struct rtp_header_extension *hdr_x = &packet->header.header_extension;
		size_t x_data_len;

		if (end - cursor < (ptrdiff_t)(2 * sizeof(uint16_t))) {
			NET_DBG("Data too small for header extension");
			return -EINVAL;
		}

		hdr_x->definition = sys_get_be16(cursor);
		cursor += sizeof(uint16_t);

		hdr_x->length = sys_get_be16(cursor);
		cursor += sizeof(uint16_t);
		x_data_len = hdr_x->length * sizeof(uint32_t);

		if (end - cursor < (ptrdiff_t)x_data_len) {
			NET_DBG("RTP extension header length too large for pkt");
			return -EINVAL;
		}

		hdr_x->data = cursor;
		cursor += x_data_len;
	}

	packet->payload_len = end - cursor;
	packet->payload = cursor;
	LOG_INF("Received: %s", packet->payload);

	if (rtp_header_get_p(&packet->header) == 1) {
		uint8_t padding;

		if (packet->payload_len == 0) {
			NET_DBG("Padding flag set but no payload");
			return -EINVAL;
		}

		padding = *(end - 1);

		if (padding == 0) {
			NET_DBG("Padding flag is set but padding is 0");
			return -EINVAL;
		}

		if (padding > packet->payload_len) {
			NET_DBG("Padding larger than payload");
			return -EINVAL;
		}

		packet->payload_len -= padding;
	}

	return 0;
}

static void srtp_socket_svc_handler(struct net_socket_service_event *pev)
{
	uint8_t data[CONFIG_RTP_TRANSPORT_SOCKET_BUF_SIZE];
	struct net_msghdr msg = {};
	struct net_iovec iov = {.iov_base = data, .iov_len = sizeof(data)};
#ifdef CONFIG_NET_PKT_TIMESTAMP
	uint8_t ctrl_buf[CMSG_SPACE(sizeof(struct net_ptp_time))] = {};
#endif
	struct rtp_packet packet = {};
	struct srtp_socket_context *ctx;
	struct srtp_session *session;
	ssize_t len;
	int ret;

	__ASSERT_NO_MSG(pev->user_data != NULL);

	if (!(pev->event.revents & ZSOCK_POLLIN)) {
		return;
	}

	ctx = (struct srtp_socket_context *)pev->user_data;
	session = NULL;

	ret = k_mutex_lock(&ctx->lock, CTX_LOCK_TIMEOUT);
	if (ret < 0) {
		NET_DBG("Failed to lock context (%d)", ret);
		return;
	}

	for (size_t i = 0; i < ARRAY_SIZE(ctx->fds); i++) {
		if (ctx->fds[i].fd == pev->event.fd) {
			session = ctx->sessions[i];
			break;
		}
	}

	if (session == NULL || session->rtp_se->rtp_context.callback == NULL) {
		goto unlock;
	}

	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
#ifdef CONFIG_NET_PKT_TIMESTAMP
	msg.msg_control = ctrl_buf;
	msg.msg_controllen = sizeof(ctrl_buf);
#endif

	len = zsock_recvmsg(pev->event.fd, &msg, 0);
	if (len < 0) {
		NET_DBG("Failed to receive from socket (%d)", errno);
		goto unlock;
	}

#ifdef CONFIG_NET_PKT_TIMESTAMP
	for (struct cmsghdr *cmsg = NET_CMSG_FIRSTHDR(&msg); cmsg != NULL;
	     cmsg = NET_CMSG_NXTHDR(&msg, cmsg)) {
		if (cmsg->cmsg_level == ZSOCK_SOL_SOCKET &&
		    cmsg->cmsg_type == ZSOCK_SO_TIMESTAMPING) {
			memcpy(&packet.timestamp, CMSG_DATA(cmsg), sizeof(packet.timestamp));
			break;
		}
	}
#endif

	(void)k_mutex_unlock(&ctx->lock);

	if (srtp_parse_raw(session->srtp_ctx, &packet, data, len) < 0) {
		return;
	}

	session->rtp_se->rtp_context.callback(session->rtp_se, &packet, session->rtp_se->rtp_context.user_data);

	return;

unlock:
	(void)k_mutex_unlock(&ctx->lock);
}
SYS_INIT(srtp_socket_init, POST_KERNEL, CONFIG_RTP_TRANSPORT_SOCKET_INIT_PRIO);

// BUILD_ASSERT(CONFIG_SRTP_TRANSPORT_SOCKET_BUF_SIZE >= RTP_MIN_HEADER_LEN);
