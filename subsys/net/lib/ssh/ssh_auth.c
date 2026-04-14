/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ssh, CONFIG_SSH_LOG_LEVEL);

#include "ssh_auth.h"

#define ZEPHYR_SSH_BANNER CONFIG_SSH_DEFAULT_WELCOME_BANNER ZEPHYR_SSH_VERSION

#ifdef CONFIG_SSH_SERVER
static int send_userauth_failure(struct ssh_transport *transport, bool partial_success);
static int send_userauth_success(struct ssh_transport *transport);
static int send_userauth_banner(struct ssh_transport *transport);
static int send_userauth_pk_ok(struct ssh_transport *transport,
			       const struct ssh_string *pub_key_alg,
			       const struct ssh_string *pub_key);
#endif

static const struct ssh_string auth_methods[] = {
	[SSH_AUTH_NONE] = SSH_STRING_LITERAL("none"),
	[SSH_AUTH_PASSWORD] = SSH_STRING_LITERAL("password"),
	[SSH_AUTH_PUBKEY] = SSH_STRING_LITERAL("publickey")
};

static int auth_method_str_to_id(const struct ssh_string *method)
{
	for (int i = 0; i < ARRAY_SIZE(auth_methods); i++) {
		if (ssh_strings_equal(&auth_methods[i], method)) {
			return i;
		}
	}

	return -ENOTSUP;
}

#ifdef CONFIG_SSH_CLIENT
int ssh_auth_send_userauth_request_none(struct ssh_transport *transport, const char *user_name)
{
	static const struct ssh_string service_name = SSH_STRING_LITERAL("ssh-connection");
	const struct ssh_string *method_name = &auth_methods[SSH_AUTH_NONE];
	int ret;

	SSH_PAYLOAD_BUF(payload, transport->tx_buf);

	if (!ssh_payload_skip_bytes(&payload, SSH_PKT_MSG_ID_OFFSET) ||
	    !ssh_payload_write_byte(&payload, SSH_MSG_USERAUTH_REQUEST) ||
	    !ssh_payload_write_cstring(&payload, user_name) ||
	    !ssh_payload_write_string(&payload, &service_name) ||
	    !ssh_payload_write_string(&payload, method_name)) {
		return -ENOBUFS;
	}

	ret = ssh_transport_send_packet(transport, &payload);
	if (ret < 0) {
		NET_WARN("Failed to send USERAUTH_REQUEST %d", ret);
	}

	return ret;
}

int ssh_auth_send_userauth_request_password(struct ssh_transport *transport,
					    const char *user_name, const char *password)
{
	static const struct ssh_string service_name = SSH_STRING_LITERAL("ssh-connection");
	const struct ssh_string *method_name = &auth_methods[SSH_AUTH_PASSWORD];
	int ret;

	SSH_PAYLOAD_BUF(payload, transport->tx_buf);

	if (!ssh_payload_skip_bytes(&payload, SSH_PKT_MSG_ID_OFFSET) ||
	    !ssh_payload_write_byte(&payload, SSH_MSG_USERAUTH_REQUEST) ||
	    !ssh_payload_write_cstring(&payload, user_name) ||
	    !ssh_payload_write_string(&payload, &service_name) ||
	    !ssh_payload_write_string(&payload, method_name) ||
	    !ssh_payload_write_bool(&payload, false) ||
	    !ssh_payload_write_cstring(&payload, password)) {
		return -ENOBUFS;
	}

	ret = ssh_transport_send_packet(transport, &payload);
	if (ret < 0) {
		NET_DBG("Failed to send USERAUTH_REQUEST %d", ret);
	}

	return ret;
}

static int ssh_auth_send_userauth_request_pubkey(struct ssh_transport *transport,
						 const char *user_name, int host_key_index)
{
	static const struct ssh_string service_name = SSH_STRING_LITERAL("ssh-connection");
	const struct ssh_string *method_name = &auth_methods[SSH_AUTH_PUBKEY];
	int ret;

	SSH_PAYLOAD_BUF(payload, transport->tx_buf);

	/* Normally you would send each public key without a signature and
	 * check for a SSH_MSG_USERAUTH_PK_OK response. However, since only
	 * a single public key is supported, just send once with the signature.
	 */
	const bool has_signature = true;
	struct ssh_string todo = SSH_STRING_LITERAL("rsa-sha2-256");
	enum ssh_host_key_alg host_key_alg;

	/* First create the signature */
	if (!ssh_payload_write_string(&payload, transport->session_id) ||
	    !ssh_payload_write_byte(&payload, SSH_MSG_USERAUTH_REQUEST) ||
	    !ssh_payload_write_cstring(&payload, user_name) ||
	    !ssh_payload_write_string(&payload, &service_name) ||
	    !ssh_payload_write_string(&payload, method_name) ||
	    !ssh_payload_write_bool(&payload, has_signature) ||
	    !ssh_payload_write_string(&payload, &todo)) {
		return -ENOBUFS;
	}

	ret = ssh_host_key_write_pub_key(&payload, host_key_index);
	if (ret != 0) {
		return ret;
	}

	/* TODO: Get this from transport->sig_algs_mask */
	host_key_alg = SSH_HOST_KEY_ALG_RSA_SHA2_256;
	ret = ssh_host_key_write_signature(&payload, host_key_index, host_key_alg,
					   payload.data, payload.len);
	if (ret != 0) {
		return ret;
	}

	if (transport->session_id->len < SSH_PKT_MSG_ID_OFFSET) {
		/* This should never occur */
		return -EINVAL;
	}

	/* Move the payload into place (stripping session_id from the front) */
	payload.len = payload.len + SSH_PKT_MSG_ID_OFFSET - transport->session_id->len - 4;
	memmove(&payload.data[SSH_PKT_MSG_ID_OFFSET],
		&payload.data[transport->session_id->len + 4], payload.len);

	ret = ssh_transport_send_packet(transport, &payload);
	if (ret < 0) {
		NET_DBG("Failed to send USERAUTH_REQUEST %d", ret);
	}

	return ret;
}
#endif

int ssh_auth_process_msg(struct ssh_transport *transport, uint8_t msg_id,
			 struct ssh_payload *rx_pkt)
{
	int ret = -EINVAL;

	switch (msg_id) {
#ifdef CONFIG_SSH_SERVER
	case SSH_MSG_USERAUTH_REQUEST: {
		struct ssh_string user_name, service_name, method_name;

		NET_DBG("USERAUTH_REQUEST");

		/* Server only */
		if (!transport->server) {
			return -EINVAL;
		}

		/* Clients can send multiple authentication requests at once.
		 * After one is accepted we must reply with failure for the rest
		 */
		if (transport->authenticated) {
			return send_userauth_failure(transport, false);
		}

		if (!ssh_payload_read_string(rx_pkt, &user_name) ||
		    !ssh_payload_read_string(rx_pkt, &service_name) ||
		    !ssh_payload_read_string(rx_pkt, &method_name)) {
			NET_ERR("Length error");
			return -EINVAL;
		}
		/* TODO: ssh_payload_read_complete for different auth methods */
		NET_DBG("USERAUTH_REQUEST: %.*s %.*s %.*s", user_name.len, user_name.data,
			service_name.len, service_name.data, method_name.len, method_name.data);

		ret = auth_method_str_to_id(&method_name);

		switch (ret) {
		case SSH_AUTH_NONE:
			/*if ((transport->auths_allowed_mask & BIT(SSH_AUTH_NONE)) != 0) {} */
			return send_userauth_failure(transport, false);

		case SSH_AUTH_PASSWORD: {
			/*if ((transport->auths_allowed_mask & BIT(SSH_AUTH_PASSWORD)) != 0) {} */
			struct ssh_server *sshd;
			struct ssh_string password;
			size_t password_len, username_len;

			if (!ssh_payload_read_bool(rx_pkt, NULL) ||
			    !ssh_payload_read_string(rx_pkt, &password) ||
			    !ssh_payload_read_complete(rx_pkt)) {
				NET_ERR("Length error");
				return -EINVAL;
			}

			sshd = transport->sshd;

			/* Check username. Allow any username if server username is empty */
			if (sshd->username[0] != '\0') {
				username_len = strlen(sshd->username);

				if (username_len == 0 || username_len != user_name.len ||
				    strncmp(sshd->username, user_name.data, username_len) != 0) {
					return send_userauth_failure(transport, false);
				}
			}

			/* Check password */
			password_len = strlen(sshd->password);
			if (password_len == 0 || password_len != password.len ||
			    strncmp(sshd->password, password.data, password_len) != 0) {
				return send_userauth_failure(transport, false);
			}

			break;
		}
		case SSH_AUTH_PUBKEY: {
			/*if ((transport->auths_allowed_mask & BIT(SSH_AUTH_PUBKEY)) != 0) {} */
			bool has_signature;
			struct ssh_string pub_key_alg, pub_key, signature;
			struct ssh_server *sshd;
			size_t username_len;
			int pub_key_index = -1;

			if (!ssh_payload_read_bool(rx_pkt, &has_signature) ||
			    !ssh_payload_read_string(rx_pkt, &pub_key_alg) ||
			    !ssh_payload_read_string(rx_pkt, &pub_key)) {
				NET_ERR("Length error");
				return -EINVAL;
			}

			if (has_signature) {
				if (!ssh_payload_read_string(rx_pkt, &signature)) {
					NET_ERR("Length error");
					return -EINVAL;
				}
			}

			if (!ssh_payload_read_complete(rx_pkt)) {
				NET_ERR("Length error");
				return -EINVAL;
			}

			sshd = transport->sshd;

			/* Check username. Allow any username if server username is empty */
			if (sshd->username[0] != '\0') {
				username_len = strlen(sshd->username);

				if (username_len == 0 || username_len != user_name.len ||
				    strncmp(sshd->username, user_name.data, username_len) != 0) {
					return send_userauth_failure(transport, false);
				}
			}

			/* Try to find a matching public key */
			for (size_t i = 0 ; i < sshd->authorized_keys_len; i++) {
				/* Re-use the TX buf for temporary storage */
				struct ssh_string tmp_pub_key;

				SSH_PAYLOAD_BUF(payload, transport->tx_buf);

				ret = ssh_host_key_write_pub_key(&payload,
								 sshd->authorized_keys[i]);
				payload.size = payload.len;
				payload.len = 0;
				if (ret == 0 && ssh_payload_read_string(&payload, &tmp_pub_key) &&
				    ssh_strings_equal(&pub_key, &tmp_pub_key)) {
					pub_key_index = sshd->authorized_keys[i];
					break;
				}
			}

			if (pub_key_index < 0) {
				return send_userauth_failure(transport, false);
			}

			if (has_signature) {
				/* Re-use the TX buf for temporary storage */
				SSH_PAYLOAD_BUF(payload, transport->tx_buf);

				if (!ssh_payload_write_string(&payload, transport->session_id) ||
				    !ssh_payload_write_raw(&payload,
							   &rx_pkt->data[SSH_PKT_MSG_ID_OFFSET],
							   rx_pkt->size - SSH_PKT_MSG_ID_OFFSET -
							   4 - signature.len)) {
					/* Too big */
					return send_userauth_failure(transport, false);
				}

				ret = ssh_host_key_verify_signature(
					NULL, &pub_key, &signature, payload.data, payload.len);
				if (ret < 0) {
					return send_userauth_failure(transport, false);
				}
			} else {
				return send_userauth_pk_ok(transport, &pub_key_alg, &pub_key);
			}
			break;
		}
		default:
			/* Unsupported auth method */
			return send_userauth_failure(transport, false);
		}

		ret = send_userauth_banner(transport);
		if (ret < 0) {
			return ret;
		}

		ret = send_userauth_success(transport);
		if (ret < 0) {
			return ret;
		}

		transport->authenticated = true;
		break;
	}
#endif
#ifdef CONFIG_SSH_CLIENT
	case SSH_MSG_USERAUTH_FAILURE: {
		struct ssh_payload auths_allowed_name_list;
		bool partial_success;
		struct ssh_string auth_name;
		uint32_t auths_allowed_mask = 0;
		struct ssh_client *sshc;

		NET_DBG("USERAUTH_FAILURE");

		/* Client only */
		if (transport->server || transport->authenticated) {
			return -EINVAL;
		}

		if (!ssh_payload_read_name_list(rx_pkt, &auths_allowed_name_list) ||
		    !ssh_payload_read_bool(rx_pkt, &partial_success) ||
		    !ssh_payload_read_complete(rx_pkt)) {
			NET_ERR("Length error");
			return -EINVAL;
		}

		NET_DBG("Available auth methods: %.*s", auths_allowed_name_list.size,
			auths_allowed_name_list.data);

		while (ssh_payload_name_list_iter(&auths_allowed_name_list, &auth_name)) {
			int auth = auth_method_str_to_id(&auth_name);

			if (auth < 0 || auth == SSH_AUTH_NONE) {
				continue;
			}

			auths_allowed_mask |= BIT(auth);
		}

		transport->auths_allowed_mask &= auths_allowed_mask;

		/* Attempt public key auth if server allows it, it has
		 * not already been attempted, we have a public key and
		 * the server supports one of our signature algorithms
		 */
		sshc = transport->sshc;

		if ((transport->auths_allowed_mask & BIT(SSH_AUTH_PUBKEY)) != 0 &&
		    sshc->host_key_index >= 0 && transport->sig_algs_mask != 0) {
			ret = ssh_auth_send_userauth_request_pubkey(
				transport, sshc->user_name, sshc->host_key_index);
			transport->auths_allowed_mask &= ~BIT(SSH_AUTH_PUBKEY);

		} else if ((transport->auths_allowed_mask & BIT(SSH_AUTH_PASSWORD)) != 0 &&
			   transport->callback != NULL) {
			struct ssh_transport_event event;

			event.type = SSH_TRANSPORT_EVENT_SERVICE_ACCEPTED;
			ret = transport->callback(transport, &event, transport->callback_user_data);
			transport->auths_allowed_mask &= ~BIT(SSH_AUTH_PASSWORD);
		} else {
			NET_DBG("No available auth methods");
			return -EINVAL;
		}

		break;
	}
	case SSH_MSG_USERAUTH_SUCCESS: {
		NET_DBG("USERAUTH_SUCCESS");

		/* Client only */
		if (transport->server || transport->authenticated) {
			return -EINVAL;
		}

		if (!ssh_payload_read_complete(rx_pkt)) {
			NET_ERR("Length error");
			return -EINVAL;
		}

		transport->authenticated = true;

		if (transport->callback != NULL) {
			struct ssh_transport_event event;

			event.type = SSH_TRANSPORT_EVENT_AUTHENTICATE_RESULT;
			event.authenticate_result.success = true;
			ret = transport->callback(transport, &event, transport->callback_user_data);
		}
		break;
	}
	case SSH_MSG_USERAUTH_BANNER: {
		NET_DBG("USERAUTH_BANNER");
		struct ssh_string zephyr_ssh_banner, language_tag;

		if (!ssh_payload_read_string(rx_pkt, &zephyr_ssh_banner) ||
		    !ssh_payload_read_string(rx_pkt, &language_tag) ||
		    !ssh_payload_read_complete(rx_pkt)) {
			NET_ERR("Length error");
			return -EINVAL;
		}

		NET_DBG("USERAUTH_BANNER: \"%.*s\"", zephyr_ssh_banner.len, zephyr_ssh_banner.data);
		ret = 0;
		break;
	}
	case SSH_MSG_USERAUTH_PK_OK: {
		NET_DBG("USERAUTH_PK_OK");

		/* This is currently not used (see ssh_auth_send_userauth_request_pubkey) */
		ret = -EINVAL;
		break;
	}
#endif
	default:
		break;
	}

	return ret;
}

#ifdef CONFIG_SSH_SERVER
static int send_userauth_failure(struct ssh_transport *transport, bool partial_success)
{
	struct ssh_string auths_allowed_name_list[ARRAY_SIZE(auth_methods)];
	uint32_t num_auths_allowed = 0;
	int ret;

	SSH_PAYLOAD_BUF(payload, transport->tx_buf);

	for (uint32_t i = 0; i < ARRAY_SIZE(auth_methods); i++) {
		/* Auth method "none" MUST NOT be listed as supported by the server. */
		if (i == SSH_AUTH_NONE) {
			continue;
		}

		if ((BIT(i) & transport->auths_allowed_mask) != 0) {
			auths_allowed_name_list[num_auths_allowed++] = auth_methods[i];
		}
	}

	if (!ssh_payload_skip_bytes(&payload, SSH_PKT_MSG_ID_OFFSET) ||
	    !ssh_payload_write_byte(&payload, SSH_MSG_USERAUTH_FAILURE) ||
	    !ssh_payload_write_name_list(&payload, auths_allowed_name_list, num_auths_allowed) ||
	    !ssh_payload_write_bool(&payload, partial_success)) {
		return -ENOBUFS;
	}

	ret = ssh_transport_send_packet(transport, &payload);
	if (ret < 0) {
		NET_DBG("Failed to send USERAUTH_FAILURE %d", ret);
	}

	return ret;
}

static int send_userauth_success(struct ssh_transport *transport)
{
	int ret;

	SSH_PAYLOAD_BUF(payload, transport->tx_buf);

	if (!ssh_payload_skip_bytes(&payload, SSH_PKT_MSG_ID_OFFSET) ||
	    !ssh_payload_write_byte(&payload, SSH_MSG_USERAUTH_SUCCESS)) {
		return -ENOBUFS;
	}

	ret = ssh_transport_send_packet(transport, &payload);
	if (ret < 0) {
		NET_DBG("Failed to send USERAUTH_SUCCESS %d", ret);
	}

	return ret;
}

static int send_userauth_banner(struct ssh_transport *transport)
{
#if defined(CONFIG_SSH_HAS_INFO_BANNER)
#define INFO_BANNER "\n" CONFIG_SSH_DEFAULT_INFO_BANNER
#else
#define INFO_BANNER ""
#endif
	static const struct ssh_string zephyr_ssh_banner =
		SSH_STRING_LITERAL(ZEPHYR_SSH_BANNER INFO_BANNER);
	static const struct ssh_string language_tag = SSH_STRING_LITERAL("");
	int ret;

	SSH_PAYLOAD_BUF(payload, transport->tx_buf);

	if (!ssh_payload_skip_bytes(&payload, SSH_PKT_MSG_ID_OFFSET) ||
	    !ssh_payload_write_byte(&payload, SSH_MSG_USERAUTH_BANNER) ||
	    !ssh_payload_write_string(&payload, &zephyr_ssh_banner) ||
	    !ssh_payload_write_string(&payload, &language_tag)) {
		return -ENOBUFS;
	}

	ret = ssh_transport_send_packet(transport, &payload);
	if (ret < 0) {
		NET_DBG("Failed to send USERAUTH_BANNER %d", ret);
	}

	return ret;
}

static int send_userauth_pk_ok(struct ssh_transport *transport,
			       const struct ssh_string *pub_key_alg,
			       const struct ssh_string *pub_key)
{
	int ret;

	SSH_PAYLOAD_BUF(payload, transport->tx_buf);

	if (!ssh_payload_skip_bytes(&payload, SSH_PKT_MSG_ID_OFFSET) ||
	    !ssh_payload_write_byte(&payload, SSH_MSG_USERAUTH_PK_OK) ||
	    !ssh_payload_write_string(&payload, pub_key_alg) ||
	    !ssh_payload_write_string(&payload, pub_key)) {
		return -ENOBUFS;
	}

	ret = ssh_transport_send_packet(transport, &payload);
	if (ret < 0) {
		NET_DBG("Failed to send USERAUTH_PK_OK %d", ret);
	}

	return ret;
}
#endif
