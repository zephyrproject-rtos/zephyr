/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ssh, CONFIG_SSH_LOG_LEVEL);

#include "ssh_kex.h"
#include "ssh_host_key.h"

#include <psa/crypto.h>

#ifdef CONFIG_SSH_SERVER
static int send_kex_ecdh_reply(struct ssh_transport *transport);
#endif

static int gen_shared_secret(struct ssh_transport *transport,
			     const struct ssh_string *remote_ephemeral_key);

#ifdef CONFIG_SSH_CLIENT
int ssh_kex_ecdh_send_kex_ecdh_init(struct ssh_transport *transport)
{
	int ret;
	uint8_t pubkey_buff[32];
	size_t pubkey_len;
	psa_status_t status;
	struct ssh_string pubkey_str;

	SSH_PAYLOAD_BUF(payload, transport->tx_buf);

	/* Header / Message ID */
	if (!ssh_payload_skip_bytes(&payload, SSH_PKT_MSG_ID_OFFSET) ||
	    !ssh_payload_write_byte(&payload, SSH_MSG_KEX_ECDH_INIT)) {
		return -ENOBUFS;
	}

	/* Ephemeral key */
	status = psa_export_public_key(
		transport->ecdh_ephemeral_key,
		pubkey_buff, sizeof(pubkey_buff), &pubkey_len);
	if (status != PSA_SUCCESS) {
		NET_ERR("Failed to write pubkey: %d", status);
		return -EIO;
	}

	pubkey_str = (struct ssh_string){
		.len = pubkey_len,
		.data = pubkey_buff
	};

	if (!ssh_payload_write_string(&payload, &pubkey_str)) {
		return -ENOBUFS;
	}

	ret = ssh_transport_send_packet(transport, &payload);
	if (ret < 0) {
		NET_DBG("Failed to send %s (%d)", "KEX_ECDH_REPLY", ret);
	}

	return ret;
}
#endif

int ssh_kex_ecdh_process_msg(struct ssh_transport *transport, uint8_t msg_id,
			     struct ssh_payload *rx_pkt)
{
	int ret = -EINVAL;

	switch (msg_id) {
#ifdef CONFIG_SSH_SERVER
	case SSH_MSG_KEX_ECDH_INIT: {
		struct ssh_string client_ephemeral_key;
		psa_key_attributes_t attr;
		psa_status_t kex_status;

		NET_DBG("KEX_ECDH_INIT");

		/* Server only */
		if (!transport->server || !transport->kexinit_received ||
		    transport->newkeys_sent) {
			return -EINVAL;
		}

		if (!ssh_payload_read_string(rx_pkt, &client_ephemeral_key) ||
		    !ssh_payload_read_complete(rx_pkt)) {
			NET_ERR("Length error");
			return -EINVAL;
		}

		/* Generate ephemeral Curve25519 key */
		attr = (psa_key_attributes_t)PSA_KEY_ATTRIBUTES_INIT;

		psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY));
		psa_set_key_bits(&attr, 255);
		psa_set_key_algorithm(&attr, PSA_ALG_ECDH);
		psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE | PSA_KEY_USAGE_EXPORT);

		kex_status = psa_generate_key(&attr, &transport->ecdh_ephemeral_key);
		psa_reset_key_attributes(&attr);
		if (kex_status != PSA_SUCCESS) {
			NET_ERR("genkey curve25519 failed: %d", kex_status);
			return -EIO;
		}

		ret = gen_shared_secret(transport, &client_ephemeral_key);
		if (ret != 0) {
			NET_ERR("gen shared secret failed: %d", ret);
			return ret;
		}

		/* Generate exchange hash */
		ret = ssh_kex_gen_exchange_hash(transport, &client_ephemeral_key, NULL);
		if (ret != 0) {
			NET_ERR("gen exchange hash failed: %d", ret);
			return ret;
		}

		/* Only set session ID on first kex */
		if (transport->session_id == NULL) {
			transport->session_id = ssh_payload_string_alloc(
				&transport->kex_heap, transport->exchange_hash->data,
				transport->exchange_hash->len);
			if (transport->session_id == NULL) {
				return -ENOMEM;
			}
		}

		ret = send_kex_ecdh_reply(transport);
		if (ret < 0) {
			return ret;
		}

		ret = ssh_kex_send_newkeys(transport);
		if (ret < 0) {
			return ret;
		}

		transport->newkeys_sent = true;
		break;
	}
#endif
#ifdef CONFIG_SSH_CLIENT
	case SSH_MSG_KEX_ECDH_REPLY: {
		struct ssh_string server_host_key, server_ephemeral_key,
			exchange_hash_signature;

		NET_DBG("KEX_ECDH_REPLY");

		/* Client only */
		if (transport->server || !transport->kexinit_received ||
		    transport->newkeys_sent) {
			return -EINVAL;
		}

		if (!ssh_payload_read_string(rx_pkt, &server_host_key) ||
		    !ssh_payload_read_string(rx_pkt, &server_ephemeral_key) ||
		    !ssh_payload_read_string(rx_pkt, &exchange_hash_signature) ||
		    !ssh_payload_read_complete(rx_pkt)) {
			NET_ERR("Length error");
			return -EINVAL;
		}

		ret = gen_shared_secret(transport, &server_ephemeral_key);
		if (ret != 0) {
			NET_ERR("gen shared secret failed: %d", ret);
			return ret;
		}

		/* Generate exchange hash */
		ret = ssh_kex_gen_exchange_hash(transport, &server_ephemeral_key,
						&server_host_key);
		if (ret != 0) {
			NET_ERR("gen exchange hash failed: %d", ret);
			return ret;
		}

		/* TODO: Validate the servers host key (known hosts) */

		/* Verify the servers signature on the exchange hash */
		ret = ssh_host_key_verify_signature(
			&transport->algs.server_host_key, &server_host_key,
			&exchange_hash_signature, transport->exchange_hash->data,
			transport->exchange_hash->len);
		if (ret < 0) {
			NET_DBG("Failed to verify signature (%d)", ret);
			return ret;
		}

		/* Only set session ID on first kex */
		if (transport->session_id == NULL) {
			transport->session_id = ssh_payload_string_alloc(
				&transport->kex_heap, transport->exchange_hash->data,
				transport->exchange_hash->len);
			if (transport->session_id == NULL) {
				return -ENOMEM;
			}
		}

		ret = ssh_kex_send_newkeys(transport);
		if (ret < 0) {
			return ret;
		}

		transport->newkeys_sent = true;
		break;
	}
#endif
	default:
		break;
	}

	return ret;
}

#ifdef CONFIG_SSH_SERVER
static int send_kex_ecdh_reply(struct ssh_transport *transport)
{
	int ret;
	uint8_t pubkey_buff[32];
	size_t pubkey_len;
	psa_status_t pub_status;
	struct ssh_string pubkey_str;

	SSH_PAYLOAD_BUF(payload, transport->tx_buf);

	/* Header / Message ID */
	if (!ssh_payload_skip_bytes(&payload, SSH_PKT_MSG_ID_OFFSET) ||
	    !ssh_payload_write_byte(&payload, SSH_MSG_KEX_ECDH_REPLY)) {
		ret = -ENOBUFS;
		goto exit;
	}

	/* Host key */
	ret = ssh_host_key_write_pub_key(&payload, transport->host_key_index);
	if (ret != 0) {
		goto exit;
	}

	/* Ephemeral key */
	pub_status = psa_export_public_key(
		transport->ecdh_ephemeral_key,
		pubkey_buff, sizeof(pubkey_buff), &pubkey_len);
	if (pub_status != PSA_SUCCESS) {
		NET_ERR("Failed to write pubkey: %d", pub_status);
		ret = -EIO;
		goto exit;
	}

	pubkey_str = (struct ssh_string){
		.len = pubkey_len,
		.data = pubkey_buff
	};

	if (!ssh_payload_write_string(&payload, &pubkey_str)) {
		ret = -ENOBUFS;
		goto exit;
	}

	/* Signature on the exchange hash */
	ret = ssh_host_key_write_signature(
		&payload, transport->host_key_index, transport->algs.server_host_key,
		transport->exchange_hash->data, transport->exchange_hash->len);
	if (ret < 0) {
		NET_DBG("Failed to write signature (%d)", ret);
		goto exit;
	}

	ret = ssh_transport_send_packet(transport, &payload);
	if (ret < 0) {
		NET_DBG("Failed to send %s (%d)", "KEX_ECDH_REPLY", ret);
	}

exit:
	return ret;
}
#endif

static int gen_shared_secret(struct ssh_transport *transport,
			     const struct ssh_string *remote_ephemeral_key)
{
	uint8_t shared_secret_buf[32];
	size_t shared_secret_len;
	psa_status_t status;

	NET_HEXDUMP_DBG(remote_ephemeral_key->data, remote_ephemeral_key->len, "remote_pub");

	/* PSA raw key agreement: takes our private key handle and peer's raw public key */
	status = psa_raw_key_agreement(PSA_ALG_ECDH,
				       transport->ecdh_ephemeral_key,
				       remote_ephemeral_key->data,
				       remote_ephemeral_key->len,
				       shared_secret_buf,
				       sizeof(shared_secret_buf),
				       &shared_secret_len);
	if (status != PSA_SUCCESS) {
		NET_ERR("ecdh compute shared failed: %d", status);
		return -EIO;
	}

	transport->shared_secret = ssh_payload_string_alloc(
		&transport->kex_heap, shared_secret_buf, shared_secret_len);
	if (transport->shared_secret == NULL) {
		return -ENOMEM;
	}

	NET_DBG("shared_secret %zu", transport->shared_secret->len);
	NET_HEXDUMP_DBG(transport->shared_secret->data,
			transport->shared_secret->len, "shared_secret");

	return 0;
}
