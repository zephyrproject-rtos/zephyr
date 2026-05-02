/** @file
 * @brief QUIC private header
 *
 *  This is not to be included by the application.
 */

/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/smf.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/net_ip.h>

#if !defined(MBEDTLS_ALLOW_PRIVATE_ACCESS)
#define MBEDTLS_ALLOW_PRIVATE_ACCESS
#endif

#if defined(CONFIG_MBEDTLS)
#include <mbedtls/net_sockets.h>
#include <mbedtls/x509.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/ssl.h>
#include <mbedtls/ssl_cookie.h>
#include <mbedtls/error.h>
#include <mbedtls/platform.h>
#include <mbedtls/ssl_cache.h>
#endif /* CONFIG_MBEDTLS */

#if defined(CONFIG_QUIC_TLS_MAX_APP_PROTOCOLS)
#define ALPN_MAX_PROTOCOLS CONFIG_QUIC_TLS_MAX_APP_PROTOCOLS
#else
#define ALPN_MAX_PROTOCOLS 0
#endif /* CONFIG_QUIC_TLS_MAX_APP_PROTOCOLS */

#define MAX_CONN_ID_LEN 20
#define MAX_MY_CONN_ID_LEN 8

#define QUIC_VERSION_1 0x00000001

#define QUIC_STREAM_ID_UNASSIGNED UINT64_MAX

/**
 * Description of external buffers for payload and receiving
 */
struct quic_buffer {
	/* external buffer */
	uint8_t *buf;
	/* size of external buffer */
	size_t size;
	/* data length in external buffer */
	size_t count;
};

#define QUIC_HP_SAMPLE_LEN 16
#define QUIC_HP_MASK_LEN 5
#define QUIC_HP_MAX_PN_LEN 4

#define QUIC_HASH_SHA2_384_LEN 48
#define QUIC_HASH_SHA2_256_LEN 32
#define QUIC_HASH_SHA2_128_LEN 16
#define QUIC_HASH_CHACHA20_LEN 32

#define QUIC_HASH_MAX_LEN QUIC_HASH_SHA2_384_LEN

#define QUIC_AEAD_TAG_LEN 16
#define QUIC_RESET_TOKEN_LEN 16

#define MAX_QUIC_MIN_INITIAL_SIZE 1200
#define MAX_QUIC_FRAME_SIZE 1300

/* Maximum QUIC long header size:
 * 1 (first byte) + 4 (version) + 1 (DCID len) + 20 (DCID) +
 * 1 (SCID len) + 20 (SCID) + 8 (token len varint) + 8 (length varint) +
 * 4 (packet number) = 67 bytes
 *
 * Round up to 128 for safety with tokens.
 */
#define MAX_QUIC_HEADER_SIZE 128

/* Maximum short header size:
 * 1 (first byte) + 20 (DCID max) + 4 (PN max) = 25 bytes
 */
#define MAX_QUIC_SHORT_HEADER_SIZE 25

/* RFC 9002 Section 6.1.1: packet reordering threshold */
#define QUIC_PACKET_THRESHOLD   3

/* RFC 9002 Section 6.2.2: Initial RTT estimate used before any RTT sample
 * has been measured. 333 ms is the value mandated by the RFC. It is used
 * to compute the first PTO timeout and to seed smoothed_rtt / rtt_var in
 * quic_recovery_init() before any real measurement is available. Once the
 * first ACK is received, smoothed_rtt is replaced with the actual sample
 * and this constant is no longer used.
 */
#define QUIC_INITIAL_RTT_US         333000U   /* 333 ms in microseconds */

/* RFC 9002 Section 6.1.2: Multiplier for the time-based loss threshold.
 * Represents 9/8 = 1.125, scaled by 1000 to avoid floating point.
 * A packet is declared lost if it was sent more than
 * (SRTT * QUIC_TIME_THRESHOLD / 1000) ago.
 */
#define QUIC_TIME_THRESHOLD         1125U

/* Minimum time granularity for loss detection timers (1 ms in microseconds).
 * Prevents the loss delay from collapsing to zero on very low-latency links.
 */
#define QUIC_GRANULARITY_US         1000U

/* Max ACK delay assumed from peer (ms) when not yet negotiated */
#define QUIC_MAX_ACK_DELAY_MS   25

/**
 * Information about a sent packet for RTT measurement and loss detection.
 * This is stored in a ring buffer per packet number space.
 */
struct quic_sent_pkt_info {
	/** Packet number */
	uint64_t pkt_num;

	/** Time packet was sent (k_uptime_get() value in ms) */
	int64_t sent_time;

	/** Size of packet in bytes (for bytes_in_flight tracking) */
	uint16_t sent_bytes;

	/** True if this packet contains ack-eliciting frames */
	bool ack_eliciting;

	/** True if packet is still in flight (not yet acked/lost) */
	bool in_flight;

	/* Stream frame carried by this packet (for retransmission).
	 * Only valid when has_stream_frame is true.
	 * One STREAM frame per packet is assumed, which matches the
	 * current quic_stream_send() behaviour.
	 */
	bool has_stream_frame;
	bool stream_fin;
	uint64_t stream_id;
	uint64_t stream_offset;   /* byte offset within the stream */
	uint16_t stream_data_len; /* payload bytes in this frame */
};

#include "quic_tls.h"

enum quic_handshake_status {
	QUIC_HANDSHAKE_IN_PROGRESS = 0,
	QUIC_HANDSHAKE_COMPLETED,
};

/* Cipher suite identifiers */
enum quic_cipher_algo {
	QUIC_CIPHER_AES_128_GCM = 0,
	QUIC_CIPHER_AES_256_GCM,
	QUIC_CIPHER_CHACHA20_POLY1305,
	QUIC_CIPHER_AES_128_CCM
};

enum quic_hash_algo {
	QUIC_HASH_SHA256 = 0,
	QUIC_HASH_SHA384
};

enum quic_frame_type {
	QUIC_FRAME_TYPE_PADDING = 0x00,
	QUIC_FRAME_TYPE_PING = 0x01,
	QUIC_FRAME_TYPE_ACK = 0x02,
	QUIC_FRAME_TYPE_ACK_ECN = 0x03,
	QUIC_FRAME_TYPE_RESET_STREAM = 0x04,
	QUIC_FRAME_TYPE_STOP_SENDING = 0x05,
	QUIC_FRAME_TYPE_CRYPTO = 0x06,
	QUIC_FRAME_TYPE_NEW_TOKEN = 0x07,
	QUIC_FRAME_TYPE_STREAM_BASE = 0x08, /* 0x08 - 0x0f */
	QUIC_FRAME_TYPE_STREAM_END = 0x0f,
	QUIC_FRAME_TYPE_MAX_DATA = 0x10,
	QUIC_FRAME_TYPE_MAX_STREAM_DATA = 0x11,
	QUIC_FRAME_TYPE_MAX_STREAMS_BIDI = 0x12,
	QUIC_FRAME_TYPE_MAX_STREAMS_UNI = 0x13,
	QUIC_FRAME_TYPE_DATA_BLOCKED = 0x14,
	QUIC_FRAME_TYPE_STREAM_DATA_BLOCKED = 0x15,
	QUIC_FRAME_TYPE_STREAMS_BLOCKED_BIDI = 0x16,
	QUIC_FRAME_TYPE_STREAMS_BLOCKED_UNI = 0x17,
	QUIC_FRAME_TYPE_NEW_CONNECTION_ID = 0x18,
	QUIC_FRAME_TYPE_RETIRE_CONNECTION_ID = 0x19,
	QUIC_FRAME_TYPE_PATH_CHALLENGE = 0x1a,
	QUIC_FRAME_TYPE_PATH_RESPONSE = 0x1b,
	QUIC_FRAME_TYPE_CONNECTION_CLOSE_TRANSPORT = 0x1c,
	QUIC_FRAME_TYPE_CONNECTION_CLOSE_APPLICATION = 0x1d,
	QUIC_FRAME_TYPE_HANDSHAKE_DONE = 0x1e
};

enum quic_error_code {
	QUIC_ERROR_NO_ERROR = 0x00,                  /* Graceful close, no error */
	QUIC_ERROR_INTERNAL_ERROR = 0x01,            /* Implementation bug */
	QUIC_ERROR_CONNECTION_REFUSED = 0x02,        /* Server refused connection */
	QUIC_ERROR_FLOW_CONTROL_ERROR = 0x03,        /* Flow control violation */
	QUIC_ERROR_STREAM_LIMIT_ERROR = 0x04,        /* Stream limit exceeded */
	QUIC_ERROR_STREAM_STATE_ERROR = 0x05,        /* Invalid stream state */
	QUIC_ERROR_FINAL_SIZE_ERROR = 0x06,          /* Final size mismatch */
	QUIC_ERROR_FRAME_ENCODING_ERROR = 0x07,      /* Malformed frame */
	QUIC_ERROR_TRANSPORT_PARAMETER_ERROR = 0x08, /* Invalid transport parameters */
	QUIC_ERROR_CONNECTION_ID_LIMIT_ERROR = 0x09, /* Too many connection IDs */
	QUIC_ERROR_PROTOCOL_VIOLATION = 0x0a,        /* General protocol violation */
	QUIC_ERROR_INVALID_TOKEN = 0x0b,             /* Invalid Retry token */
	QUIC_ERROR_APPLICATION_ERROR = 0x0c,         /* Application-layer error (generic */
	QUIC_ERROR_CRYPTO_BUFFER_EXCEEDED = 0x0d,    /* Crypto buffer overflow */
	QUIC_ERROR_KEY_UPDATE_ERROR = 0x0e,          /* Key update failure */
	QUIC_ERROR_AEAD_LIMIT_REACHED = 0x0f,        /* Encryption limit reached */
	QUIC_ERROR_NO_VIABLE_PATH = 0x10,            /* No working network path */
	/* TLS alert codes are mapped to 0x100 + alert_code (RFC 9001 Section 4.8) */
	QUIC_ERROR_CRYPTO_BASE = 0x100,
};

/* From RFC 8446 Section 6, TLS alert codes that can be sent in QUIC CONNECTION_CLOSE */
enum quic_tls_error_code {
	QUIC_CRYPTO_ERROR_CLOSE_NOTIFY = 0,
	QUIC_CRYPTO_ERROR_UNEXPECTED_MESSAGE = 10,
	QUIC_CRYPTO_ERROR_BAD_RECORD_MAC = 20,
	QUIC_CRYPTO_ERROR_RECORD_OVERFLOW = 22,
	QUIC_CRYPTO_ERROR_HANDSHAKE_FAILURE = 40,
	QUIC_CRYPTO_ERROR_BAD_CERTIFICATE = 42,
	QUIC_CRYPTO_ERROR_UNSUPPORTED_CERTIFICATE = 43,
	QUIC_CRYPTO_ERROR_CERTIFICATE_REVOKED = 44,
	QUIC_CRYPTO_ERROR_CERTIFICATE_EXPIRED = 45,
	QUIC_CRYPTO_ERROR_CERTIFICATE_UNKNOWN = 46,
	QUIC_CRYPTO_ERROR_ILLEGAL_PARAMETER = 47,
	QUIC_CRYPTO_ERROR_UNKNOWN_CA = 48,
	QUIC_CRYPTO_ERROR_ACCESS_DENIED = 49,
	QUIC_CRYPTO_ERROR_DECODE_ERROR = 50,
	QUIC_CRYPTO_ERROR_DECRYPT_ERROR = 51,
	QUIC_CRYPTO_ERROR_PROTOCOL_VERSION = 70,
	QUIC_CRYPTO_ERROR_INSUFFICIENT_SECURITY = 71,
	QUIC_CRYPTO_ERROR_INTERNAL_ERROR = 80,
	QUIC_CRYPTO_ERROR_INAPPROPRIATE_FALLBACK = 86,
	QUIC_CRYPTO_ERROR_USER_CANCELED = 90,
	QUIC_CRYPTO_ERROR_MISSING_EXTENSION = 109,
	QUIC_CRYPTO_ERROR_UNSUPPORTED_EXTENSION = 110,
	QUIC_CRYPTO_ERROR_UNRECOGNIZED_NAME = 112,
	QUIC_CRYPTO_ERROR_BAD_CERTIFICATE_STATUS_RESPONSE = 113,
	QUIC_CRYPTO_ERROR_UNKNOWN_PSK_IDENTITY = 115,
	QUIC_CRYPTO_ERROR_CERTIFICATE_REQUIRED = 116,
	QUIC_CRYPTO_ERROR_NO_APPLICATION_PROTOCOL = 120,
};

/* Transport parameter IDs from RFC 9000 Section 18.2 */
#define QUIC_MAX_IDLE_TIMEOUT                    0x01
#define QUIC_MAX_UDP_PAYLOAD_SIZE                0x03
#define QUIC_INITIAL_MAX_DATA                    0x04
#define QUIC_INITIAL_MAX_STREAM_DATA_BIDI_LOCAL  0x05
#define QUIC_INITIAL_MAX_STREAM_DATA_BIDI_REMOTE 0x06
#define QUIC_INITIAL_MAX_STREAM_DATA_UNI         0x07
#define QUIC_INITIAL_MAX_STREAMS_BIDI            0x08
#define QUIC_INITIAL_MAX_STREAMS_UNI             0x09

/** A list of secure tags that TLS context should use. */
struct sec_tag_list {
	/** An array of secure tags referencing TLS credentials. */
	sec_tag_t sec_tags[CONFIG_QUIC_TLS_MAX_CREDENTIALS];

	/** Number of configured secure tags. */
	int sec_tag_count;
};

/** TLS context information. */
struct quic_tls_context {
	/** Associated QUIC endpoint. */
	struct quic_endpoint *ep;

	/** Socket type. */
	enum net_sock_type type;

	/** Secure protocol version running on TLS context. */
	enum net_ip_protocol_secure tls_version;

	/** Socket flags passed to a socket call. */
	int flags;

	/** Indicates whether socket is in error state at TLS level. */
	int error;
	const char *error_msg;

	/** Information whether TLS handshake is complete or not. */
	struct k_sem tls_established;

	/* TLS socket mutex lock. */
	struct k_mutex *lock;

	/** TLS specific option values. */
	struct {
		/** Select which credentials to use with TLS. */
		struct sec_tag_list sec_tag_list;

		/** 0-terminated list of allowed ciphersuites (mbedTLS format).
		 * TODO: this is not used for anything yet
		 */
		int ciphersuites[CONFIG_QUIC_TLS_MAX_CIPHERSUITES + 1];

		/** Peer verification level. */
		int8_t verify_level;

		/** Indicating on whether DER certificates should not be copied
		 * to the heap.
		 */
		int8_t cert_nocopy;

		/** NULL-terminated list of allowed application layer
		 * protocols.
		 */
		const char *alpn_list[ALPN_MAX_PROTOCOLS + 1];

#if defined(CONFIG_QUIC_TLS_CERT_VERIFY_CALLBACK)
		struct tls_cert_verify_cb cert_verify;
#endif /* CONFIG_NET_SOCKETS_TLS_CERT_VERIFY_CALLBACK */
	} options;

	/* Current TLS state */
	enum quic_tls_state state;

	/* Key schedule */
	struct tls13_key_schedule ks;

	/* Handshake transcript (accumulated handshake messages) */
	uint8_t transcript_buffer[CONFIG_QUIC_TLS_TRANSCRIPT_BUF_LEN];
	size_t transcript_len;

	/* Random values */
	uint8_t client_random[32];
	uint8_t server_random[32];

	/* Certificate request context */
#define QUIC_CERT_REQ_CONTEXT_LEN 8
	uint8_t cert_request_context[QUIC_CERT_REQ_CONTEXT_LEN];
	uint8_t cert_request_context_len;

	/* Key exchange (ECDHE with secp256r1 or x25519) */
	psa_key_id_t ecdh_key_id;
	uint8_t ecdh_public_key[65];  /* Uncompressed point */
	size_t ecdh_public_key_len;
	uint8_t peer_public_key[65];
	size_t peer_public_key_len;
	uint8_t shared_secret[32];

	/* Certificates (for server or client auth) */
	const uint8_t *my_cert;      /* DER-encoded certificate */
	size_t my_cert_len;

	const uint8_t *my_key;       /* DER-encoded private key */
	size_t my_key_len;
	psa_key_id_t signing_key_id;  /* Imported signing key */

	/* Intermediate certificate chain (sec_tags resolved at handshake time) */
	sec_tag_t cert_chain_tags[CONFIG_QUIC_TLS_MAX_CERT_CHAIN_DEPTH];
	size_t cert_chain_count;

	/* CA chain (for peer verification) */
	bool ca_cert;

	/* Client certificate request settings (in server-side) */
	bool expecting_client_cert;

	/* Whether server requested a certificate from the client (in client side) */
	bool server_requested_cert;

	/* Received peer certificate for verification */
	uint8_t peer_cert[CONFIG_QUIC_TLS_MAX_CERT_SIZE];
	size_t peer_cert_len;

	/* QUIC transport parameters */
#define MAX_QUIC_TP_LEN 256
	uint8_t local_tp[MAX_QUIC_TP_LEN];
	size_t local_tp_len;
	uint8_t peer_tp[MAX_QUIC_TP_LEN];
	size_t peer_tp_len;

	/* Negotiated ALPN protocol (pointer into alpn_list) */
	const char *negotiated_alpn;

	/* Callbacks */
	quic_tls_secret_cb secret_cb;
	quic_tls_send_cb send_cb;
	void *user_data;

#if defined(CONFIG_MBEDTLS)
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	/** mbedTLS structure for CA chain. */
	mbedtls_x509_crt ca_chain;

	/** mbedTLS structure for own certificate. */
	mbedtls_x509_crt own_cert;

	/** mbedTLS structure for own private key. */
	mbedtls_pk_context priv_key;
#endif /* MBEDTLS_X509_CRT_PARSE_C */
#endif /* CONFIG_MBEDTLS */

	/** Information whether TLS context was initialized. */
	bool is_initialized : 1;

	/** Information whether TLS handshake is currently in progress. */
	bool handshake_in_progress : 1;

	/** Session ended at the TLS/DTLS level. */
	bool session_closed : 1;
};

struct quic_hp_cipher {
	psa_key_id_t key_id;      /* Header protection key ID */
	psa_algorithm_t algo;     /* Cipher algorithm type */
	psa_key_type_t key_type;  /* Cipher key type */
	int cipher_algo;
	bool initialized;
};

#define TLS13_AEAD_NONCE_LENGTH 12

struct quic_pp_cipher {
	psa_key_id_t key_id;  /* Packet protection key ID */
	psa_algorithm_t algo;     /* Cipher algorithm type */
	psa_key_type_t key_type;  /* Cipher key type */
	int cipher_algo;
	int cipher_mode;
	uint8_t iv[TLS13_AEAD_NONCE_LENGTH];
	int iv_len;
	bool initialized;
};

struct quic_ciphers {
	struct quic_hp_cipher hp; /* Header protection cipher */
	struct quic_pp_cipher pp; /* Packet protection cipher */
};

/** QUIC cryptographic context */
struct quic_crypto_context {
	/** Ciphers used for packet protection and header protection.
	 */
	struct quic_ciphers tx; /* Ciphers for transmitting */
	struct quic_ciphers rx; /* Ciphers for receiving */

	/** Are the settings initialized
	 */
	bool initialized;
};

/**
 * Out-of-order CRYPTO frame segment metadata.
 * Data is stored directly into the reassembly buffer at the correct offset.
 */
struct quic_crypto_ooo_seg {
	uint32_t offset;  /* Byte offset in CRYPTO stream */
	uint16_t len;     /* Length of this segment */
	bool valid;       /* Slot in use */
};

/**
 * Crypto stream state per encryption level
 */
struct quic_crypto_stream {
	uint32_t tx_offset;  /* Next byte to send */
	uint32_t rx_offset;  /* Next byte expected (contiguous data up to here) */
	uint32_t tls_offset; /* Bytes already passed to TLS processing */
	size_t rx_buffer_len;
};

/**
 * QUIC endpoint information
 */
struct quic_endpoint {
	/**
	 * First element is reserved for slab allocator's internal use.
	 */
	intptr_t internal_use_only;

	/** Reference count.
	 */
	atomic_t refcount;

	/** Slab pointer from where it belongs to */
	struct k_mem_slab *slab;

	/** Index of this endpoint pointer in the endpoints array. This
	 * is used to traverse allocated endpoints in the slab.
	 */
	int slab_index;

	/** Stream list node */
	sys_snode_t node;

	/** Parent endpoint, if any. If the endpoint is listening, a new
	 * endpoint is created for each incoming connection, and the
	 * parent points to the listening endpoint.
	 */
	struct quic_endpoint *parent;

	/** Cryptographic context for this endpoint.
	 */
	struct {
		/** TLS context for this endpoint (embedded, not pointer).
		 */
		struct quic_tls_context tls;

		struct quic_crypto_context initial;
		struct quic_crypto_context handshake;
		struct quic_crypto_context application;

		/** Crypto stream state per level (Initial, Handshake, 1-RTT) */
		struct quic_crypto_stream stream[3];

		/** TX buffer for outgoing packets */
		uint8_t tx_buffer[CONFIG_QUIC_TX_BUFFER_SIZE];

		/**
		 * Shared RX reassembly buffer for CRYPTO frames.
		 * Since handshake progresses sequentially through levels,
		 * a single buffer is shared. When advancing to a new level,
		 * reset rx_buf_level and rx_buf_len.
		 */
		uint8_t rx_buffer[CONFIG_QUIC_CRYPTO_RX_BUFFER_SIZE];
		uint32_t rx_buf_len;           /* Highest byte offset written + 1 */
		enum quic_secret_level rx_buf_level; /* Current level using buffer */

		/** Out-of-order segment tracking (shared across levels) */
		struct quic_crypto_ooo_seg ooo[CONFIG_QUIC_CRYPTO_OOO_SLOTS];
	} crypto;

	/** Largest packet number tracking per encryption level for TX */
	struct {
		uint64_t initial;
		uint64_t handshake;
		uint64_t application;
	} tx_pn;

	/** Largest packet number tracking per encryption level for RX */
	struct {
		uint64_t initial;
		uint64_t handshake;
		uint64_t application;
	} rx_pn;

	/** The real UDP socket to use when sending QUIC data to peer.
	 * Different QUIC connections may share the same UDP socket.
	 */
	int sock;

	/** Remote address of the endpoint.
	 */
	struct net_sockaddr_storage remote_addr;

	/** Local address of the endpoint.
	 */
	struct net_sockaddr_storage local_addr;

	/** Peer connection id length.
	 */
	uint8_t peer_cid_len;

	/** Peer connection id.
	 */
	uint8_t peer_cid[MAX_CONN_ID_LEN];

	/** My connection id length.
	 */
	uint8_t my_cid_len;

	/** My connection id.
	 */
	uint8_t my_cid[MAX_CONN_ID_LEN];

	/** Original destination connection id from peer.
	 */
	uint8_t peer_orig_dcid[MAX_CONN_ID_LEN];

	/** Original connection id length.
	 */
	uint8_t peer_orig_dcid_len;

	/** Pending data buffer for this endpoint.
	 */
	struct {
		/** Mutex to protect pending data buffer.
		 */
		struct k_mutex lock;

		/** Position in the pending data buffer.
		 */
		int pos;

		/** Length of pending data in the buffer.
		 */
		int len;

		/** Pending data we have received on this endpoint but not yet
		 * processed.
		 */
		uint8_t data[CONFIG_QUIC_ENDPOINT_PENDING_DATA_LEN];
	} pending;

	/** Peer connection id pool */
	struct {
		uint8_t cid[MAX_CONN_ID_LEN];
		uint8_t cid_len;
		uint64_t seq_num;
		uint8_t stateless_reset_token[16];
		bool active;
	} peer_cid_pool[CONFIG_QUIC_MAX_PEER_CIDS];

	/** Tracks the highest retire_prior_to seen
	 */
	uint64_t peer_cid_retire_prior_to;

	/** Connection-level flow control, TX (sending to peer) */
	struct {
		uint64_t max_data;       /* Max data peer allows us to send */
		uint64_t bytes_sent;     /* Total bytes sent on all streams */
		uint64_t blocked_sent;   /* max_data value when DATA_BLOCKED was last sent */
	} tx_fc;

	/** Connection-level flow control, RX (receiving from peer) */
	struct {
		uint64_t max_data;       /* Max data we allow peer to send */
		uint64_t bytes_received; /* Total bytes received on all streams */
		uint64_t max_data_sent;  /* Last MAX_DATA value we sent */
		bool need_window_update; /* Flag to send MAX_DATA */
	} rx_fc;

	/** Stream-count limits, how many streams we allow the peer to open.
	 * Mirrored from our own transport parameters and grown in response to
	 * STREAMS_BLOCKED frames (RFC 9000 ch. 19.14 / 4.6).
	 */
	struct {
		uint64_t max_bidi;   /* Current bidi stream limit advertised to peer */
		uint64_t max_uni;    /* Current uni stream limit advertised to peer */
		uint64_t open_bidi;  /* peer-initiated bidi streams currently open */
		uint64_t open_uni;   /* peer-initiated uni  streams currently open */
	} rx_sl;

	/** Peer's transport parameters */
	struct {
		uint64_t initial_max_data;
		uint64_t initial_max_stream_data_bidi_local;
		uint64_t initial_max_stream_data_bidi_remote;
		uint64_t initial_max_stream_data_uni;
		uint64_t initial_max_streams_bidi;
		uint64_t initial_max_streams_uni;
		uint64_t max_idle_timeout;
		uint64_t max_streams_bidi;
		uint64_t max_streams_uni;
		bool parsed;
	} peer_params;

	/** Idle timeout tracking */
	struct {
		/** Last time we received ANY packet from peer (in milliseconds) */
		int64_t last_rx_time;

		/** Negotiated idle timeout (milliseconds)
		 *  = MIN(our max_idle_timeout, peer's max_idle_timeout)
		 */
		uint64_t negotiated_timeout_ms;

		/** Our advertised max idle timeout (from transport params) */
		uint64_t local_max_idle_timeout_ms;

		/** Flag to disable idle timeout (when negotiated to 0) */
		bool idle_timeout_disabled;
	} idle;

	/** Handshake timeout tracking */
	struct {
		/** Time when endpoint was created / handshake started */
		int64_t start_time;

		/** Maximum time allowed for handshake completion (milliseconds) */
		uint64_t timeout_ms;

		/** Flag indicating handshake is complete */
		bool completed;

		/** Semaphore to wait for handshake completion */
		struct k_sem sem;
	} handshake;

	/** RTT estimation and congestion control state (RFC 9002) */
	struct {
		/** Smoothed RTT estimate (microseconds) */
		uint64_t smoothed_rtt;

		/** RTT variance (microseconds) */
		uint64_t rtt_var;

		/** Minimum RTT observed (microseconds) */
		uint64_t min_rtt;

		/** Most recent RTT sample (microseconds) */
		uint64_t latest_rtt;

		/** Whether we have a valid RTT sample yet */
		bool rtt_initialized;

		/** Bytes currently in flight (sent but not yet ACKed) */
		uint64_t bytes_in_flight;

		/** Ring buffer of sent packet info for RTT/loss tracking.
		 *  One buffer per packet number space (Initial, Handshake, App).
		 */
		struct quic_sent_pkt_info sent_pkts[3][CONFIG_QUIC_SENT_PKT_HISTORY_SIZE];

		/** Write index for each sent packet ring buffer */
		uint16_t sent_pkts_idx[3];

		/** Largest packet number ACKed per PN space */
		uint64_t largest_acked[3];

		/** Probe Timeout (RFC 9002 Section 6.2) */
		struct k_work_delayable pto_work;

		/** Max. PTO count */
		uint32_t max_pto_count;

		/** Incremented on each PTO expiry for backoff */
		uint32_t pto_count;
	} recovery;

	/** Max TX payload size for this endpoint, based on path MTU discovery.
	 * Initialized to a default value and updated based on peer's
	 * transport parameters and any Path MTU Discovery we perform.
	 */
	uint16_t max_tx_payload_size;

	/** Is this endpoint in a server role.
	 */
	bool is_server : 1;

	/** Have we notified streams about endpoint closing */
	bool is_closing_notified : 1;
};

struct quic_context;

/* Out-of-order segment, stored until the gap before it is filled */
struct quic_ooo_segment {
	uint64_t offset;
	uint16_t len;
	uint8_t data[CONFIG_QUIC_STREAM_OOO_SEG_SIZE];
};

struct quic_stream_rx_buffer {
	uint8_t data[CONFIG_QUIC_STREAM_RX_BUFFER_SIZE];
	size_t size;           /* Buffer capacity (needed if we change data to be from heap) */
	size_t head;           /* Read position */
	size_t tail;           /* Write position */
	uint64_t read_offset;  /* Stream offset of head */
	bool fin_received;     /* FIN flag seen */

	/* Small fixed queue for out-of-order segments */
	struct quic_ooo_segment ooo[CONFIG_QUIC_STREAM_OOO_SLOTS];
	uint8_t ooo_count;
};

/**
 * Retransmit buffer for sent-but-unACKed stream data.
 *
 * data[0] corresponds to stream byte offset base_offset.
 * As ACKs arrive, base_offset advances and data is compacted.
 * The buffer is full when len == CONFIG_QUIC_STREAM_TX_BUFFER_SIZE.
 */
struct quic_stream_tx_buffer {
	uint8_t  data[CONFIG_QUIC_STREAM_TX_BUFFER_SIZE];
	uint64_t base_offset; /* stream offset of data[0] */
	size_t   len;         /* bytes of unACKed data currently stored */
};

struct quic_stream_state {
	/** State machine context for this stream. Must be the first member.
	 */
	struct smf_ctx ctx;
};

/**
 * QUIC stream information
 */
__net_socket struct quic_stream {
	/** The fifo is used to queue incoming connection requests.
	 */
	intptr_t fifo;

	/** Stream list node */
	sys_snode_t node;

	/** Stream state machine context
	 */
	struct quic_stream_state state;

	/** Stream id.
	 */
	uint64_t id;

	/** Reference count.
	 */
	atomic_t refcount;

	/** Connection id this stream belongs to.
	 */
	struct quic_context *conn;

	/** Endpoint with crypto context for this stream.
	 */
	struct quic_endpoint *ep;

	/** The stream socket id bound to one stream.
	 */
	int sock;

	/** Stream type (combination of direction and initiator)
	 */
	int type;

	/** TX flow control, max data peer allows us to send on this stream */
	uint64_t remote_max_data;

	/** TX flow control, bytes sent on this stream */
	uint64_t bytes_sent;

	/** TX flow control, highest contiguously ACKed stream offset */
	uint64_t bytes_acked;

	/** TX flow control, remote_max_data when STREAM_DATA_BLOCKED was last sent */
	uint64_t blocked_sent;

	/** RX flow control, max data we allow peer to send on this stream */
	uint64_t local_max_data;

	/** RX flow control, bytes received on this stream */
	uint64_t bytes_received;

	/** RX flow control, last MAX_STREAM_DATA value we sent */
	uint64_t local_max_data_sent;

	/**
	 * Error code to use in STOP_SENDING frame (set via setsockopt).
	 * Defaults to QUIC_ERROR_NO_ERROR. The value is opaque to QUIC;
	 * its meaning is defined by the application protocol (e.g. HTTP/3).
	 */
	uint64_t stop_sending_error_code;

	/** BSD socket private data */
	void *socket_data;

	struct {
		/** Event used to signal incoming data or state changes on this stream */
		struct k_poll_event event;

		/** Signal for recv_event */
		struct k_poll_signal signal;
	} recv;

	struct {
		/** Signal for send readiness (TX buffer has space) */
		struct k_poll_signal signal;
	} send;

	struct {
		/** Condition variable used when receiving data */
		struct k_condvar recv;

		/** Mutex used by condition variable */
		struct k_mutex data_available;
	} cond;

	/** Receive buffer */
	struct quic_stream_rx_buffer rx_buf;

	/** TX retransmit buffer (sent but not yet ACKed data). */
	struct quic_stream_tx_buffer tx_buf;

	/** Stream priority (to order streams within a connection) */
	uint8_t priority;

	/** Flag indicating stream needs window update (MAX_STREAM_DATA) */
	bool need_window_update : 1;

	/** Read half shut down via shutdown(SHUT_RD), discard incoming data */
	bool read_closed : 1;

	/** TX side reset because peer sent STOP_SENDING */
	bool tx_reset : 1;
};

/**
 * QUIC connection information
 */
__net_socket struct quic_context {
	/** The fifo is used to queue incoming connection requests.
	 */
	intptr_t fifo;

	/** Reference count.
	 */
	atomic_t refcount;

	/** Internal lock for protecting this context from multiple access.
	 */
	struct k_mutex lock;

	/** Connection id */
	int id;

	/** The QUIC socket id. If data is sent via this socket, it
	 * will automatically add QUIC headers etc into the data.
	 * The socket number is valid only after handshake is done.
	 */
	int sock;

	/** Endpoints attached to this connection context */
	sys_slist_t endpoints;

	/** Listening endpoint i.e., server endpoint.
	 */
	struct quic_endpoint *listen;

	struct {
		/** Queue of streams waiting to be accepted (server-initiated accepts) */
		struct k_fifo accept_q;

		/** Semaphore signalled when a new connection enters accept_q */
		struct k_sem accept_sem;
	} pending;

	struct {
		/** Queue of incoming streams not yet accepted */
		struct k_fifo stream_q;

		/** Semaphore to signal new incoming streams */
		struct k_sem stream_sem;
	} incoming;

#if defined(CONFIG_NET_STATISTICS_QUIC)
	struct net_stats_quic stats;
#endif /* CONFIG_NET_STATISTICS_QUIC */

	/** Stream id counter */
	uint64_t stream_id_counter;

	/** Error code to be passed to application when connection is closed. */
	int error_code;

	/** Stream priority */
	uint8_t stream_priority;

	/** Streams connected to this context */
	sys_slist_t streams;

	/** Information whether this context is the listening one */
	bool is_listening : 1;
};

/**
 * @typedef quic_context_cb_t
 * @brief Callback used while iterating over Quic contexts
 *
 * @param ctx A valid pointer on current Quic context
 * @param user_data A valid pointer on some user data or NULL
 */
typedef void (*quic_context_cb_t)(struct quic_context *ctx,
				  void *user_data);

/**
 * @brief Iterate over Quic context. This is mainly used by net-shell
 * to show information about Quic connections.
 *
 * @param cb Quic context callback
 * @param user_data Caller specific data.
 */
void quic_context_foreach(quic_context_cb_t cb, void *user_data);

/**
 * @typedef quic_endpoint_cb_t
 * @brief Callback used while iterating over Quic endpoints
 *
 * @param ep A valid pointer on current Quic endpoint
 * @param user_data A valid pointer on some user data or NULL
 */
typedef void (*quic_endpoint_cb_t)(struct quic_endpoint *ep,
				   void *user_data);

/**
 * @brief Iterate over Quic context endpoints. This is mainly used by net-shell
 * to show information about Quic endpoints attached to a specific context.
 *
 * @param ctx Quic context
 * @param cb Quic endpoint callback
 * @param user_data Caller specific data.
 */
void quic_context_endpoint_foreach(struct quic_context *ctx,
				   quic_endpoint_cb_t cb, void *user_data);

/**
 * @brief Iterate over all Quic endpoints. This is mainly used by net-shell
 * to show information about Quic endpoints in the system.
 *
 * @param cb Quic endpoint callback
 * @param user_data Caller specific data.
 */
void quic_endpoint_foreach(quic_endpoint_cb_t cb, void *user_data);

/**
 * @typedef quic_stream_cb_t
 * @brief Callback used while iterating over Quic streams
 *
 * @param stream A valid pointer on current Quic stream
 * @param user_data A valid pointer on some user data or NULL
 */
typedef void (*quic_stream_cb_t)(struct quic_stream *stream,
				 void *user_data);

/**
 * @brief Iterate over Quic context streams. This is mainly used by net-shell
 * to show information about Quic streams attached to a specific context.
 *
 * @param ctx Quic context
 * @param cb Quic stream callback
 * @param user_data Caller specific data.
 */
void quic_context_stream_foreach(struct quic_context *ctx,
				 quic_stream_cb_t cb, void *user_data);

/**
 * @brief Iterate over all Quic streams. This is mainly used by net-shell
 * to show information about Quic streams in the system.
 *
 * @param cb Quic stream callback
 * @param user_data Caller specific data.
 */
void quic_stream_foreach(quic_stream_cb_t cb, void *user_data);

#if defined(CONFIG_NET_TEST)
/* Test-only function declarations */
struct quic_context *quic_get_context(int sock);
int quic_get_len(const uint8_t *buf, size_t buf_len, uint64_t *len);
int quic_put_len(uint8_t *buf, size_t buf_len, uint64_t len);

bool quic_setup_initial_secrets(struct quic_endpoint *ep,
				const uint8_t *cid, size_t cid_len,
				uint8_t client_initial_secret[QUIC_HASH_SHA2_256_LEN],
				uint8_t server_initial_secret[QUIC_HASH_SHA2_256_LEN]);

int quic_hkdf_expand_label(const uint8_t *secret, size_t secret_len,
			   const uint8_t *label, size_t label_length,
			   uint8_t *okm, size_t okm_len);

int quic_hp_mask(psa_key_id_t hp_key_id, int cipher_algo,
		 const uint8_t *sample, uint8_t *mask);

int quic_decrypt_header(const uint8_t *packet, size_t packet_len,
			size_t pn_offset, psa_key_id_t hp_key_id,
			int cipher_algo, uint8_t *first_byte_out,
			uint32_t *packet_number_out, size_t *pn_length_out);

int quic_encrypt_header(uint8_t *packet, size_t packet_len,
			size_t pn_offset, size_t pn_length,
			psa_key_id_t hp_key_id, int cipher_algo);

bool quic_conn_init_setup(struct quic_endpoint *ep,
			  const uint8_t *cid, size_t cid_len);

void quic_construct_nonce(const uint8_t *iv, size_t iv_len,
			  uint64_t packet_number, uint8_t *nonce);

uint64_t quic_reconstruct_pn(uint32_t truncated_pn, size_t pn_nbits,
			     uint64_t largest_pn);

int quic_encrypt_payload(struct quic_pp_cipher *pp, uint64_t packet_number,
			 const uint8_t *header, size_t header_len,
			 const uint8_t *plaintext, size_t plaintext_len,
			 uint8_t *ciphertext, size_t ciphertext_size,
			 size_t *ciphertext_len);

int quic_decrypt_payload(struct quic_pp_cipher *pp, uint64_t packet_number,
			 const uint8_t *header, size_t header_len,
			 const uint8_t *ciphertext, size_t ciphertext_len,
			 uint8_t *plaintext, size_t plaintext_size,
			 size_t *plaintext_len);

void quic_crypto_context_destroy(struct quic_crypto_context *ctx);

#endif /* CONFIG_NET_TEST */
