/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/crypto.h"

/* tai64n contains 64-bit seconds and 32-bit nano offset (12 bytes) */
#define WG_TAI64N_LEN 12U

/* Authentication algorithm is chacha20pol1305 which is 128bit (16 byte) long */
#define WG_AUTHTAG_LEN 16U

/* Hash algorithm is blake2s which creates 32 byte long hashes */
#define WG_HASH_LEN 32U

/* Public key algorithm is curve22519 which uses 32 byte long keys */
#define WG_PUBLIC_KEY_LEN 32U

/* Public key algo is curve22519 which uses 32 byte keys */
#define WG_PRIVATE_KEY_LEN 32U

/* Symmetric session keys are chacha20/poly1305 which uses 32 byte long keys */
#define WG_SESSION_KEY_LEN 32U

#define WG_COOKIE_LEN       16U
#define WG_COOKIE_NONCE_LEN 24U

#define COOKIE_SECRET_MAX_AGE_MSEC (2U * SEC_PER_MIN * MSEC_PER_SEC)
#define REKEY_AFTER_MESSAGES       (1ULL << 60)
#define REKEY_TIMEOUT              5U /* seconds */
#define REKEY_AFTER_TIME           120U /* seconds */
#define REJECT_AFTER_TIME          180U /* seconds */
#define REJECT_AFTER_MESSAGES      (0xFFFFFFFFFFFFFFFFULL - (1ULL << 13))

#define KEEPALIVE_TIMEOUT          25U /* seconds */
#define KEEPALIVE_DEFAULT          0U

#define MESSAGE_INVALID               0
#define MESSAGE_HANDSHAKE_INITIATION  1
#define MESSAGE_HANDSHAKE_RESPONSE    2
#define MESSAGE_COOKIE_REPLY          3
#define MESSAGE_TRANSPORT_DATA        4

#define WIREGUARD_CTRL_DEVICE "WIREGUARD_CTRL"

#define MAX_INITIATIONS_PER_SECOND 2U

#define PKT_ALLOC_WAIT_TIME K_MSEC(100U)
#define BUF_ALLOC_TIMEOUT PKT_ALLOC_WAIT_TIME

#define WG_MTU 1420U

/* 5.4.2 First Message: Initiator to Responder */
struct msg_handshake_init {
	uint8_t type;
	uint8_t reserved[3];
	uint32_t sender;
	uint8_t ephemeral[32];
	uint8_t enc_static[32 + WG_AUTHTAG_LEN];
	uint8_t enc_timestamp[WIREGUARD_TIMESTAMP_LEN + WG_AUTHTAG_LEN];
	uint8_t mac1[WG_COOKIE_LEN];
	uint8_t mac2[WG_COOKIE_LEN];
} __packed;

/* 5.4.3 Second Message: Responder to Initiator */
struct msg_handshake_response {
	uint8_t type;
	uint8_t reserved[3];
	uint32_t sender;
	uint32_t receiver;
	uint8_t ephemeral[32];
	uint8_t enc_empty[WG_AUTHTAG_LEN];
	uint8_t mac1[WG_COOKIE_LEN];
	uint8_t mac2[WG_COOKIE_LEN];
} __packed;

/* 5.4.7 Under Load: Cookie Reply Message */
struct msg_cookie_reply {
	uint8_t type;
	uint8_t reserved[3];
	uint32_t receiver;
	uint8_t nonce[WG_COOKIE_NONCE_LEN];
	uint8_t enc_cookie[WG_COOKIE_LEN + WG_AUTHTAG_LEN];
} __packed;

/* 5.4.6 Subsequent Messages: Transport Data Messages */
struct msg_transport_data {
	uint8_t type;
	uint8_t reserved[3];
	uint32_t receiver;
	uint8_t counter[8];
	uint8_t enc_packet[]; /* Encrypted data follows */
} __packed;

struct wg_msg_hdr {
	uint8_t type;
	uint8_t reserved[3];
};

union wg_msg {
	struct wg_msg_hdr short_hdr;
	struct msg_handshake_init init;
	struct msg_handshake_response rsp;
	struct msg_cookie_reply rpl;
	struct msg_transport_data data;
};

struct wg_handshake {
	uint32_t local_index;
	uint32_t remote_index;
	uint8_t ephemeral_private[WG_PRIVATE_KEY_LEN];
	uint8_t remote_ephemeral[WG_PUBLIC_KEY_LEN];
	uint8_t hash[WG_HASH_LEN];
	uint8_t chaining_key[WG_HASH_LEN];
	bool is_valid : 1;
	bool is_initiator : 1;
};

struct wg_keypair {
	k_timepoint_t expires;
	k_timepoint_t rejected;

	uint8_t sending_key[WG_SESSION_KEY_LEN];
	uint64_t sending_counter;

	uint8_t receiving_key[WG_SESSION_KEY_LEN];

	uint32_t last_tx;
	uint32_t last_rx;

	uint32_t replay_bitmap;
	uint64_t replay_counter;

	uint32_t local_index;  /* Index we generated for our end */
	uint32_t remote_index; /* Index on the other end */

	bool is_sending_valid : 1;
	bool is_receiving_valid : 1;
	bool is_valid : 1;

	 /* If we have initiated this session, then send the initiation packet instead of
	  * the response packet.
	  */
	bool is_initiator : 1;
};

struct wg_allowed_ip {
	struct net_addr addr;
	uint8_t mask_len;
	bool is_valid : 1;
};

struct wg_peer {
	sys_snode_t node;

	struct {
		uint8_t public_key[WG_PUBLIC_KEY_LEN];
		uint8_t preshared[WG_SESSION_KEY_LEN];

		/* Precomputed DH(Sprivi,Spubr) with interface private key and peer public key */
		uint8_t public_dh[WG_PUBLIC_KEY_LEN];
	} key;

	/* Session keypairs */
	struct {
		struct {
			struct wg_keypair prev;
			struct wg_keypair current;
			struct wg_keypair next;
		} keypair;
	} session;

	/* Decrypted cookie from the responder */
	uint8_t cookie[WG_COOKIE_LEN];
	k_timepoint_t cookie_secret_expires;

	/* The latest mac1 we sent with initiation */
	uint8_t handshake_mac1[WG_COOKIE_LEN];

	/* Precomputed keys for use in mac validation */
	uint8_t label_cookie_key[WG_SESSION_KEY_LEN];
	uint8_t label_mac1_key[WG_SESSION_KEY_LEN];

	/* The currently active handshake */
	struct wg_handshake handshake;

	struct wg_iface_context *ctx;
	struct net_if *iface;

	struct wg_allowed_ip allowed_ip[CONFIG_WIREGUARD_MAX_SRC_IPS];
	struct net_sockaddr_storage cfg_endpoint; /* Configured peer IP address */
	struct net_sockaddr_storage endpoint; /* Latest received IP address */

	/* Keeps track of the greatest timestamp received per peer */
	uint8_t greatest_timestamp[WG_TAI64N_LEN];

	/* The last time we received a valid initiation message */
	uint32_t last_initiation_rx;
	/* The last time we sent an initiation message to this peer */
	uint32_t last_initiation_tx;

	/* Do we need to do handshake again */
	k_timepoint_t rekey_expires;

	/* Last time we sent data packets */
	uint32_t last_tx;
	/* Last time we received data packets */
	uint32_t last_rx;

	/* Keepalive interval (in seconds). Set 0 is disable it. */
	uint16_t keepalive_interval;
	k_timepoint_t keepalive_expires;

	int id;

	bool handshake_mac1_valid : 1;

	/* We set this flag on RX/TX of packets if we think that we should
	 * initiate a new handshake.
	 */
	bool send_handshake : 1;
};

static inline bool wg_is_under_load(void)
{
	/* TODO: Add proper implementation */
	return false;
}

typedef void (*wg_peer_cb_t)(struct wg_peer *peer, void *user_data);

void wireguard_peer_foreach(wg_peer_cb_t cb, void *user_data);

#if defined(WG_FUNCTION_PROTOTYPES)
static void wg_start_session(struct wg_peer *peer, bool is_initiator);
static struct wg_peer *peer_lookup_by_pubkey(struct wg_iface_context *ctx,
					     const char *public_key);
static struct wg_peer *peer_lookup_by_id(uint8_t id);
static struct wg_peer *peer_lookup_by_receiver(struct wg_iface_context *ctx,
					       uint32_t receiver);
static struct wg_peer *peer_lookup_by_handshake(struct wg_iface_context *ctx,
						uint32_t receiver);
static struct wg_keypair *get_peer_keypair_for_index(struct wg_peer *peer,
						     uint32_t idx);
static uint32_t generate_unique_index(struct wg_iface_context *ctx);

static void wg_mac(uint8_t *dst, const void *message, size_t len,
		   const uint8_t *key, size_t keylen);
static void wg_mac_key(uint8_t *key, const uint8_t *public_key,
		       const uint8_t *label, size_t label_len);
static void wg_mix_hash(uint8_t *hash, const uint8_t *src, size_t src_len);
static void wg_hmac(uint8_t *digest, const uint8_t *key, size_t key_len,
		    const uint8_t *data, size_t data_len);
static void wg_kdf1(uint8_t *tau1, const uint8_t *chaining_key,
		    const uint8_t *data, size_t data_len);
static void wg_kdf2(uint8_t *tau1, uint8_t *tau2, const uint8_t *chaining_key,
		    const uint8_t *data, size_t data_len);
static void wg_kdf3(uint8_t *tau1, uint8_t *tau2, uint8_t *tau3,
		    const uint8_t *chaining_key,
		    const uint8_t *data, size_t data_len);
static bool wg_check_replay(struct wg_keypair *keypair, uint64_t seq);
static void wg_clamp_private_key(uint8_t *key);
static void wg_generate_private_key(uint8_t *key);
static bool wg_generate_public_key(uint8_t *public_key, const uint8_t *private_key);
static bool wg_check_mac1(struct wg_iface_context *ctx,
			  const uint8_t *data, size_t len,
			  const uint8_t *mac1);
static void wg_generate_cookie_secret(struct wg_iface_context *ctx,
				      uint32_t lifetime_in_ms);
static void wg_tai64n_now(uint8_t *output);
static bool wg_check_initiation_message(struct wg_iface_context *ctx,
					struct msg_handshake_init *msg,
					struct net_sockaddr *addr);
static bool wg_check_response_message(struct wg_iface_context *ctx,
				      struct msg_handshake_response *msg,
				      struct net_sockaddr *addr);
static struct wg_peer *wg_process_initiation_message(struct wg_iface_context *ctx,
						     struct msg_handshake_init *msg);
static bool wg_create_handshake_init(struct wg_iface_context *ctx,
				     struct wg_peer *peer,
				     struct msg_handshake_init *dst);
static int wg_send_handshake_init(struct wg_iface_context *ctx,
				  struct net_if *iface,
				  struct wg_peer *peer,
				  struct msg_handshake_init *packet);
static bool wg_create_handshake_response(struct wg_iface_context *ctx,
					 struct wg_peer *peer,
					 struct msg_handshake_response *dst);
static void wg_send_handshake_response(struct wg_iface_context *ctx,
				       struct net_if *iface,
				       struct wg_peer *peer,
				       struct net_sockaddr *my_addr);
static void wg_send_handshake_cookie(struct wg_iface_context *ctx,
				     const uint8_t *mac1,
				     uint32_t index,
				     struct net_sockaddr *addr);
static bool wg_process_cookie_message(struct wg_iface_context *ctx,
				      struct wg_peer *peer,
				      struct msg_cookie_reply *src);
static void wg_create_cookie_reply(struct wg_iface_context *ctx,
				   struct msg_cookie_reply *dst,
				   const uint8_t *mac1,
				   uint32_t index,
				   uint8_t *source_addr_port,
				   size_t source_length);
static void wg_process_response_message(struct wg_iface_context *ctx,
					struct wg_peer *peer,
					struct msg_handshake_response *response,
					struct net_sockaddr *addr);
static int wg_process_data_message(struct wg_iface_context *ctx,
				   struct wg_peer *peer,
				   struct msg_transport_data *data_hdr,
				   struct net_pkt *pkt,
				   size_t ip_udp_hdr_len,
				   struct net_sockaddr *addr);
static void keypair_destroy(struct wg_keypair *keypair);
static void wg_encrypt_packet(uint8_t *dst, const uint8_t *src, size_t src_len,
			      struct wg_keypair *keypair);
#endif /* WG_FUNCTION_PROTOTYPES */
