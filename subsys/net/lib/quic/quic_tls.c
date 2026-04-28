/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* TLS Alert levels */
#define TLS_ALERT_LEVEL_WARNING	1
#define TLS_ALERT_LEVEL_FATAL	2

/* TLS Alert descriptions. Used for QUIC error codes (0x100 + alert) per RFC 9001 */
#define TLS_ALERT_CLOSE_NOTIFY			0
#define TLS_ALERT_UNEXPECTED_MESSAGE		10
#define TLS_ALERT_BAD_RECORD_MAC		20
#define TLS_ALERT_HANDSHAKE_FAILURE		40
#define TLS_ALERT_CERTIFICATE_REQUIRED		116
#define TLS_ALERT_NO_APPLICATION_PROTOCOL	120

static int build_default_transport_params(struct quic_tls_context *ctx);

/**
 * Add intermediate certificate to chain by sec_tag
 */
static int quic_tls_add_cert_chain(struct quic_tls_context *ctx,
				   sec_tag_t tag)
{
	if (ctx == NULL) {
		return -EINVAL;
	}

	if (ctx->cert_chain_count >= CONFIG_QUIC_TLS_MAX_CERT_CHAIN_DEPTH) {
		return -ENOBUFS;
	}

	for (size_t i = 0; i < ctx->cert_chain_count; i++) {
		if (ctx->cert_chain_tags[i] == tag) {
			/* Tag already in chain, ignore */
			return 0;
		}
	}

	ctx->cert_chain_tags[ctx->cert_chain_count] = tag;
	ctx->cert_chain_count++;

	return 0;
}

/**
 * Delete intermediate certificate from chain by sec_tag
 */
static int quic_tls_del_cert_chain(struct quic_tls_context *ctx,
				   const sec_tag_t *tag)
{
	if (ctx == NULL) {
		return -EINVAL;
	}

	if (tag == NULL) {
		/* Clear entire chain if tag is NULL */
		ctx->cert_chain_count = 0;
		return 0;
	}

	/* Find and remove the specified tag */
	for (size_t i = 0; i < ctx->cert_chain_count; i++) {
		if (ctx->cert_chain_tags[i] == *tag) {
			/* Shift remaining tags down */
			for (size_t j = i; j < ctx->cert_chain_count - 1; j++) {
				ctx->cert_chain_tags[j] = ctx->cert_chain_tags[j + 1];
			}

			ctx->cert_chain_count--;
			return 0;
		}
	}

	return -ENOENT;
}

#if defined(MBEDTLS_PK_PARSE_C)
static bool check_key_type(const mbedtls_pk_context *pk)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_type_t key_type;
	psa_algorithm_t alg;
	bool st = false;
	int ret;

	ret = mbedtls_pk_get_psa_attributes(pk, PSA_KEY_USAGE_SIGN_HASH, &attributes);
	if (ret != 0) {
		/* key doesn't support signing at all */
		goto error;
	}

	key_type = psa_get_key_type(&attributes);
	alg = psa_get_key_algorithm(&attributes);

	if (!PSA_KEY_TYPE_IS_ECC(key_type) || !PSA_ALG_IS_ECDSA(alg)) {
		/* not an ECDSA-capable key */
		psa_reset_key_attributes(&attributes);
		goto error;
	}

	psa_reset_key_attributes(&attributes);
	st = true;

error:
	return st;
}
#endif /* MBEDTLS_PK_PARSE_C */

/*
 * Set own certificate and private key for TLS authentication
 *
 * @param ctx TLS context
 * @param cert DER or PEM encoded certificate
 * @param cert_len Length of certificate data
 * @param key DER or PEM encoded private key
 * @param key_len Length of key data
 *
 * @return 0 on success, negative on error
 */
static int quic_tls_set_own_cert(struct quic_tls_context *ctx,
				 const uint8_t *cert, size_t cert_len,
				 const uint8_t *key, size_t key_len)
{
	int ret;

	if (ctx == NULL || cert == NULL || cert_len == 0) {
		return -EINVAL;
	}

	/* Parse and validate certificate using mbedtls */

	/* Import private key if provided */
	if (key != NULL && key_len > 0) {
#if defined(MBEDTLS_PK_PARSE_C)
		mbedtls_pk_context pk;
		psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
		psa_status_t status;

		mbedtls_pk_init(&pk);

		ret = mbedtls_pk_parse_key(&pk, key, key_len, NULL, 0);
		if (ret != 0) {
			NET_DBG("Failed to parse private key: -0x%04x", -ret);
			mbedtls_pk_free(&pk);
			return -EINVAL;
		}

		if (!check_key_type(&pk)) {
			NET_DBG("Private key must be ECDSA for TLS 1.3");
			mbedtls_pk_free(&pk);
			return -EINVAL;
		}

		/* Set up PSA attributes for the import */
		psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_MESSAGE |
					       PSA_KEY_USAGE_SIGN_HASH);
		psa_set_key_algorithm(&attr, PSA_ALG_ECDSA(PSA_ALG_ANY_HASH));
		psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
		psa_set_key_bits(&attr, 256);

		/* Directly import pk context into PSA, no raw key buffer needed */
		status = mbedtls_pk_import_into_psa(&pk, &attr, &ctx->signing_key_id);

		psa_reset_key_attributes(&attr);
		mbedtls_pk_free(&pk);

		if (status != PSA_SUCCESS) {
			NET_DBG("Failed to import signing key: %d", status);
			return -EIO;
		}

		NET_DBG("Signing key imported successfully, key_id=%u",
			ctx->signing_key_id);
#endif /* MBEDTLS_PK_PARSE_C */
	}

	return 0;
}

/*
 * Verify peer certificate using mbedtls
 * Called when processing Certificate message from peer
 */
static int verify_peer_certificate(struct quic_tls_context *ctx,
				   const uint8_t *cert_data, size_t cert_len)
{
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	mbedtls_x509_crt peer_crt;
	uint32_t flags = 0;
	int ret;
	psa_key_attributes_t pk_attr = PSA_KEY_ATTRIBUTES_INIT;
	mbedtls_svc_key_id_t pub_key_id = MBEDTLS_SVC_KEY_ID_INIT;
	psa_status_t status;
	size_t pubkey_len = 0;

	mbedtls_x509_crt_init(&peer_crt);

	/* Parse peer certificate */
	ret = mbedtls_x509_crt_parse_der(&peer_crt, cert_data, cert_len);
	if (ret != 0) {
		NET_DBG("Failed to parse peer certificate: -0x%04x", -ret);
		ret = -EINVAL;
		goto out;
	}

	/* Verify against CA chain if available */
	if (ctx->ca_cert) {
		ret = mbedtls_x509_crt_verify(&peer_crt, &ctx->ca_chain, NULL,
					      NULL, &flags, NULL, NULL);
		if (ret != 0) {
			NET_WARN("Certificate verification failed: -0x%04x, flags=0x%08x",
				 -ret, flags);

			/* Check verification level */
			if (ctx->options.verify_level > 0) {
				ret = -EACCES;
				goto out;
			}
			/* If verify_level is 0, continue despite errors */
		}
	} else {
		NET_WARN("No CA certificate configured, skipping verification");
	}

	/* Extract peer's public key for signature verification */
	status = mbedtls_pk_get_psa_attributes(&peer_crt.pk,
					       PSA_KEY_USAGE_VERIFY_HASH,
					       &pk_attr);
	if (status != PSA_SUCCESS) {
		NET_DBG("Failed to get PSA attributes from peer cert pk: %d", status);
		goto out;
	}

	/* Check it's actually an ECC key */
	if (!PSA_KEY_TYPE_IS_ECC(psa_get_key_type(&pk_attr))) {
		NET_DBG("Peer certificate public key is not ECC");
		psa_reset_key_attributes(&pk_attr);
		goto out;
	}

	status = mbedtls_pk_import_into_psa(&peer_crt.pk, &pk_attr, &pub_key_id);
	psa_reset_key_attributes(&pk_attr);
	if (status != PSA_SUCCESS) {
		NET_DBG("Failed to import peer public key into PSA: %d", status);
		goto out;
	}

	/* psa_export_public_key gives uncompressed point: 04 || X || Y */
	status = psa_export_public_key(pub_key_id,
				       ctx->peer_public_key,
				       sizeof(ctx->peer_public_key),
				       &pubkey_len);
	psa_destroy_key(pub_key_id);

	if (status != PSA_SUCCESS) {
		NET_DBG("Failed to export peer public key: %d", status);
		goto out;
	}

	ctx->peer_public_key_len = pubkey_len;
	NET_DBG("Extracted peer public key, len=%zu", pubkey_len);

out:
	mbedtls_x509_crt_free(&peer_crt);
	return ret;
#else
	return -ENOTSUP;
#endif
}

/* Helper to update transcript hash */
static int transcript_update(struct quic_tls_context *ctx,
			     const uint8_t *data, size_t len)
{
	psa_status_t status;

	/* Also keep raw copy for debugging/verification */
	if (ctx->transcript_len + len > sizeof(ctx->transcript_buffer)) {
		NET_DBG("Transcript buffer overflow");
		return -ENOBUFS;
	}

	memcpy(&ctx->transcript_buffer[ctx->transcript_len], data, len);
	ctx->transcript_len += len;

	status = psa_hash_update(&ctx->ks.transcript_hash, data, len);
	if (status != PSA_SUCCESS) {
		NET_DBG("Failed to update transcript hash (%d)", status);
		return -EIO;
	}

	return 0;
}

/* Get current transcript hash value */
static int transcript_hash_get(struct quic_tls_context *ctx,
			       uint8_t *hash, size_t *hash_len)
{
	psa_hash_operation_t clone = PSA_HASH_OPERATION_INIT;
	psa_status_t status;

	/* Clone the operation to get intermediate hash */
	status = psa_hash_clone(&ctx->ks.transcript_hash, &clone);
	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	status = psa_hash_finish(&clone, hash, ctx->ks.hash_len, hash_len);
	if (status != PSA_SUCCESS) {
		psa_hash_abort(&clone);
		return -EIO;
	}

	return 0;
}

/*
 * Derive-Secret (RFC 8446 Section 7.1)
 * Derive-Secret(Secret, Label, Messages) =
 *     HKDF-Expand-Label(Secret, Label, Transcript-Hash(Messages), Hash.length)
 */
static int quic_derive_secret(struct quic_tls_context *ctx,
			      const uint8_t *secret,
			      const char *label,
			      uint8_t *out)
{
	uint8_t transcript[QUIC_HASH_MAX_LEN];
	size_t transcript_len;
	int ret;

	ret = transcript_hash_get(ctx, transcript, &transcript_len);
	if (ret != 0) {
		return ret;
	}

	return quic_hkdf_expand_label_ex(ctx->ks.hash_alg,
					 secret, ctx->ks.hash_len,
					 (const uint8_t *)label, strlen(label),
					 transcript, transcript_len,
					 out, ctx->ks.hash_len);
}

/* Derive secret using a provided transcript hash (not the current one) */
static int quic_derive_secret_ex(struct quic_tls_context *ctx,
				 const uint8_t *secret,
				 const char *label,
				 const uint8_t *transcript,
				 size_t transcript_len,
				 uint8_t *out)
{
	return quic_hkdf_expand_label_ex(ctx->ks.hash_alg,
					 secret, ctx->ks.hash_len,
					 (const uint8_t *)label, strlen(label),
					 transcript, transcript_len,
					 out, ctx->ks.hash_len);
}

/*
 * Generate ECDH key pair
 */
static int generate_ecdh_keypair(struct quic_tls_context *ctx)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;
	size_t pubkey_len;
	uint16_t group = ctx->ks.key_exchange_group;

	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attr, PSA_ALG_ECDH);

	if (group == MBEDTLS_SSL_IANA_TLS_GROUP_X25519) {
		NET_DBG("Setting up x25519 key (Montgomery, 255 bits)");
		psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY));
		psa_set_key_bits(&attr, 255);
	} else if (group == MBEDTLS_SSL_IANA_TLS_GROUP_SECP256R1) {
		NET_DBG("Setting up secp256r1 key (SECP_R1, 256 bits)");
		psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
		psa_set_key_bits(&attr, 256);
	} else {
		NET_DBG("Unknown group 0x%04x, defaulting to secp256r1", group);
		/* Default to secp256r1 for client-initiated connections */
		psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
		psa_set_key_bits(&attr, 256);
		ctx->ks.key_exchange_group = MBEDTLS_SSL_IANA_TLS_GROUP_SECP256R1;
	}

	status = psa_generate_key(&attr, &ctx->ecdh_key_id);
	if (status != PSA_SUCCESS) {
		NET_DBG("Failed to generate ECDH key (%d)", status);
		return -EIO;
	}

	NET_DBG("Generated key with id=%u", (unsigned int)ctx->ecdh_key_id);

	/* Export public key */
	status = psa_export_public_key(ctx->ecdh_key_id,
				       ctx->ecdh_public_key,
				       sizeof(ctx->ecdh_public_key),
				       &pubkey_len);
	if (status != PSA_SUCCESS) {
		NET_DBG("Failed to export public key (%d)", status);
		psa_destroy_key(ctx->ecdh_key_id);
		return -EIO;
	}

	ctx->ecdh_public_key_len = pubkey_len;
	NET_DBG("Generated ECDH keypair for group 0x%04x, pubkey len %zu",
		ctx->ks.key_exchange_group, pubkey_len);

	return 0;
}

/*
 * Compute ECDH shared secret
 */
static int compute_shared_secret(struct quic_tls_context *ctx)
{
	psa_status_t status;
	size_t secret_len;

	NET_DBG("Computing shared secret: our key_id=%u, peer_pubkey_len=%zu, group=0x%04x",
		(unsigned int)ctx->ecdh_key_id, ctx->peer_public_key_len,
		ctx->ks.key_exchange_group);

	if (ctx->peer_public_key_len == 0) {
		NET_DBG("Peer public key not set!");
		return -EINVAL;
	}

	status = psa_raw_key_agreement(PSA_ALG_ECDH,
				       ctx->ecdh_key_id,
				       ctx->peer_public_key,
				       ctx->peer_public_key_len,
				       ctx->shared_secret,
				       sizeof(ctx->shared_secret),
				       &secret_len);
	if (status != PSA_SUCCESS) {
		NET_DBG("ECDH key agreement failed (%d)", status);
		return -EIO;
	}

	if (IS_ENABLED(CONFIG_QUIC_TXRX_DEBUG)) {
		NET_HEXDUMP_DBG(ctx->shared_secret, secret_len, "ECDH shared secret:");
	}

	return 0;
}

static const char *cipher_suite_name(uint16_t suite)
{
	if (suite == TLS_AES_128_GCM_SHA256) {
		return "TLS_AES_128_GCM_SHA256";
	} else if (suite == TLS_AES_256_GCM_SHA384) {
		return "TLS_AES_256_GCM_SHA384";
	} else if (suite == TLS_CHACHA20_POLY1305_SHA256) {
		return "TLS_CHACHA20_POLY1305_SHA256";
	} else {
		return "Unknown";
	}
}

static const char *key_group_name(uint16_t group)
{
	if (group == MBEDTLS_SSL_IANA_TLS_GROUP_SECP256R1) {
		return "secp256r1";
	} else if (group == MBEDTLS_SSL_IANA_TLS_GROUP_SECP384R1) {
		return "secp384r1";
	} else if (group == MBEDTLS_SSL_IANA_TLS_GROUP_SECP521R1) {
		return "secp521r1";
	} else if (group == MBEDTLS_SSL_IANA_TLS_GROUP_X25519) {
		return "x25519";
	} else if (group == MBEDTLS_SSL_IANA_TLS_GROUP_X448) {
		return "x448";
	} else {
		return "Unknown";
	}
}

static void check_alpn_list(struct quic_tls_context *ctx,
			    const uint8_t *data,
			    size_t alpn_pos,
			    uint8_t proto_len)
{
	for (int i = 0; ctx->options.alpn_list[i] != NULL; i++) {
		size_t our_len = strlen(ctx->options.alpn_list[i]);

		if (our_len == proto_len &&
		    memcmp(&data[alpn_pos], ctx->options.alpn_list[i], proto_len) == 0) {
			ctx->negotiated_alpn = ctx->options.alpn_list[i];

			if (IS_ENABLED(CONFIG_QUIC_LOG_LEVEL_DBG)) {
				struct quic_endpoint *ep = CONTAINER_OF(ctx,
									struct quic_endpoint,
									crypto.tls);
				struct quic_context *quic_ctx = quic_find_context(ep);

				NET_DBG("[%p] ALPN negotiated: %s",
					quic_ctx, ctx->negotiated_alpn);
			}

			break;
		}
	}
}

static bool handle_key_share_ext(struct quic_tls_context *ctx,
				 uint16_t group,
				 const uint8_t *key,
				 size_t key_len,
				 size_t shares_pos,
				 size_t shares_end)
{
	/* Support both x25519 (preferred) and secp256r1 */
	if (group == MBEDTLS_SSL_IANA_TLS_GROUP_X25519) {
		/* x25519 key is always 32 bytes */
		if (key_len == 32 &&
		    shares_pos + 4 + key_len <= shares_end) {
			memcpy(ctx->peer_public_key, key, key_len);
			ctx->peer_public_key_len = key_len;
			ctx->ks.key_exchange_group = group;
			NET_DBG("Selected x25519 key exchange");
			return true;
		}
	} else if (group == MBEDTLS_SSL_IANA_TLS_GROUP_SECP256R1) {
		/* secp256r1 uncompressed point is 65 bytes (04 || x || y) */
		if (key_len <= sizeof(ctx->peer_public_key) &&
		    shares_pos + 4 + key_len <= shares_end) {
			memcpy(ctx->peer_public_key, key, key_len);
			ctx->peer_public_key_len = key_len;
			ctx->ks.key_exchange_group = group;
			NET_DBG("Selected secp256r1 key exchange");
			return true;
		}
	}

	return false;
}

/*
 * Parse ClientHello message
 */
static int parse_client_hello(struct quic_tls_context *ctx,
			      const uint8_t *data, size_t len)
{
#define MIN_TLS_CLIENT_HELLO_SIZE 38
	size_t pos = 0;
	uint16_t legacy_version;
	uint8_t session_id_len;
	uint16_t cipher_suites_len;
	uint8_t compression_len;
	uint16_t extensions_len;
	size_t ext_end;

	if (len < MIN_TLS_CLIENT_HELLO_SIZE) {
		return -EINVAL;
	}

	/* Legacy version (2 bytes), should be 0x0303 for TLS 1.2 */
	legacy_version = (data[pos] << 8) | data[pos + 1];
	pos += 2;

	if (legacy_version != 0x0303) {
		NET_WARN("Unexpected legacy version: 0x%04x", legacy_version);
		return -EINVAL;
	}

	/* Client random (32 bytes) */
	memcpy(ctx->client_random, &data[pos], 32);
	pos += 32;

	/* Legacy session ID */
	session_id_len = data[pos++];
	if (pos + session_id_len > len) {
		return -EINVAL;
	}
	pos += session_id_len;

	/* Cipher suites */
	cipher_suites_len = (data[pos] << 8) | data[pos + 1];
	pos += 2;

	if (pos + cipher_suites_len > len) {
		return -EINVAL;
	}

	/* Find a supported cipher suite */
	ctx->ks.cipher_suite = 0;
	for (size_t i = 0; i < cipher_suites_len; i += 2) {
		uint16_t suite = (data[pos + i] << 8) | data[pos + i + 1];

		if (suite == TLS_AES_128_GCM_SHA256 ||
		    suite == TLS_AES_256_GCM_SHA384 ||
		    suite == TLS_CHACHA20_POLY1305_SHA256) {
			ctx->ks.cipher_suite = suite;
			break;
		}
	}

	if (ctx->ks.cipher_suite == 0) {
		NET_DBG("No supported cipher suite found");
		return -ENOTSUP;
	}

	NET_DBG("Selected cipher suite %s (0x%04x)",
		cipher_suite_name(ctx->ks.cipher_suite),
		ctx->ks.cipher_suite);

	pos += cipher_suites_len;

	/* Legacy compression methods */
	compression_len = data[pos++];
	pos += compression_len;

	/* Extensions */
	if (pos + 2 > len) {
		return -EINVAL;
	}

	extensions_len = (data[pos] << 8) | data[pos + 1];
	pos += 2;

	/* Parse extensions */
	ext_end = pos + extensions_len;

	while (pos + 4 <= ext_end) {
		uint16_t ext_type = (data[pos] << 8) | data[pos + 1];
		uint16_t ext_len = (data[pos + 2] << 8) | data[pos + 3];

		pos += 4;

		if (pos + ext_len > ext_end) {
			break;
		}

		switch (ext_type) {
		case TLS_EXT_ALPN:
			NET_DBG("ALPN extension present, len: %d", ext_len);
			/* Parse ALPN extension and select matching protocol */
			if (ext_len >= 2) {
				uint16_t alpn_list_len = (data[pos] << 8) | data[pos + 1];
				size_t alpn_pos = pos + 2;
				size_t alpn_end = pos + 2 + alpn_list_len;

				if (alpn_end > pos + ext_len) {
					alpn_end = pos + ext_len;
				}

				/* Iterate through client's ALPN list */
				while (alpn_pos < alpn_end) {
					uint8_t proto_len = data[alpn_pos++];

					if (alpn_pos + proto_len > alpn_end) {
						break;
					}

					/* Check against our configured ALPN list */
					check_alpn_list(ctx, data, alpn_pos, proto_len);

					if (ctx->negotiated_alpn != NULL) {
						break;
					}

					alpn_pos += proto_len;
				}
			}
			break;

		case TLS_EXT_SUPPORTED_VERSIONS:
			if (ext_len >= 3) {
				size_t ver_pos = pos + 1;
				uint16_t version = (data[ver_pos] << 8) | data[ver_pos + 1];

				if (version != 0x0304) {  /* TLS 1.3 */
					NET_DBG("Unsupported TLS version: 0x%04x", version);
					return -ENOTSUP;
				}

				NET_DBG("Supported version %s (0x%04x)", "TLS1.3", version);
			}
			break;

		case TLS_EXT_KEY_SHARE:
			/* Parse key share extension to get peer's public key */
			if (ext_len >= 4) {
				/* First 2 bytes:  client_shares length */
				size_t shares_pos = pos + 2;
				size_t shares_end = pos + ext_len;

				while (shares_pos + 4 <= shares_end) {
					uint16_t group = (data[shares_pos] << 8) |
							 data[shares_pos + 1];
					uint16_t key_len = (data[shares_pos + 2] << 8) |
							   data[shares_pos + 3];

					NET_DBG("Key share group %s (0x%04x), len: %d",
						key_group_name(group), group, key_len);

					if (handle_key_share_ext(ctx, group, &data[shares_pos + 4],
								 key_len, shares_pos, shares_end)) {
						break;
					}

					shares_pos += 4 + key_len;
				}
			}
			break;

		case TLS_EXT_QUIC_TRANSPORT_PARAMS:
			if (ext_len <= sizeof(ctx->peer_tp)) {
				NET_DBG("QUIC TP extension present, len: %d", ext_len);
				memcpy(ctx->peer_tp, &data[pos], ext_len);
				ctx->peer_tp_len = ext_len;
			}
			break;

		default:
			break;
		}

		pos += ext_len;
	}

	/* If server has ALPN configured but no protocol matched, reject */
	if (ctx->options.alpn_list[0] != NULL && ctx->negotiated_alpn == NULL) {
		NET_DBG("No matching ALPN protocol found");
		return -ENOPROTOOPT;  /* Specific error for ALPN mismatch */
	}

	return 0;
}

/*
 * Build ClientHello message (TLS 1.3 format for QUIC)
 *
 * ClientHello {
 *   ProtocolVersion legacy_version = 0x0303;    // TLS 1.2
 *   Random random;
 *   opaque legacy_session_id<0..32>;            // empty for QUIC
 *   CipherSuite cipher_suites<2..2^16-2>;
 *   opaque legacy_compression_methods<1..2^8-1>; // single null byte
 *   Extension extensions<8..2^16-1>;
 * }
 */
static int build_client_hello(struct quic_tls_context *ctx,
			      uint8_t *buf, size_t buf_size, size_t *out_len)
{
	size_t pos = 0;
	size_t ext_start;
	size_t ext_len;
	size_t ks_ext_len;
	int ret;

	if (buf_size < 256) {
		return -ENOBUFS;
	}

	/* Legacy version: TLS 1.2 (0x0303) */
	buf[pos++] = 0x03;
	buf[pos++] = 0x03;

	/* Client random (32 bytes) */
	sys_rand_get(ctx->client_random, 32);
	memcpy(&buf[pos], ctx->client_random, 32);
	pos += 32;

	/* Legacy session ID (empty for QUIC) */
	buf[pos++] = 0;

	/* Cipher suites (2 bytes length + suites) */
	buf[pos++] = 0x00;
	buf[pos++] = 0x06;  /* 3 cipher suites = 6 bytes */
	buf[pos++] = (TLS_AES_128_GCM_SHA256 >> 8) & 0xFF;
	buf[pos++] = TLS_AES_128_GCM_SHA256 & 0xFF;
	buf[pos++] = (TLS_AES_256_GCM_SHA384 >> 8) & 0xFF;
	buf[pos++] = TLS_AES_256_GCM_SHA384 & 0xFF;
	buf[pos++] = (TLS_CHACHA20_POLY1305_SHA256 >> 8) & 0xFF;
	buf[pos++] = TLS_CHACHA20_POLY1305_SHA256 & 0xFF;

	/* Legacy compression methods (single null byte) */
	buf[pos++] = 0x01;  /* length */
	buf[pos++] = 0x00;  /* null compression */

	/* Extensions */
	ext_start = pos;
	pos += 2;  /* Reserve for extensions length */

	/* supported_versions extension (mandatory for TLS 1.3) */
	buf[pos++] = 0x00;
	buf[pos++] = 0x2b;  /* Extension type: supported_versions */
	buf[pos++] = 0x00;
	buf[pos++] = 0x03;  /* Extension length */
	buf[pos++] = 0x02;  /* Supported versions length */
	buf[pos++] = 0x03;
	buf[pos++] = 0x04;  /* TLS 1.3 */

	/* signature_algorithms extension */
	buf[pos++] = 0x00;
	buf[pos++] = 0x0d;  /* Extension type: signature_algorithms */
	buf[pos++] = 0x00;
	buf[pos++] = 0x04;  /* Extension length */
	buf[pos++] = 0x00;
	buf[pos++] = 0x02;  /* Algorithms length */
	buf[pos++] = 0x04;
	buf[pos++] = 0x03;  /* ecdsa_secp256r1_sha256 */

	/* supported_groups extension */
	buf[pos++] = 0x00;
	buf[pos++] = 0x0a;  /* Extension type: supported_groups */
	buf[pos++] = 0x00;
	buf[pos++] = 0x04;  /* Extension length */
	buf[pos++] = 0x00;
	buf[pos++] = 0x02;  /* Groups length */
	buf[pos++] = 0x00;
	buf[pos++] = 0x17;  /* secp256r1 */

	/* Generate ECDH key pair for key_share */
	ret = generate_ecdh_keypair(ctx);
	if (ret != 0) {
		return ret;
	}

	/* key_share extension */
	buf[pos++] = 0x00;
	buf[pos++] = 0x33;  /* Extension type: key_share */
	ks_ext_len = 2 + 2 + 2 + ctx->ecdh_public_key_len;  /* list len + group + key len + key */
	buf[pos++] = (ks_ext_len >> 8) & 0xFF;
	buf[pos++] = ks_ext_len & 0xFF;
	/* Client key share list length */
	buf[pos++] = ((2 + 2 + ctx->ecdh_public_key_len) >> 8) & 0xFF;
	buf[pos++] = (2 + 2 + ctx->ecdh_public_key_len) & 0xFF;
	/* Key share entry */
	buf[pos++] = 0x00;
	buf[pos++] = 0x17;  /* secp256r1 */
	buf[pos++] = (ctx->ecdh_public_key_len >> 8) & 0xFF;
	buf[pos++] = ctx->ecdh_public_key_len & 0xFF;
	memcpy(&buf[pos], ctx->ecdh_public_key, ctx->ecdh_public_key_len);
	pos += ctx->ecdh_public_key_len;

	/* ALPN extension (if configured) */
	if (ctx->options.alpn_list[0] != NULL) {
		size_t alpn_list_len = 0;

		/* Calculate total ALPN list length */
		for (int i = 0; ctx->options.alpn_list[i] != NULL; i++) {
			alpn_list_len += 1 + strlen(ctx->options.alpn_list[i]);
		}

		buf[pos++] = 0x00;
		buf[pos++] = 0x10;  /* Extension type: ALPN */
		buf[pos++] = ((alpn_list_len + 2) >> 8) & 0xFF;
		buf[pos++] = (alpn_list_len + 2) & 0xFF;
		buf[pos++] = (alpn_list_len >> 8) & 0xFF;
		buf[pos++] = alpn_list_len & 0xFF;

		for (int i = 0; ctx->options.alpn_list[i] != NULL; i++) {
			size_t proto_len = strlen(ctx->options.alpn_list[i]);

			buf[pos++] = proto_len;
			memcpy(&buf[pos], ctx->options.alpn_list[i], proto_len);
			pos += proto_len;
		}
	}

	/* Build transport parameters if not already set */
	if (ctx->local_tp_len == 0) {
		ret = build_default_transport_params(ctx);
		if (ret != 0) {
			return ret;
		}
	}

	/* QUIC transport parameters extension */
	buf[pos++] = 0x00;
	buf[pos++] = 0x39;  /* Extension type: quic_transport_parameters */
	buf[pos++] = (ctx->local_tp_len >> 8) & 0xFF;
	buf[pos++] = ctx->local_tp_len & 0xFF;
	memcpy(&buf[pos], ctx->local_tp, ctx->local_tp_len);
	pos += ctx->local_tp_len;

	/* Fill in extensions length */
	ext_len = pos - ext_start - 2;
	buf[ext_start] = (ext_len >> 8) & 0xFF;
	buf[ext_start + 1] = ext_len & 0xFF;

	*out_len = pos;

	NET_DBG("Built ClientHello, %zu bytes", pos);

	return 0;
}

/*
 * Build ServerHello message
 */
static int build_server_hello(struct quic_tls_context *ctx,
			      uint8_t *buf, size_t buf_size, size_t *out_len)
{
	size_t pos = 0;
	size_t ext_start;
	size_t ks_len;
	size_t ext_len;

	if (buf_size < 128) {
		return -ENOBUFS;
	}

	/* Legacy version: TLS 1.2 */
	buf[pos++] = 0x03;
	buf[pos++] = 0x03;

	/* Server random */
	sys_rand_get(ctx->server_random, 32);
	memcpy(&buf[pos], ctx->server_random, 32);
	pos += 32;

	/* Legacy session ID echo (empty for QUIC) */
	buf[pos++] = 0;

	/* Cipher suite */
	buf[pos++] = (ctx->ks.cipher_suite >> 8) & 0xFF;
	buf[pos++] = ctx->ks.cipher_suite & 0xFF;

	/* Legacy compression method */
	buf[pos++] = 0;

	/* Extensions */
	ext_start = pos;
	pos += 2;  /* Reserve for extensions length */

	/* supported_versions extension (mandatory for TLS 1.3) */
	buf[pos++] = 0x00;
	buf[pos++] = 0x2b;  /* Extension type: supported_versions */
	buf[pos++] = 0x00;
	buf[pos++] = 0x02;  /* Extension length */
	buf[pos++] = 0x03;
	buf[pos++] = 0x04;  /* TLS 1.3 */

	/* key_share extension */
	buf[pos++] = 0x00;
	buf[pos++] = 0x33;  /* Extension type: key_share */

	ks_len = 2 + 2 + ctx->ecdh_public_key_len;
	buf[pos++] = (ks_len >> 8) & 0xFF;
	buf[pos++] = ks_len & 0xFF;
	buf[pos++] = (ctx->ks.key_exchange_group >> 8) & 0xFF;
	buf[pos++] = ctx->ks.key_exchange_group & 0xFF;
	buf[pos++] = (ctx->ecdh_public_key_len >> 8) & 0xFF;
	buf[pos++] = ctx->ecdh_public_key_len & 0xFF;
	memcpy(&buf[pos], ctx->ecdh_public_key, ctx->ecdh_public_key_len);
	pos += ctx->ecdh_public_key_len;

	/* Fill in extensions length */
	ext_len = pos - ext_start - 2;
	buf[ext_start] = (ext_len >> 8) & 0xFF;
	buf[ext_start + 1] = ext_len & 0xFF;

	*out_len = pos;

	return 0;
}

/*
 * Wrap message with handshake header
 */
static int wrap_handshake_message(uint8_t msg_type,
				  const uint8_t *msg, size_t msg_len,
				  uint8_t *out, size_t out_size, size_t *out_len)
{
	if (out_size < msg_len + 4) {
		return -ENOBUFS;
	}

	out[0] = msg_type;
	out[1] = (msg_len >> 16) & 0xFF;
	out[2] = (msg_len >> 8) & 0xFF;
	out[3] = msg_len & 0xFF;
	memcpy(&out[4], msg, msg_len);

	*out_len = msg_len + 4;

	return 0;
}

/*
 * Initialize key schedule after cipher suite selection
 */
static int key_schedule_init(struct quic_tls_context *ctx)
{
	psa_status_t status;
	uint8_t zero_psk[QUIC_HASH_MAX_LEN] = {0};
	size_t psk_len;

	/* Determine hash algorithm based on cipher suite */
	switch (ctx->ks.cipher_suite) {
	case TLS_AES_128_GCM_SHA256:
	case TLS_CHACHA20_POLY1305_SHA256:
		ctx->ks.hash_alg = PSA_ALG_SHA_256;
		ctx->ks.hash_len = 32;
		psk_len = 32;
		break;
	case TLS_AES_256_GCM_SHA384:
		ctx->ks.hash_alg = PSA_ALG_SHA_384;
		ctx->ks.hash_len = 48;
		psk_len = 48;
		break;
	default:
		return -ENOTSUP;
	}

	/* Initialize transcript hash operation to zero/clean state */
	ctx->ks.transcript_hash = psa_hash_operation_init();

	status = psa_hash_setup(&ctx->ks.transcript_hash, ctx->ks.hash_alg);
	if (status != PSA_SUCCESS) {
		NET_DBG("Failed to setup transcript hash (%d)", status);
		return -EIO;
	}

	/* Early Secret = HKDF-Extract(0, PSK), using zero PSK for now */
	if (quic_hkdf_extract_ex(ctx->ks.hash_alg, NULL, 0,
				 zero_psk, psk_len,
				 ctx->ks.early_secret, ctx->ks.hash_len) != 0) {
		NET_DBG("Failed to derive early secret");
		return -EIO;
	}

	ctx->ks.initialized = true;

	return 0;
}

/*
 * Derive handshake secrets after ECDH
 */
static int derive_handshake_secrets(struct quic_tls_context *ctx)
{
	uint8_t derived[QUIC_HASH_MAX_LEN];
	uint8_t empty_hash[QUIC_HASH_MAX_LEN];
	size_t empty_hash_len;
	psa_status_t status;
	int ret;

	/* Get hash of empty string */
	status = psa_hash_compute(ctx->ks.hash_alg, NULL, 0,
				  empty_hash, sizeof(empty_hash), &empty_hash_len);
	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	/* derived_secret = Derive-Secret(early_secret, "derived", "") */
	ret = quic_hkdf_expand_label_ex(ctx->ks.hash_alg,
					ctx->ks.early_secret, ctx->ks.hash_len,
					(const uint8_t *)TLS13_LABEL_DERIVED,
					strlen(TLS13_LABEL_DERIVED),
					empty_hash, empty_hash_len,
					derived, ctx->ks.hash_len);
	if (ret != 0) {
		return ret;
	}

	/* Handshake Secret = HKDF-Extract(derived_secret, ECDHE) */
	ret = quic_hkdf_extract_ex(ctx->ks.hash_alg,
				   derived, ctx->ks.hash_len,
				   ctx->shared_secret, 32,
				   ctx->ks.handshake_secret, ctx->ks.hash_len);
	if (ret != 0) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_QUIC_TXRX_DEBUG)) {
		LOG_HEXDUMP_DBG(ctx->ks.handshake_secret, ctx->ks.hash_len,
				"Handshake secret:");
	}

	/* Derive client/server handshake traffic secrets */
	ret = quic_derive_secret(ctx, ctx->ks.handshake_secret,
				 TLS13_LABEL_C_HS_TRAFFIC,
				 ctx->ks.client_hs_traffic_secret);
	if (ret != 0) {
		return ret;
	}

	ret = quic_derive_secret(ctx, ctx->ks.handshake_secret,
				 TLS13_LABEL_S_HS_TRAFFIC,
				 ctx->ks.server_hs_traffic_secret);
	if (ret != 0) {
		return ret;
	}

	/* Notify QUIC layer of new secrets */
	if (ctx->secret_cb != NULL) {
		ctx->secret_cb(ctx->user_data, QUIC_SECRET_LEVEL_HANDSHAKE,
			       ctx->ep->is_server ? ctx->ks.client_hs_traffic_secret :
						    ctx->ks.server_hs_traffic_secret,
			       ctx->ep->is_server ? ctx->ks.server_hs_traffic_secret :
						    ctx->ks.client_hs_traffic_secret,
			       ctx->ks.hash_len);
	}

	return 0;
}

/*
 * Build default server transport parameters (RFC 9000 Section 18)
 *
 * These must be sent in EncryptedExtensions for QUIC to work.
 */
static int build_default_transport_params(struct quic_tls_context *ctx)
{
	uint8_t *buf = ctx->local_tp;
	size_t pos = 0;
	size_t max_len = sizeof(ctx->local_tp);
	size_t max_payload_size;
	struct net_if *iface;
	int val_size;
	int ret;

	/*
	 * Transport parameters are encoded as a sequence of:
	 *   parameter_id (varint)
	 *   parameter_length (varint)
	 *   parameter_value (bytes)
	 */

	/* TODO: use limits from Kconfig or API instead of hardcoding. Also change
	 *       to use quic_put_len() instead of hand crafted values.
	 */

	/* original_destination_connection_id (0x00), server must echo client's DCID */
	if (ctx->ep != NULL && ctx->ep->peer_orig_dcid_len > 0) {
		if (pos + 2 + ctx->ep->peer_orig_dcid_len > max_len) {
			return -ENOBUFS;
		}

		buf[pos++] = 0x00;  /* parameter ID */
		buf[pos++] = ctx->ep->peer_orig_dcid_len;  /* length */
		memcpy(&buf[pos], ctx->ep->peer_orig_dcid, ctx->ep->peer_orig_dcid_len);
		pos += ctx->ep->peer_orig_dcid_len;
	}

	/* initial_source_connection_id (0x0f), this is our CID */
	if (ctx->ep != NULL && ctx->ep->my_cid_len > 0) {
		if (pos + 2 + ctx->ep->my_cid_len > max_len) {
			return -ENOBUFS;
		}

		buf[pos++] = 0x0f;  /* parameter ID */
		buf[pos++] = ctx->ep->my_cid_len;  /* length */
		memcpy(&buf[pos], ctx->ep->my_cid, ctx->ep->my_cid_len);
		pos += ctx->ep->my_cid_len;
	}

	/* max_idle_timeout (0x01) in milliseconds */
	ret = quic_put_varint(&buf[pos], max_len - pos, 0x01);
	if (ret <= 0) {
		return -ENOBUFS;
	}
	pos += ret;

	val_size = quic_get_varint_size(CONFIG_QUIC_MAX_IDLE_TIMEOUT);
	ret = quic_put_varint(&buf[pos], max_len - pos, val_size);
	if (ret == 0) {
		return -ENOBUFS;
	}
	pos += ret;

	ret = quic_put_varint(&buf[pos], max_len - pos, CONFIG_QUIC_MAX_IDLE_TIMEOUT);
	if (ret == 0) {
		return -ENOBUFS;
	}
	pos += ret;

	/* max_udp_payload_size (0x03)
	 * We can only handle max size calculated from interface MTU.
	 */
	ret = quic_put_varint(&buf[pos], max_len - pos, 0x03);
	if (ret <= 0) {
		return -ENOBUFS;
	}
	pos += ret;

	iface = net_if_select_src_iface((struct net_sockaddr *)&ctx->ep->remote_addr);
	if (iface == NULL) {
		max_payload_size = 1280;  /* Default minimum for QUIC */
	} else {
		max_payload_size =
			net_if_get_mtu(iface) -
			(ctx->ep->remote_addr.ss_family == NET_AF_INET ?
			 sizeof(struct net_ipv4_hdr) : sizeof(struct net_ipv6_hdr)) -
			sizeof(struct net_udp_hdr);
		max_payload_size = MAX(max_payload_size, 1280);  /* Ensure we meet QUIC minimum */
	}

	val_size = quic_get_varint_size(max_payload_size);
	ret = quic_put_varint(&buf[pos], max_len - pos, val_size);
	if (ret == 0) {
		return -ENOBUFS;
	}
	pos += ret;

	ret = quic_put_varint(&buf[pos], max_len - pos, max_payload_size);
	if (ret == 0) {
		return -ENOBUFS;
	}
	pos += ret;

	/* initial_max_data (0x04), connection-level flow control */
	ret = quic_put_varint(&buf[pos], max_len - pos, 0x04);
	if (ret <= 0) {
		return -ENOBUFS;
	}
	pos += ret;

	val_size = quic_get_varint_size(CONFIG_QUIC_INITIAL_MAX_DATA);
	ret = quic_put_varint(&buf[pos], max_len - pos, val_size);
	if (ret == 0) {
		return -ENOBUFS;
	}
	pos += ret;

	ret = quic_put_varint(&buf[pos], max_len - pos,
			      CONFIG_QUIC_INITIAL_MAX_DATA);
	if (ret == 0) {
		return -ENOBUFS;
	}
	pos += ret;

	/* initial_max_stream_data_bidi_local (0x05) */
	ret = quic_put_varint(&buf[pos], max_len - pos, 0x05);
	if (ret <= 0) {
		return -ENOBUFS;
	}
	pos += ret;

	val_size = quic_get_varint_size(CONFIG_QUIC_INITIAL_MAX_STREAM_DATA_BIDI_LOCAL);
	ret = quic_put_varint(&buf[pos], max_len - pos, val_size);
	if (ret == 0) {
		return -ENOBUFS;
	}
	pos += ret;

	ret = quic_put_varint(&buf[pos], max_len - pos,
			      CONFIG_QUIC_INITIAL_MAX_STREAM_DATA_BIDI_LOCAL);
	if (ret == 0) {
		return -ENOBUFS;
	}
	pos += ret;

	/* initial_max_stream_data_bidi_remote (0x06) */
	ret = quic_put_varint(&buf[pos], max_len - pos, 0x06);
	if (ret <= 0) {
		return -ENOBUFS;
	}
	pos += ret;

	val_size = quic_get_varint_size(CONFIG_QUIC_INITIAL_MAX_STREAM_DATA_BIDI_REMOTE);
	ret = quic_put_varint(&buf[pos], max_len - pos, val_size);
	if (ret == 0) {
		return -ENOBUFS;
	}
	pos += ret;

	ret = quic_put_varint(&buf[pos], max_len - pos,
			      CONFIG_QUIC_INITIAL_MAX_STREAM_DATA_BIDI_REMOTE);
	if (ret == 0) {
		return -ENOBUFS;
	}
	pos += ret;

	/* initial_max_stream_data_uni (0x07) */
	ret = quic_put_varint(&buf[pos], max_len - pos, 0x07);
	if (ret <= 0) {
		return -ENOBUFS;
	}
	pos += ret;

	val_size = quic_get_varint_size(CONFIG_QUIC_INITIAL_MAX_STREAM_DATA_UNI);
	ret = quic_put_varint(&buf[pos], max_len - pos, val_size);
	if (ret == 0) {
		return -ENOBUFS;
	}
	pos += ret;

	ret = quic_put_varint(&buf[pos], max_len - pos,
			      CONFIG_QUIC_INITIAL_MAX_STREAM_DATA_UNI);
	if (ret == 0) {
		return -ENOBUFS;
	}
	pos += ret;

	/* initial_max_streams_bidi (0x08) */
	ret = quic_put_varint(&buf[pos], max_len - pos, 0x08);
	if (ret <= 0) {
		return -ENOBUFS;
	}
	pos += ret;

	val_size = quic_get_varint_size(CONFIG_QUIC_INITIAL_MAX_STREAMS_BIDI);
	ret = quic_put_varint(&buf[pos], max_len - pos, val_size);
	if (ret == 0) {
		return -ENOBUFS;
	}
	pos += ret;

	ret = quic_put_varint(&buf[pos], max_len - pos,
			      CONFIG_QUIC_INITIAL_MAX_STREAMS_BIDI);
	if (ret == 0) {
		return -ENOBUFS;
	}
	pos += ret;

	/* initial_max_streams_uni (0x09) */
	ret = quic_put_varint(&buf[pos], max_len - pos, 0x09);
	if (ret <= 0) {
		return -ENOBUFS;
	}
	pos += ret;

	val_size = quic_get_varint_size(CONFIG_QUIC_INITIAL_MAX_STREAMS_UNI);
	ret = quic_put_varint(&buf[pos], max_len - pos, val_size);
	if (ret == 0) {
		return -ENOBUFS;
	}
	pos += ret;

	ret = quic_put_varint(&buf[pos], max_len - pos,
			      CONFIG_QUIC_INITIAL_MAX_STREAMS_UNI);
	if (ret == 0) {
		return -ENOBUFS;
	}
	pos += ret;

	ctx->local_tp_len = pos;

	if (IS_ENABLED(CONFIG_QUIC_TXRX_DEBUG)) {
		NET_DBG("Built transport parameters, len=%zu", pos);
		NET_HEXDUMP_DBG(ctx->local_tp, ctx->local_tp_len, "Transport params:");
	}

	return 0;
}

/*
 * Build EncryptedExtensions message
 * Must include QUIC transport parameters (extension type 0x39)
 */
static int build_encrypted_extensions(struct quic_tls_context *ctx,
				      uint8_t *buf, size_t buf_size,
				      size_t *out_len)
{
	size_t pos = 0;
	size_t ext_len_pos;
	size_t total_ext_len;

	/* Extensions length placeholder */
	ext_len_pos = pos;
	pos += 2;

	/* ALPN extension (type 0x10) if we negotiated a protocol */
	if (ctx->negotiated_alpn != NULL) {
		uint16_t proto_list_len;
		size_t alpn_len = strlen(ctx->negotiated_alpn);
		size_t ext_data_len = 2 + 1 + alpn_len; /* list len + proto len + proto */

		if (pos + 4 + ext_data_len > buf_size) {
			return -ENOBUFS;
		}

		/* Extension type: ALPN (0x0010) */
		buf[pos++] = 0x00;
		buf[pos++] = 0x10;

		/* Extension length */
		buf[pos++] = (ext_data_len >> 8) & 0xFF;
		buf[pos++] = ext_data_len & 0xFF;

		/* ALPN protocol list length (2 bytes) */
		proto_list_len = 1 + alpn_len;

		buf[pos++] = (proto_list_len >> 8) & 0xFF;
		buf[pos++] = proto_list_len & 0xFF;

		/* Protocol name length (1 byte) + protocol name */
		buf[pos++] = alpn_len;
		memcpy(&buf[pos], ctx->negotiated_alpn, alpn_len);
		pos += alpn_len;

		NET_DBG("Added ALPN extension: %s", ctx->negotiated_alpn);
	}

	/* QUIC Transport Parameters extension (type 0x39) */
	if (ctx->local_tp_len > 0) {
		if (pos + 4 + ctx->local_tp_len > buf_size) {
			return -ENOBUFS;
		}

		/* Extension type:  quic_transport_parameters */
		buf[pos++] = 0x00;
		buf[pos++] = 0x39;

		/* Extension length */
		buf[pos++] = (ctx->local_tp_len >> 8) & 0xFF;
		buf[pos++] = ctx->local_tp_len & 0xFF;

		/* Extension data */
		memcpy(&buf[pos], ctx->local_tp, ctx->local_tp_len);
		pos += ctx->local_tp_len;
	}

	/* Fill in total extensions length */
	total_ext_len = pos - ext_len_pos - 2;
	buf[ext_len_pos] = (total_ext_len >> 8) & 0xFF;
	buf[ext_len_pos + 1] = total_ext_len & 0xFF;

	*out_len = pos;
	return 0;
}

/*
 * Build Certificate message with full chain (TLS 1.3 format)
 *
 * Certificate {
 *   opaque certificate_request_context<0..2^8-1>;
 *   CertificateEntry certificate_list<0..2^24-1>;
 * }
 *
 * CertificateEntry {
 *   opaque cert_data<1..2^24-1>;
 *   Extension extensions<0..2^16-1>;
 * }
 */
#if !defined(MBEDTLS_X509_CRT_PARSE_C)
static int build_certificate(struct quic_tls_context *ctx,
			     uint8_t *buf, size_t buf_size,
			     size_t *out_len)
{
	size_t pos = 0;
	size_t cert_list_len_pos;
	size_t cert_list_len;

	/* Certificate request context
	 * - Empty (0 length) for server certificate
	 * - Non-empty for client certificate (echoes server's CertificateRequest context)
	 */
	if (ctx->server_requested_cert && ctx->cert_request_context_len > 0) {
		/* Client certificate, include context from CertificateRequest */
		buf[pos++] = ctx->cert_request_context_len;
		memcpy(&buf[pos], ctx->cert_request_context, ctx->cert_request_context_len);
		pos += ctx->cert_request_context_len;
	} else {
		/* Server certificate, empty context */
		buf[pos++] = 0;
	}

	/* Certificate list length placeholder (3 bytes) */
	cert_list_len_pos = pos;
	pos += 3;

	/* Add each intermediate certificate in the chain, resolved from sec_tags */
	credentials_lock();

	for (size_t i = 0; i < ctx->cert_chain_count; i++) {
		struct tls_credential *cred;

		cred = credential_get(ctx->cert_chain_tags[i],
				      TLS_CREDENTIAL_CA_CERTIFICATE);
		if (cred == NULL) {
			cred = credential_get(ctx->cert_chain_tags[i],
					      TLS_CREDENTIAL_PUBLIC_CERTIFICATE);
		}

		if (cred == NULL || cred->buf == NULL || cred->len == 0) {
			NET_DBG("No certificate found for chain tag %d",
				ctx->cert_chain_tags[i]);
			credentials_unlock();
			return -ENOENT;
		}

		if (pos + 3 + cred->len + 2 > buf_size) {
			NET_DBG("Certificate buffer overflow at chain index %zu", i);
			credentials_unlock();
			return -ENOBUFS;
		}

		/* cert_data length (3 bytes, big-endian) */
		buf[pos++] = (cred->len >> 16) & 0xFF;
		buf[pos++] = (cred->len >> 8) & 0xFF;
		buf[pos++] = cred->len & 0xFF;

		/* cert_data */
		memcpy(&buf[pos], cred->buf, cred->len);
		pos += cred->len;

		/* Extensions (empty for now) */
		buf[pos++] = 0x00;
		buf[pos++] = 0x00;
	}

	credentials_unlock();

	/* Fill in certificate list length */
	cert_list_len = pos - cert_list_len_pos - 3;
	buf[cert_list_len_pos] = (cert_list_len >> 16) & 0xFF;
	buf[cert_list_len_pos + 1] = (cert_list_len >> 8) & 0xFF;
	buf[cert_list_len_pos + 2] = cert_list_len & 0xFF;

	*out_len = pos;

	NET_DBG("Built Certificate message with %zu certs, total %zu bytes",
		ctx->cert_chain_count, pos);

	return 0;
}
#else /* !MBEDTLS_X509_CRT_PARSE_C */
/*
 * This version iterates over the mbedtls_x509_crt chain structure
 * instead of using raw certificate pointers.
 */
static int build_certificate(struct quic_tls_context *ctx,
			     uint8_t *buf, size_t buf_size,
			     size_t *out_len)
{
	size_t pos = 0;
	size_t cert_list_len_pos;
	size_t cert_list_len;
	mbedtls_x509_crt *cert;
	size_t cert_count = 0;

	/* Certificate request context
	 * - Empty (0 length) for server certificate
	 * - Non-empty for client certificate (echoes server's CertificateRequest context)
	 */
	if (ctx->server_requested_cert && ctx->cert_request_context_len > 0) {
		/* Client certificate, include context from CertificateRequest */
		buf[pos++] = ctx->cert_request_context_len;
		memcpy(&buf[pos], ctx->cert_request_context, ctx->cert_request_context_len);
		pos += ctx->cert_request_context_len;
	} else {
		/* Server certificate, empty context */
		buf[pos++] = 0;
	}

	/* Certificate list length placeholder (3 bytes) */
	cert_list_len_pos = pos;
	pos += 3;

	/* Iterate over the certificate chain using mbedtls_x509_crt linked list */
	for (cert = &ctx->own_cert; cert != NULL; cert = cert->next) {
		/* Skip if this cert entry has no data */
		if (cert->raw.len == 0) {
			continue;
		}

		if (pos + 3 + cert->raw.len + 2 > buf_size) {
			NET_DBG("Certificate buffer overflow at chain index %zu", cert_count);
			return -ENOBUFS;
		}

		/* cert_data length (3 bytes, big-endian) */
		buf[pos++] = (cert->raw.len >> 16) & 0xFF;
		buf[pos++] = (cert->raw.len >> 8) & 0xFF;
		buf[pos++] = cert->raw.len & 0xFF;

		/* cert_data, use the DER-encoded certificate from mbedtls structure */
		memcpy(&buf[pos], cert->raw.p, cert->raw.len);
		pos += cert->raw.len;

		/* Extensions (empty for now) */
		buf[pos++] = 0x00;
		buf[pos++] = 0x00;

		cert_count++;
	}

	/* Fill in certificate list length */
	cert_list_len = pos - cert_list_len_pos - 3;
	buf[cert_list_len_pos] = (cert_list_len >> 16) & 0xFF;
	buf[cert_list_len_pos + 1] = (cert_list_len >> 8) & 0xFF;
	buf[cert_list_len_pos + 2] = cert_list_len & 0xFF;

	*out_len = pos;

	NET_DBG("Built Certificate message with %zu certs, total %zu bytes",
		cert_count, pos);

	return 0;
}
#endif /* !MBEDTLS_X509_CRT_PARSE_C */

/*
 * Convert raw ECDSA signature (r||s) to DER-encoded ASN.1 format.
 * TLS 1.3 requires DER encoding: SEQUENCE { INTEGER r, INTEGER s }
 *
 * @param raw_sig Raw signature (r || s), typically 64 bytes for P-256
 * @param raw_len Length of raw signature
 * @param der_sig Output buffer for DER-encoded signature
 * @param der_size Size of output buffer
 * @param der_len Output: actual length of DER signature
 * @return 0 on success, negative on error
 */
static int ecdsa_raw_to_der(const uint8_t *raw_sig, size_t raw_len,
			    uint8_t *der_sig, size_t der_size, size_t *der_len)
{
	size_t half = raw_len / 2;
	const uint8_t *r = raw_sig;
	const uint8_t *s = raw_sig + half;
	size_t r_len = half;
	size_t s_len = half;
	size_t pos = 0;
	size_t r_pad, s_pad;
	size_t seq_len;

	/* Skip leading zeros but keep at least one byte */
	while (r_len > 1 && r[0] == 0) {
		r++;
		r_len--;
	}
	while (s_len > 1 && s[0] == 0) {
		s++;
		s_len--;
	}

	/* Add padding byte if high bit is set (to keep it positive) */
	r_pad = (r[0] & 0x80) ? 1 : 0;
	s_pad = (s[0] & 0x80) ? 1 : 0;

	seq_len = 2 + r_len + r_pad + 2 + s_len + s_pad;

	/* Check output buffer size */
	if (der_size < 2 + seq_len) {
		return -ENOBUFS;
	}

	/* SEQUENCE tag and length */
	der_sig[pos++] = 0x30;
	if (seq_len < 128) {
		der_sig[pos++] = seq_len;
	} else {
		/* For signatures, length should fit in one byte */
		return -EINVAL;
	}

	/* INTEGER r */
	der_sig[pos++] = 0x02;
	der_sig[pos++] = r_len + r_pad;
	if (r_pad) {
		der_sig[pos++] = 0x00;
	}
	memcpy(&der_sig[pos], r, r_len);
	pos += r_len;

	/* INTEGER s */
	der_sig[pos++] = 0x02;
	der_sig[pos++] = s_len + s_pad;
	if (s_pad) {
		der_sig[pos++] = 0x00;
	}
	memcpy(&der_sig[pos], s, s_len);
	pos += s_len;

	*der_len = pos;
	return 0;
}

/*
 * Build CertificateVerify message
 */
static int build_certificate_verify(struct quic_tls_context *ctx,
				    uint8_t *buf, size_t buf_size,
				    size_t *out_len)
{
	/*
	 * CertificateVerify contains a signature over:
	 * - 64 spaces (0x20)
	 * - "TLS 1.3, server CertificateVerify" or "TLS 1.3, client CertificateVerify"
	 * - 0x00
	 * - transcript hash
	 */
	static const char server_context[] = TLS13_SIG_CONTEXT_SERVER;
	static const char client_context[] = TLS13_SIG_CONTEXT_CLIENT;
	const char *context_string;
	size_t context_len;
	uint8_t transcript[QUIC_HASH_MAX_LEN];
	size_t transcript_len;
	uint8_t to_sign[TLS13_CERT_VERIFY_PADDING_LEN +
			sizeof(server_context) + QUIC_HASH_MAX_LEN];
	size_t to_sign_len;
	uint8_t signature[PSA_SIGNATURE_MAX_SIZE];
	size_t signature_len;
	size_t key_bits;
	psa_key_attributes_t key_attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_algorithm_t sig_hash_alg;
	uint16_t sig_alg_id;
	uint8_t der_sig[PSA_SIGNATURE_MAX_SIZE];
	size_t der_len;
	psa_status_t attr_status;
	psa_status_t status;
	int ret;
	size_t pos = 0;

	/* Use correct context string based on whether we are server or client */
	if (ctx->ep->is_server) {
		context_string = server_context;
		context_len = sizeof(server_context);
	} else {
		context_string = client_context;
		context_len = sizeof(client_context);
	}

	/* Build content to sign */
	memset(to_sign, TLS13_CERT_VERIFY_PADDING_BYTE, TLS13_CERT_VERIFY_PADDING_LEN);
	to_sign_len = TLS13_CERT_VERIFY_PADDING_LEN;

	memcpy(&to_sign[to_sign_len], context_string, context_len);
	to_sign_len += context_len; /* includes null terminator */

	ret = transcript_hash_get(ctx, transcript, &transcript_len);
	if (ret != 0) {
		return ret;
	}

	memcpy(&to_sign[to_sign_len], transcript, transcript_len);
	to_sign_len += transcript_len;

	/* For TLS 1.3 CertificateVerify, the signature algorithm is determined by
	 * the key type, NOT the cipher suite's hash. Query the key to determine
	 * the correct hash algorithm.
	 */
	attr_status = psa_get_key_attributes(ctx->signing_key_id, &key_attr);
	if (attr_status != PSA_SUCCESS) {
		NET_DBG("Failed to get signing key attributes: %d", attr_status);
		return -EIO;
	}

	key_bits = psa_get_key_bits(&key_attr);
	psa_reset_key_attributes(&key_attr);

	/* Determine hash algorithm based on EC key size:
	 * - P-256 (256 bits) -> SHA-256
	 * - P-384 (384 bits) -> SHA-384
	 */
	if (key_bits == 256) {
		sig_hash_alg = PSA_ALG_SHA_256;
		sig_alg_id = 0x0403; /* ecdsa_secp256r1_sha256 */
	} else if (key_bits == 384) {
		sig_hash_alg = PSA_ALG_SHA_384;
		sig_alg_id = 0x0503; /* ecdsa_secp384r1_sha384 */
	} else {
		NET_DBG("Unsupported EC key size: %zu bits", key_bits);
		return -EINVAL;
	}

	NET_DBG("CertificateVerify: to_sign_len=%zu, transcript_len=%zu, key_bits=%zu",
		to_sign_len, transcript_len, key_bits);

	if (IS_ENABLED(CONFIG_QUIC_TXRX_DEBUG)) {
		NET_HEXDUMP_DBG(to_sign, MIN(to_sign_len, 128), "Content to sign (first 128):");
		NET_HEXDUMP_DBG(transcript, transcript_len, "Transcript hash:");
	}

	/* Sign the content directly, ECDSA will hash internally with key-appropriate hash */
	status = psa_sign_message(ctx->signing_key_id,
				  PSA_ALG_ECDSA(sig_hash_alg),
				  to_sign, to_sign_len,
				  signature, sizeof(signature),
				  &signature_len);
	if (status != PSA_SUCCESS) {
		NET_DBG("psa_sign_message failed: %d", status);
		return -EIO;
	}

	if (IS_ENABLED(CONFIG_QUIC_TXRX_DEBUG)) {
		NET_DBG("Raw signature length: %zu", signature_len);
		NET_HEXDUMP_DBG(signature, signature_len, "Raw ECDSA signature:");
	}

	/* Convert raw ECDSA signature to DER format for TLS */
	ret = ecdsa_raw_to_der(signature, signature_len, der_sig, sizeof(der_sig), &der_len);
	if (ret != 0) {
		NET_DBG("Failed to convert signature to DER: %d", ret);
		return ret;
	}

	if (IS_ENABLED(CONFIG_QUIC_TXRX_DEBUG)) {
		NET_DBG("DER signature length: %zu", der_len);
		NET_HEXDUMP_DBG(der_sig, der_len, "DER-encoded signature:");
	}

	/* Build CertificateVerify message */
	/* Use sig_alg_id determined earlier from key size */
	NET_DBG("Using signature algorithm 0x%04x", sig_alg_id);
	buf[pos++] = (sig_alg_id >> 8) & 0xFF;
	buf[pos++] = sig_alg_id & 0xFF;

	/* Signature length (DER-encoded) */
	buf[pos++] = (der_len >> 8) & 0xFF;
	buf[pos++] = der_len & 0xFF;

	/* DER-encoded signature */
	memcpy(&buf[pos], der_sig, der_len);
	pos += der_len;

	NET_DBG("CertificateVerify message length: %zu", pos);

	*out_len = pos;
	return 0;
}

/*
 * Build CertificateRequest message (TLS 1.3)
 *
 * CertificateRequest {
 *   opaque certificate_request_context<0..2^8-1>;
 *   Extension extensions<2.. 2^16-1>;
 * }
 *
 * Required extension: signature_algorithms
 */
static int build_certificate_request(struct quic_tls_context *ctx,
				     uint8_t *buf, size_t buf_size,
				     size_t *out_len)
{
	size_t pos = 0;
	size_t ext_len_pos;
	size_t sig_algs_len_pos;
	size_t sig_algs_data_len;
	size_t ext_len;

	/* Certificate request context (can be used to correlate request/response) */
	/* Using a random 8-byte context */
	buf[pos++] = QUIC_CERT_REQ_CONTEXT_LEN;  /* context length */
	sys_rand_get(&buf[pos], QUIC_CERT_REQ_CONTEXT_LEN);

	/* Save context for later verification */
	memcpy(ctx->cert_request_context, &buf[pos], QUIC_CERT_REQ_CONTEXT_LEN);
	ctx->cert_request_context_len = QUIC_CERT_REQ_CONTEXT_LEN;
	pos += QUIC_CERT_REQ_CONTEXT_LEN;

	/* Extensions length placeholder */
	ext_len_pos = pos;
	pos += 2;

	/* signature_algorithms extension (required) */
	buf[pos++] = 0x00;
	buf[pos++] = 0x0D;  /* signature_algorithms type */

	sig_algs_len_pos = pos;
	pos += 2;  /* extension length placeholder */
	pos += 2;  /* supported_signature_algorithms length placeholder */

	/* List supported signature algorithms */
	/* ecdsa_secp256r1_sha256 (0x0403) */
	buf[pos++] = 0x04;
	buf[pos++] = 0x03;

	/* ecdsa_secp384r1_sha384 (0x0503) */
	buf[pos++] = 0x05;
	buf[pos++] = 0x03;

	/* rsa_pss_rsae_sha256 (0x0804) */
	buf[pos++] = 0x08;
	buf[pos++] = 0x04;

	/* Fill in lengths */
	sig_algs_data_len = pos - sig_algs_len_pos - 4;
	buf[sig_algs_len_pos] = 0x00;
	buf[sig_algs_len_pos + 1] = (sig_algs_data_len + 2) & 0xFF;
	buf[sig_algs_len_pos + 2] = 0x00;
	buf[sig_algs_len_pos + 3] = sig_algs_data_len & 0xFF;

	/* Fill in total extensions length */
	ext_len = pos - ext_len_pos - 2;
	buf[ext_len_pos] = (ext_len >> 8) & 0xFF;
	buf[ext_len_pos + 1] = ext_len & 0xFF;

	*out_len = pos;

	NET_DBG("Built CertificateRequest message, %zu bytes", pos);

	return 0;
}

/*
 * Build Finished message
 */
static int build_finished(struct quic_tls_context *ctx,
			  uint8_t *buf, size_t buf_size,
			  size_t *out_len)
{
	/*
	 * Finished = HMAC(finished_key, transcript_hash)
	 * finished_key = HKDF-Expand-Label(server_hs_traffic_secret, "finished", "", hash_len)
	 */
	uint8_t finished_key[QUIC_HASH_MAX_LEN];
	uint8_t transcript[QUIC_HASH_MAX_LEN];
	size_t transcript_len;
	uint8_t verify_data[QUIC_HASH_MAX_LEN];
	size_t verify_data_len;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t hmac_key_id;
	psa_status_t status;
	int ret;

	/* Derive finished_key */
	ret = quic_hkdf_expand_label_ex(ctx->ks.hash_alg,
					ctx->ks.server_hs_traffic_secret,
					ctx->ks.hash_len,
					(const uint8_t *)"finished",
					sizeof("finished") - 1,
					NULL, 0,
					finished_key, ctx->ks.hash_len);
	if (ret != 0) {
		NET_DBG("Failed to derive finished_key: %d", ret);
		return ret;
	}

	/* Get current transcript hash */
	ret = transcript_hash_get(ctx, transcript, &transcript_len);
	if (ret != 0) {
		NET_DBG("Failed to get transcript hash: %d", ret);
		return ret;
	}

	/* Compute HMAC */
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_MESSAGE);
	psa_set_key_algorithm(&attr, PSA_ALG_HMAC(ctx->ks.hash_alg));
	psa_set_key_type(&attr, PSA_KEY_TYPE_HMAC);

	status = psa_import_key(&attr, finished_key, ctx->ks.hash_len, &hmac_key_id);
	if (status != PSA_SUCCESS) {
		NET_DBG("Failed to import HMAC key: %d", status);
		return -EIO;
	}

	status = psa_mac_compute(hmac_key_id,
				 PSA_ALG_HMAC(ctx->ks.hash_alg),
				 transcript, transcript_len,
				 verify_data, ctx->ks.hash_len,
				 &verify_data_len);

	psa_destroy_key(hmac_key_id);

	if (status != PSA_SUCCESS) {
		NET_DBG("Failed to compute HMAC: %d", status);
		return -EIO;
	}

	/* Copy verify_data to output */
	if (verify_data_len > buf_size) {
		return -ENOBUFS;
	}

	memcpy(buf, verify_data, verify_data_len);
	*out_len = verify_data_len;

	return 0;
}

/*
 * Build client's Finished message
 * Uses client_hs_traffic_secret instead of server_hs_traffic_secret
 */
static int build_client_finished(struct quic_tls_context *ctx,
				 uint8_t *buf, size_t buf_size,
				 size_t *out_len)
{
	uint8_t finished_key[QUIC_HASH_MAX_LEN];
	uint8_t transcript[QUIC_HASH_MAX_LEN];
	size_t transcript_len;
	uint8_t verify_data[QUIC_HASH_MAX_LEN];
	size_t verify_data_len;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t hmac_key_id;
	psa_status_t status;
	int ret;

	/* Derive finished_key from CLIENT handshake traffic secret */
	ret = quic_hkdf_expand_label_ex(ctx->ks.hash_alg,
					ctx->ks.client_hs_traffic_secret,
					ctx->ks.hash_len,
					(const uint8_t *)"finished",
					sizeof("finished") - 1,
					NULL, 0,
					finished_key, ctx->ks.hash_len);
	if (ret != 0) {
		NET_DBG("Failed to derive client finished_key: %d", ret);
		return ret;
	}

	/* Get current transcript hash */
	ret = transcript_hash_get(ctx, transcript, &transcript_len);
	if (ret != 0) {
		NET_DBG("Failed to get transcript hash: %d", ret);
		return ret;
	}

	/* Compute HMAC */
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_MESSAGE);
	psa_set_key_algorithm(&attr, PSA_ALG_HMAC(ctx->ks.hash_alg));
	psa_set_key_type(&attr, PSA_KEY_TYPE_HMAC);

	status = psa_import_key(&attr, finished_key, ctx->ks.hash_len, &hmac_key_id);
	if (status != PSA_SUCCESS) {
		NET_DBG("Failed to import HMAC key: %d", status);
		return -EIO;
	}

	status = psa_mac_compute(hmac_key_id,
				 PSA_ALG_HMAC(ctx->ks.hash_alg),
				 transcript, transcript_len,
				 verify_data, ctx->ks.hash_len,
				 &verify_data_len);

	psa_destroy_key(hmac_key_id);

	if (status != PSA_SUCCESS) {
		NET_DBG("Failed to compute HMAC: %d", status);
		return -EIO;
	}

	if (verify_data_len > buf_size) {
		return -ENOBUFS;
	}

	memcpy(buf, verify_data, verify_data_len);
	*out_len = verify_data_len;

	return 0;
}

/*
 * Send client's Certificate and CertificateVerify messages
 * Called when server requested client authentication via CertificateRequest
 *
 * Uses endpoint's rx_buffer as scratch space to avoid large stack allocations.
 * The rx_buffer (4096 bytes) is not used during handshake message sending.
 */
static int send_client_certificate(struct quic_tls_context *ctx)
{
	struct quic_endpoint *ep = ctx->ep;
	size_t buf_size = sizeof(ep->crypto.rx_buffer);
	uint8_t *msg_buf = ep->crypto.rx_buffer;
	size_t msg_buf_size = buf_size / 2;
	uint8_t *wrapped = ep->crypto.rx_buffer + msg_buf_size;
	size_t wrapped_size = buf_size - msg_buf_size;
	size_t msg_len, wrapped_len;
	int ret;

	/* Send Certificate message */
	if (ctx->my_cert != NULL && ctx->my_cert_len > 0 && ctx->signing_key_id != 0) {
		ret = build_certificate(ctx, msg_buf, msg_buf_size, &msg_len);
		if (ret != 0) {
			NET_DBG("Failed to build client Certificate: %d", ret);
			return ret;
		}

		ret = wrap_handshake_message(TLS_HS_CERTIFICATE,
					     msg_buf, msg_len,
					     wrapped, wrapped_size,
					     &wrapped_len);
		if (ret != 0) {
			return ret;
		}

		ret = transcript_update(ctx, wrapped, wrapped_len);
		if (ret != 0) {
			return ret;
		}

		if (ctx->send_cb != NULL) {
			ret = ctx->send_cb(ctx->user_data, QUIC_SECRET_LEVEL_HANDSHAKE,
					   wrapped, wrapped_len);
			if (ret != 0) {
				NET_DBG("Failed to send client Certificate: %d", ret);
				return ret;
			}
		}

		NET_DBG("[%p] Client sent Certificate", ctx->ep);

		/* Send CertificateVerify */
		ret = build_certificate_verify(ctx, msg_buf, msg_buf_size, &msg_len);
		if (ret != 0) {
			NET_DBG("Failed to build client CertificateVerify: %d", ret);
			return ret;
		}

		ret = wrap_handshake_message(TLS_HS_CERTIFICATE_VERIFY,
					     msg_buf, msg_len,
					     wrapped, wrapped_size,
					     &wrapped_len);
		if (ret != 0) {
			return ret;
		}

		ret = transcript_update(ctx, wrapped, wrapped_len);
		if (ret != 0) {
			return ret;
		}

		if (ctx->send_cb != NULL) {
			ret = ctx->send_cb(ctx->user_data, QUIC_SECRET_LEVEL_HANDSHAKE,
					   wrapped, wrapped_len);
			if (ret != 0) {
				NET_DBG("Failed to send client CertificateVerify: %d", ret);
				return ret;
			}
		}

		NET_DBG("[%p] Client sent CertificateVerify", ctx->ep);
	} else {
		/* No client certificate available, send empty Certificate message */
		ret = build_certificate(ctx, msg_buf, msg_buf_size, &msg_len);
		if (ret != 0) {
			NET_DBG("Failed to build empty client Certificate: %d", ret);
			return ret;
		}

		ret = wrap_handshake_message(TLS_HS_CERTIFICATE,
					     msg_buf, msg_len,
					     wrapped, wrapped_size,
					     &wrapped_len);
		if (ret != 0) {
			return ret;
		}

		ret = transcript_update(ctx, wrapped, wrapped_len);
		if (ret != 0) {
			return ret;
		}

		if (ctx->send_cb != NULL) {
			ret = ctx->send_cb(ctx->user_data, QUIC_SECRET_LEVEL_HANDSHAKE,
					   wrapped, wrapped_len);
			if (ret != 0) {
				NET_DBG("Failed to send empty client Certificate: %d", ret);
				return ret;
			}
		}

		NET_DBG("[%p] Client sent empty Certificate (no client cert configured)", ctx->ep);
	}

	return 0;
}

/*
 * Send client's Finished message
 */
static int send_client_finished(struct quic_tls_context *ctx)
{
	uint8_t msg_buf[64];
	uint8_t wrapped[72];
	size_t msg_len, wrapped_len;
	int ret;

	ret = build_client_finished(ctx, msg_buf, sizeof(msg_buf), &msg_len);
	if (ret != 0) {
		NET_DBG("Failed to build client Finished: %d", ret);
		return ret;
	}

	ret = wrap_handshake_message(TLS_HS_FINISHED,
				     msg_buf, msg_len,
				     wrapped, sizeof(wrapped),
				     &wrapped_len);
	if (ret != 0) {
		return ret;
	}

	/* Update transcript with client Finished */
	ret = transcript_update(ctx, wrapped, wrapped_len);
	if (ret != 0) {
		return ret;
	}

	/* Send at HANDSHAKE level */
	if (ctx->send_cb != NULL) {
		ret = ctx->send_cb(ctx->user_data, QUIC_SECRET_LEVEL_HANDSHAKE,
				   wrapped, wrapped_len);
		if (ret != 0) {
			NET_DBG("Failed to send client Finished: %d", ret);
			return ret;
		}
	}

	NET_DBG("[EP:%p/%d] Client send finished", ctx->ep, quic_get_by_ep(ctx->ep));

	return 0;
}

/*
 * Build and send EncryptedExtensions, Certificate, CertificateVerify, Finished
 *
 * Uses endpoint's rx_buffer as scratch space to avoid large stack allocations.
 * The rx_buffer (4096 bytes) is not used during handshake message sending.
 * Buffer is split: first half for message building, second half for wrapped output.
 */
static int send_server_handshake_flight(struct quic_tls_context *ctx)
{
	struct quic_endpoint *ep = ctx->ep;
	size_t buf_size = sizeof(ep->crypto.rx_buffer);
	uint8_t *msg_buf = ep->crypto.rx_buffer;
	size_t msg_buf_size = buf_size / 2;
	uint8_t *wrapped = ep->crypto.rx_buffer + msg_buf_size;
	size_t wrapped_size = buf_size - msg_buf_size;
	size_t msg_len, wrapped_len;
	int ret;

	/* 1. EncryptedExtensions (must include QUIC transport parameters) */
	ret = build_encrypted_extensions(ctx, msg_buf, msg_buf_size, &msg_len);
	if (ret != 0) {
		return ret;
	}

	ret = wrap_handshake_message(TLS_HS_ENCRYPTED_EXTENSIONS,
				     msg_buf, msg_len,
				     wrapped, wrapped_size,
				     &wrapped_len);
	if (ret != 0) {
		return ret;
	}

	ret = transcript_update(ctx, wrapped, wrapped_len);
	if (ret != 0) {
		return ret;
	}

	if (ctx->send_cb != NULL) {
		ret = ctx->send_cb(ctx->user_data, QUIC_SECRET_LEVEL_HANDSHAKE,
				   wrapped, wrapped_len);
		if (ret != 0) {
			return ret;
		}
	}

	/* 2. CertificateRequest (optional, if we want client cert) */
	if (ctx->options.verify_level > 0) {
		ret = build_certificate_request(ctx, msg_buf, msg_buf_size, &msg_len);
		if (ret != 0) {
			return ret;
		}

		ret = wrap_handshake_message(TLS_HS_CERTIFICATE_REQUEST,
					     msg_buf, msg_len,
					     wrapped, wrapped_size, &wrapped_len);
		if (ret != 0) {
			return ret;
		}

		ret = transcript_update(ctx, wrapped, wrapped_len);
		if (ret != 0) {
			return ret;
		}

		if (ctx->send_cb != NULL) {
			ret = ctx->send_cb(ctx->user_data, QUIC_SECRET_LEVEL_HANDSHAKE,
					   wrapped, wrapped_len);
			if (ret != 0) {
				return ret;
			}
		}

		/* Update state to expect client certificate */
		if (ctx->options.verify_level == MBEDTLS_SSL_VERIFY_REQUIRED) {
			ctx->expecting_client_cert = true;
		}
	}

	/* 3. Certificate (if we have one with a valid signing key) */
	if (ctx->my_cert != NULL && ctx->my_cert_len > 0 && ctx->signing_key_id != 0) {
		ret = build_certificate(ctx, msg_buf, msg_buf_size, &msg_len);
		if (ret != 0) {
			return ret;
		}

		ret = wrap_handshake_message(TLS_HS_CERTIFICATE,
					     msg_buf, msg_len,
					     wrapped, wrapped_size, &wrapped_len);
		if (ret != 0) {
			return ret;
		}

		ret = transcript_update(ctx, wrapped, wrapped_len);
		if (ret != 0) {
			return ret;
		}

		if (ctx->send_cb != NULL) {
			ret = ctx->send_cb(ctx->user_data, QUIC_SECRET_LEVEL_HANDSHAKE,
					   wrapped, wrapped_len);
			if (ret != 0) {
				return ret;
			}
		}

		/* 4. CertificateVerify */
		ret = build_certificate_verify(ctx, msg_buf, msg_buf_size, &msg_len);
		if (ret != 0) {
			return ret;
		}

		ret = wrap_handshake_message(TLS_HS_CERTIFICATE_VERIFY,
					     msg_buf, msg_len,
					     wrapped, wrapped_size, &wrapped_len);
		if (ret != 0) {
			return ret;
		}

		ret = transcript_update(ctx, wrapped, wrapped_len);
		if (ret != 0) {
			return ret;
		}

		if (ctx->send_cb != NULL) {
			ret = ctx->send_cb(ctx->user_data, QUIC_SECRET_LEVEL_HANDSHAKE,
					   wrapped, wrapped_len);
			if (ret != 0) {
				return ret;
			}
		}
	}

	/* 5. Finished */
	ret = build_finished(ctx, msg_buf, msg_buf_size, &msg_len);
	if (ret != 0) {
		return ret;
	}

	ret = wrap_handshake_message(TLS_HS_FINISHED,
				     msg_buf, msg_len,
				     wrapped, wrapped_size,
				     &wrapped_len);
	if (ret != 0) {
		return ret;
	}

	ret = transcript_update(ctx, wrapped, wrapped_len);
	if (ret != 0) {
		return ret;
	}

	/* Save transcript hash after Server Finished for app secret derivation */
	ret = transcript_hash_get(ctx, ctx->ks.server_finished_transcript,
				  &ctx->ks.server_finished_transcript_len);
	if (ret != 0) {
		NET_DBG("Failed to save Server Finished transcript hash");
		return ret;
	}

	if (ctx->send_cb != NULL) {
		ret = ctx->send_cb(ctx->user_data, QUIC_SECRET_LEVEL_HANDSHAKE,
				   wrapped, wrapped_len);
		if (ret != 0) {
			return ret;
		}
	}

	/* Server can derive application secrets immediately after sending Finished.
	 * Unlike TLS over TCP, in QUIC the server doesn't wait for client's Finished
	 * to start using 1-RTT keys, both sides derive the same secrets.
	 */
	ret = derive_application_secrets(ctx);
	if (ret != 0) {
		NET_DBG("Failed to derive application secrets: %d", ret);
		return ret;
	}

	return 0;
}

/*
 * Parse ServerHello message (client side)
 *
 * ServerHello format:
 * - legacy_version: 0x0303 (TLS 1.2)
 * - random: 32 bytes
 * - legacy_session_id_echo: 1 byte length + data
 * - cipher_suite: 2 bytes
 * - legacy_compression_method: 1 byte (0x00)
 * - extensions: 2 bytes length + extension data
 */
static int parse_server_hello(struct quic_tls_context *ctx,
			      const uint8_t *msg, size_t msg_len)
{
	size_t pos = 0;
	uint8_t session_id_len;
	uint16_t cipher_suite;
	size_t extensions_len;
	size_t extensions_end;
	bool got_key_share = false;
	bool got_version = false;

	if (msg_len < 38) {
		NET_DBG("ServerHello too short");
		return -EINVAL;
	}

	/* Legacy version (skip) */
	pos += 2;

	/* Server random */
	memcpy(ctx->server_random, &msg[pos], 32);
	pos += 32;

	/* Legacy session ID echo */
	session_id_len = msg[pos++];
	if (pos + session_id_len > msg_len) {
		return -EINVAL;
	}
	pos += session_id_len;

	/* Cipher suite */
	if (pos + 2 > msg_len) {
		return -EINVAL;
	}
	cipher_suite = (msg[pos] << 8) | msg[pos + 1];
	pos += 2;

	/* Verify cipher suite matches what we offered */
	if (cipher_suite != TLS_AES_128_GCM_SHA256 &&
	    cipher_suite != TLS_AES_256_GCM_SHA384 &&
	    cipher_suite != TLS_CHACHA20_POLY1305_SHA256) {
		NET_DBG("Unsupported cipher suite 0x%04x", cipher_suite);
		return -ENOTSUP;
	}
	ctx->ks.cipher_suite = cipher_suite;

	NET_DBG("Server selected cipher suite 0x%04x", cipher_suite);

	/* Legacy compression method (skip) */
	if (pos >= msg_len) {
		return -EINVAL;
	}
	pos++;

	/* Extensions length */
	if (pos + 2 > msg_len) {
		return -EINVAL;
	}
	extensions_len = (msg[pos] << 8) | msg[pos + 1];
	pos += 2;

	if (pos + extensions_len > msg_len) {
		return -EINVAL;
	}
	extensions_end = pos + extensions_len;

	/* Parse extensions */
	while (pos + 4 <= extensions_end) {
		uint16_t ext_type = (msg[pos] << 8) | msg[pos + 1];
		uint16_t ext_len = (msg[pos + 2] << 8) | msg[pos + 3];

		pos += 4;
		if (pos + ext_len > extensions_end) {
			return -EINVAL;
		}

		switch (ext_type) {
		case TLS_EXT_SUPPORTED_VERSIONS:
			if (ext_len >= 2) {
				uint16_t version = (msg[pos] << 8) | msg[pos + 1];

				if (version != 0x0304) {
					NET_DBG("Server selected non-TLS 1.3 version 0x%04x",
						version);
					return -ENOTSUP;
				}
				got_version = true;
			}
			break;

		case TLS_EXT_KEY_SHARE:
			if (ext_len >= 4) {
				uint16_t group = (msg[pos] << 8) | msg[pos + 1];
				uint16_t key_len = (msg[pos + 2] << 8) | msg[pos + 3];

				if (group != 0x0017) {  /* secp256r1 */
					NET_DBG("Unsupported key share group 0x%04x", group);
					return -ENOTSUP;
				}

				if (pos + 4 + key_len > extensions_end) {
					return -EINVAL;
				}

				/* Store server's public key */
				if (key_len > sizeof(ctx->peer_public_key)) {
					return -ENOBUFS;
				}

				memcpy(ctx->peer_public_key, &msg[pos + 4], key_len);
				ctx->peer_public_key_len = key_len;
				got_key_share = true;

				NET_DBG("Got server key share, len: %u", key_len);
			}
			break;

		default:
			/* Ignore unknown extensions */
			break;
		}

		pos += ext_len;
	}

	if (!got_version) {
		NET_DBG("ServerHello missing supported_versions extension");
		return -EINVAL;
	}

	if (!got_key_share) {
		NET_DBG("ServerHello missing key_share extension");
		return -EINVAL;
	}

	return 0;
}

/*
 * Parse EncryptedExtensions message (client side)
 *
 * EncryptedExtensions format:
 * - extensions length: 2 bytes
 * - extensions: variable
 *
 * For QUIC, the transport parameters extension is critical.
 */
static int parse_encrypted_extensions(struct quic_tls_context *ctx,
				      const uint8_t *msg, size_t msg_len)
{
	size_t pos = 0;
	size_t extensions_len;
	size_t extensions_end;

	if (msg_len < 2) {
		NET_DBG("EncryptedExtensions too short");
		return -EINVAL;
	}

	/* Extensions length */
	extensions_len = (msg[pos] << 8) | msg[pos + 1];
	pos += 2;

	if (pos + extensions_len > msg_len) {
		return -EINVAL;
	}
	extensions_end = pos + extensions_len;

	/* Parse extensions */
	while (pos + 4 <= extensions_end) {
		uint16_t ext_type = (msg[pos] << 8) | msg[pos + 1];
		uint16_t ext_len = (msg[pos + 2] << 8) | msg[pos + 3];

		pos += 4;
		if (pos + ext_len > extensions_end) {
			return -EINVAL;
		}

		switch (ext_type) {
		case TLS_EXT_QUIC_TRANSPORT_PARAMS:
			if (ext_len <= sizeof(ctx->peer_tp)) {
				NET_DBG("Got server transport params, len: %u", ext_len);
				memcpy(ctx->peer_tp, &msg[pos], ext_len);
				ctx->peer_tp_len = ext_len;
			}
			break;

		case TLS_EXT_ALPN:
			/* Parse ALPN if present */
			if (ext_len >= 2) {
				size_t alpn_list_len = (msg[pos] << 8) | msg[pos + 1];
				size_t alpn_pos = 2;

				if (alpn_list_len > 0 && alpn_pos < ext_len) {
					uint8_t proto_len = msg[pos + alpn_pos];

					if (proto_len > 0 && alpn_pos + 1 + proto_len <= ext_len) {
						/* Store negotiated ALPN */
						NET_DBG("Server selected ALPN: %.*s",
							proto_len, &msg[pos + alpn_pos + 1]);
					}
				}
			}
			break;

		default:
			/* Ignore unknown extensions */
			break;
		}

		pos += ext_len;
	}

	return 0;
}

/*
 * Handle ServerHello message on client side
 */
static int handle_server_hello(struct quic_tls_context *ctx,
			       const uint8_t *msg, size_t msg_len,
			       const uint8_t *full_msg, size_t full_msg_len)
{
	uint16_t old_cipher_suite = ctx->ks.cipher_suite;
	psa_algorithm_t old_hash_alg = ctx->ks.hash_alg;
	size_t saved_transcript_len;
	int ret;

	/* Save transcript length before ServerHello, this is the ClientHello */
	saved_transcript_len = ctx->transcript_len;

	/* Parse ServerHello to extract server's key share and cipher suite */
	ret = parse_server_hello(ctx, msg, msg_len);
	if (ret != 0) {
		return ret;
	}

	/* Check if server selected a cipher suite with different hash algorithm.
	 * If so, we need to reinitialize the key schedule with the new hash
	 * algorithm and re-hash the ClientHello from the transcript buffer.
	 */
	if (ctx->ks.cipher_suite != old_cipher_suite) {
		psa_algorithm_t new_hash_alg;

		/* Determine new hash algorithm */
		switch (ctx->ks.cipher_suite) {
		case TLS_AES_256_GCM_SHA384:
			new_hash_alg = PSA_ALG_SHA_384;
			break;
		case TLS_AES_128_GCM_SHA256:
		case TLS_CHACHA20_POLY1305_SHA256:
		default:
			new_hash_alg = PSA_ALG_SHA_256;
			break;
		}

		if (new_hash_alg != old_hash_alg) {
			psa_status_t status;

			NET_DBG("Cipher suite changed from 0x%04x to 0x%04x, "
				"reinitializing key schedule",
				old_cipher_suite, ctx->ks.cipher_suite);

			/* Abort the old transcript hash */
			psa_hash_abort(&ctx->ks.transcript_hash);

			/* Reinitialize key schedule with new cipher suite */
			ret = key_schedule_init(ctx);
			if (ret != 0) {
				NET_DBG("Failed to reinit key schedule: %d", ret);
				return ret;
			}

			/* Re-hash the ClientHello from transcript buffer */
			status = psa_hash_update(&ctx->ks.transcript_hash,
						 ctx->transcript_buffer,
						 saved_transcript_len);
			if (status != PSA_SUCCESS) {
				NET_DBG("Failed to re-hash ClientHello (%d)", status);
				return -EIO;
			}
		}
	}

	/* Update transcript with ServerHello */
	ret = transcript_update(ctx, full_msg, full_msg_len);
	if (ret != 0) {
		return ret;
	}

	/* Compute shared secret using server's public key */
	ret = compute_shared_secret(ctx);
	if (ret != 0) {
		return ret;
	}

	/* Derive handshake secrets */
	ret = derive_handshake_secrets(ctx);
	if (ret != 0) {
		return ret;
	}

	NET_DBG("[%p] Client processed ServerHello, handshake keys derived", ctx->ep);

	ctx->state = QUIC_TLS_STATE_WAIT_ENCRYPTED_EXTENSIONS;

	return 0;
}

/*
 * Process ClientHello and generate server response
 *
 * Uses endpoint's rx_buffer as scratch space to avoid large stack allocations.
 */
static int handle_client_hello(struct quic_tls_context *ctx,
			       const uint8_t *msg, size_t msg_len,
			       const uint8_t *full_msg, size_t full_msg_len)
{
	struct quic_endpoint *ep = ctx->ep;
	size_t buf_size = sizeof(ep->crypto.rx_buffer);
	uint8_t *server_hello = ep->crypto.rx_buffer;
	size_t server_hello_size = buf_size / 2;
	uint8_t *wrapped = ep->crypto.rx_buffer + server_hello_size;
	size_t wrapped_size = buf_size - server_hello_size;
	size_t sh_len, wrapped_len;
	int ret;

	ret = parse_client_hello(ctx, msg, msg_len);
	if (ret != 0) {
		/* In QUIC, TLS alerts are sent via CONNECTION_CLOSE frames
		 * with error code 0x100 + TLS_alert_code (RFC 9001 Section 4.8)
		 */
		if (ctx->ep != NULL) {
			if (ret == -ENOPROTOOPT) {
				/* ALPN mismatch, TLS alert 120 (no_application_protocol) */
				quic_endpoint_send_connection_close(ctx->ep,
					QUIC_ERROR_CRYPTO_BASE + TLS_ALERT_NO_APPLICATION_PROTOCOL,
					"No matching ALPN protocol");
			} else {
				/* Generic handshake failure, TLS alert 40 */
				quic_endpoint_send_connection_close(ctx->ep,
					QUIC_ERROR_CRYPTO_BASE + TLS_ALERT_HANDSHAKE_FAILURE,
					"TLS handshake failure");
			}
		}
		return ret;
	}

	/* Build default transport parameters if not already set */
	if (ctx->local_tp_len == 0) {
		ret = build_default_transport_params(ctx);
		if (ret != 0) {
			NET_DBG("Failed to build transport parameters: %d", ret);
			return ret;
		}
	}

	/* Initialize key schedule with selected cipher */
	ret = key_schedule_init(ctx);
	if (ret != 0) {
		return ret;
	}

	/* Generate our ECDH key pair */
	ret = generate_ecdh_keypair(ctx);
	if (ret != 0) {
		return ret;
	}

	/* Compute shared secret */
	ret = compute_shared_secret(ctx);
	if (ret != 0) {
		return ret;
	}

	/* Update transcript with ClientHello (full message with header) */
	ret = transcript_update(ctx, full_msg, full_msg_len);
	if (ret != 0) {
		return ret;
	}

	/* Build ServerHello */
	ret = build_server_hello(ctx, server_hello, server_hello_size, &sh_len);
	if (ret != 0) {
		return ret;
	}

	ret = wrap_handshake_message(TLS_HS_SERVER_HELLO,
				     server_hello, sh_len,
				     wrapped, wrapped_size,
				     &wrapped_len);
	if (ret != 0) {
		return ret;
	}

	/* Update transcript with ServerHello */
	ret = transcript_update(ctx, wrapped, wrapped_len);
	if (ret != 0) {
		return ret;
	}

	/* Derive handshake secrets */
	ret = derive_handshake_secrets(ctx);
	if (ret != 0) {
		return ret;
	}

	/* Send ServerHello via callback (Initial level) */
	if (ctx->send_cb != NULL) {
		ret = ctx->send_cb(ctx->user_data, QUIC_SECRET_LEVEL_INITIAL,
				   wrapped, wrapped_len);
		if (ret != 0) {
			NET_DBG("Failed to send ServerHello:  %d", ret);
			return ret;
		}
	}

	/*
	 * Now send the rest at HANDSHAKE level.
	 * The handshake secrets were just installed by derive_handshake_secrets().
	 */
	ret = send_server_handshake_flight(ctx);
	if (ret != 0) {
		NET_DBG("Failed to send server handshake flight: %d", ret);
		return ret;
	}

	ctx->state = QUIC_TLS_STATE_WAIT_FINISHED;

	return 0;
}

/*
 * Parse Certificate message from peer
 */
static int parse_certificate(struct quic_tls_context *ctx,
			     const uint8_t *data, size_t len)
{
	size_t pos = 0;
	uint8_t context_len;
	uint32_t cert_list_len;
	int cert_count = 0;
	size_t cert_list_end;

	if (len < 4) {
		return -EINVAL;
	}

	/* Certificate request context */
	context_len = data[pos++];

	/* If we sent a CertificateRequest, verify the context matches */
	if (ctx->expecting_client_cert && ctx->cert_request_context_len > 0) {
		if (context_len != ctx->cert_request_context_len ||
		    memcmp(&data[pos], ctx->cert_request_context, context_len) != 0) {
			NET_DBG("Certificate context mismatch");
			return -EINVAL;
		}
	}

	pos += context_len;

	/* Certificate list length (3 bytes) */
	if (pos + 3 > len) {
		return -EINVAL;
	}

	cert_list_len = ((uint32_t)data[pos] << 16) |
			((uint32_t)data[pos + 1] << 8) |
			 (uint32_t)data[pos + 2];
	pos += 3;

	if (pos + cert_list_len > len) {
		NET_DBG("Certificate list truncated");
		return -EINVAL;
	}

	/* Empty certificate list is valid (no client cert) */
	if (cert_list_len == 0) {
		if (ctx->expecting_client_cert) {
			if (ctx->options.verify_level == MBEDTLS_SSL_VERIFY_REQUIRED) {
				NET_DBG("Client certificate required but not provided");
				return -EACCES;
			}

			NET_DBG("Client did not provide certificate (client auth optional)");
		} else {
			NET_DBG("Peer sent empty certificate (no client auth)");
		}

		return 0;
	}

	/* Parse certificate entries */
	cert_list_end = pos + cert_list_len;

	while (pos < cert_list_end) {
		uint32_t cert_data_len;
		uint16_t extensions_len;

		/* cert_data length */
		if (pos + 3 > cert_list_end) {
			return -EINVAL;
		}

		cert_data_len = ((uint32_t)data[pos] << 16) |
			((uint32_t)data[pos + 1] << 8) |
			(uint32_t)data[pos + 2];
		pos += 3;

		if (pos + cert_data_len > cert_list_end) {
			return -EINVAL;
		}

		/* Store first certificate (end-entity cert) for verification */
		if (cert_count == 0) {
			if (cert_data_len > sizeof(ctx->peer_cert)) {
				NET_DBG("Peer certificate too large:  %u > %zu",
					cert_data_len, sizeof(ctx->peer_cert));
				return -ENOBUFS;
			}

			memcpy(ctx->peer_cert, &data[pos], cert_data_len);
			ctx->peer_cert_len = cert_data_len;

			NET_DBG("Stored peer certificate, %u bytes", cert_data_len);
		}

		pos += cert_data_len;
		cert_count++;

		/* Extensions */
		if (pos + 2 > cert_list_end) {
			return -EINVAL;
		}

		extensions_len = ((uint16_t)data[pos] << 8) | data[pos + 1];
		pos += 2;

		/* Skip extensions for now */
		if (pos + extensions_len > cert_list_end) {
			return -EINVAL;
		}

		pos += extensions_len;
	}

	NET_DBG("Parsed %d certificates from peer", cert_count);

	return 0;
}

/*
 * Parse CertificateRequest message (client-side)
 */
static int parse_certificate_request(struct quic_tls_context *ctx,
						    const uint8_t *data, size_t len)
{
	size_t pos = 0;
	uint8_t context_len;
	uint16_t extensions_len;
	size_t ext_end;

	if (len < 3) {
		return -EINVAL;
	}

	/* Certificate request context, save for response */
	context_len = data[pos++];
	if (pos + context_len > len) {
		return -EINVAL;
	}

	if (context_len > sizeof(ctx->cert_request_context)) {
		return -ENOBUFS;
	}

	memcpy(ctx->cert_request_context, &data[pos], context_len);
	ctx->cert_request_context_len = context_len;
	pos += context_len;

	/* Extensions */
	if (pos + 2 > len) {
		return -EINVAL;
	}

	extensions_len = ((uint16_t)data[pos] << 8) | data[pos + 1];
	pos += 2;

	/* Parse extensions to find signature_algorithms */
	ext_end = pos + extensions_len;

	while (pos + 4 <= ext_end) {
		uint16_t ext_type = ((uint16_t)data[pos] << 8) | data[pos + 1];
		uint16_t ext_len = ((uint16_t)data[pos + 2] << 8) | data[pos + 3];

		pos += 4;

		if (pos + ext_len > ext_end) {
			break;
		}

		if (ext_type == 0x000D) {  /* signature_algorithms */
			NET_DBG("Server requested client certificate with signature_algorithms");
			/* TODO: Store acceptable algorithms */
		}

		pos += ext_len;
	}

	ctx->server_requested_cert = true;

	NET_DBG("Received CertificateRequest, context_len=%u", context_len);

	return 0;
}

/*
 * Extract public key from DER-encoded X. 509 certificate using mbedtls
 */
static int extract_pubkey_from_cert(const uint8_t *cert, size_t cert_len,
				    uint8_t *pubkey, size_t pubkey_size,
				    size_t *pubkey_len)
{
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	mbedtls_x509_crt crt;
	psa_key_attributes_t pk_attr = PSA_KEY_ATTRIBUTES_INIT;
	mbedtls_svc_key_id_t pub_key_id = MBEDTLS_SVC_KEY_ID_INIT;
	psa_status_t status;
	int ret = 0;

	mbedtls_x509_crt_init(&crt);

	ret = mbedtls_x509_crt_parse_der(&crt, cert, cert_len);
	if (ret != 0) {
		NET_DBG("Failed to parse certificate for pubkey extraction: -0x%04x", -ret);
		mbedtls_x509_crt_free(&crt);
		return -EINVAL;
	}

	status = mbedtls_pk_get_psa_attributes(&crt.pk, PSA_KEY_USAGE_VERIFY_HASH, &pk_attr);
	if (status != PSA_SUCCESS) {
		NET_DBG("Failed to get PSA attributes from cert pk: %d", status);
		ret = -EINVAL;
		goto out;
	}

	if (!PSA_KEY_TYPE_IS_ECC(psa_get_key_type(&pk_attr))) {
		NET_DBG("Certificate does not contain an EC key");
		psa_reset_key_attributes(&pk_attr);
		ret = -EINVAL;
		goto out;
	}

	status = mbedtls_pk_import_into_psa(&crt.pk, &pk_attr, &pub_key_id);
	psa_reset_key_attributes(&pk_attr);
	if (status != PSA_SUCCESS) {
		NET_DBG("Failed to import cert public key into PSA: %d", status);
		ret = -EINVAL;
		goto out;
	}

	/* psa_export_public_key gives uncompressed point: 04 || X || Y */
	status = psa_export_public_key(pub_key_id, pubkey, pubkey_size, pubkey_len);
	psa_destroy_key(pub_key_id);
	if (status != PSA_SUCCESS) {
		NET_DBG("Failed to export public key: %d", status);
		ret = -EINVAL;
		goto out;
	}

out:
	mbedtls_x509_crt_free(&crt);
	return ret;
#else
	return -ENOTSUP;
#endif
}

/* Convert DER-encoded ECDSA signature to raw concatenated r||s format for PSA */
static int ecdsa_der_to_raw(const uint8_t *der, size_t der_len,
			    uint8_t *raw, size_t raw_size, size_t *raw_len,
			    /* scalar_len = 32 for P-256, 48 for P-384 */
			    size_t scalar_len)
{
	size_t pos = 0;

	/* SEQUENCE */
	if (pos >= der_len) {
		return -EINVAL;
	}

	if (der[pos++] != 0x30) {
		return -EINVAL;
	}

	/* Skip length (could be 1 or 2 bytes) */
	if (pos >= der_len) {
		return -EINVAL;
	}

	if (der[pos] & 0x80) {
		pos += (der[pos] & 0x7f) + 1;
	} else {
		pos++;
	}

	if (raw_size < 2 * scalar_len) {
		return -ENOMEM;
	}

	memset(raw, 0, 2 * scalar_len);

	for (int i = 0; i < 2; i++) {
		size_t out_off = i * scalar_len;
		size_t int_len;

		/* INTEGER tag */
		if (pos >= der_len) {
			return -EINVAL;
		}

		if (der[pos++] != 0x02) {
			return -EINVAL;
		}

		if (pos >= der_len) {
			return -EINVAL;
		}

		int_len = der[pos++];

		if (pos + int_len > der_len) {
			return -EINVAL;
		}

		/* Strip leading zero padding (DER uses it to signal positive) */
		while (int_len > scalar_len && der[pos] == 0x00) {
			pos++;
			int_len--;
		}

		if (int_len > scalar_len) {
			return -EINVAL;
		}

		/* Right-align into the output slot */
		memcpy(&raw[out_off + scalar_len - int_len], &der[pos], int_len);
		pos += int_len;
	}

	*raw_len = 2 * scalar_len;

	return 0;
}

/*
 * Verify signature using public key from X.509 certificate
 */
static int verify_signature_with_cert(const uint8_t *cert, size_t cert_len,
				      uint16_t sig_alg,
				      const uint8_t *data, size_t data_len,
				      const uint8_t *sig, size_t sig_len)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = 0;
	psa_status_t status;
	psa_algorithm_t alg;
	int ret = 0;
	uint8_t pubkey[65];  /* Max size for uncompressed P-256 point */
	size_t pubkey_len;
	uint8_t raw_sig[96]; /* 2 * 48 for P-384 max */
	size_t raw_sig_len;
	size_t scalar_len = (sig_alg == MBEDTLS_TLS1_3_SIG_ECDSA_SECP384R1_SHA384) ? 48 : 32;

	/* Determine PSA algorithm from TLS signature algorithm */
	switch (sig_alg) {
	case MBEDTLS_TLS1_3_SIG_ECDSA_SECP256R1_SHA256: /* 0x0403 */
		alg = PSA_ALG_ECDSA(PSA_ALG_SHA_256);
		break;
	case MBEDTLS_TLS1_3_SIG_ECDSA_SECP384R1_SHA384: /* 0x0503 */
		alg = PSA_ALG_ECDSA(PSA_ALG_SHA_384);
		break;
	case MBEDTLS_TLS1_3_SIG_RSA_PSS_RSAE_SHA256:    /* 0x0804 */
		alg = PSA_ALG_RSA_PSS(PSA_ALG_SHA_256);
		break;
	default:
		NET_DBG("Unsupported signature algorithm:  0x%04x", sig_alg);
		return -ENOTSUP;
	}

	/* Extract public key from DER-encoded X.509 certificate */
	/* TODO/FIXME: This is a simplified extraction, we need
	 * proper ASN.1 parsing or use mbedtls_x509_crt_parse
	 */
	ret = extract_pubkey_from_cert(cert, cert_len, pubkey, sizeof(pubkey), &pubkey_len);
	if (ret != 0) {
		NET_DBG("Failed to extract public key from certificate");
		return ret;
	}

	/* Import public key */
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_VERIFY_MESSAGE);
	psa_set_key_algorithm(&attr, alg);
	psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));

	status = psa_import_key(&attr, pubkey, pubkey_len, &key_id);
	if (status != PSA_SUCCESS) {
		NET_DBG("Failed to import public key:  %d", status);
		return -EIO;
	}

	ret = ecdsa_der_to_raw(sig, sig_len, raw_sig, sizeof(raw_sig),
			       &raw_sig_len, scalar_len);
	if (ret != 0) {
		NET_DBG("Failed to convert DER signature to raw: %d", ret);
		psa_destroy_key(key_id);
		return ret;
	}

	/* Verify signature */
	status = psa_verify_message(key_id, alg, data, data_len, raw_sig, raw_sig_len);

	psa_destroy_key(key_id);

	if (status != PSA_SUCCESS) {
		NET_DBG("Signature verification failed: %d", status);
		return -EBADMSG;
	}

	return 0;
}

/*
 * Verify CertificateVerify message
 */
static int verify_certificate_verify(struct quic_tls_context *ctx,
				     const uint8_t *data, size_t len)
{
	static const char server_context[] = "TLS 1.3, server CertificateVerify";
	static const char client_context[] = "TLS 1.3, client CertificateVerify";
	const char *context_string;
	size_t context_len;
	uint16_t sig_alg;
	uint16_t sig_len;
	size_t pos = 0;
	uint8_t transcript[QUIC_HASH_MAX_LEN];
	size_t transcript_len;
	uint8_t to_verify[64 + 34 + QUIC_HASH_MAX_LEN];  /* 64 spaces + context + hash */
	size_t to_verify_len;
	int ret;

	if (len < 4) {
		return -EINVAL;
	}

	/* Signature algorithm */
	sig_alg = ((uint16_t)data[pos] << 8) | data[pos + 1];
	pos += 2;

	/* Signature length */
	sig_len = ((uint16_t)data[pos] << 8) | data[pos + 1];
	pos += 2;

	if (pos + sig_len > len) {
		return -EINVAL;
	}

	NET_DBG("CertificateVerify: sig_alg=0x%04x, sig_len=%u", sig_alg, sig_len);

	/* Determine context string based on who sent this */
	if (ctx->ep->is_server) {
		/* We're server, so this is from client */
		context_string = client_context;
	} else {
		/* We're client, so this is from server */
		context_string = server_context;
	}

	context_len = strlen(context_string) + 1;  /* Include null terminator */

	/* Get transcript hash at this point (before CertificateVerify message).
	 * This is called BEFORE transcript_update, so we get the correct hash.
	 */
	ret = transcript_hash_get(ctx, transcript, &transcript_len);
	if (ret != 0) {
		return ret;
	}

	/* Build content that was signed:
	 * - 64 bytes of 0x20 (space)
	 * - context string with null terminator
	 * - transcript hash
	 */
	memset(to_verify, 0x20, 64);
	to_verify_len = 64;

	memcpy(&to_verify[to_verify_len], context_string, context_len);
	to_verify_len += context_len;

	memcpy(&to_verify[to_verify_len], transcript, transcript_len);
	to_verify_len += transcript_len;

	/* Verify signature using peer's public key from certificate */
	if (ctx->peer_cert_len == 0) {
		NET_DBG("No peer certificate to verify signature");
		return -EINVAL;
	}

	/* Extract public key from peer certificate and verify */
	/* This requires parsing the X.509 certificate */
	ret = verify_signature_with_cert(ctx->peer_cert, ctx->peer_cert_len,
					 sig_alg,
					 to_verify, to_verify_len,
					 &data[pos], sig_len);
	if (ret != 0) {
		return ret;
	}

	NET_DBG("CertificateVerify signature verified successfully");

	return 0;
}

/*
 * Process a single handshake message
 */
static int process_handshake_message(struct quic_tls_context *ctx,
				     uint8_t msg_type,
				     const uint8_t *msg, size_t msg_len,
				     const uint8_t *full_msg, size_t full_msg_len)
{
	int ret;

	NET_DBG("Processing handshake message type %d, len %zu", msg_type, msg_len);

	switch (msg_type) {
	case TLS_HS_CLIENT_HELLO:
		if (!ctx->ep->is_server) {
			return -EINVAL;
		}
		return handle_client_hello(ctx, msg, msg_len, full_msg, full_msg_len);

	case TLS_HS_SERVER_HELLO:
		if (ctx->ep->is_server) {
			return -EINVAL;
		}
		return handle_server_hello(ctx, msg, msg_len, full_msg, full_msg_len);

	case TLS_HS_ENCRYPTED_EXTENSIONS:
		/* Parse transport parameters and other extensions */
		if (!ctx->ep->is_server) {
			ret = parse_encrypted_extensions(ctx, msg, msg_len);
			if (ret != 0) {
				NET_DBG("Failed to parse EncryptedExtensions: %d", ret);
				return ret;
			}
		}
		/* Update transcript */
		ret = transcript_update(ctx, full_msg, full_msg_len);
		if (ret != 0) {
			return ret;
		}
		break;

	case TLS_HS_CERTIFICATE:
		ret = transcript_update(ctx, full_msg, full_msg_len);
		if (ret != 0) {
			return ret;
		}

		/* Parse the Certificate message to extract peer's certificate */
		ret = parse_certificate(ctx, msg, msg_len);
		if (ret != 0) {
			NET_DBG("Failed to parse Certificate message: %d", ret);
			return ret;
		}

		/* Verify the peer certificate against CA chain if configured */
		if (ctx->peer_cert_len > 0) {
			ret = verify_peer_certificate(ctx, ctx->peer_cert, ctx->peer_cert_len);
			if (ret != 0) {
				NET_DBG("Peer certificate verification failed: %d", ret);
				/* Continue if verify_level allows it (checked inside function) */
			}
		}
		break;

	case TLS_HS_CERTIFICATE_VERIFY:
		/* Verify signature BEFORE updating transcript, since
		 * the signature is over the transcript up to this point
		 */
		ret = verify_certificate_verify(ctx, msg, msg_len);
		if (ret != 0) {
			NET_DBG("CertificateVerify verification failed: %d", ret);
			return ret;
		}

		ret = transcript_update(ctx, full_msg, full_msg_len);
		if (ret != 0) {
			return ret;
		}
		break;

	case TLS_HS_CERTIFICATE_REQUEST:
		/* Server is requesting client certificate (client-side only) */
		if (ctx->ep->is_server) {
			NET_DBG("Server received CertificateRequest, invalid in server context");
			return -EINVAL;
		}

		ret = parse_certificate_request(ctx, msg, msg_len);
		if (ret != 0) {
			NET_DBG("Failed to parse CertificateRequest: %d", ret);
			return ret;
		}

		ret = transcript_update(ctx, full_msg, full_msg_len);
		if (ret != 0) {
			return ret;
		}
		break;

	case TLS_HS_FINISHED:
		NET_DBG("[%p] HS finished", ctx);
		/* Update transcript with Finished message */
		ret = transcript_update(ctx, full_msg, full_msg_len);
		if (ret != 0) {
			return ret;
		}

		/* For client receiving server's Finished, save the transcript hash
		 * for application secret derivation (RFC 8446 Section 7.1),
		 * then send client's Finished to complete the handshake.
		 */
		if (!ctx->ep->is_server) {
			ret = transcript_hash_get(ctx, ctx->ks.server_finished_transcript,
						  &ctx->ks.server_finished_transcript_len);
			if (ret != 0) {
				NET_DBG("Failed to save server finished transcript");
				return ret;
			}

			/* If server requested client certificate, send it before Finished */
			if (ctx->server_requested_cert) {
				ret = send_client_certificate(ctx);
				if (ret != 0) {
					NET_DBG("Failed to send client certificate: %d", ret);
					return ret;
				}
			}

			/* Send client's Finished message to server */
			ret = send_client_finished(ctx);
			if (ret != 0) {
				NET_DBG("Failed to send client Finished: %d", ret);
				return ret;
			}
		}

		ctx->state = QUIC_TLS_STATE_CONNECTED;
		return 1;  /* Handshake complete */

	default:
		NET_DBG("Unknown handshake message type (%d)", msg_type);
		break;
	}

	return 0;
}

static void quic_tls_free(struct quic_tls_context *ctx)
{
	if (ctx->ecdh_key_id != 0) {
		psa_destroy_key(ctx->ecdh_key_id);
	}

	if (ctx->signing_key_id != 0) {
		psa_destroy_key(ctx->signing_key_id);
	}

	if (ctx->ks.initialized) {
		psa_hash_abort(&ctx->ks.transcript_hash);
	}

	/* Clear sensitive data */
	memset(ctx->shared_secret, 0, sizeof(ctx->shared_secret));
	memset(&ctx->ks, 0, sizeof(ctx->ks));
}

/*
 * Get next packet number for the given encryption level
 * and increment the counter.
 */
static uint64_t get_next_packet_number(struct quic_endpoint *ep,
				       enum quic_secret_level level)
{
	uint64_t pn;

	switch (level) {
	case QUIC_SECRET_LEVEL_INITIAL:
		pn = ep->tx_pn.initial++;
		break;
	case QUIC_SECRET_LEVEL_HANDSHAKE:
		pn = ep->tx_pn.handshake++;
		break;
	case QUIC_SECRET_LEVEL_APPLICATION:
	default:
		pn = ep->tx_pn.application++;
		break;
	}

	return pn;
}

/*
 * Determine packet number encoding length based on value.
 * RFC 9000 Section 17.1 recommends encoding at least one more
 * byte than needed for the actual value.
 */
static size_t get_pn_length(uint64_t pn)
{
	if (pn < 0x80) {
		return 1;
	} else if (pn < 0x4000) {
		return 2;
	} else if (pn < 0x200000) {
		return 3;
	}

	return 4;
}

/*
 * Calculate padding needed to meet minimum packet size requirements.
 *
 * RFC 9000 Section 14.1:  A client MUST expand the payload of all UDP
 * datagrams carrying Initial packets to at least the smallest allowed
 * maximum datagram size of 1200 bytes.
 */
static size_t calculate_padding_for_level(struct quic_endpoint *ep,
					  enum quic_secret_level level,
					  size_t current_payload_len,
					  size_t pn_len)
{
	size_t min_packet_size;
	size_t header_estimate;
	size_t current_total;
	size_t min_plaintext_for_hp;
	size_t padding = 0;

	/*
	 * Header protection sample is 16 bytes starting at (4 - pn_len) bytes
	 * into the ciphertext. With typical pn_len=1, sample starts at byte 3.
	 * Ciphertext = plaintext + 16 (AEAD tag).
	 * We need: sample_offset + 16 <= ciphertext_len
	 *          (4 - pn_len) + 16 <= plaintext_len + 16
	 *          4 - pn_len <= plaintext_len
	 * With pn_len=1: plaintext >= 3 bytes
	 * To be safe (pn_len can be 1-4), require at least 4 bytes of plaintext.
	 */
	min_plaintext_for_hp = QUIC_HP_MAX_PN_LEN; /* 4 bytes */
	if (current_payload_len < min_plaintext_for_hp) {
		padding = min_plaintext_for_hp - current_payload_len;
	}

	/* Only client Initial packets require padding to 1200 bytes */
	if (level != 0 || ep->is_server) {
		return padding;
	}

	min_packet_size = 1200;

	/* Estimate header size:
	 * 1 (first byte) + 4 (version) + 1 (DCID len) + DCID +
	 * 1 (SCID len) + SCID + 1 (token len) + 2 (length) + 4 (PN max)
	 */
	header_estimate = 1 + 4 + 1 + ep->peer_cid_len + 1 + ep->my_cid_len + 1 + 2 + pn_len;

	current_total = header_estimate + current_payload_len + padding + QUIC_AEAD_TAG_LEN;

	if (current_total >= min_packet_size) {
		return padding;
	}

	return padding + (min_packet_size - current_total);
}

/*
 * Build Initial packet header (RFC 9000 Section 17.2.2)
 *
 * Initial Packet {
 *   Header Form (1) = 1,
 *   Fixed Bit (1) = 1,
 *   Long Packet Type (2) = 0,
 *   Reserved Bits (2),
 *   Packet Number Length (2),
 *   Version (32),
 *   Destination Connection ID Length (8),
 *   Destination Connection ID (0..160),
 *   Source Connection ID Length (8),
 *   Source Connection ID (0..160),
 *   Token Length (i),
 *   Token (...),
 *   Length (i),
 *   Packet Number (8..32),
 * }
 */
static int build_initial_header(struct quic_endpoint *ep,
				uint64_t packet_number,
				size_t pn_len,
				size_t payload_len,
				const uint8_t *token, size_t token_len,
				uint8_t *out, size_t out_size,
				size_t *out_len, size_t *pn_offset)
{
	size_t pos = 0;
	size_t length_val;
	int token_len_size;
	int ret;

	/* First byte: 1 1 00 RR PP
	 * Header Form = 1 (long)
	 * Fixed Bit = 1
	 * Long Packet Type = 0 (Initial)
	 * Reserved = 0 (will be protected)
	 * Packet Number Length = pn_len - 1
	 */
	if (pos >= out_size) {
		return -ENOBUFS;
	}

	out[pos++] = QUIC_LONG_HEADER_INITIAL | ((pn_len - 1) & 0x03);

	/* Version (4 bytes, big-endian) */
	if (pos + 4 > out_size) {
		return -ENOBUFS;
	}

	out[pos++] = (QUIC_VERSION_1 >> 24) & 0xFF;
	out[pos++] = (QUIC_VERSION_1 >> 16) & 0xFF;
	out[pos++] = (QUIC_VERSION_1 >> 8) & 0xFF;
	out[pos++] = QUIC_VERSION_1 & 0xFF;

	/* Destination Connection ID Length (1 byte) + DCID */
	if (pos + 1 + ep->peer_cid_len > out_size) {
		return -ENOBUFS;
	}

	out[pos++] = ep->peer_cid_len;
	if (ep->peer_cid_len > 0) {
		memcpy(&out[pos], ep->peer_cid, ep->peer_cid_len);
		pos += ep->peer_cid_len;
	}

	/* Source Connection ID Length (1 byte) + SCID */
	if (pos + 1 + ep->my_cid_len > out_size) {
		return -ENOBUFS;
	}

	out[pos++] = ep->my_cid_len;
	if (ep->my_cid_len > 0) {
		memcpy(&out[pos], ep->my_cid, ep->my_cid_len);
		pos += ep->my_cid_len;
	}

	/* Token Length (variable-length integer) + Token */
	if (token == NULL) {
		token_len = 0;
	}

	token_len_size = quic_get_varint_size(token_len);
	if (pos + token_len_size + token_len > out_size) {
		return -ENOBUFS;
	}

	ret = quic_put_len(&out[pos], out_size - pos, token_len);
	if (ret != 0) {
		return ret;
	}

	pos += token_len_size;

	if (token_len > 0) {
		memcpy(&out[pos], token, token_len);
		pos += token_len;
	}

	/*
	 * Length field (variable-length integer)
	 * Contains:  packet number length + encrypted payload length (with tag)
	 *
	 * Use 2-byte encoding (0x40 prefix) to ensure consistent sample offset
	 * for header protection. This works for lengths up to 16383.
	 */
	length_val = pn_len + payload_len;
	if (length_val > 0x3FFF) {
		/* Need 4-byte encoding for very large packets */
		if (pos + 4 > out_size) {
			return -ENOBUFS;
		}
		out[pos++] = 0x80 | ((length_val >> 24) & 0x3F);
		out[pos++] = (length_val >> 16) & 0xFF;
		out[pos++] = (length_val >> 8) & 0xFF;
		out[pos++] = length_val & 0xFF;
	} else {
		/* 2-byte encoding */
		if (pos + 2 > out_size) {
			return -ENOBUFS;
		}
		out[pos++] = 0x40 | ((length_val >> 8) & 0x3F);
		out[pos++] = length_val & 0xFF;
	}

	/* Record packet number offset (before writing PN) */
	*pn_offset = pos;

	/* Packet Number (1-4 bytes, big-endian) */
	if (pos + pn_len > out_size) {
		return -ENOBUFS;
	}

	for (size_t i = 0; i < pn_len; i++) {
		out[pos++] = (packet_number >> (8 * (pn_len - 1 - i))) & 0xFF;
	}

	*out_len = pos;

	return 0;
}

/*
 * Build Handshake packet header (RFC 9000 Section 17.2.4)
 *
 * Handshake Packet {
 *   Header Form (1) = 1,
 *   Fixed Bit (1) = 1,
 *   Long Packet Type (2) = 2,
 *   Reserved Bits (2),
 *   Packet Number Length (2),
 *   Version (32),
 *   Destination Connection ID Length (8),
 *   Destination Connection ID (0..160),
 *   Source Connection ID Length (8),
 *   Source Connection ID (0..160),
 *   Length (i),
 *   Packet Number (8..32),
 * }
 *
 * Note: Handshake packets don't have a Token field (unlike Initial).
 */
static int build_handshake_header(struct quic_endpoint *ep,
				  uint64_t packet_number,
				  size_t pn_len,
				  size_t payload_len,
				  uint8_t *out, size_t out_size,
				  size_t *out_len, size_t *pn_offset)
{
	size_t pos = 0;
	size_t length_val;

	/* First byte:  1 1 10 RR PP
	 * Header Form = 1 (long)
	 * Fixed Bit = 1
	 * Long Packet Type = 2 (Handshake)
	 * Reserved = 0 (will be protected)
	 * Packet Number Length = pn_len - 1
	 */
	if (pos >= out_size) {
		return -ENOBUFS;
	}

	out[pos++] = QUIC_LONG_HEADER_HANDSHAKE | ((pn_len - 1) & 0x03);

	/* Version (4 bytes, big-endian) */
	if (pos + 4 > out_size) {
		return -ENOBUFS;
	}
	out[pos++] = (QUIC_VERSION_1 >> 24) & 0xFF;
	out[pos++] = (QUIC_VERSION_1 >> 16) & 0xFF;
	out[pos++] = (QUIC_VERSION_1 >> 8) & 0xFF;
	out[pos++] = QUIC_VERSION_1 & 0xFF;

	/* Destination Connection ID Length (1 byte) + DCID */
	if (pos + 1 + ep->peer_cid_len > out_size) {
		return -ENOBUFS;
	}

	out[pos++] = ep->peer_cid_len;
	if (ep->peer_cid_len > 0) {
		memcpy(&out[pos], ep->peer_cid, ep->peer_cid_len);
		pos += ep->peer_cid_len;
	}

	/* Source Connection ID Length (1 byte) + SCID */
	if (pos + 1 + ep->my_cid_len > out_size) {
		return -ENOBUFS;
	}

	out[pos++] = ep->my_cid_len;
	if (ep->my_cid_len > 0) {
		memcpy(&out[pos], ep->my_cid, ep->my_cid_len);
		pos += ep->my_cid_len;
	}

	/*
	 * Length field (variable-length integer)
	 * Use 2-byte encoding for consistency with Initial packets.
	 */
	length_val = pn_len + payload_len;
	if (length_val > 0x3FFF) {
		/* 4-byte encoding */
		if (pos + 4 > out_size) {
			return -ENOBUFS;
		}
		out[pos++] = 0x80 | ((length_val >> 24) & 0x3F);
		out[pos++] = (length_val >> 16) & 0xFF;
		out[pos++] = (length_val >> 8) & 0xFF;
		out[pos++] = length_val & 0xFF;
	} else {
		/* 2-byte encoding */
		if (pos + 2 > out_size) {
			return -ENOBUFS;
		}
		out[pos++] = 0x40 | ((length_val >> 8) & 0x3F);
		out[pos++] = length_val & 0xFF;
	}

	/* Record packet number offset */
	*pn_offset = pos;

	/* Packet Number (1-4 bytes, big-endian) */
	if (pos + pn_len > out_size) {
		return -ENOBUFS;
	}

	for (size_t i = 0; i < pn_len; i++) {
		out[pos++] = (packet_number >> (8 * (pn_len - 1 - i))) & 0xFF;
	}

	*out_len = pos;

	return 0;
}

/*
 * Build short (1-RTT) packet header (RFC 9000 Section 17.3)
 *
 * 1-RTT Packet {
 *   Header Form (1) = 0,
 *   Fixed Bit (1) = 1,
 *   Spin Bit (1),
 *   Reserved Bits (2),
 *   Key Phase (1),
 *   Packet Number Length (2),
 *   Destination Connection ID (0..160),
 *   Packet Number (8..32),
 * }
 */
static int build_short_header(struct quic_endpoint *ep,
			      uint64_t packet_number,
			      size_t pn_len,
			      uint8_t *out, size_t out_size,
			      size_t *out_len, size_t *pn_offset)
{
	size_t pos = 0;

	/* First byte: 0 1 S RR K PP
	 * Header Form = 0 (short)
	 * Fixed Bit = 1
	 * Spin Bit = 0 (TODO: implement latency spin bit)
	 * Reserved = 0 (will be protected)
	 * Key Phase = 0 (TODO: implement key update)
	 * Packet Number Length = pn_len - 1
	 */
	if (pos >= out_size) {
		return -ENOBUFS;
	}

	out[pos++] = 0x40 | ((pn_len - 1) & 0x03);

	/* Destination Connection ID (no length prefix in short header) */
	if (pos + ep->peer_cid_len > out_size) {
		return -ENOBUFS;
	}

	if (ep->peer_cid_len > 0) {
		memcpy(&out[pos], ep->peer_cid, ep->peer_cid_len);
		pos += ep->peer_cid_len;
	}

	/* Record packet number offset */
	*pn_offset = pos;

	/* Packet Number (1-4 bytes, big-endian) */
	if (pos + pn_len > out_size) {
		return -ENOBUFS;
	}

	for (size_t i = 0; i < pn_len; i++) {
		out[pos++] = (packet_number >> (8 * (pn_len - 1 - i))) & 0xFF;
	}

	*out_len = pos;

	return 0;
}

/*
 * Build packet header for the specified encryption level.
 */
static int build_packet_header(struct quic_endpoint *ep,
			       enum quic_secret_level level,
			       uint64_t packet_number,
			       size_t pn_len,
			       size_t payload_len,
			       uint8_t *out, size_t out_size,
			       size_t *header_len, size_t *pn_offset)
{
	switch (level) {
	case QUIC_SECRET_LEVEL_INITIAL:
		return build_initial_header(ep, packet_number, pn_len,
					    payload_len, NULL, 0,
					    out, out_size,
					    header_len, pn_offset);
	case QUIC_SECRET_LEVEL_HANDSHAKE:
		return build_handshake_header(ep, packet_number, pn_len,
					      payload_len,
					      out, out_size,
					      header_len, pn_offset);
	case QUIC_SECRET_LEVEL_APPLICATION:
		return build_short_header(ep, packet_number, pn_len,
					  out, out_size,
					  header_len, pn_offset);
	default:
		break;
	}

	return -EINVAL;
}

/*
 * Get crypto context by encryption level
 */
static struct quic_crypto_context *
quic_get_crypto_context_by_level(struct quic_endpoint *ep,
				 enum quic_secret_level level)
{
	struct quic_crypto_context *ctx;

	switch (level) {
	case QUIC_SECRET_LEVEL_INITIAL:
		ctx = &ep->crypto.initial;
		break;
	case QUIC_SECRET_LEVEL_HANDSHAKE:
		ctx = &ep->crypto.handshake;
		break;
	case QUIC_SECRET_LEVEL_APPLICATION:
		ctx = &ep->crypto.application;
		break;
	default:
		NET_DBG("Invalid encryption level: %d", level);
		return NULL;
	}

	if (!ctx->initialized) {
		NET_DBG("Crypto context not initialized for level %d", level);
		NET_DBG("  ctx=%p, initialized=%d", ctx, ctx->initialized);
		NET_DBG("  tx.hp.initialized=%d, tx.pp.initialized=%d",
			ctx->tx.hp.initialized, ctx->tx.pp.initialized);
		NET_DBG("  rx.hp.initialized=%d, rx.pp.initialized=%d",
			ctx->rx.hp.initialized, ctx->rx.pp.initialized);
		return NULL;
	}

	return ctx;
}

/*
 * Apply header protection when header and payload are in separate buffers
 *
 * The sample for HP mask generation is taken from the ciphertext,
 * starting at offset (4 - pn_len) to account for variable PN length.
 */
static int quic_apply_header_protection_split(uint8_t *header, size_t header_len,
					      size_t pn_offset, size_t pn_len,
					      const uint8_t *ciphertext,
					      size_t ciphertext_len,
					      psa_key_id_t hp_key_id,
					      int cipher_algo)
{
	uint8_t mask[QUIC_HP_MASK_LEN];
	uint8_t sample[QUIC_HP_SAMPLE_LEN];
	size_t sample_offset;
	int ret;

	/*
	 * Sample location (RFC 9001 Section 5.4.2):
	 * sample_offset = pn_offset + 4
	 *
	 * Since pn_offset is within header, and ciphertext starts right after
	 * the packet number, the sample is at:
	 * ciphertext[4 - pn_len]
	 *
	 * This works because:
	 * - Header ends at header_len (which is pn_offset + pn_len)
	 * - Sample starts 4 bytes after pn_offset
	 * - So sample is at (pn_offset + 4) - header_len = 4 - pn_len bytes into ciphertext
	 */
	sample_offset = QUIC_HP_MAX_PN_LEN - pn_len;

	if (sample_offset + QUIC_HP_SAMPLE_LEN > ciphertext_len) {
		NET_DBG("Ciphertext too short for HP sample");
		return -EINVAL;
	}

	memcpy(sample, &ciphertext[sample_offset], QUIC_HP_SAMPLE_LEN);

	/* Generate mask */
	ret = quic_hp_mask(hp_key_id, cipher_algo, sample, mask);
	if (ret != 0) {
		return ret;
	}

	/* Mask first byte of header */
	if ((header[0] & 0x80) != 0) {
		/* Long header:  mask lower 4 bits */
		header[0] ^= (mask[0] & 0x0F);
	} else {
		/* Short header: mask lower 5 bits */
		header[0] ^= (mask[0] & 0x1F);
	}

	/* Mask packet number bytes */
	for (size_t i = 0; i < pn_len; i++) {
		header[pn_offset + i] ^= mask[1 + i];
	}

	return 0;
}

/*
 * Build CRYPTO frame directly into endpoint's TX buffer
 * Returns pointer to where TLS data should be written
 */
static uint8_t *quic_prepare_crypto_frame(struct quic_endpoint *ep,
					  enum quic_secret_level level,
					  size_t data_len,
					  size_t *frame_header_len)
{
	uint32_t offset = ep->crypto.stream[level].tx_offset;
	uint8_t *buf = ep->crypto.tx_buffer;
	size_t pos = 0;
	int offset_size, len_size;

	/* Frame type */
	buf[pos++] = QUIC_FRAME_TYPE_CRYPTO;

	/* Offset */
	offset_size = quic_get_varint_size(offset);
	quic_put_len(&buf[pos], sizeof(ep->crypto.tx_buffer) - pos, offset);
	pos += offset_size;

	/* Length */
	len_size = quic_get_varint_size(data_len);
	quic_put_len(&buf[pos], sizeof(ep->crypto.tx_buffer) - pos, data_len);
	pos += len_size;

	*frame_header_len = pos;

	/* Return pointer where caller should write TLS data */
	return &buf[pos];
}

/*
 * Send a QUIC packet using scatter-gather I/O
 *
 * Uses zsock_sendmsg() to avoid copying header and payload into
 * a single buffer. The header is built in a small stack buffer,
 * and the encrypted payload uses the endpoint's tx_buffer.
 * Encrypt and send ep->crypto.tx_buffer[0..plaintext_len).
 * Called by both quic_send_packet() and quic_send_packet_sg().
 */
static int quic_send_packet_from_txbuf(struct quic_endpoint *ep,
				       enum quic_secret_level level,
				       size_t payload_len)
{
	struct quic_crypto_context *crypto_ctx;
	uint8_t header[MAX_QUIC_HEADER_SIZE];
	size_t header_len;
	size_t pn_offset;
	size_t ciphertext_len;
	size_t plaintext_len;
	uint64_t packet_number;
	size_t pn_len;
	size_t padding;
	int ret;
	ssize_t sent;
	size_t total_len;
	bool ack_eliciting;

	/* Encrypted payload goes into endpoint's tx_buffer */
	uint8_t *ciphertext = ep->crypto.tx_buffer;
	size_t ciphertext_size = sizeof(ep->crypto.tx_buffer);

	/* Track sent packet for RTT measurement.
	 * Determine if packet is ack-eliciting by checking first frame type.
	 * ACK frames (0x02, 0x03), PADDING (0x00) and CONNECTION_CLOSE frames
	 * are not ack-eliciting. Do this check before encryption since frame
	 * types are visible in plaintext.
	 */
	ack_eliciting = true;

	if (payload_len > 0) {
		uint8_t first_frame = ep->crypto.tx_buffer[0];

		if (first_frame == QUIC_FRAME_TYPE_PADDING ||
		    first_frame == QUIC_FRAME_TYPE_ACK ||
		    first_frame == QUIC_FRAME_TYPE_ACK_ECN ||
		    first_frame == QUIC_FRAME_TYPE_CONNECTION_CLOSE_TRANSPORT ||
		    first_frame == QUIC_FRAME_TYPE_CONNECTION_CLOSE_APPLICATION) {
			/* Could be ACK-only or padding-only packet */
			ack_eliciting = false;

			/* TODO: Check if there are other frames after ACK */
		}
	}

	/* Get crypto context */
	crypto_ctx = quic_get_crypto_context_by_level(ep, level);
	if (crypto_ctx == NULL || !crypto_ctx->initialized) {
		NET_DBG("Crypto context not initialized for level %d", level);
		return -EINVAL;
	}

	/* Get next packet number */
	packet_number = get_next_packet_number(ep, level);
	pn_len = get_pn_length(packet_number);

	/* Calculate plaintext size with padding */
	plaintext_len = payload_len;
	padding = calculate_padding_for_level(ep, level, payload_len, pn_len);
	plaintext_len += padding;

	/* Verify we have space for ciphertext */
	if (plaintext_len + QUIC_AEAD_TAG_LEN > ciphertext_size) {
		return -ENOBUFS;
	}

	/* Build header */
	ret = build_packet_header(ep, level, packet_number, pn_len,
				  plaintext_len + QUIC_AEAD_TAG_LEN,
				  header, sizeof(header),
				  &header_len, &pn_offset);
	if (ret != 0) {
		NET_DBG("Failed to build packet header (%d)", ret);
		return ret;
	}

	/*
	 * For AEAD encryption, we need header as AAD.
	 * The payload needs to be in a buffer we can pass to PSA.
	 * We'll build plaintext temporarily in tx_buffer, then encrypt in-place.
	 */
	if (padding > 0) {
		memset(&ciphertext[payload_len], 0, padding);
	}

	/* Encrypt:  plaintext in ciphertext buffer -> ciphertext in same buffer */
	ret = quic_encrypt_payload(&crypto_ctx->tx.pp,
				   packet_number,
				   header, header_len,
				   ciphertext, plaintext_len,
				   ciphertext, ciphertext_size,
				   &ciphertext_len);
	if (ret != 0) {
		NET_DBG("Failed to encrypt payload (%d)", ret);
		return ret;
	}

	/*
	 * Now we need to apply header protection.
	 * The sample is taken from ciphertext at offset (4 - pn_len) from start.
	 * Since header and ciphertext are separate, we need to handle this carefully.
	 *
	 * Sample offset in ciphertext = 4 - pn_len (because PN is at end of header)
	 * For typical pn_len=1, sample starts at ciphertext[3]
	 */
	ret = quic_apply_header_protection_split(header, header_len, pn_offset, pn_len,
						 ciphertext, ciphertext_len,
						 crypto_ctx->tx.hp.key_id,
						 crypto_ctx->tx.hp.cipher_algo);
	if (ret != 0) {
		NET_DBG("Failed to apply header protection (%d)", ret);
		return ret;
	}

	/* Send using scatter-gather I/O */
	struct net_iovec iov[2] = {
		{ .iov_base = header, .iov_len = header_len },
		{ .iov_base = ciphertext, .iov_len = ciphertext_len },
	};

	struct net_msghdr msg = {
		.msg_name = &ep->remote_addr,
		.msg_namelen = ep->remote_addr.ss_family == NET_AF_INET ?
			       sizeof(struct net_sockaddr_in) :
			       sizeof(struct net_sockaddr_in6),
		.msg_iov = iov,
		.msg_iovlen = 2,
		.msg_control = NULL,
		.msg_controllen = 0,
		.msg_flags = 0,
	};

	NET_DBG("[EP:%p/%d] Sending %zu bytes to %s%s%s:%d on socket %d",
		ep, quic_get_by_ep(ep), header_len + ciphertext_len,
		ep->remote_addr.ss_family == NET_AF_INET6 ? "[" : "",
		net_sprint_addr(ep->remote_addr.ss_family,
				ep->remote_addr.ss_family == NET_AF_INET ?
				(void *)&net_sin(net_sad(&ep->remote_addr))->sin_addr :
				(void *)&net_sin6(net_sad(&ep->remote_addr))->sin6_addr),
		ep->remote_addr.ss_family == NET_AF_INET6 ? "]" : "",
		net_ntohs(ep->remote_addr.ss_family == NET_AF_INET ?
		      net_sin(net_sad(&ep->remote_addr))->sin_port :
		      net_sin6(net_sad(&ep->remote_addr))->sin6_port),
		ep->sock);

	sent = zsock_sendmsg(ep->sock, &msg, 0);
	if (sent < 0) {
		ret = -errno;

		NET_DBG("Failed to send packet (%d)", ret);
		return ret;
	}

	/* XXX: TODO: Handle partial sends properly */
	total_len = header_len + ciphertext_len;
	if ((size_t)sent != total_len) {
		NET_WARN("Partial send: %zd of %zu bytes", sent, total_len);
	}

	quic_recovery_on_packet_sent(ep, level, packet_number, total_len, ack_eliciting);

	NET_DBG("[EP:%p/%d] Sent %zd bytes at level %d, pn=%" PRIu64 ", ack-eliciting=%d",
		ep, quic_get_by_ep(ep), sent, level, packet_number, ack_eliciting);

	return 0;
}

static int quic_send_packet(struct quic_endpoint *ep,
			    enum quic_secret_level level,
			    const uint8_t *payload,
			    size_t payload_len)
{
	if (payload_len > sizeof(ep->crypto.tx_buffer)) {
		return -ENOBUFS;
	}

	memcpy(ep->crypto.tx_buffer, payload, payload_len);
	return quic_send_packet_from_txbuf(ep, level, payload_len);
}

/*
 * Finalize and send the prepared frame
 */
static int quic_send_prepared_frame(struct quic_endpoint *ep,
				    enum quic_secret_level level,
				    size_t frame_header_len,
				    size_t data_len)
{
	size_t total_frame_len = frame_header_len + data_len;

	/* Update crypto stream offset */
	ep->crypto.stream[level].tx_offset += data_len;

	return quic_send_packet(ep, level, ep->crypto.tx_buffer, total_frame_len);
}

/* Updated TLS send callback, zero copy for TLS data */
static int quic_tls_send_callback(void *user_data,
				  enum quic_secret_level level,
				  const uint8_t *data, size_t len)
{
	struct quic_endpoint *ep = user_data;
	size_t frame_header_len;
	uint8_t *dest;

	if (ep == NULL || data == NULL || len == 0) {
		return -EINVAL;
	}

	/* Get current offset for this crypto stream */
	if (level < QUIC_SECRET_LEVEL_INITIAL || level > QUIC_SECRET_LEVEL_APPLICATION) {
		return -EINVAL;
	}

	NET_DBG("TLS send callback: level=%d, len=%zu", level, len);

	/* Prepare frame header, get destination for TLS data */
	dest = quic_prepare_crypto_frame(ep, level, len, &frame_header_len);
	if (dest == NULL) {
		return -ENOBUFS;
	}

	/* Copy TLS data into frame */
	memcpy(dest, data, len);

	/* Send */
	return quic_send_prepared_frame(ep, level, frame_header_len, len);
}

static void quic_tls_set_callbacks(struct quic_tls_context *ctx,
				   quic_tls_secret_cb secret_cb,
				   quic_tls_send_cb send_cb,
				   void *user_data)
{
	ctx->secret_cb = secret_cb;
	ctx->send_cb = send_cb;
	ctx->user_data = user_data;
}

static int quic_tls_process(struct quic_tls_context *ctx,
			    enum quic_secret_level level,
			    const uint8_t *data,
			    size_t len,
			    size_t *consumed)
{
	size_t pos = 0;
	int ret;

	NET_DBG("Processing TLS data, level %d, len %zu", level, len);

	if (IS_ENABLED(CONFIG_QUIC_TXRX_DEBUG)) {
		NET_HEXDUMP_DBG(data, MIN(len, 64), "TLS data (first 64 bytes):");
	}

	/* Parse handshake messages */
	while (pos + 4 <= len) {
		uint8_t msg_type = data[pos];
		uint32_t msg_len = ((uint32_t)data[pos + 1] << 16) |
				   ((uint32_t)data[pos + 2] << 8) |
				   (uint32_t)data[pos + 3];

		if (pos + 4 + msg_len > len) {
			/* Incomplete message, we need more data from CRYPTO reassembly.
			 * This is expected during handshake when data arrives fragmented.
			 * Return bytes consumed so far and in-progress status.
			 */
			NET_DBG("Incomplete handshake message: have %zu, need %u",
				len - pos, 4 + msg_len);

			if (consumed) {
				*consumed = pos;
			}

			return QUIC_HANDSHAKE_IN_PROGRESS;
		}

		/* Process message, pass both body and full message with header */
		ret = process_handshake_message(ctx, msg_type,
						&data[pos + 4], msg_len,
						&data[pos], 4 + msg_len);
		if (ret < 0) {
			if (consumed) {
				*consumed = pos;
			}
			return ret;
		}

		if (ret == 1) {
			/* Handshake complete */
			if (consumed) {
				*consumed = pos + 4 + msg_len;
			}
			return QUIC_HANDSHAKE_COMPLETED;
		}

		pos += 4 + msg_len;
	}

	if (consumed) {
		*consumed = pos;
	}
	return QUIC_HANDSHAKE_IN_PROGRESS;
}

/*
 * Start client TLS handshake by building and sending ClientHello
 *
 * This should be called when the client wants to initiate a QUIC connection.
 * It builds the ClientHello, wraps it in a handshake message, and sends it
 * via the send callback (which will wrap it in a CRYPTO frame).
 *
 * Uses endpoint's rx_buffer as scratch space to avoid large stack allocations.
 */
static int quic_tls_client_start(struct quic_tls_context *ctx)
{
	struct quic_endpoint *ep;
	size_t buf_size;
	uint8_t *client_hello;
	size_t client_hello_size;
	uint8_t *wrapped;
	size_t wrapped_size;
	size_t ch_len, wrapped_len;
	int ret;

	if (ctx == NULL || ctx->ep == NULL) {
		return -EINVAL;
	}

	ep = ctx->ep;
	buf_size = sizeof(ep->crypto.rx_buffer);
	client_hello = ep->crypto.rx_buffer;
	client_hello_size = buf_size / 2;
	wrapped = ep->crypto.rx_buffer + client_hello_size;
	wrapped_size = buf_size - client_hello_size;

	if (ctx->ep->is_server) {
		NET_DBG("%s called on server context", __func__);
		return -EINVAL;
	}

	if (ctx->state != QUIC_TLS_STATE_INITIAL) {
		NET_DBG("Invalid TLS state for client start: %d", ctx->state);
		return -EINVAL;
	}

	NET_DBG("[EP:%p/%d] Starting client TLS handshake", ctx->ep,
		quic_get_by_ep(ctx->ep));

	/* Initialize key schedule (will be updated when we receive ServerHello) */
	ctx->ks.cipher_suite = TLS_AES_128_GCM_SHA256;  /* Default, may change */
	ret = key_schedule_init(ctx);
	if (ret != 0) {
		NET_DBG("Failed to initialize key schedule: %d", ret);
		return ret;
	}

	/* Build ClientHello */
	ret = build_client_hello(ctx, client_hello, client_hello_size, &ch_len);
	if (ret != 0) {
		NET_DBG("Failed to build ClientHello: %d", ret);
		return ret;
	}

	/* Wrap in handshake message format */
	ret = wrap_handshake_message(TLS_HS_CLIENT_HELLO,
				     client_hello, ch_len,
				     wrapped, wrapped_size, &wrapped_len);
	if (ret != 0) {
		NET_DBG("Failed to wrap ClientHello: %d", ret);
		return ret;
	}

	/* Update transcript with ClientHello */
	ret = transcript_update(ctx, wrapped, wrapped_len);
	if (ret != 0) {
		NET_DBG("Failed to update transcript: %d", ret);
		return ret;
	}

	/* Send ClientHello via CRYPTO frame at Initial level */
	if (ctx->send_cb != NULL) {
		ret = ctx->send_cb(ctx->user_data, QUIC_SECRET_LEVEL_INITIAL,
				   wrapped, wrapped_len);
		if (ret < 0) {
			NET_DBG("[EP:%p/%d] Failed to send ClientHello: %d",
				ctx->ep, quic_get_by_ep(ctx->ep), ret);
			return ret;
		}
	}

	ctx->state = QUIC_TLS_STATE_WAIT_SERVER_HELLO;

	NET_DBG("[EP:%p/%d] ClientHello sent, waiting for ServerHello",
		ctx->ep, quic_get_by_ep(ctx->ep));

	return 0;
}

static int quic_tls_get_peer_transport_params(struct quic_tls_context *ctx,
					      uint8_t *params, size_t *len)
{
	if (ctx->peer_tp_len == 0) {
		return -ENOENT;
	}

	if (*len < ctx->peer_tp_len) {
		return -ENOBUFS;
	}

	memcpy(params, ctx->peer_tp, ctx->peer_tp_len);
	*len = ctx->peer_tp_len;

	return 0;
}

#if defined(MBEDTLS_X509_CRT_PARSE_C)
static bool crt_is_pem(const unsigned char *buf, size_t buflen)
{
	return (buflen != 0 && buf[buflen - 1] == '\0' &&
		strstr((const char *)buf, "-----BEGIN CERTIFICATE-----") != NULL);
}
#endif

static int tls_add_ca_certificate(struct quic_tls_context *tls,
				  struct tls_credential *ca_cert)
{
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	int err;

	if (tls->options.cert_nocopy == ZSOCK_TLS_CERT_NOCOPY_NONE ||
	    crt_is_pem(ca_cert->buf, ca_cert->len)) {
		err = mbedtls_x509_crt_parse(&tls->ca_chain, ca_cert->buf,
					     ca_cert->len);
	} else {
		err = mbedtls_x509_crt_parse_der_nocopy(&tls->ca_chain,
							ca_cert->buf,
							ca_cert->len);
	}

	if (err != 0) {
		NET_DBG("Failed to parse CA certificate, err: -0x%x", -err);
		return -EINVAL;
	}

	return 0;
#else
	return -ENOTSUP;
#endif /* MBEDTLS_X509_CRT_PARSE_C */
}

static void tls_set_ca_chain(struct quic_tls_context *tls)
{
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	tls->ca_cert = true;
#endif /* MBEDTLS_X509_CRT_PARSE_C */
}

static int tls_add_own_cert(struct quic_tls_context *tls,
			    struct tls_credential *own_cert)
{
#if defined(MBEDTLS_X509_CRT_PARSE_C)
#define TMP_BUF_LEN 128
	char tmp_buf[TMP_BUF_LEN];
	int err;

	if (tls->options.cert_nocopy == ZSOCK_TLS_CERT_NOCOPY_NONE ||
	    crt_is_pem(own_cert->buf, own_cert->len)) {
		err = mbedtls_x509_crt_parse(&tls->own_cert,
					     own_cert->buf, own_cert->len);
	} else {
		err = mbedtls_x509_crt_parse_der_nocopy(&tls->own_cert,
							own_cert->buf,
							own_cert->len);
	}

	if (err != 0) {
		return -EINVAL;
	}

	NET_DBG("Own certificate parsed successfully:");

	err = mbedtls_x509_dn_gets(tmp_buf, sizeof(tmp_buf), &tls->own_cert.subject);
	NET_DBG("  Subject: %s", err > 0 ? tmp_buf : "N/A");

	err = mbedtls_x509_dn_gets(tmp_buf, sizeof(tmp_buf), &tls->own_cert.issuer);
	NET_DBG("   Issuer: %s", err > 0 ? tmp_buf : "N/A");

	tls->my_cert = own_cert->buf;
	tls->my_cert_len = own_cert->len;

	return 0;
#endif /* MBEDTLS_X509_CRT_PARSE_C */

	return -ENOTSUP;
}

static int tls_set_own_cert(struct quic_tls_context *tls)
{
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	/* TODO: Handle any own cert options here if needed */
	return 0;
#else
	return -ENOTSUP;
#endif /* MBEDTLS_X509_CRT_PARSE_C */
}

static int tls_set_private_key(struct quic_tls_context *tls,
			       struct tls_credential *priv_key)
{
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	int err;

	err = mbedtls_pk_parse_key(&tls->priv_key, priv_key->buf,
				   priv_key->len, NULL, 0);
	if (err != 0) {
		return -EINVAL;
	}

	/* Store key for later use in quic_tls_set_own_cert */
	tls->my_key = priv_key->buf;
	tls->my_key_len = priv_key->len;

	return 0;
#endif /* MBEDTLS_X509_CRT_PARSE_C */

	return -ENOTSUP;
}

static int tls_set_psk(struct quic_tls_context *tls,
		       struct tls_credential *psk,
		       struct tls_credential *psk_id)
{
	/* TODO: Implement PSK support here and remove this stub function */
#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_PSK_ENABLED)
	return 0;
#else
	return -ENOTSUP;
#endif /* MBEDTLS_SSL_HANDSHAKE_WITH_PSK_ENABLED */
}

static int tls_set_credential(struct quic_tls_context *tls,
			      struct tls_credential *cred)
{
	switch (cred->type) {
	case TLS_CREDENTIAL_CA_CERTIFICATE:
		return tls_add_ca_certificate(tls, cred);

	case TLS_CREDENTIAL_PUBLIC_CERTIFICATE:
		return tls_add_own_cert(tls, cred);

	case TLS_CREDENTIAL_PRIVATE_KEY:
		return tls_set_private_key(tls, cred);
	break;

	case TLS_CREDENTIAL_PSK:
	{
		struct tls_credential *psk_id =
			credential_get(cred->tag, TLS_CREDENTIAL_PSK_ID);
		if (!psk_id) {
			return -ENOENT;
		}

		return tls_set_psk(tls, cred, psk_id);
	}

	case TLS_CREDENTIAL_PSK_ID:
		/* Ignore PSK ID, it will be used together with PSK */
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int quic_tls_mbedtls_set_credentials(struct quic_tls_context *tls)
{
	struct tls_credential *cred;
	sec_tag_t tag;
	int i, err = 0;
	bool tag_found, ca_cert_present = false, own_cert_present = false;

	credentials_lock();

	for (i = 0; i < tls->options.sec_tag_list.sec_tag_count; i++) {
		tag = tls->options.sec_tag_list.sec_tags[i];
		cred = NULL;
		tag_found = false;

		while ((cred = credential_next_get(tag, cred)) != NULL) {
			tag_found = true;

			err = tls_set_credential(tls, cred);
			if (err != 0) {
				NET_DBG("Failed to set credential of type %d on tag %d (%d)",
					cred->type, tag, err);
				goto exit;
			}

			if (cred->type == TLS_CREDENTIAL_CA_CERTIFICATE) {
				ca_cert_present = true;
			} else if (cred->type == TLS_CREDENTIAL_PUBLIC_CERTIFICATE) {
				own_cert_present = true;
			}
		}

		if (!tag_found) {
			NET_DBG("No credentials found for sec tag %d", tag);
			err = -ENOENT;
			goto exit;
		}
	}

exit:
	credentials_unlock();

	if (err == 0) {
		if (ca_cert_present) {
			tls_set_ca_chain(tls);
		}
		if (own_cert_present) {
			err = tls_set_own_cert(tls);
		}
	}

	return err;
}

static int tls_check_cert(struct tls_credential *cert)
{
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	mbedtls_x509_crt cert_ctx;
	int err;

	mbedtls_x509_crt_init(&cert_ctx);

	if (crt_is_pem(cert->buf, cert->len)) {
		err = mbedtls_x509_crt_parse(&cert_ctx, cert->buf, cert->len);
	} else {
		/* For DER case, use the no copy version of the parsing function
		 * to avoid unnecessary heap allocations.
		 */
		err = mbedtls_x509_crt_parse_der_nocopy(&cert_ctx, cert->buf,
							cert->len);
	}

	if (err != 0) {
		char errbuf[128];

#if defined(CONFIG_MBEDTLS_DEBUG)
		mbedtls_strerror(err, errbuf, sizeof(errbuf));
#else
		snprintf(errbuf, sizeof(errbuf), "<error string unavailable>");
#endif

		NET_DBG("Failed to parse %s on tag %d, err: -0x%x (%s)",
			"certificate", cert->tag, -err, errbuf);
		return -EINVAL;
	}

	mbedtls_x509_crt_free(&cert_ctx);

	return err;
#else
	NET_WARN("TLS with certificates disabled. "
		 "Reconfigure mbed TLS to support certificate based key exchange.");

	return -ENOTSUP;
#endif /* MBEDTLS_X509_CRT_PARSE_C */
}

static int tls_check_priv_key(struct tls_credential *priv_key)
{
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	mbedtls_pk_context key_ctx;
	int err;

	mbedtls_pk_init(&key_ctx);

	err = mbedtls_pk_parse_key(&key_ctx, priv_key->buf,
				   priv_key->len, NULL, 0);
	if (err != 0) {
		char errbuf[128];

#if defined(CONFIG_MBEDTLS_DEBUG)
		mbedtls_strerror(err, errbuf, sizeof(errbuf));
#else
		snprintf(errbuf, sizeof(errbuf), "<error string unavailable>");
#endif

		NET_DBG("Failed to parse %s on tag %d, err: -0x%x (%s)",
			"private key", priv_key->tag, -err, errbuf);
		err = -EINVAL;
	}

	mbedtls_pk_free(&key_ctx);

	return err;
#else
	NET_WARN("TLS with certificates disabled. "
		 "Reconfigure mbed TLS to support certificate based key exchange.");

	return -ENOTSUP;
#endif /* MBEDTLS_X509_CRT_PARSE_C */
}

static int tls_check_psk(struct tls_credential *psk)
{
#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_PSK_ENABLED)
	struct tls_credential *psk_id;

	psk_id = credential_get(psk->tag, TLS_CREDENTIAL_PSK_ID);
	if (psk_id == NULL) {
		NET_ERR("No matching PSK ID found for tag %d", psk->tag);
		return -EINVAL;
	}

	if (psk->len == 0 || psk_id->len == 0) {
		NET_ERR("PSK or PSK ID empty on tag %d", psk->tag);
		return -EINVAL;
	}

	return 0;
#else
	NET_WARN("TLS with PSK disabled. "
		 "Reconfigure mbed TLS to support PSK based key exchange.");

	return -ENOTSUP;
#endif
}

static int tls_check_credentials(const sec_tag_t *sec_tags, int sec_tag_count)
{
	int err = 0;

	credentials_lock();

	for (int i = 0; i < sec_tag_count; i++) {
		sec_tag_t tag = sec_tags[i];
		struct tls_credential *cred = NULL;
		bool tag_found = false;

		while ((cred = credential_next_get(tag, cred)) != NULL) {
			tag_found = true;

			switch (cred->type) {
			case TLS_CREDENTIAL_CA_CERTIFICATE:
				__fallthrough;
			case TLS_CREDENTIAL_PUBLIC_CERTIFICATE:
				err = tls_check_cert(cred);
				if (err != 0) {
					NET_DBG("Cannot validate certificate on tag %d", tag);
					goto exit;
				}

				break;
			case TLS_CREDENTIAL_PRIVATE_KEY:
				err = tls_check_priv_key(cred);
				if (err != 0) {
					NET_DBG("Cannot validate private key on tag %d", tag);
					goto exit;
				}

				break;
			case TLS_CREDENTIAL_PSK:
				err = tls_check_psk(cred);
				if (err != 0) {
					NET_DBG("Cannot validate psk on tag %d", tag);
					goto exit;
				}

				break;
			case TLS_CREDENTIAL_PSK_ID:
				/* Ignore PSK ID, it will be verified together with PSK */
				break;
			default:
				return -EINVAL;
			}
		}

		/* If no credential is found with such a tag, report an error. */
		if (!tag_found) {
			NET_DBG("No TLS credential found with tag %d", tag);
			err = -ENOENT;
			goto exit;
		}
	}

exit:
	credentials_unlock();

	return err;
}

static int tls_opt_sec_tag_list_set(struct quic_tls_context *context,
				    const void *optval, net_socklen_t optlen)
{
	int sec_tag_cnt;
	int ret;

	if (optval == NULL) {
		return -EINVAL;
	}

	if (optlen % sizeof(sec_tag_t) != 0) {
		return -EINVAL;
	}

	sec_tag_cnt = optlen / sizeof(sec_tag_t);
	if (sec_tag_cnt > ARRAY_SIZE(context->options.sec_tag_list.sec_tags)) {
		return -EINVAL;
	}

	ret = tls_check_credentials((const sec_tag_t *)optval, sec_tag_cnt);
	if (ret < 0) {
		NET_DBG("Invalid credentials in sec_tag_list (%d)", ret);
		return ret;
	}

	memcpy(context->options.sec_tag_list.sec_tags, optval, optlen);
	context->options.sec_tag_list.sec_tag_count = sec_tag_cnt;

	NET_DBG("Configured %d sec tags to TLS context %p", sec_tag_cnt, context);

	return 0;
}

static int tls_opt_hostname_set(struct quic_tls_context *context,
				const void *optval, net_socklen_t optlen)
{
	ARG_UNUSED(optlen);
	ARG_UNUSED(optval);
	ARG_UNUSED(context);

	return -ENOPROTOOPT;
}

static int tls_opt_ciphersuite_list_set(struct quic_tls_context *context,
					const void *optval, net_socklen_t optlen)
{
	int cipher_cnt;

	if (!optval) {
		return -EINVAL;
	}

	if (optlen % sizeof(int) != 0) {
		return -EINVAL;
	}

	cipher_cnt = optlen / sizeof(int);

	/* + 1 for 0-termination. */
	if (cipher_cnt + 1 > ARRAY_SIZE(context->options.ciphersuites)) {
		return -EINVAL;
	}

	memcpy(context->options.ciphersuites, optval, optlen);
	context->options.ciphersuites[cipher_cnt] = 0;

	return 0;
}

static int tls_opt_alpn_list_set(struct quic_tls_context *context,
				 const void *optval, net_socklen_t optlen)
{
	int alpn_cnt;

	if (ALPN_MAX_PROTOCOLS == 0) {
		return -EINVAL;
	}

	if (optval == NULL) {
		return -EINVAL;
	}

	if (optlen % sizeof(const char *) != 0) {
		return -EINVAL;
	}

	alpn_cnt = optlen / sizeof(const char *);

	/* alpn list must be NULL terminated. */
	if (alpn_cnt > ARRAY_SIZE(context->options.alpn_list)) {
		return -EINVAL;
	}

	/* NOTE: this relies to the fact that the ALPN list points to const
	 * strings and not to other variables. So this is ok
	 *   const char *alpn_list[] = {"h3", "hq-29", NULL};
	 * but this is not
	 *   const char *alpn_list[] = {http3, other_var, NULL};
	 */
	memcpy(context->options.alpn_list, optval, optlen);
	context->options.alpn_list[alpn_cnt] = NULL;

	if (IS_ENABLED(CONFIG_QUIC_LOG_LEVEL_DBG)) {
		struct quic_endpoint *ep = CONTAINER_OF(context,
							struct quic_endpoint,
							crypto.tls);
		struct quic_context *quic_ctx = quic_find_context(ep);

		NET_DBG("[TLS:%p] Configured ALPN list:", quic_ctx);

		for (int i = 0; i < alpn_cnt; i++) {
			if (context->options.alpn_list[i] == NULL) {
				break;
			}

			NET_DBG("    ALPN[%d]: %s", i, context->options.alpn_list[i]);
		}
	}

	return 0;
}

static int tls_opt_peer_verify_set(struct quic_tls_context *context,
				   const void *optval, net_socklen_t optlen)
{
	int *peer_verify;

	if (!optval) {
		return -EINVAL;
	}

	if (optlen != sizeof(int)) {
		return -EINVAL;
	}

	peer_verify = (int *)optval;

	if (*peer_verify != MBEDTLS_SSL_VERIFY_NONE &&
	    *peer_verify != MBEDTLS_SSL_VERIFY_OPTIONAL &&
	    *peer_verify != MBEDTLS_SSL_VERIFY_REQUIRED) {
		return -EINVAL;
	}

	context->options.verify_level = *peer_verify;

	return 0;
}

static int tls_opt_cert_nocopy_set(struct quic_tls_context *context,
				   const void *optval, net_socklen_t optlen)
{
	int *cert_nocopy;

	if (!optval) {
		return -EINVAL;
	}

	if (optlen != sizeof(int)) {
		return -EINVAL;
	}

	cert_nocopy = (int *)optval;

	if (*cert_nocopy != ZSOCK_TLS_CERT_NOCOPY_NONE &&
	    *cert_nocopy != ZSOCK_TLS_CERT_NOCOPY_OPTIONAL) {
		return -EINVAL;
	}

	context->options.cert_nocopy = *cert_nocopy;

	return 0;
}

#if defined(CONFIG_QUIC_TLS_CERT_VERIFY_CALLBACK)
static int tls_opt_cert_verify_callback_set(struct quic_tls_context *context,
					    const void *optval,
					    net_socklen_t optlen)
{
	struct tls_cert_verify_cb *cert_verify;

	if (!optval) {
		return -EINVAL;
	}

	if (optlen != sizeof(struct tls_cert_verify_cb)) {
		return -EINVAL;
	}

	cert_verify = (struct tls_cert_verify_cb *)optval;
	if (cert_verify->cb == NULL) {
		return -EINVAL;
	}

	context->options.cert_verify = *cert_verify;

	return 0;
}
#else /* CONFIG_QUIC_TLS_CERT_VERIFY_CALLBACK */
static int tls_opt_cert_verify_callback_set(struct quic_tls_context *context,
					    const void *optval,
					    net_socklen_t optlen)
{
	NET_WARN("TLS_CERT_VERIFY_CALLBACK option requires "
		 "CONFIG_QUIC_TLS_CERT_VERIFY_CALLBACK enabled");

	return -ENOPROTOOPT;
}
#endif /* CONFIG_QUIC_TLS_CERT_VERIFY_CALLBACK */

/**
 * Initialize a TLS context (embedded in endpoint, no allocation needed)
 */
static void quic_tls_context_init(struct quic_tls_context *tls)
{
	tls->options.verify_level = -1;

	k_sem_init(&tls->tls_established, 0, 1);

#if defined(MBEDTLS_X509_CRT_PARSE_C)
	mbedtls_x509_crt_init(&tls->ca_chain);
	mbedtls_x509_crt_init(&tls->own_cert);
	mbedtls_pk_init(&tls->priv_key);
#endif

	NET_DBG("Initialized TLS context %p", tls);
}

/* Clone TLS context settings from source to target (both embedded). */
static int quic_tls_clone(struct quic_tls_context *target_tls,
			  struct quic_tls_context *source_tls)
{
	if (source_tls == NULL || target_tls == NULL) {
		return -EINVAL;
	}

	/* Clear target and copy settings from source */
	memset(target_tls, 0, sizeof(*target_tls));

	target_tls->tls_version = source_tls->tls_version;
	target_tls->type = source_tls->type;

	memcpy(&target_tls->options, &source_tls->options,
	       sizeof(target_tls->options));

	return 0;
}

static int quic_tls_init(struct quic_tls_context *tls, bool is_server)
{
	int ret;

	ret = quic_tls_mbedtls_set_credentials(tls);
	if (ret != 0) {
		NET_DBG("Cannot set credentials (%d)", ret);
		goto out;
	}

	ret = quic_tls_set_own_cert(tls, tls->my_cert, tls->my_cert_len,
				    tls->my_key, tls->my_key_len);
	if (ret != 0) {
		/* It is possible that certificate or key is not available at this
		 * point, for example if credentials are being loaded asynchronously.
		 * In that case, we will try to set them later when they become available.
		 * Do not log a warning in order not to confuse the user.
		 */
		NET_DBG("Cannot find own certificate. "
			"Will try to set it later when it becomes available");
		ret = 0;  /* Certificate is optional at this point,
			   * don't fail initialization
			   */
	}

#if defined(MBEDTLS_X509_CRT_PARSE_C)
	/* Parse intermediate certificates from cert_chain_tags into the
	 * own_cert mbedtls chain so build_certificate() includes them.
	 */
	if (tls->cert_chain_count > 0) {
		credentials_lock();

		for (size_t i = 0; i < tls->cert_chain_count; i++) {
			struct tls_credential *cred;

			cred = credential_get(tls->cert_chain_tags[i],
					      TLS_CREDENTIAL_CA_CERTIFICATE);
			if (cred == NULL) {
				cred = credential_get(tls->cert_chain_tags[i],
						      TLS_CREDENTIAL_PUBLIC_CERTIFICATE);
			}

			if (cred == NULL || cred->buf == NULL || cred->len == 0) {
				NET_DBG("No certificate for chain tag %d",
					tls->cert_chain_tags[i]);
				credentials_unlock();
				ret = -ENOENT;
				goto out;
			}

			ret = mbedtls_x509_crt_parse(&tls->own_cert,
						     cred->buf, cred->len);
			if (ret != 0) {
				NET_DBG("Failed to parse chain cert tag %d: "
					"-0x%04x", tls->cert_chain_tags[i],
					-ret);
				credentials_unlock();
				ret = -EINVAL;
				goto out;
			}

			NET_DBG("Added intermediate cert tag %d to chain",
				tls->cert_chain_tags[i]);
		}

		credentials_unlock();
	}
#endif /* MBEDTLS_X509_CRT_PARSE_C */

	NET_DBG("[EP:%p/%d] TLS credentials set", tls->ep, quic_get_by_ep(tls->ep));

	tls->is_initialized = true;
	tls->state = is_server ? QUIC_TLS_STATE_WAIT_CLIENT_HELLO :
				 QUIC_TLS_STATE_INITIAL;

out:
	return ret;
}

static void quic_tls_cleanup(struct quic_tls_context *tls)
{
	if (tls == NULL) {
		return;
	}

	if (tls->is_initialized) {
		quic_tls_free(tls);
	}

#if defined(MBEDTLS_X509_CRT_PARSE_C)
	mbedtls_x509_crt_free(&tls->ca_chain);
	mbedtls_x509_crt_free(&tls->own_cert);
	mbedtls_pk_free(&tls->priv_key);
#endif

	NET_DBG("[EP:%p/%d] TLS %p context cleaned up",
		tls->ep, quic_get_by_ep(tls->ep), tls);

	tls->is_initialized = false;
}

static struct quic_tls_context *tls_init(struct quic_endpoint *ep)
{
	struct quic_tls_context *tls = &ep->crypto.tls;

	memset(&ep->crypto.initial, 0, sizeof(ep->crypto.initial));

	/* Clear and initialize the embedded TLS context */
	memset(tls, 0, sizeof(*tls));
	quic_tls_context_init(tls);

	NET_DBG("[EP:%p/%d] Initialized TLS %p", ep, quic_get_by_ep(ep), tls);

	tls->ep = ep;

	quic_tls_set_callbacks(tls,
			       quic_tls_secret_callback,
			       quic_tls_send_callback,
			       ep);

	return tls;
}
