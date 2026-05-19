/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stddef.h>
#include <psa/crypto.h>

/* TLS 1.3 Handshake message types */
#define TLS_HS_CLIENT_HELLO          1
#define TLS_HS_SERVER_HELLO          2
#define TLS_HS_ENCRYPTED_EXTENSIONS  8
#define TLS_HS_CERTIFICATE           11
#define TLS_HS_CERTIFICATE_REQUEST   13
#define TLS_HS_CERTIFICATE_VERIFY    15
#define TLS_HS_FINISHED              20

/* Key schedule labels */
#define TLS13_LABEL_DERIVED        "derived"
#define TLS13_LABEL_C_HS_TRAFFIC   "c hs traffic"
#define TLS13_LABEL_S_HS_TRAFFIC   "s hs traffic"
#define TLS13_LABEL_C_AP_TRAFFIC   "c ap traffic"
#define TLS13_LABEL_S_AP_TRAFFIC   "s ap traffic"
#define TLS13_LABEL_FINISHED       "finished"
#define TLS13_LABEL_RES_MASTER     "res master"

/* Supported cipher suites for QUIC */
#define TLS_AES_128_GCM_SHA256       0x1301
#define TLS_AES_256_GCM_SHA384       0x1302
#define TLS_CHACHA20_POLY1305_SHA256 0x1303

/* TLS extension types */
#define TLS_EXT_ALPN                  0x10
#define TLS_EXT_SUPPORTED_VERSIONS    0x2b
#define TLS_EXT_KEY_SHARE             0x33
#define TLS_EXT_QUIC_TRANSPORT_PARAMS 0x39

/* TLS 1.3 CertificateVerify constants (RFC 8446 Section 4.4.3) */
#define TLS13_CERT_VERIFY_PADDING_LEN   64
#define TLS13_CERT_VERIFY_PADDING_BYTE  0x20

#define TLS13_SIG_CONTEXT_SERVER "TLS 1.3, server CertificateVerify"
#define TLS13_SIG_CONTEXT_CLIENT "TLS 1.3, client CertificateVerify"

struct quic_tls_context;

/* TLS handshake state */
enum quic_tls_state {
	QUIC_TLS_STATE_INITIAL,
	QUIC_TLS_STATE_WAIT_CLIENT_HELLO,
	QUIC_TLS_STATE_WAIT_SERVER_HELLO,
	QUIC_TLS_STATE_WAIT_ENCRYPTED_EXTENSIONS,
	QUIC_TLS_STATE_WAIT_CERT,
	QUIC_TLS_STATE_WAIT_CERT_VERIFY,
	QUIC_TLS_STATE_WAIT_FINISHED,
	QUIC_TLS_STATE_CONNECTED,
	QUIC_TLS_STATE_ERROR,
};

enum quic_secret_level {
	QUIC_SECRET_LEVEL_INITIAL = 0,
	QUIC_SECRET_LEVEL_HANDSHAKE = 1,
	QUIC_SECRET_LEVEL_APPLICATION = 2,
};

/*
 * Long header packet type values (bits 4-5 of first byte)
 * First byte format:  1 1 TT RR PP
 *   1 = Header Form (long)
 *   1 = Fixed Bit
 *   TT = Long Packet Type
 *   RR = Reserved (protected)
 *   PP = Packet Number Length - 1
 */
#define QUIC_LONG_HEADER_INITIAL    0xC0  /* TT = 00 */
#define QUIC_LONG_HEADER_0RTT       0xD0  /* TT = 01 */
#define QUIC_LONG_HEADER_HANDSHAKE  0xE0  /* TT = 10 */
#define QUIC_LONG_HEADER_RETRY      0xF0  /* TT = 11 */

/*
 * Short header first byte format:  0 1 S RR K PP
 *   0 = Header Form (short)
 *   1 = Fixed Bit
 *   S = Spin Bit
 *   RR = Reserved (protected)
 *   K = Key Phase
 *   PP = Packet Number Length - 1
 */
#define QUIC_SHORT_HEADER_BASE      0x40

/*
 * Mask values for extracting fields from first byte
 */
#define QUIC_HEADER_FORM_MASK       0x80  /* Long (1) vs Short (0) */
#define QUIC_LONG_PACKET_TYPE_MASK  0x30  /* Bits 4-5 */
#define QUIC_PACKET_NUMBER_LEN_MASK 0x03  /* Bits 0-1 */
#define QUIC_SHORT_SPIN_BIT_MASK    0x20  /* Bit 5 */
#define QUIC_SHORT_KEY_PHASE_MASK   0x04  /* Bit 2 */

enum quic_packet_type {
	QUIC_PACKET_TYPE_INITIAL   = 0x0,
	QUIC_PACKET_TYPE_0RTT      = 0x1,
	QUIC_PACKET_TYPE_HANDSHAKE = 0x2,
	QUIC_PACKET_TYPE_RETRY     = 0x3,
	QUIC_PACKET_TYPE_1RTT      = 0x4, /* Short header (1-RTT) packet */
};

/*
 * Extract packet type from long header first byte
 */
static inline enum quic_packet_type quic_get_long_packet_type(uint8_t first_byte)
{
	return (enum quic_packet_type)((first_byte & QUIC_LONG_PACKET_TYPE_MASK) >> 4);
}

/*
 * Check if packet has long header
 */
static inline bool quic_is_long_header(uint8_t first_byte)
{
	return (first_byte & QUIC_HEADER_FORM_MASK) != 0;
}

/*
 * Extract packet number length from first byte (works for both header types)
 */
static inline size_t quic_get_pn_len_from_header(uint8_t first_byte)
{
	return (first_byte & QUIC_PACKET_NUMBER_LEN_MASK) + 1;
}

/* Callback to provide handshake secrets to QUIC layer */
typedef int (*quic_tls_secret_cb)(void *user_data,
				  enum quic_secret_level level,
				  const uint8_t *rx_secret,
				  const uint8_t *tx_secret,
				  size_t secret_len);

/* Callback to send handshake data (CRYPTO frames) */
typedef int (*quic_tls_send_cb)(void *user_data,
				enum quic_secret_level level,
				const uint8_t *data,
				size_t len);

/* TLS 1.3 key schedule state */
struct tls13_key_schedule {
	uint8_t early_secret[QUIC_HASH_MAX_LEN];
	uint8_t handshake_secret[QUIC_HASH_MAX_LEN];
	uint8_t master_secret[QUIC_HASH_MAX_LEN];
	uint8_t client_hs_traffic_secret[QUIC_HASH_MAX_LEN];
	uint8_t server_hs_traffic_secret[QUIC_HASH_MAX_LEN];
	uint8_t client_ap_traffic_secret[QUIC_HASH_MAX_LEN];
	uint8_t server_ap_traffic_secret[QUIC_HASH_MAX_LEN];
	/* Transcript hash after Server Finished (used for app secrets) */
	uint8_t server_finished_transcript[QUIC_HASH_MAX_LEN];
	size_t server_finished_transcript_len;
	psa_hash_operation_t transcript_hash;
	psa_algorithm_t hash_alg;
	size_t hash_len;
	uint16_t cipher_suite;
	uint16_t key_exchange_group;  /* Selected ECDHE group (x25519 or secp256r1) */
	bool initialized;
};
