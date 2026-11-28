/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include <mbedtls/platform_util.h>
#include <zephyr/sys/base64.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "control_internal.h"

#if defined(CONFIG_TAILSCALE) && \
	(!defined(MBEDTLS_THREADING_C) || !defined(MBEDTLS_THREADING_PTHREAD))
#error "Tailscale requires the Mbed TLS pthread backend in the TF-PSA-Crypto user configuration"
#endif

/* The workspace's WireGuard library uses this implementation of RFC 7693. */
#include "refc/blake2s.c"

#define NOISE_ALGORITHM             PSA_ALG_CHACHA20_POLY1305
#define NOISE_TAG_SIZE              16
#define NOISE_HEADER_SIZE           3
#define NOISE_HANDSHAKE_HEADER_SIZE 5
#define NOISE_RESPONSE_SIZE         (CONTROL_KEY_SIZE + NOISE_TAG_SIZE)
#define NOISE_INITIATION_SIZE       (NOISE_HANDSHAKE_HEADER_SIZE + 2 * NOISE_RESPONSE_SIZE)
#define NOISE_UPGRADE               "tailscale-control-protocol"

struct noise_handshake {
	uint8_t transcript[CONTROL_KEY_SIZE];
	uint8_t chaining_key[CONTROL_KEY_SIZE];
	uint8_t cipher_key[CONTROL_KEY_SIZE];
};

static psa_key_attributes_t x25519_attributes(void)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY));
	psa_set_key_bits(&attributes, 255);
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attributes, PSA_ALG_ECDH);
	return attributes;
}

int control_x25519_import(const uint8_t private_key[CONTROL_KEY_SIZE], mbedtls_svc_key_id_t *key)
{
	psa_key_attributes_t attributes = x25519_attributes();

	return psa_import_key(&attributes, private_key, CONTROL_KEY_SIZE, key) == PSA_SUCCESS
		       ? 0
		       : -EIO;
}

int control_x25519_public(mbedtls_svc_key_id_t key, uint8_t public_key[CONTROL_KEY_SIZE])
{
	size_t len;
	psa_status_t rc = psa_export_public_key(key, public_key, CONTROL_KEY_SIZE, &len);

	return rc == PSA_SUCCESS && len == CONTROL_KEY_SIZE ? 0 : -EIO;
}

int control_x25519_agree(mbedtls_svc_key_id_t key, const uint8_t peer[CONTROL_KEY_SIZE],
			 uint8_t shared[CONTROL_KEY_SIZE])
{
	size_t len;
	psa_status_t rc = psa_raw_key_agreement(PSA_ALG_ECDH, key, peer, CONTROL_KEY_SIZE, shared,
						CONTROL_KEY_SIZE, &len);

	return rc == PSA_SUCCESS && len == CONTROL_KEY_SIZE ? 0 : -EACCES;
}

static int cipher_import(const uint8_t raw[CONTROL_KEY_SIZE], psa_key_usage_t usage,
			 mbedtls_svc_key_id_t *key)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&attributes, PSA_KEY_TYPE_CHACHA20);
	psa_set_key_bits(&attributes, 256);
	psa_set_key_usage_flags(&attributes, usage);
	psa_set_key_algorithm(&attributes, NOISE_ALGORITHM);
	return psa_import_key(&attributes, raw, CONTROL_KEY_SIZE, key) == PSA_SUCCESS ? 0 : -EIO;
}

static void hash_transcript(struct noise_handshake *hs, const void *bytes, size_t len)
{
	blake2s_ctx hash;

	(void)blake2s_init(&hash, CONTROL_KEY_SIZE, NULL, 0);
	blake2s_update(&hash, hs->transcript, sizeof(hs->transcript));
	blake2s_update(&hash, bytes, len);
	blake2s_final(&hash, hs->transcript);
}

static void hmac(const uint8_t key[CONTROL_KEY_SIZE], const void *data, size_t len,
		 uint8_t out[CONTROL_KEY_SIZE])
{
	blake2s_ctx hash;
	uint8_t pad[BLAKE2S_BLOCK_SIZE];
	uint8_t inner[CONTROL_KEY_SIZE];

	memset(pad, 0x36, sizeof(pad));
	for (size_t i = 0; i < CONTROL_KEY_SIZE; i++) {
		pad[i] ^= key[i];
	}
	(void)blake2s_init(&hash, CONTROL_KEY_SIZE, NULL, 0);
	blake2s_update(&hash, pad, sizeof(pad));
	blake2s_update(&hash, data, len);
	blake2s_final(&hash, inner);

	for (size_t i = 0; i < sizeof(pad); i++) {
		pad[i] ^= 0x36 ^ 0x5c;
	}
	(void)blake2s_init(&hash, CONTROL_KEY_SIZE, NULL, 0);
	blake2s_update(&hash, pad, sizeof(pad));
	blake2s_update(&hash, inner, sizeof(inner));
	blake2s_final(&hash, out);
	mbedtls_platform_zeroize(&hash, sizeof(hash));
	mbedtls_platform_zeroize(pad, sizeof(pad));
	mbedtls_platform_zeroize(inner, sizeof(inner));
}

static void expand_keys(const uint8_t salt[CONTROL_KEY_SIZE], const void *secret, size_t len,
			uint8_t first[CONTROL_KEY_SIZE], uint8_t second[CONTROL_KEY_SIZE])
{
	uint8_t extracted[CONTROL_KEY_SIZE];
	uint8_t info[CONTROL_KEY_SIZE + 1];
	const uint8_t block_one = 1;

	hmac(salt, secret, len, extracted);
	hmac(extracted, &block_one, 1, first);
	memcpy(info, first, CONTROL_KEY_SIZE);
	info[CONTROL_KEY_SIZE] = 2;
	hmac(extracted, info, sizeof(info), second);
	mbedtls_platform_zeroize(extracted, sizeof(extracted));
	mbedtls_platform_zeroize(info, sizeof(info));
}

static int mix_dh(struct noise_handshake *hs, mbedtls_svc_key_id_t private,
		  const uint8_t public[CONTROL_KEY_SIZE])
{
	uint8_t shared[CONTROL_KEY_SIZE];
	int rc = control_x25519_agree(private, public, shared);

	if (rc == 0) {
		expand_keys(hs->chaining_key, shared, sizeof(shared), hs->chaining_key,
			    hs->cipher_key);
	}
	mbedtls_platform_zeroize(shared, sizeof(shared));
	return rc;
}

static int handshake_encrypt(struct noise_handshake *hs, const uint8_t *plain, size_t len,
			     uint8_t *ciphertext)
{
	const uint8_t nonce[12] = {0};
	mbedtls_svc_key_id_t key = MBEDTLS_SVC_KEY_ID_INIT;
	size_t sealed_len;
	int rc = cipher_import(hs->cipher_key, PSA_KEY_USAGE_ENCRYPT, &key);

	if (rc != 0) {
		return rc;
	}
	psa_status_t status = psa_aead_encrypt(key, NOISE_ALGORITHM, nonce, sizeof(nonce),
					       hs->transcript, sizeof(hs->transcript), plain, len,
					       ciphertext, len + NOISE_TAG_SIZE, &sealed_len);

	(void)psa_destroy_key(key);
	if (status != PSA_SUCCESS || sealed_len != len + NOISE_TAG_SIZE) {
		return -EIO;
	}
	hash_transcript(hs, ciphertext, sealed_len);
	return 0;
}

static int handshake_verify(const struct noise_handshake *hs, const uint8_t tag[NOISE_TAG_SIZE])
{
	const uint8_t nonce[12] = {0};
	mbedtls_svc_key_id_t key = MBEDTLS_SVC_KEY_ID_INIT;
	uint8_t unused;
	size_t plain_len;
	int rc = cipher_import(hs->cipher_key, PSA_KEY_USAGE_DECRYPT, &key);

	if (rc != 0) {
		return rc;
	}
	psa_status_t status = psa_aead_decrypt(key, NOISE_ALGORITHM, nonce, sizeof(nonce),
					       hs->transcript, sizeof(hs->transcript), tag,
					       NOISE_TAG_SIZE, &unused, sizeof(unused), &plain_len);

	(void)psa_destroy_key(key);
	return status == PSA_SUCCESS && plain_len == 0 ? 0 : -EACCES;
}

static int send_upgrade(struct control_io *io, const uint8_t message[NOISE_INITIATION_SIZE])
{
	uint8_t encoded[4 * ((NOISE_INITIATION_SIZE + 2) / 3) + 1];
	char request[512];
	size_t encoded_len;
	int rc = base64_encode(encoded, sizeof(encoded), &encoded_len, message,
			       NOISE_INITIATION_SIZE);

	if (rc != 0) {
		return rc;
	}
	rc = snprintf(request, sizeof(request),
		      "POST /ts2021 HTTP/1.1\r\nHost: " CONTROL_HOST "\r\n"
		      "Connection: Upgrade\r\nUpgrade: " NOISE_UPGRADE "\r\n"
		      "X-Tailscale-Handshake: %.*s\r\nContent-Length: 0\r\n\r\n",
		      (int)encoded_len, encoded);
	if (rc < 0 || rc >= sizeof(request)) {
		return -EMSGSIZE;
	}
	return control_io_write(io, request, rc);
}

static int read_upgrade(struct control_noise *noise, uint8_t response[NOISE_RESPONSE_SIZE])
{
	char *header = (char *)noise->record;
	uint8_t frame[NOISE_HEADER_SIZE];
	int rc = control_http_reply(noise->io, header, sizeof(noise->record), 101);

	if (rc != 0) {
		return rc;
	}
	char *line = strstr(header, "\r\n");
	bool upgraded = false;

	while (line != NULL && line[2] != '\r') {
		line += 2;
		static const char upgrade_header[] = "Upgrade:";

		if (strncasecmp(line, upgrade_header, sizeof(upgrade_header) - 1) == 0) {
			char *value = line + sizeof(upgrade_header) - 1;

			while (*value == ' ' || *value == '\t') {
				value++;
			}
			upgraded = strncasecmp(value, NOISE_UPGRADE "\r\n",
					       sizeof(NOISE_UPGRADE "\r\n") - 1) == 0;
			break;
		}
		line = strstr(line, "\r\n");
	}
	if (!upgraded) {
		return -EPROTO;
	}
	rc = control_io_read(noise->io, frame, sizeof(frame));
	if (rc != 0) {
		return rc;
	}
	if (frame[0] != 2 || sys_get_be16(frame + 1) != NOISE_RESPONSE_SIZE) {
		return -EPROTO;
	}
	return control_io_read(noise->io, response, NOISE_RESPONSE_SIZE);
}

int control_noise_connect(struct control_noise *noise, mbedtls_svc_key_id_t machine,
			  const uint8_t server[CONTROL_KEY_SIZE])
{
	static const char protocol[] = "Noise_IK_25519_ChaChaPoly_BLAKE2s";
	static const char prologue[] = "Tailscale Control Protocol v" CONTROL_VERSION_STRING;
	psa_key_attributes_t attributes = x25519_attributes();
	mbedtls_svc_key_id_t ephemeral = MBEDTLS_SVC_KEY_ID_INIT;
	struct noise_handshake hs;
	uint8_t message[NOISE_INITIATION_SIZE];
	uint8_t response[NOISE_RESPONSE_SIZE];
	uint8_t machine_public[CONTROL_KEY_SIZE];
	uint8_t *ephemeral_public = message + NOISE_HANDSHAKE_HEADER_SIZE;
	uint8_t *machine_ciphertext = ephemeral_public + CONTROL_KEY_SIZE;
	uint8_t *tag = machine_ciphertext + NOISE_RESPONSE_SIZE;
	int rc = -EIO;

	(void)blake2s(hs.transcript, sizeof(hs.transcript), NULL, 0, protocol,
		      sizeof(protocol) - 1);
	memcpy(hs.chaining_key, hs.transcript, sizeof(hs.chaining_key));
	hash_transcript(&hs, prologue, sizeof(prologue) - 1);
	hash_transcript(&hs, server, CONTROL_KEY_SIZE);

	sys_put_be16(CONTROL_VERSION, message);
	message[2] = 1;
	sys_put_be16(sizeof(message) - NOISE_HANDSHAKE_HEADER_SIZE, message + 3);
	if (psa_generate_key(&attributes, &ephemeral) != PSA_SUCCESS) {
		goto out;
	}
	rc = control_x25519_public(ephemeral, ephemeral_public);
	if (rc == 0) {
		hash_transcript(&hs, ephemeral_public, CONTROL_KEY_SIZE);
		rc = mix_dh(&hs, ephemeral, server);
	}
	if (rc == 0) {
		rc = control_x25519_public(machine, machine_public);
	}
	if (rc == 0) {
		rc = handshake_encrypt(&hs, machine_public, sizeof(machine_public),
				       machine_ciphertext);
	}
	if (rc == 0) {
		rc = mix_dh(&hs, machine, server);
	}
	if (rc == 0) {
		rc = handshake_encrypt(&hs, NULL, 0, tag);
	}
	if (rc == 0) {
		rc = send_upgrade(noise->io, message);
	}
	if (rc == 0) {
		rc = read_upgrade(noise, response);
	}
	if (rc == 0) {
		hash_transcript(&hs, response, CONTROL_KEY_SIZE);
		rc = mix_dh(&hs, ephemeral, response);
	}
	if (rc == 0) {
		rc = mix_dh(&hs, machine, response);
	}
	if (rc == 0) {
		rc = handshake_verify(&hs, response + CONTROL_KEY_SIZE);
	}
	if (rc == 0) {
		expand_keys(hs.chaining_key, NULL, 0, hs.transcript, hs.cipher_key);
		rc = cipher_import(hs.transcript, PSA_KEY_USAGE_ENCRYPT, &noise->tx_key);
	}
	if (rc == 0) {
		rc = cipher_import(hs.cipher_key, PSA_KEY_USAGE_DECRYPT, &noise->rx_key);
	}
out:
	(void)psa_destroy_key(ephemeral);
	mbedtls_platform_zeroize(&hs, sizeof(hs));
	return rc;
}

int control_noise_write(struct control_noise *noise, const void *data, size_t len)
{
	const uint8_t *next = data;

	while (len > 0) {
		size_t part = MIN(len, NOISE_PLAINTEXT_SIZE);
		uint8_t nonce[12] = {0};
		size_t sealed_len;

		if (noise->tx_counter == UINT64_MAX) {
			return -EOVERFLOW;
		}
		sys_put_be64(noise->tx_counter++, nonce + 4);
		psa_status_t status =
			psa_aead_encrypt(noise->tx_key, NOISE_ALGORITHM, nonce, sizeof(nonce), NULL,
					 0, next, part, noise->record + NOISE_HEADER_SIZE,
					 sizeof(noise->record) - NOISE_HEADER_SIZE, &sealed_len);

		if (status != PSA_SUCCESS || sealed_len != part + NOISE_TAG_SIZE) {
			return -EIO;
		}
		noise->record[0] = 4;
		sys_put_be16(sealed_len, noise->record + 1);
		int rc = control_io_write(noise->io, noise->record, NOISE_HEADER_SIZE + sealed_len);

		if (rc != 0) {
			return rc;
		}
		next += part;
		len -= part;
	}
	return 0;
}

static int read_record(struct control_noise *noise)
{
	uint8_t frame[NOISE_HEADER_SIZE];
	uint8_t nonce[12] = {0};
	/* The header has its own storage. An idle write can use the shared
	 * record buffer until the first byte of the record body is read.
	 */
#ifdef CONFIG_TAILSCALE_TRANSPORT
	noise->io->reading_noise_header = true;
#endif
	int rc = control_io_read(noise->io, frame, sizeof(frame));
#ifdef CONFIG_TAILSCALE_TRANSPORT
	noise->io->reading_noise_header = false;
#endif

	if (rc != 0) {
		return rc;
	}
	uint16_t len = sys_get_be16(frame + 1);

	if (frame[0] != 4 || len < NOISE_TAG_SIZE || len > NOISE_RECORD_SIZE - NOISE_HEADER_SIZE) {
		return -EPROTO;
	}
	if (noise->rx_counter == UINT64_MAX) {
		return -EOVERFLOW;
	}
	rc = control_io_read(noise->io, noise->record, len);
	if (rc != 0) {
		return rc;
	}
	sys_put_be64(noise->rx_counter++, nonce + 4);
	noise->rx_pos = 0;
	psa_status_t status = psa_aead_decrypt(noise->rx_key, NOISE_ALGORITHM, nonce, sizeof(nonce),
					       NULL, 0, noise->record, len, noise->plaintext,
					       sizeof(noise->plaintext), &noise->rx_len);

	return status == PSA_SUCCESS && noise->rx_len == len - NOISE_TAG_SIZE ? 0 : -EACCES;
}

int control_noise_read(struct control_noise *noise, void *data, size_t len)
{
	uint8_t *next = data;

	while (len > 0) {
		if (noise->rx_pos == noise->rx_len) {
			int rc = read_record(noise);

			if (rc != 0) {
				return rc;
			}
		}
		size_t part = MIN(len, noise->rx_len - noise->rx_pos);

		memcpy(next, noise->plaintext + noise->rx_pos, part);
		noise->rx_pos += part;
		next += part;
		len -= part;
	}
	return 0;
}
