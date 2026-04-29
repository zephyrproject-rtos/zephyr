/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ssh, CONFIG_SSH_LOG_LEVEL);

#include "ssh_kex.h"
#include "ssh_host_key.h"

#include <psa/crypto.h>

#define KEXINIT_COOKIE_SIZE 16

static int gen_all_keys(struct ssh_transport *transport);
static int gen_key(struct ssh_transport *transport, char c, uint8_t *buf, size_t len);

/* Note: The order of the strings is important */
static const struct ssh_string supported_kex_algs[] = {
	[SSH_KEX_ALG_CURVE25519_SHA256] = SSH_STRING_LITERAL("curve25519-sha256"),
	/* SSH_STRING_LITERAL("curve25519-sha256@libssh.org"), */
};
static const struct ssh_string supported_server_host_key_algs[] = {
	/* SSH_STRING_LITERAL("ssh-ed25519"), */
#ifdef CONFIG_SSH_HOST_KEY_ALG_RSA_SHA2_256
	[SSH_HOST_KEY_ALG_RSA_SHA2_256] = SSH_STRING_LITERAL("rsa-sha2-256"),
#endif
#ifdef CONFIG_SSH_HOST_KEY_ALG_RSA_SHA2_512
	[SSH_HOST_KEY_ALG_RSA_SHA2_512] = SSH_STRING_LITERAL("rsa-sha2-512"),
#endif
};

static const struct ssh_string supported_encryption_algs[] = {
	/* SSH_STRING_LITERAL("chacha20-poly1305@openssh.com"), */
	[SSH_ENCRYPTION_ALG_AES128_CTR] = SSH_STRING_LITERAL("aes128-ctr"),
};

static const struct ssh_string supported_mac_algs[] = {
	[SSH_MAC_ALG_HMAC_SHA2_256] = SSH_STRING_LITERAL("hmac-sha2-256"),
};

static const struct ssh_string supported_compression_algs[] = {
	[SSH_COMPRESSION_ALG_NONE] = SSH_STRING_LITERAL("none"),
};

static const struct ssh_string supported_languages[] = {
	SSH_STRING_LITERAL(""),
};

BUILD_ASSERT(ARRAY_SIZE(supported_kex_algs) > 0);
BUILD_ASSERT(ARRAY_SIZE(supported_server_host_key_algs) > 0);
BUILD_ASSERT(ARRAY_SIZE(supported_encryption_algs) > 0);
BUILD_ASSERT(ARRAY_SIZE(supported_mac_algs) > 0);
BUILD_ASSERT(ARRAY_SIZE(supported_compression_algs) > 0);
BUILD_ASSERT(ARRAY_SIZE(supported_languages) > 0);

int ssh_kex_send_kexinit(struct ssh_transport *transport)
{
	const bool first_kex_pkt_follows = false;
	const uint32_t reserved = 0;
	int ret;

	/* Add "ext-info-c" to client kex algs to enable RFC8308 extensions
	 * (server "ext-info-s" does not appear to be needed).
	 */
	struct ssh_string kex_algs[ARRAY_SIZE(supported_kex_algs) + 1];
	uint32_t num_kex_algs = 0;
	struct ssh_string **local_kexinit;

	SSH_PAYLOAD_BUF(payload, transport->tx_buf);

	for (; num_kex_algs < ARRAY_SIZE(supported_kex_algs); num_kex_algs++) {
		kex_algs[num_kex_algs] = supported_kex_algs[num_kex_algs];
	}

	if (IS_ENABLED(CONFIG_SSH_CLIENT) && !transport->server) {
		kex_algs[num_kex_algs++] = (struct ssh_string)SSH_STRING_LITERAL("ext-info-c");
	}

	/* Header / Message ID / Cookie / Kex algorithms / Server host key algorithms
	 * Encryption algorithms client to server / Encryption algorithms server to client
	 * MAC algorithms client to server / MAC algorithms server to client
	 * Compression algorithms client to server / Compression algorithms server to client
	 * Languages client to server / Languages server to client
	 * First kex packet follows / Reserved for future extension
	 */
	if (!ssh_payload_skip_bytes(&payload, SSH_PKT_MSG_ID_OFFSET) ||
	    !ssh_payload_write_byte(&payload, SSH_MSG_KEXINIT) ||
	    !ssh_payload_write_csrand(&payload, KEXINIT_COOKIE_SIZE) ||
	    !ssh_payload_write_name_list(&payload, kex_algs, num_kex_algs) ||
	    !ssh_payload_write_name_list(&payload, supported_server_host_key_algs,
					 ARRAY_SIZE(supported_server_host_key_algs)) ||
	    !ssh_payload_write_name_list(&payload, supported_encryption_algs,
					 ARRAY_SIZE(supported_encryption_algs)) ||
	    !ssh_payload_write_name_list(&payload, supported_encryption_algs,
					 ARRAY_SIZE(supported_encryption_algs)) ||
	    !ssh_payload_write_name_list(&payload, supported_mac_algs,
					 ARRAY_SIZE(supported_mac_algs)) ||
	    !ssh_payload_write_name_list(&payload, supported_mac_algs,
					 ARRAY_SIZE(supported_mac_algs)) ||
	    !ssh_payload_write_name_list(&payload, supported_compression_algs,
					 ARRAY_SIZE(supported_compression_algs)) ||
	    !ssh_payload_write_name_list(&payload, supported_compression_algs,
					 ARRAY_SIZE(supported_compression_algs)) ||
	    !ssh_payload_write_name_list(&payload, supported_languages,
					 ARRAY_SIZE(supported_languages)) ||
	    !ssh_payload_write_name_list(&payload, supported_languages,
					 ARRAY_SIZE(supported_languages)) ||
	    !ssh_payload_write_bool(&payload, first_kex_pkt_follows) ||
	    !ssh_payload_write_u32(&payload, reserved)) {
		return -ENOBUFS;
	}

	/* Save local kexinit payload for KEX hash
	 * ssh_transport_send_packet encrypts in-place, so we need to make a copy here.
	 */
	local_kexinit = transport->server ? &transport->server_kexinit :
					    &transport->client_kexinit;

	*local_kexinit = ssh_payload_string_alloc(&transport->kex_heap,
						  &payload.data[SSH_PKT_MSG_ID_OFFSET],
						  payload.len - SSH_PKT_MSG_ID_OFFSET);
	if (*local_kexinit == NULL) {
		return -ENOMEM;
	}

	ret = ssh_transport_send_packet(transport, &payload);
	if (ret < 0) {
		NET_DBG("Failed to send %s (%d)", "KEXINIT", ret);
		return ret;
	}

	transport->kexinit_sent = true;

	return ret;
}

int ssh_kex_send_newkeys(struct ssh_transport *transport)
{
	int ret;

	SSH_PAYLOAD_BUF(payload, transport->tx_buf);

	if (!ssh_payload_skip_bytes(&payload, SSH_PKT_MSG_ID_OFFSET) ||
	    !ssh_payload_write_byte(&payload, SSH_MSG_NEWKEYS)) {
		return -ENOBUFS;
	}

	ret = ssh_transport_send_packet(transport, &payload);
	if (ret < 0) {
		NET_DBG("Failed to send %s (%d)", "NEWKEYS", ret);
	}

	return ret;
}

static int kex_name_list_find_first(bool server, const struct ssh_string *lut, int lut_len,
				    struct ssh_payload *name_list, struct ssh_string *name_out)
{
	/* We want the first item in the clients name list that matches a name
	 * in the servers name list. This changes the order of how we iterate
	 * the name list vs look up table depending on if we are a server or client.
	 */
	if (IS_ENABLED(CONFIG_SSH_SERVER) && server) {
		while (ssh_payload_name_list_iter(name_list, name_out)) {
			for (int i = 0; i < lut_len; i++) {
				if (ssh_strings_equal(&lut[i], name_out)) {
					return i;
				}
			}
		}
	} else if (IS_ENABLED(CONFIG_SSH_CLIENT)) {
		for (int i = 0; i < lut_len; i++) {
			struct ssh_payload name_list_copy = *name_list;

			while (ssh_payload_name_list_iter(&name_list_copy, name_out)) {
				if (ssh_strings_equal(&lut[i], name_out)) {
					*name_list = name_list_copy;
					return i;
				}
			}
		}
	}

	return -ENOTSUP;
}

int ssh_kex_process_kexinit(struct ssh_transport *transport, struct ssh_payload *rx_pkt)
{
	int ret;
	struct ssh_payload name_list;
	struct ssh_string name;
	bool first_kex_pkt_follows;
	struct ssh_string **remote_kexinit;

	NET_DBG("KEXINIT");

	if (transport->kexinit_received) {
		return -1;
	}

	/* Skip cookie */
	if (!ssh_payload_skip_bytes(rx_pkt, KEXINIT_COOKIE_SIZE)) {
		NET_ERR("Length error");
		return -1;
	}

	/* Kex algorithm */
	if (!ssh_payload_read_name_list(rx_pkt, &name_list)) {
		NET_ERR("Length error");
		return -1;
	}

	NET_DBG("Remote %s algs: %.*s", "kex", name_list.size, name_list.data);

	if (IS_ENABLED(CONFIG_SSH_SERVER) && transport->server &&
	    transport->auths_allowed_mask & BIT(SSH_AUTH_PUBKEY)) {
		/* Check if the client supports RFC8308 extensions. */
		/* Required for the "server-sig-algs" extension to */
		/* avoid using SHA-1 signatures in pubkey auth. */
		struct ssh_payload name_list_copy = name_list;
		static const struct ssh_string ext_info_c = SSH_STRING_LITERAL("ext-info-c");

		ret = kex_name_list_find_first(
			transport->server, &ext_info_c, 1,
			&name_list_copy, &name);
		if (ret < 0) {
			NET_DBG("Client does not support RFC8308 extensions, "
				"disabling pubkey auth");
			transport->auths_allowed_mask &= ~BIT(SSH_AUTH_PUBKEY);
		}
	}

	ret = kex_name_list_find_first(transport->server, supported_kex_algs,
				       ARRAY_SIZE(supported_kex_algs), &name_list, &name);
	if (ret < 0) {
		NET_DBG("No suitable %s alg", "kex");
		return ret;
	}

	transport->algs.kex = ret;

	NET_DBG("Using %s alg %.*s", "kex", name.len, name.data);

	/* Server host key algorithm */
	if (!ssh_payload_read_name_list(rx_pkt, &name_list)) {
		NET_ERR("Length error");
		return -1;
	}

	NET_DBG("Remote %s algs: %.*s", "host key", name_list.size, name_list.data);

	ret = kex_name_list_find_first(transport->server, supported_server_host_key_algs,
				       ARRAY_SIZE(supported_server_host_key_algs),
				       &name_list, &name);
	if (ret < 0) {
		NET_DBG("No suitable %s alg", "host key");
		return ret;
	}

	transport->algs.server_host_key = ret;

	NET_DBG("Using %s alg %.*s", "host key", name.len, name.data);

	/* Encryption algorithm (client to server) */
	if (!ssh_payload_read_name_list(rx_pkt, &name_list)) {
		NET_ERR("Length error");
		return -1;
	}

	NET_DBG("Remote %s algs: %.*s", "encryption (C2S)", name_list.size, name_list.data);

	ret = kex_name_list_find_first(
		transport->server, supported_encryption_algs,
		ARRAY_SIZE(supported_encryption_algs), &name_list, &name);
	if (ret < 0) {
		NET_DBG("No suitable %s alg", "encryption (C2S)");
		return ret;
	}

	transport->algs.encryption_c2s = ret;

	NET_DBG("Using %s alg %.*s", "encryption (C2S)", name.len, name.data);

	/* Encryption algorithm (server to client) */
	if (!ssh_payload_read_name_list(rx_pkt, &name_list)) {
		NET_ERR("Length error");
		return -1;
	}

	NET_DBG("Remote %s algs: %.*s", "encryption (S2C)", name_list.size, name_list.data);

	ret = kex_name_list_find_first(
		transport->server, supported_encryption_algs,
		ARRAY_SIZE(supported_encryption_algs), &name_list, &name);
	if (ret < 0) {
		NET_DBG("No suitable %s alg", "encryption (S2C)");
		return ret;
	}

	transport->algs.encryption_s2c = ret;

	NET_DBG("Using %s alg %.*s", "encryption (S2C)", name.len, name.data);

	/* MAC algorithm (client to server) */
	if (!ssh_payload_read_name_list(rx_pkt, &name_list)) {
		NET_ERR("Length error");
		return -1;
	}

	NET_DBG("Remote %s algs: %.*s", "MAC (C2S)", name_list.size, name_list.data);

	ret = kex_name_list_find_first(
		transport->server, supported_mac_algs,
		ARRAY_SIZE(supported_mac_algs), &name_list, &name);
	if (ret < 0) {
		NET_DBG("No suitable %s alg", "MAC (C2S)");
		return ret;
	}

	transport->algs.mac_c2s = ret;

	NET_DBG("Using %s alg %.*s", "MAC (C2S)", name.len, name.data);

	/* MAC algorithm (server to client) */
	if (!ssh_payload_read_name_list(rx_pkt, &name_list)) {
		NET_ERR("Length error");
		return -1;
	}

	NET_DBG("Remote %s algs: %.*s", "MAC (S2C)", name_list.size, name_list.data);

	ret = kex_name_list_find_first(
		transport->server, supported_mac_algs,
		ARRAY_SIZE(supported_mac_algs), &name_list, &name);
	if (ret < 0) {
		NET_DBG("No suitable %s alg", "MAC (S2C)");
		return ret;
	}

	transport->algs.mac_s2c = ret;

	NET_DBG("Using %s alg %.*s", "MAC (S2C)", name.len, name.data);

	/* Compression algorithm (client to server) */
	if (!ssh_payload_read_name_list(rx_pkt, &name_list)) {
		NET_ERR("Length error");
		return -1;
	}

	NET_DBG("Remote %s algs: %.*s", "compression (C2S)", name_list.size, name_list.data);

	ret = kex_name_list_find_first(
		transport->server, supported_compression_algs,
		ARRAY_SIZE(supported_compression_algs), &name_list, &name);
	if (ret < 0) {
		NET_DBG("No suitable %s alg", "compression (C2S)");
		return ret;
	}

	transport->algs.compression_c2s = ret;

	NET_DBG("Using %s alg %.*s", "compression (C2S)", name.len, name.data);

	/* Compression algorithm (server to client) */
	if (!ssh_payload_read_name_list(rx_pkt, &name_list)) {
		NET_ERR("Length error");
		return -1;
	}

	NET_DBG("Remote %s algs: %.*s", "compression (S2C)", name_list.size, name_list.data);

	ret = kex_name_list_find_first(
		transport->server, supported_compression_algs,
		ARRAY_SIZE(supported_compression_algs), &name_list, &name);
	if (ret < 0) {
		NET_DBG("No suitable %s alg", "compression (S2C)");
		return ret;
	}

	transport->algs.compression_s2c = ret;

	NET_DBG("Using %s alg %.*s", "compression (S2C)", name.len, name.data);

	/* Languages (server to client) - Not supported */
	if (!ssh_payload_read_name_list(rx_pkt, &name_list)) {
		NET_ERR("Length error");
		return -1;
	}

	NET_DBG("Remote %s algs: %.*s", "languages (C2S)", name_list.size, name_list.data);

	/* Languages (server to client) - Not supported */
	if (!ssh_payload_read_name_list(rx_pkt, &name_list)) {
		NET_ERR("Length error");
		return -1;
	}

	NET_DBG("Remote %s algs: %.*s", "languages (S2C)", name_list.size, name_list.data);

	/* First kex packet follows (TODO: add support for "first_kex_pkt_follows") */
	if (!ssh_payload_read_bool(rx_pkt, &first_kex_pkt_follows)) {
		NET_ERR("Length error");
		return -1;
	}

	/* Reserved for future extension */
	if (!ssh_payload_read_u32(rx_pkt, NULL)) {
		NET_ERR("Length error");
		return -1;
	}

	if (!ssh_payload_read_complete(rx_pkt)) {
		NET_ERR("Unexpected extra data");
		return -1;
	}

	/* Save remote kexinit payload for KEX hash */
	remote_kexinit = transport->server ?
			&transport->client_kexinit : &transport->server_kexinit;
	*remote_kexinit = ssh_payload_string_alloc(
		&transport->kex_heap, &rx_pkt->data[SSH_PKT_MSG_ID_OFFSET],
		rx_pkt->len - SSH_PKT_MSG_ID_OFFSET);
	if (*remote_kexinit == NULL) {
		return -ENOMEM;
	}

	transport->kexinit_received = true;

	if (!transport->kexinit_sent) {
		ret = ssh_kex_send_kexinit(transport);
		if (ret != 0) {
			return ret;
		}
	}

#ifdef CONFIG_SSH_CLIENT
	if (!transport->server) {
		/* Generate ephemeral Curve25519 key */
		psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
		psa_status_t status;

		psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY));
		psa_set_key_bits(&attr, 255);
		psa_set_key_algorithm(&attr, PSA_ALG_ECDH);
		psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE | PSA_KEY_USAGE_EXPORT);

		status = psa_generate_key(&attr, &transport->ecdh_ephemeral_key);
		psa_reset_key_attributes(&attr);
		if (status != PSA_SUCCESS) {
			NET_ERR("genkey curve25519 failed: %d", status);
			return -EIO;
		}

		ret = ssh_kex_ecdh_send_kex_ecdh_init(transport);
		if (ret < 0) {
			return ret;
		}
	}
#endif

	return 0;
}

int ssh_kex_process_newkeys(struct ssh_transport *transport, struct ssh_payload *rx_pkt)
{
	psa_key_attributes_t mac_attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	struct ssh_cipher_aes128_ctr *cipher;
	const uint8_t *tx_integ;
	const uint8_t *rx_integ;
	psa_key_id_t *tx_mac;
	psa_key_id_t *rx_mac;
	psa_status_t status;
	int ret;

	NET_DBG("NEWKEYS");

	/* TODO: if transport->awaiting_newkeys */
	if (!transport->kexinit_received || !transport->kexinit_sent || !transport->newkeys_sent) {
		return -1;
	}

	if (!ssh_payload_read_complete(rx_pkt)) {
		NET_ERR("Unexpected extra data");
		return -1;
	}

	ret = gen_all_keys(transport);
	if (ret < 0) {
		return ret;
	}

	/* Import AES keys and set up persistent cipher operations */
	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, 128);
	psa_set_key_algorithm(&attr, PSA_ALG_CTR);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);

	/* Server encryption key → server's TX cipher or client's RX cipher */
	cipher = transport->server ? &transport->tx_cipher : &transport->rx_cipher;
	status = psa_import_key(&attr, transport->encrypt_key_server,
				sizeof(transport->encrypt_key_server), &cipher->key_id);
	if (status != PSA_SUCCESS) {
		NET_ERR("Cipher key import error");
		return -1;
	}

	cipher->op = psa_cipher_operation_init();

	if (transport->server) {
		status = psa_cipher_encrypt_setup(&cipher->op, cipher->key_id, PSA_ALG_CTR);
	} else {
		status = psa_cipher_decrypt_setup(&cipher->op, cipher->key_id, PSA_ALG_CTR);
	}

	if (status != PSA_SUCCESS) {
		NET_ERR("Cipher setup error");
		return -1;
	}

	status = psa_cipher_set_iv(&cipher->op, transport->iv_server, 16);
	if (status != PSA_SUCCESS) {
		NET_ERR("Cipher IV error");
		return -1;
	}

	/* Client encryption key → server's RX cipher or client's TX cipher */
	cipher = transport->server ? &transport->rx_cipher : &transport->tx_cipher;
	status = psa_import_key(&attr, transport->encrypt_key_client,
				sizeof(transport->encrypt_key_client), &cipher->key_id);
	if (status != PSA_SUCCESS) {
		NET_ERR("Cipher key import error");
		return -1;
	}

	cipher->op = psa_cipher_operation_init();

	if (transport->server) {
		status = psa_cipher_decrypt_setup(&cipher->op, cipher->key_id, PSA_ALG_CTR);
	} else {
		status = psa_cipher_encrypt_setup(&cipher->op, cipher->key_id, PSA_ALG_CTR);
	}

	if (status != PSA_SUCCESS) {
		NET_ERR("Cipher setup error");
		return -1;
	}

	status = psa_cipher_set_iv(&cipher->op, transport->iv_client, 16);
	if (status != PSA_SUCCESS) {
		NET_ERR("Cipher IV error");
		return -1;
	}

	psa_reset_key_attributes(&attr);

	/* Import HMAC keys */
	psa_set_key_type(&mac_attr, PSA_KEY_TYPE_HMAC);
	psa_set_key_algorithm(&mac_attr, PSA_ALG_HMAC(PSA_ALG_SHA_256));
	psa_set_key_usage_flags(&mac_attr, PSA_KEY_USAGE_SIGN_MESSAGE |
				PSA_KEY_USAGE_VERIFY_MESSAGE |
				PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_HASH);

	tx_mac = &transport->tx_mac_key_id;
	rx_mac = &transport->rx_mac_key_id;
	tx_integ = transport->server ? transport->integ_key_server : transport->integ_key_client;
	rx_integ = transport->server ? transport->integ_key_client : transport->integ_key_server;

	status = psa_import_key(&mac_attr, tx_integ, sizeof(transport->integ_key_server), tx_mac);
	if (status != PSA_SUCCESS) {
		NET_ERR("MAC key import error");
		return -1;
	}

	status = psa_import_key(&mac_attr, rx_integ, sizeof(transport->integ_key_client), rx_mac);
	if (status != PSA_SUCCESS) {
		NET_ERR("MAC key import error");
		return -1;
	}

	psa_reset_key_attributes(&mac_attr);

	transport->encrypted = true;
	transport->kexinit_sent = false;
	transport->kexinit_received = false;
	transport->newkeys_sent = false;
	transport->kex_expiry = sys_timepoint_calc(K_HOURS(1));
	transport->tx_bytes_since_kex = 0;
	transport->rx_bytes_since_kex = 0;

	/* Free/clear everything we no longer need */
	psa_destroy_key(transport->ecdh_ephemeral_key);

	transport->ecdh_ephemeral_key = PSA_KEY_ID_NULL;
	if (transport->client_kexinit != NULL) {
		sys_heap_free(&transport->kex_heap, transport->client_kexinit);
		transport->client_kexinit = NULL;
	}

	if (transport->server_kexinit != NULL) {
		sys_heap_free(&transport->kex_heap, transport->server_kexinit);
		transport->server_kexinit = NULL;
	}

	if (transport->shared_secret != NULL) {
		ssh_zeroize((void *)transport->shared_secret->data, transport->shared_secret->len);
		sys_heap_free(&transport->kex_heap, transport->shared_secret);
		transport->shared_secret = NULL;
	}

	if (transport->exchange_hash != NULL) {
		ssh_zeroize((void *)transport->exchange_hash->data, transport->exchange_hash->len);
		sys_heap_free(&transport->kex_heap, transport->exchange_hash);
		transport->exchange_hash = NULL;
	}

#ifdef CONFIG_SSH_CLIENT
	if (!transport->server && !transport->authenticated) {
		static const struct ssh_string service_name = SSH_STRING_LITERAL("ssh-userauth");

		ret = ssh_transport_send_service_request(transport, &service_name);
		if (ret < 0) {
			return ret;
		}
	}
#endif
#ifdef CONFIG_SSH_SERVER
	if (transport->server && !transport->authenticated &&
	    transport->auths_allowed_mask & BIT(SSH_AUTH_PUBKEY)) {
		/* Send "server-sig-algs" RFC8308 extension to */
		/* avoid using SHA-1 signatures in pubkey auth */
		ret = ssh_transport_send_server_sig_algs(transport);
		if (ret < 0) {
			return ret;
		}
	}
#endif

	return 0;
}

int ssh_kex_process_msg(struct ssh_transport *transport, uint8_t msg_id, struct ssh_payload *rx_pkt)
{
	return ssh_kex_ecdh_process_msg(transport, msg_id, rx_pkt);
}

int ssh_kex_gen_exchange_hash(struct ssh_transport *transport,
			      const struct ssh_string *remote_ephemeral_key,
			      const struct ssh_string *server_host_key)
{
	int ret = -EINVAL;
	uint8_t buf[4];
	psa_hash_operation_t hash_op = PSA_HASH_OPERATION_INIT;
	psa_status_t status;
	size_t hash_len;

	SSH_PAYLOAD_BUF(payload, transport->tx_buf);

	/*
	 * The exchange hash H is computed as the hash of the concatenation of
	 * the following.
	 *     string   V_C, client's identification string (CR and LF excluded)
	 *     string   V_S, server's identification string (CR and LF excluded)
	 *     string   I_C, payload of the client's SSH_MSG_KEXINIT
	 *     string   I_S, payload of the server's SSH_MSG_KEXINIT
	 *     string   K_S, server's public host key
	 *     string   Q_C, client's ephemeral public key octet string
	 *     string   Q_S, server's ephemeral public key octet string
	 *     mpint    K,   shared secret
	 */

	/* The exchange hash data is quite large, each item is hashed piecemeal to
	 * reduce memory usage.
	 */

	status = psa_hash_setup(&hash_op, PSA_ALG_SHA_256);
	if (status != PSA_SUCCESS) {
		ret = -EIO;
		goto exit;
	}

	/* Client identity */
	sys_put_be32(transport->client_identity->len, buf);
	if (psa_hash_update(&hash_op, buf, sizeof(buf)) != PSA_SUCCESS) {
		ret = -EIO;
		goto exit;
	}

	if (psa_hash_update(&hash_op, transport->client_identity->data,
			    transport->client_identity->len) != PSA_SUCCESS) {
		ret = -EIO;
		goto exit;
	}

	/* Server identity */
	sys_put_be32(transport->server_identity->len, buf);
	if (psa_hash_update(&hash_op, buf, sizeof(buf)) != PSA_SUCCESS) {
		ret = -EIO;
		goto exit;
	}

	if (psa_hash_update(&hash_op, transport->server_identity->data,
			    transport->server_identity->len) != PSA_SUCCESS) {
		ret = -EIO;
		goto exit;
	}

	/* Client kexinit payload */
	sys_put_be32(transport->client_kexinit->len, buf);
	if (psa_hash_update(&hash_op, buf, sizeof(buf)) != PSA_SUCCESS) {
		ret = -EIO;
		goto exit;
	}

	if (psa_hash_update(&hash_op, transport->client_kexinit->data,
			    transport->client_kexinit->len) != PSA_SUCCESS) {
		ret = -EIO;
		goto exit;
	}

	/* Server kexinit payload */
	sys_put_be32(transport->server_kexinit->len, buf);
	if (psa_hash_update(&hash_op, buf, sizeof(buf)) != PSA_SUCCESS) {
		ret = -EIO;
		goto exit;
	}

	if (psa_hash_update(&hash_op, transport->server_kexinit->data,
			    transport->server_kexinit->len) != PSA_SUCCESS) {
		ret = -EIO;
		goto exit;
	}

	/* Server public host key */
	if (transport->server) {
		payload.len = 0;
		ret = ssh_host_key_write_pub_key(&payload, transport->host_key_index);
		if (ret != 0) {
			goto exit;
		}
	} else {
		payload.len = 0;
		if (!ssh_payload_write_string(&payload, server_host_key)) {
			ret = -ENOBUFS;
			goto exit;
		}
	}

	if (psa_hash_update(&hash_op, payload.data, payload.len) != PSA_SUCCESS) {
		ret = -EIO;
		goto exit;
	}

	/* Client then server ephemeral pub key */
	for (int i = 0; i < 2; i++) {
		if ((transport->server && i == 0) || (!transport->server && i == 1)) {
			/* Remote key, first if we are a server otherwise second */
			sys_put_be32(remote_ephemeral_key->len, buf);
			if (psa_hash_update(&hash_op, buf, sizeof(buf)) != PSA_SUCCESS) {
				ret = -EIO;
				goto exit;
			}

			if (psa_hash_update(&hash_op, remote_ephemeral_key->data,
					    remote_ephemeral_key->len) != PSA_SUCCESS) {
				ret = -EIO;
				goto exit;
			}
		} else {
			/* Local key, first if we are a client otherwise second */
			uint8_t pubkey_buff[32];
			size_t pubkey_len;

			status = psa_export_public_key(
				transport->ecdh_ephemeral_key,
				pubkey_buff, sizeof(pubkey_buff), &pubkey_len);
			if (status != PSA_SUCCESS) {
				NET_ERR("Failed to write pubkey: %d", status);
				ret = -EIO;
				goto exit;
			}

			sys_put_be32(pubkey_len, buf);

			if (psa_hash_update(&hash_op, buf, sizeof(buf)) != PSA_SUCCESS) {
				ret = -EIO;
				goto exit;
			}

			if (psa_hash_update(&hash_op, pubkey_buff, pubkey_len) != PSA_SUCCESS) {
				ret = -EIO;
				goto exit;
			}
		}
	}

	/* mpint K, shared secret */
	payload.len = 0;

	if (!ssh_payload_write_mpint(&payload, transport->shared_secret->data,
				     transport->shared_secret->len, false)) {
		ret = -ENOBUFS;
		goto exit;
	}

	if (psa_hash_update(&hash_op, payload.data, payload.len) != PSA_SUCCESS) {
		ret = -EIO;
		goto exit;
	}

	transport->exchange_hash = ssh_payload_string_alloc(&transport->kex_heap, NULL, 32);
	if (transport->exchange_hash == NULL) {
		ret = -ENOMEM;
		goto exit;
	}

	if (psa_hash_finish(&hash_op, (void *)transport->exchange_hash->data, 32,
			    &hash_len) != PSA_SUCCESS) {
		ret = -EIO;
		goto exit;
	}

	NET_HEXDUMP_DBG(transport->exchange_hash->data,
			transport->exchange_hash->len, "exchange_hash");

	ret = 0;

exit:
	psa_hash_abort(&hash_op);

	return ret;
}

static int gen_all_keys(struct ssh_transport *transport)
{
	/*
	 * o  Initial IV client to server: HASH(K || H || "A" || session_id)
	 *      (Here K is encoded as mpint and "A" as byte and session_id as raw
	 *      data.  "A" means the single character A, ASCII 65).
	 * o  Initial IV server to client: HASH(K || H || "B" || session_id)
	 * o  Encryption key client to server: HASH(K || H || "C" || session_id)
	 * o  Encryption key server to client: HASH(K || H || "D" || session_id)
	 * o  Integrity key client to server: HASH(K || H || "E" || session_id)
	 * o  Integrity key server to client: HASH(K || H || "F" || session_id)
	 */

	struct {
		char c;
		uint8_t *buf;
		size_t len;
	} keygen[] = {
		{
			.c = 'A',
			.buf = transport->iv_client,
			.len = sizeof(transport->iv_client)
		},
		{
			.c = 'B',
			.buf = transport->iv_server,
			.len = sizeof(transport->iv_server)
		},
		{
			.c = 'C',
			.buf = transport->encrypt_key_client,
			.len = sizeof(transport->encrypt_key_client)
		},
		{
			.c = 'D',
			.buf = transport->encrypt_key_server,
			.len = sizeof(transport->encrypt_key_server)
		},
		{
			.c = 'E',
			.buf = transport->integ_key_client,
			.len = sizeof(transport->integ_key_client)
		},
		{
			.c = 'F',
			.buf = transport->integ_key_server,
			.len = sizeof(transport->integ_key_server)
		},
	};

	for (unsigned int i = 0; i < ARRAY_SIZE(keygen); i++) {
		int ret;

		ret = gen_key(transport, keygen[i].c, keygen[i].buf, keygen[i].len);
		if (ret != 0) {
			NET_ERR("Failed to generate keys");
			return ret;
		}

		NET_DBG("Key %d (%c)", i, keygen[i].c);
		NET_HEXDUMP_DBG(keygen[i].buf, keygen[i].len, "key");
	}

	return 0;
}

static int gen_key(struct ssh_transport *transport, char c, uint8_t *buf, size_t len)
{
	int ret = 0;
	uint8_t hash[32];
	psa_hash_operation_t hash_op = PSA_HASH_OPERATION_INIT;
	size_t offset = 0;

	SSH_PAYLOAD_BUF(payload, transport->tx_buf);

	/* mpint K, shared secret */
	if (!ssh_payload_write_mpint(&payload, transport->shared_secret->data,
				     transport->shared_secret->len, false)) {
		ret = -ENOBUFS;
		goto exit;
	}

	while (offset < len) {
		size_t hash_len;
		size_t copy_len;

		hash_op = psa_hash_operation_init();
		if (psa_hash_setup(&hash_op, PSA_ALG_SHA_256) != PSA_SUCCESS) {
			ret = -EIO;
			goto exit;
		}

		if (psa_hash_update(&hash_op, payload.data, payload.len) != PSA_SUCCESS) {
			ret = -EIO;
			goto exit;
		}

		if (psa_hash_update(&hash_op, transport->exchange_hash->data,
				    transport->exchange_hash->len) != PSA_SUCCESS) {
			ret = -EIO;
			goto exit;
		}

		if (offset == 0) {
			if (psa_hash_update(&hash_op, &c, 1) != PSA_SUCCESS) {
				ret = -EIO;
				goto exit;
			}

			if (psa_hash_update(&hash_op, transport->session_id->data,
					    transport->session_id->len) != PSA_SUCCESS) {
				ret = -EIO;
				goto exit;
			}
		} else {
			if (psa_hash_update(&hash_op, buf, offset) != PSA_SUCCESS) {
				ret = -EIO;
				goto exit;
			}
		}

		if (psa_hash_finish(&hash_op, hash, sizeof(hash), &hash_len) != PSA_SUCCESS) {
			ret = -EIO;
			goto exit;
		}

		copy_len = MIN(len - offset, sizeof(hash));
		memcpy(&buf[offset], hash, copy_len);
		offset += copy_len;
	}

exit:
	psa_hash_abort(&hash_op);

	return ret;
}
